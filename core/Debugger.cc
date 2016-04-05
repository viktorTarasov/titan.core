/******************************************************************************
 * Copyright (c) 2000-2016 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Baranyi, Botond – initial implementation
 *
 ******************************************************************************/

#include "Debugger.hh"

//////////////////////////////////////////////////////
////////////////// TTCN3_Debugger ////////////////////
//////////////////////////////////////////////////////

TTCN3_Debugger ttcn3_debugger;

void TTCN3_Debugger::switch_off()
{
  if (!active) {
    print("The debugger is already switched off.\n");
  }
  else {
    print("Debugger switched off.\n");
  }
  active = false;
}

void TTCN3_Debugger::switch_on()
{
  if (active) {
    print("The debugger is already switched on.\n");
  }
  else {
    print("Debugger switched on.\n");
  }
  active = true;
}

void TTCN3_Debugger::add_breakpoint(const char* p_module, int p_line /*const char* batch_file*/)
{
  if (find_breakpoint(p_module, p_line) == breakpoints.size()) {
    breakpoint_t bp;
    bp.module = mcopystr(p_module);
    bp.line = p_line;
    breakpoints.push_back(bp);
    print("Breakpoint added in module '%s' at line %d.\n", p_module, p_line);
  }
  else {
    print("Breakpoint already set in module '%s' at line %d.\n", p_module, p_line);
  }
}

void TTCN3_Debugger::remove_breakpoint(const char* p_module, int p_line)
{
  size_t pos = find_breakpoint(p_module, p_line);
  if (pos != breakpoints.size()) {
    Free(breakpoints[pos].module);
    breakpoints.erase_at(pos);
    print("Breakpoint removed in module '%s' from line %d.\n", p_module, p_line);
  }
  else {
    print("No breakpoint found in module '%s' at line %d.\n", p_module, p_line);
  }
}

void TTCN3_Debugger::set_special_breakpoint(special_breakpoint_t p_type, const char* p_state_str)
{
  bool new_state;
  if (!strcmp(p_state_str, "yes")) {
    new_state = true;
  }
  else if(!strcmp(p_state_str, "no")) {
    new_state = false;
  }
  // else if "batch"
  else {
    print("Argument 1 is invalid.\n");
    return;
  }
  const char* sbp_type_str;
  bool state_changed;
  switch (p_type) {
  case SBP_FAIL_VERDICT:
    state_changed = (fail_behavior != new_state);
    fail_behavior = new_state;
    sbp_type_str = "Fail";
    break;
  case SBP_ERROR_VERDICT:
    state_changed = (error_behavior != new_state);
    error_behavior = new_state;
    sbp_type_str = "Error";
    break;
  default:
    // should never happen
    return;
  }
  print("%s verdict behavior %sset to %s.\n", sbp_type_str,
    state_changed ? "" : "was already ",
    new_state ? "halt the program" : "do nothing");
}

void TTCN3_Debugger::print_call_stack()
{
  for (size_t i = call_stack.size(); i != 0; --i) {
    print("%d.\t", (int)call_stack.size() - (int)i + 1);
    call_stack[i - 1]->print_function();
  }
}

void TTCN3_Debugger::set_stack_level(int new_level)
{
  if (new_level < 0 || (size_t)new_level > call_stack.size()) {
    print("Invalid new stack level.\n");
  }
  else {
    stack_level = new_level;
  }
}

void TTCN3_Debugger::print_variable(const TTCN3_Debugger::variable_t* p_var) const
{
  print("%s := %s\n", p_var->name, (const char*)p_var->print_function(*p_var));
}

void TTCN3_Debugger::set_output(const char* p_output_type, const char* p_file_name)
{
  FILE* new_fp; 
  if (!strcmp(p_output_type, "stdout")) {
    new_fp = stdout;
  }
  else if (!strcmp(p_output_type, "stderr")) {
    new_fp = stderr;
  }
  else if (!strcmp(p_output_type, "file")) {
    if (p_file_name == NULL) {
      print("Missing output file name.\n");
      return;
    }
    new_fp = fopen(p_file_name, "w");
    if (new_fp == NULL) {
      print("Failed to open file '%s' for writing.\n");
      return;
    }
  }
  else {
    print("Argument 1 is invalid.\n");
    return;
  }
  // don't close the previous file, if the command's parameters are invalid
  if (output != stdout && output != stderr) {
    fclose(output);
  }
  output = new_fp;
}

size_t TTCN3_Debugger::find_breakpoint(const char* p_module, int p_line) const
{
  for (size_t i = 0; i < breakpoints.size(); ++i) {
    if (!strcmp(breakpoints[i].module, p_module) && breakpoints[i].line == p_line) {
      return i;
    }
  }
  return breakpoints.size();
}

TTCN3_Debugger::variable_t* TTCN3_Debugger::find_variable(const void* p_value) const
{
  for (size_t i = 0; i < variables.size(); ++i) {
    if (variables[i]->value == p_value) {
      return variables[i];
    }
  }
  return NULL;
}

TTCN3_Debugger::TTCN3_Debugger()
{
  active = false;
  output = stderr;
  snapshots = NULL;
  last_breakpoint_entry.module = NULL;
  last_breakpoint_entry.line = 0;
  stack_level = -1;
  fail_behavior = false;
  error_behavior = false;
}

TTCN3_Debugger::~TTCN3_Debugger()
{
  if (output != stdout && output != stderr) {
    fclose(output);
  }
  for (size_t i = 0; i < breakpoints.size(); ++i) {
    Free(breakpoints[i].module);
  }
  for (size_t i = 0; i < global_scopes.size(); ++i) {
    delete global_scopes[i].scope;
  }
  for (size_t i = 0; i < component_scopes.size(); ++i) {
    delete component_scopes[i].scope;
  }
  for (size_t i = 0; i < variables.size(); ++i) {
    delete variables[i];
  }
  Free(snapshots);
}

TTCN3_Debug_Scope* TTCN3_Debugger::add_global_scope(const char* p_module)
{
  named_scope_t global_scope;
  global_scope.name = p_module;
  global_scope.scope = new TTCN3_Debug_Scope();
  global_scopes.push_back(global_scope);
  return global_scope.scope;
}

TTCN3_Debug_Scope* TTCN3_Debugger::add_component_scope(const char* p_component)
{
  named_scope_t component_scope;
  component_scope.name = p_component;
  component_scope.scope = new TTCN3_Debug_Scope();
  component_scopes.push_back(component_scope);
  return component_scope.scope;
}

void TTCN3_Debugger::set_return_value(const CHARSTRING& p_value)
{
  if (active) {
    call_stack[call_stack.size() - 1]->set_return_value(p_value);
  }
}

void TTCN3_Debugger::breakpoint_entry(int p_line /*bool p_stepping_helper*/)
{
  if (active && !call_stack.empty()) {
    const char* module_name = call_stack[call_stack.size() - 1]->get_module_name();
    bool trigger = false;
    const char* trigger_type;
    int actual_line;
    switch (p_line) {
    case SBP_FAIL_VERDICT:
      trigger = fail_behavior;
      trigger_type = "Fail verdict";
      actual_line = last_breakpoint_entry.line;
      break;
    case SBP_ERROR_VERDICT:
      trigger = error_behavior;
      trigger_type = "Error verdict";
      actual_line = last_breakpoint_entry.line;
      break;
    default:
      // code lines
      trigger = (last_breakpoint_entry.line == 0 || p_line != last_breakpoint_entry.line ||
        module_name != last_breakpoint_entry.module) &&
        find_breakpoint(module_name, p_line) != breakpoints.size();
      trigger_type = "Breakpoint";
      actual_line = p_line;
      break;
    }
    // make sure it's not the same breakpoint entry as last time
    if (trigger) {
      stack_level = call_stack.size() - 1;
      print("%s reached in module '%s' at line %d.\n", trigger_type,
        module_name, actual_line);
      ///////////////////////////////////////////////////////////////////////////////////
      /*print("##################################################\n");
      print("Call stack:\n");
      charstring_list params = NULL_VALUE;
      execute_command(D_PRINT_CALL_STACK, params);
      print("##################################################\n");
      print("Variables: ");
      params[0] = "global";
      execute_command(D_LIST_VARIABLES, params);
      params.set_size(0);
      size_t idx = 0;
      const TTCN3_Debug_Scope* glob_scope = get_global_scope(module_name);
      for (size_t i = 0; i < variables.size(); ++i) {
        if (glob_scope->find_variable(variables[i]->name) != NULL) {
          params[idx++] = variables[i]->name;
        }
      }
      execute_command(D_PRINT_VARIABLE, params);
      print("##################################################\n");
      print("Function call snapshots:\n");
      params.set_size(0);
      execute_command(D_PRINT_SNAPSHOTS, params);*/
      ///////////////////////////////////////////////////////////////////////////////////
    }
    last_breakpoint_entry.module = (char*)module_name;
    last_breakpoint_entry.line = p_line;
  }
}

CHARSTRING TTCN3_Debugger::print_base_var(const TTCN3_Debugger::variable_t& p_var)
{
  TTCN_Logger::begin_event_log2str();
  if (!strcmp(p_var.type_name, "bitstring")) {
    ((const BITSTRING*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "bitstring template")) {
    ((const BITSTRING_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "boolean")) {
    ((const BOOLEAN*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "boolean template")) {
    ((const BOOLEAN_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "charstring")) {
    ((const CHARSTRING*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "charstring template")) {
    ((const CHARSTRING_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "float")) {
    ((const FLOAT*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "float template")) {
    ((const FLOAT_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "hexstring")) {
    ((const HEXSTRING*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "hexstring template")) {
    ((const HEXSTRING_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "integer")) {
    ((const INTEGER*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "integer template")) {
    ((const INTEGER_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "objid")) {
    ((const OBJID*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "objid template")) {
    ((const OBJID_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "octetstring")) {
    ((const OCTETSTRING*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "octetstring template")) {
    ((const OCTETSTRING_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "universal charstring")) {
    ((const UNIVERSAL_CHARSTRING*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "universal charstring template")) {
    ((const UNIVERSAL_CHARSTRING_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "verdicttype")) {
    ((const VERDICTTYPE*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "verdicttype template")) {
    ((const VERDICTTYPE_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "component")) {
    ((const COMPONENT*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "component template")) {
    ((const COMPONENT_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "default")) {
    ((const DEFAULT*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "default template")) {
    ((const DEFAULT_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "timer")) {
    ((const TIMER*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "NULL")) {
    ((const ASN_NULL*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "NULL template")) {
    ((const ASN_NULL_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "CHARACTER STRING")) {
    ((const CHARACTER_STRING*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "CHARACTER STRING template")) {
    ((const CHARACTER_STRING_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "EMBEDDED PDV")) {
    ((const EMBEDDED_PDV*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "EMBEDDED PDV template")) {
    ((const EMBEDDED_PDV_template*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "EXTERNAL")) {
    ((const EXTERNAL*)p_var.value)->log();
  }
  else if (!strcmp(p_var.type_name, "EXTERNAL template")) {
    ((const EXTERNAL_template*)p_var.value)->log();
  }
  else {
    TTCN_Logger::log_event_str("<unrecognized value or template>");
  }
  return TTCN_Logger::end_event_log2str();
}

void TTCN3_Debugger::print(const char* fmt, ...) const
{
  va_list parameters;
  va_start(parameters, fmt);
  vfprintf(output, fmt, parameters);
  va_end(parameters);
  fflush(output);
}

void TTCN3_Debugger::add_function(TTCN3_Debug_Function* p_function)
{
  if (active) {
    call_stack.push_back(p_function);
  }
}

void TTCN3_Debugger::add_scope(TTCN3_Debug_Scope* p_scope)
{
  if (active && !call_stack.empty()) {
    call_stack[call_stack.size() - 1]->add_scope(p_scope);
  }
}

void TTCN3_Debugger::remove_function(TTCN3_Debug_Function* p_function)
{
  if (!call_stack.empty() && call_stack[call_stack.size() - 1] == p_function) {
    call_stack.erase_at(call_stack.size() - 1);
  }
}

void TTCN3_Debugger::remove_scope(TTCN3_Debug_Scope* p_scope)
{
  if (!call_stack.empty()) {
    call_stack[call_stack.size() - 1]->remove_scope(p_scope);
  }
}

const TTCN3_Debugger::variable_t* TTCN3_Debugger::add_variable(const void* p_value,
                                                               const char* p_name,
                                                               const char* p_type,
                                                               CHARSTRING (*p_print_function)(const TTCN3_Debugger::variable_t&))
{
   
  if (call_stack.empty()) {
    // no call stack yet, so this is a global or component variable
    variable_t* var = find_variable(p_value);
    if (var == NULL) {
      var = new TTCN3_Debugger::variable_t;
      var->value = p_value;
      var->name = p_name;
      var->type_name = p_type;
      var->print_function = p_print_function;
      variables.push_back(var);
    }
    return var;
  }
  else if (active) {
    // it's a local variable for the top-most function
    return call_stack[call_stack.size() - 1]->add_variable(p_value, p_name, p_type, p_print_function);
  }
  return NULL;
}

void TTCN3_Debugger::remove_variable(const variable_t* p_var)
{
  if (active && !call_stack.empty()) {
    call_stack[call_stack.size() - 1]->remove_variable(p_var);
  }
}

const TTCN3_Debug_Scope* TTCN3_Debugger::get_global_scope(const char* p_module) const
{
  for (size_t i = 0; i < global_scopes.size(); ++i) {
    if (strcmp(global_scopes[i].name, p_module) == 0) {
      return global_scopes[i].scope;
    }
  }
  return NULL;
}

const TTCN3_Debug_Scope* TTCN3_Debugger::get_component_scope(const char* p_component) const
{
  for (size_t i = 0; i < component_scopes.size(); ++i) {
    if (strcmp(component_scopes[i].name, p_component) == 0) {
      return component_scopes[i].scope;
    }
  }
  return NULL;
}

void TTCN3_Debugger::add_snapshot(const char* p_snapshot)
{
  snapshots = mputstr(snapshots, p_snapshot);
}

#define CHECK_NOF_ARGUMENTS(exp_num) \
  if (exp_num != p_arguments.size_of()) { \
    print("Invalid number of arguments. Expected %d, got %d.\n", \
      (int)exp_num, (int)p_arguments.size_of()); \
    return; \
  }

#define CHECK_NOF_ARGUMENTS_RANGE(min, max) \
  if ((int)min > p_arguments.size_of() || (int)max < p_arguments.size_of()) { \
    print("Invalid number of arguments. Expected at least %d and at most %d, got %d.\n", \
      (int)min, (int)max, p_arguments.size_of()); \
    return; \
  }

#define CHECK_NOF_ARGUMENTS_MIN(min) \
  if ((int)min > p_arguments.size_of()) { \
    print("Invalid number of arguments. Expected at least %d, got %d.\n", \
      (int)min, p_arguments.size_of()); \
    return; \
  }

#define CHECK_INT_ARGUMENT(arg_idx) \
  { \
    const char* str = (const char*)p_arguments[arg_idx]; \
    for (int i = 0; i < p_arguments[arg_idx].lengthof(); ++i) { \
      if (str[i] < '0' || str[i] > '9') { \
        print("Argument %d is not an integer.\n", (int)(arg_idx + 1)); \
        return; \
      } \
    } \
  }

#define CHECK_CALL_STACK \
  if (call_stack.empty()) { \
    print("This command can only be executed when the program is running.\n"); \
    return; \
  }

void TTCN3_Debugger::execute_command(TTCN3_Debugger::debug_command_t p_command,
                                     const charstring_list& p_arguments)
{
  if (!active && p_command != D_SWITCH_ON && p_command != D_SWITCH_OFF) {
    print("Cannot run debug commands while the debugger is switched off.\n");
    return;
  }
  for (int i = 0; i < p_arguments.size_of(); ++i) {
    if (!p_arguments[i].is_bound()) {
      print("Argument %d is unbound.\n", i + 1);
      return;
    }
  }
  switch (p_command) {
  case D_SWITCH_OFF:
    CHECK_NOF_ARGUMENTS(0)
    switch_off();
    break;
  case D_SWITCH_ON:
    CHECK_NOF_ARGUMENTS(0)
    switch_on();
    break;
  case D_ADD_BREAKPOINT:
    CHECK_NOF_ARGUMENTS(2)
    CHECK_INT_ARGUMENT(1)
    add_breakpoint(p_arguments[0], str2int(p_arguments[1]));
    break;
  case D_REMOVE_BREAKPOINT:
    CHECK_NOF_ARGUMENTS(2)
    CHECK_INT_ARGUMENT(1)
    remove_breakpoint(p_arguments[0], str2int(p_arguments[1]));
    break;
  case D_SET_ERROR_BEHAVIOR:
    CHECK_NOF_ARGUMENTS(1)
    set_special_breakpoint(SBP_ERROR_VERDICT, p_arguments[0]);
    break;
  case D_SET_FAIL_BEHAVIOR:
    CHECK_NOF_ARGUMENTS(1)
    set_special_breakpoint(SBP_FAIL_VERDICT, p_arguments[0]);
    break;
  // ...
  case D_SET_OUTPUT:
    CHECK_NOF_ARGUMENTS_RANGE(1, 2)
    set_output(p_arguments[0], (p_arguments.size_of() == 2) ? (const char*)p_arguments[1] : NULL);
    break;
  // ...
  case D_PRINT_CALL_STACK:
    CHECK_CALL_STACK
    CHECK_NOF_ARGUMENTS(0)
    print_call_stack();
    break;
  case D_SET_STACK_LEVEL:
    CHECK_CALL_STACK
    CHECK_NOF_ARGUMENTS(1)
    CHECK_INT_ARGUMENT(0)
    set_stack_level(str2int(p_arguments[0]));
    break;
  case D_LIST_VARIABLES:
    CHECK_CALL_STACK
    CHECK_NOF_ARGUMENTS_RANGE(1, 2)
    call_stack[stack_level]->list_variables(p_arguments[0],
      (p_arguments.size_of() == 2) ? (const char*)p_arguments[1] : NULL);
    break;
  case D_PRINT_VARIABLE:
    CHECK_CALL_STACK
    CHECK_NOF_ARGUMENTS_MIN(1)
    for (int i = 0; i < p_arguments.size_of(); ++i) {
      const variable_t* var = call_stack[stack_level]->find_variable(p_arguments[i]);
      if (var != NULL) {
        print_variable(var);
      }
      else {
        print("Variable '%s' not found.\n", (const char*)p_arguments[i]);
      }
    }
    break;
  // ...
  case D_PRINT_SNAPSHOTS:
    CHECK_NOF_ARGUMENTS(0)
    print("%s", snapshots);
    break;
  // ...
  default:
    print("Command not implemented.\n");
    break;
  }
}

//////////////////////////////////////////////////////
//////////////// TTCN3_Debug_Scope ///////////////////
//////////////////////////////////////////////////////

TTCN3_Debug_Scope::TTCN3_Debug_Scope()
{
  ttcn3_debugger.add_scope(this);
}

TTCN3_Debug_Scope::~TTCN3_Debug_Scope()
{
  for (size_t i = 0; i < variables.size(); ++i) {
    ttcn3_debugger.remove_variable(variables[i]);
  }
  ttcn3_debugger.remove_scope(this);
}

void TTCN3_Debug_Scope::add_variable(const void* p_value,
                                     const char* p_name,
                                     const char* p_type,
                                     CHARSTRING (*p_print_function)(const TTCN3_Debugger::variable_t&))
{
  const TTCN3_Debugger::variable_t* var = ttcn3_debugger.add_variable(p_value, p_name, p_type, p_print_function);
  if (var != NULL) {
    variables.push_back(var);
  }
}

const TTCN3_Debugger::variable_t* TTCN3_Debug_Scope::find_variable(const char* p_name) const
{
  for (size_t i = 0; i < variables.size(); ++i) {
    if (strcmp(variables[i]->name, p_name) == 0) {
      return variables[i];
    }
  }
  return NULL;
}

void TTCN3_Debug_Scope::list_variables(const char* p_filter, bool& p_first) const
{
  for (size_t i = 0; i < variables.size(); ++i) {
    // the filter is currently ignored
    ttcn3_debugger.print("%s%s", p_first ? "" : " ", variables[i]->name);
    p_first = false;
  }
}

//////////////////////////////////////////////////////
/////////////// TTCN3_Debug_Function /////////////////
//////////////////////////////////////////////////////

TTCN3_Debug_Function::TTCN3_Debug_Function(const char* p_name,
                                           const char* p_type,
                                           const char* p_module,
                                           const charstring_list& p_parameter_names,
                                           const charstring_list& p_parameter_types,
                                           const char* p_component_name)
: function_name(p_name), function_type(p_type), module_name(p_module)
, parameter_names(new charstring_list(p_parameter_names))
, parameter_types(new charstring_list(p_parameter_types))
{
  ttcn3_debugger.add_function(this);
  global_scope = ttcn3_debugger.get_global_scope(p_module);
  component_scope = (p_component_name != NULL) ?
    ttcn3_debugger.get_component_scope(p_component_name) : NULL;
  if (function_name == NULL) {
    function_name = p_module; // for control parts
  }
}

TTCN3_Debug_Function::~TTCN3_Debug_Function()
{
  if (ttcn3_debugger.is_on()) {
    char* snapshot = mprintf("[%s]\tfinished\t%s(", function_type, function_name);
    if (parameter_names->size_of() > 0) {
      for (int i = 0; i < parameter_names->size_of(); ++i) {
        if (i > 0) {
          snapshot = mputstr(snapshot, ", ");
        }
        snapshot = mputprintf(snapshot, "[%s] %s := ", (const char*)((*parameter_types)[i]),
          (const char*)((*parameter_names)[i]));
        if ((*parameter_types)[i] == "out" || (*parameter_types)[i] == "inout") {
          const TTCN3_Debugger::variable_t* parameter = find_variable((*parameter_names)[i]);
          snapshot = mputstr(snapshot, parameter->print_function(*parameter));
        }
        else {
          snapshot = mputc(snapshot, '-');
        }
      }
    }
    snapshot = mputc(snapshot, ')');
    if (return_value.is_bound()) {
      snapshot = mputprintf(snapshot, " returned %s", (const char*)return_value);
    }
    snapshot = mputc(snapshot, '\n');
    ttcn3_debugger.add_snapshot(snapshot);
    Free(snapshot);
  }
  for (size_t i = 0; i < variables.size(); ++i) {
    delete variables[i];
  }
  delete parameter_names;
  delete parameter_types;
  ttcn3_debugger.remove_function(this);
}

const TTCN3_Debugger::variable_t* TTCN3_Debug_Function::add_variable(const void* p_value,
                                                                     const char* p_name,
                                                                     const char* p_type,
                                                                     CHARSTRING (*p_print_function)(const TTCN3_Debugger::variable_t&))
{
  if (ttcn3_debugger.is_on()) {
    TTCN3_Debugger::variable_t* var = new TTCN3_Debugger::variable_t;
    var->value = p_value;
    var->name = p_name;
    var->type_name = p_type;
    var->print_function = p_print_function;
    variables.push_back(var);
    return var;
  }
  return NULL;
}

void TTCN3_Debug_Function::set_return_value(const CHARSTRING& p_value)
{
  return_value = p_value;
}

void TTCN3_Debug_Function::initial_snapshot() const
{
  if (ttcn3_debugger.is_on()) {
    char* snapshot = mprintf("[%s]\tstarted \t%s(", function_type, function_name);
    if (parameter_names->size_of() > 0) {
      for (int i = 0; i < parameter_names->size_of(); ++i) {
        if (i > 0) {
          snapshot = mputstr(snapshot, ", ");
        }
        snapshot = mputprintf(snapshot, "[%s] %s := ", (const char*)((*parameter_types)[i]),
          (const char*)((*parameter_names)[i]));
        if ((*parameter_types)[i] == "in" || (*parameter_types)[i] == "inout") {
          const TTCN3_Debugger::variable_t* parameter = find_variable((*parameter_names)[i]);
          snapshot = mputstr(snapshot, parameter->print_function(*parameter));
        }
        else {
          snapshot = mputc(snapshot, '-');
        }
      }
    }
    snapshot = mputstr(snapshot, ")\n");
    ttcn3_debugger.add_snapshot(snapshot);
    Free(snapshot);
  }
}

void TTCN3_Debug_Function::add_scope(TTCN3_Debug_Scope* p_scope)
{
  scopes.push_back(p_scope);
}

void TTCN3_Debug_Function::remove_scope(TTCN3_Debug_Scope* p_scope)
{
  if (scopes[scopes.size() - 1] == p_scope) {
    scopes.erase_at(scopes.size() - 1);
  }
}

void TTCN3_Debug_Function::remove_variable(const TTCN3_Debugger::variable_t* p_var)
{
  for (size_t i = 0; i < variables.size(); ++i) {
    if (variables[i] == p_var) {
      variables.erase_at(i);
      delete p_var;
      break;
    }
  }
}

const TTCN3_Debugger::variable_t* TTCN3_Debug_Function::find_variable(const char* p_name) const
{
  for (size_t i = 0; i < variables.size(); ++i) {
    if (strcmp(variables[i]->name, p_name) == 0) {
      return variables[i];
    }
  }
  // it's not a local variable, it might still be a global or component variable
  if (component_scope != NULL) {
    const TTCN3_Debugger::variable_t* res = component_scope->find_variable(p_name);
    if (res != NULL) {
      return res;
    }
  }
  return (global_scope != NULL) ? global_scope->find_variable(p_name) : NULL;
}

void TTCN3_Debug_Function::print_function() const
{
  ttcn3_debugger.print("[%s]\t%s(", function_type, function_name);
  if (parameter_names->size_of() > 0) {
    for (int i = 0; i < parameter_names->size_of(); ++i) {
      if (i > 0) {
        ttcn3_debugger.print(", ");
      }
      const TTCN3_Debugger::variable_t* parameter = find_variable((*parameter_names)[i]);
      ttcn3_debugger.print("[%s] %s := %s", (const char*)(*parameter_types)[i],
        (const char*)(*parameter_names)[i], (const char*)parameter->print_function(*parameter));
    }
  }
  ttcn3_debugger.print(")\n");
}

void TTCN3_Debug_Function::list_variables(const char* p_scope, const char* p_filter) const
{
  bool first = true;
  bool list_local = false;
  bool list_global = false;
  bool list_comp = false;
  if (!strcmp(p_scope, "local")) {
    list_local = true;
  }
  else if (!strcmp(p_scope, "global")) {
    list_global = true;
  }
  else if (!strcmp(p_scope, "comp") || !strcmp(p_scope, "component")) {
    list_comp = true;
  }
  else {
    if (strcmp(p_scope, "all")) {
      ttcn3_debugger.print("Invalid scope. Listing variables in all scopes.\n");
    }
    list_local = true;
    list_global = true;
    list_comp = true;
  }
  if (list_local) {
    for (size_t i = 0; i < variables.size(); ++i) {
      ttcn3_debugger.print("%s%s", first ? "" : " ", variables[i]->name);
      first = false;
    }
  }
  if (list_global && global_scope != NULL && global_scope->has_variables()) {
    global_scope->list_variables(p_filter, first);
  }
  if (list_comp && component_scope != NULL && component_scope->has_variables()) {
    component_scope->list_variables(p_filter, first);
  }
  if (first) {
    ttcn3_debugger.print("No variables found.");
  }
  ttcn3_debugger.print("\n");
}

