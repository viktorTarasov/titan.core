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
#include "DebugCommands.hh"
#include "Communication.hh"
#include "../common/pattern.hh"
#include <unistd.h>
#include <pwd.h>

//////////////////////////////////////////////////////
////////////////// TTCN3_Debugger ////////////////////
//////////////////////////////////////////////////////

TTCN3_Debugger ttcn3_debugger;

void TTCN3_Debugger::switch_state(const char* p_state_str)
{
  if (!strcmp(p_state_str, "on")) {
    if (active) {
      print(DRET_NOTIFICATION, "The debugger is already switched on.");
    }
    else {
      active = true;
      print(DRET_SETTING_CHANGE, "Debugger switched on.");
    }
  }
  else if(!strcmp(p_state_str, "off")) {
    if (!active) {
      print(DRET_NOTIFICATION, "The debugger is already switched off.");
    }
    else {
      active = false;
      print(DRET_SETTING_CHANGE, "Debugger switched off.");
    }
  }
  else {
    print(DRET_NOTIFICATION, "Argument 1 is invalid. Expected 'yes' or 'no'.");
  }
}

void TTCN3_Debugger::add_breakpoint(const char* p_module, int p_line /*const char* batch_file*/)
{
  if (find_breakpoint(p_module, p_line) == breakpoints.size()) {
    breakpoint_t bp;
    bp.module = mcopystr(p_module);
    bp.line = p_line;
    breakpoints.push_back(bp);
    print(DRET_SETTING_CHANGE, "Breakpoint added in module '%s' at line %d.",
      p_module, p_line);
  }
  else {
    print(DRET_NOTIFICATION, "Breakpoint already set in module '%s' at line %d.",
      p_module, p_line);
  }
}

void TTCN3_Debugger::remove_breakpoint(const char* p_module, int p_line)
{
  size_t pos = find_breakpoint(p_module, p_line);
  if (pos != breakpoints.size()) {
    Free(breakpoints[pos].module);
    breakpoints.erase_at(pos);
    print(DRET_SETTING_CHANGE, "Breakpoint removed in module '%s' from line %d.",
      p_module, p_line);
  }
  else {
    print(DRET_NOTIFICATION, "No breakpoint found in module '%s' at line %d.",
      p_module, p_line);
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
    print(DRET_NOTIFICATION, "Argument 1 is invalid. Expected 'yes' or 'no'.");
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
  print(DRET_SETTING_CHANGE, "%s verdict behavior %sset to %s.", sbp_type_str,
    state_changed ? "" : "was already ",
    new_state ? "halt test execution" : "do nothing");
}

void TTCN3_Debugger::print_call_stack()
{
  for (size_t i = call_stack.size(); i != 0; --i) {
    add_to_result("%d.\t", (int)call_stack.size() - (int)i + 1);
    call_stack[i - 1]->print_function();
    if (i != 1) {
      add_to_result("\n");
    }
  }
}

void TTCN3_Debugger::set_stack_level(int new_level)
{
  if (!halted) {
    print(DRET_NOTIFICATION, "Stack level can only be set if test execution is halted.");
  }
  else if (new_level <= 0 || (size_t)new_level > call_stack.size()) {
    print(DRET_NOTIFICATION, "Invalid new stack level. Expected 1 - %d.",
      (int)call_stack.size());
  }
  else {
    stack_level = (int)call_stack.size() - new_level;
    call_stack[stack_level]->print_function();
    print(DRET_NOTIFICATION, "Stack level set to:\n%d.\t%s", new_level, command_result);
  }
}

void TTCN3_Debugger::print_variable(const TTCN3_Debugger::variable_t* p_var)
{
  add_to_result("[%s] %s := %s", p_var->type_name, p_var->name,
    (const char*)p_var->print_function(*p_var));
}

void TTCN3_Debugger::set_output(const char* p_output_type, const char* p_file_name)
{
  FILE* new_fp = NULL;
  bool file, console;
  bool same_file = false;
  char* final_file_name = NULL;
  // check the command's parameters before actually changing anything
  if (!strcmp(p_output_type, "console")) {
    file = false;
    console = true;
  }
  else if (!strcmp(p_output_type, "file")) {
    file = true;
    console = false;
  }
  else if (!strcmp(p_output_type, "both")) {
    file = true;
    console = true;
  }
  else {
    print(DRET_NOTIFICATION, "Argument 1 is invalid. Expected 'console', 'file' or 'both'.");
    return;
  }
  if (file) {
    if (p_file_name == NULL) {
      print(DRET_NOTIFICATION, "Argument 2 (output file name) is missing.");
      return;
    }
    if (output_file_name != NULL && !strcmp(p_file_name, output_file_name)) {
      // don't reopen it if it's the same file as before
      same_file = true;
    }
    else if (!TTCN_Runtime::is_hc()) {
        // don't open any files on HCs, just store settings for future PTCs
      final_file_name = finalize_file_name(p_file_name);
      new_fp = fopen(final_file_name, "w");
      if (new_fp == NULL) {
        print(DRET_NOTIFICATION, "Failed to open file '%s' for writing.", final_file_name);
        return;
      }
    }
  }
  // print the change notification to the old output
  char* file_str = file ? mprintf("file '%s'", TTCN_Runtime::is_hc() ? p_file_name
    : final_file_name) : NULL;
  Free(final_file_name);
  print(DRET_SETTING_CHANGE, "Debugger set to print its output to %s%s%s.",
    console ? "the console" : "", (console && file) ? " and to " : "",
    file ? file_str : "");
  if (file) {
    Free(file_str);
  }
  if (!same_file && !TTCN_Runtime::is_hc()) {
    if (output_file != NULL) {
      fclose(output_file);
    }
    output_file = new_fp;
  }
  send_to_console = console;
  Free(output_file_name);
  if (file) {
    output_file_name = mcopystr(p_file_name);
  }
}

void TTCN3_Debugger::halt()
{
  if (!halted) {
    halted = true;
    stack_level = call_stack.size() - 1;
    print(DRET_NOTIFICATION, "Test execution halted.");
    TTCN_Communication::process_debug_messages();
  }
  else {
    print(DRET_NOTIFICATION, "Test execution is already halted.");
  }
}

void TTCN3_Debugger::resume()
{
  if (halted) {
    halted = false;
    stack_level = -1;
    print(DRET_NOTIFICATION, "Test execution resumed.");
  }
  else {
    print(DRET_NOTIFICATION, "Test execution is not halted.");
  }
}

void TTCN3_Debugger::exit_(const char* p_what)
{
  bool exit_all;
  if (!strcmp(p_what, "test")) {
    exit_all = false;
  }
  else if (!strcmp(p_what, "all")) {
    exit_all = true;
  }
  else {
    print(DRET_NOTIFICATION, "Argument 1 is invalid. Expected 'test' or 'all'.");
    return;
  }
  halted = false;
  print((exit_all && TTCN_Runtime::is_mtc()) ? DRET_EXIT_ALL : DRET_NOTIFICATION,
    "Exiting %s.", exit_all ? "test execution" : "current test");
  TTCN_Runtime::stop_execution();
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

char* TTCN3_Debugger::finalize_file_name(const char* p_file_name_skeleton)
{
  if (p_file_name_skeleton == NULL) {
    return NULL;
  }
  size_t len = strlen(p_file_name_skeleton);
  size_t next_idx = 0;
  char* ret_val = NULL;
  for (size_t i = 0; i < len - 1; ++i) {
    if (p_file_name_skeleton[i] == '%') {
      ret_val = mputstrn(ret_val, p_file_name_skeleton + next_idx, i - next_idx);
      switch (p_file_name_skeleton[i + 1]) {
      case 'e': // %e -> executable name
        ret_val = mputstr(ret_val, TTCN_Logger::get_executable_name());
        break;
      case 'h': // %h -> host name
        ret_val = mputstr(ret_val, TTCN_Runtime::get_host_name());
        break;
      case 'p': // %p -> process ID
        ret_val = mputprintf(ret_val, "%ld", (long)getpid());
        break;
      case 'l': { // %l -> login name
        setpwent();
        struct passwd *p = getpwuid(getuid());
        if (NULL != p) {
          ret_val = mputstr(ret_val, p->pw_name);
        }
        endpwent();
        break; }
      case 'r': // %r -> component reference
        if (TTCN_Runtime::is_single()) {
          ret_val = mputstr(ret_val, "single");
        }
        else if (TTCN_Runtime::is_mtc()) {
          ret_val = mputstr(ret_val, "mtc");
        }
        else if (TTCN_Runtime::is_ptc()) {
          ret_val = mputprintf(ret_val, "%d", (component)self);
        }
        break;
      case 'n': // %n -> component name
        if (TTCN_Runtime::is_mtc()) {
          ret_val = mputstr(ret_val, "MTC");
        }
        else if (TTCN_Runtime::is_ptc()) {
          ret_val = mputstr(ret_val, TTCN_Runtime::get_component_name());
        }
        break;
      case '%': // %% -> single %
        ret_val = mputc(ret_val, '%');
        break;
      default: // unknown sequence -> leave it as it is 
        ret_val = mputstrn(ret_val, p_file_name_skeleton + i, 2);
        break;
      }
      next_idx = i + 2;
      ++i;
    }
  }
  if (next_idx < len) {
    ret_val = mputstr(ret_val, p_file_name_skeleton + next_idx);
  }
  return ret_val;
}

void TTCN3_Debugger::print(int return_type, const char* fmt, ...) const
{
  va_list parameters;
  va_start(parameters, fmt);
  char* str = mprintf_va_list(fmt, parameters);
  va_end(parameters);
  TTCN_Communication::send_debug_return_value(return_type, send_to_console ? str : NULL);
  if (output_file != NULL) {
    fprintf(output_file, "%s\n", str);
    fflush(output_file);
  }
  Free(str);
}

TTCN3_Debugger::TTCN3_Debugger()
{
  enabled = false;
  active = false;
  halted = false;
  output_file = NULL;
  output_file_name = NULL;
  send_to_console = true;
  snapshots = NULL;
  last_breakpoint_entry.module = NULL;
  last_breakpoint_entry.line = 0;
  stack_level = -1;
  fail_behavior = false;
  error_behavior = false;
  command_result = NULL;
}

TTCN3_Debugger::~TTCN3_Debugger()
{
  if (output_file != NULL) {
    fclose(output_file);
    Free(output_file_name);
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
  Free(command_result);
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
  if (active && !call_stack.empty()) {
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
      trigger_type = "Automatic breakpoint (fail verdict)";
      actual_line = last_breakpoint_entry.line;
      break;
    case SBP_ERROR_VERDICT:
      trigger = error_behavior;
      trigger_type = "Automatic breakpoint (error verdict)";
      actual_line = last_breakpoint_entry.line;
      break;
    default: // code lines
      // make sure it's not the same breakpoint entry as last time
      trigger = (last_breakpoint_entry.line == 0 || p_line != last_breakpoint_entry.line ||
        module_name != last_breakpoint_entry.module) &&
        find_breakpoint(module_name, p_line) != breakpoints.size();
      trigger_type = "User breakpoint";
      actual_line = p_line;
      break;
    }
    if (trigger) {
      print(DRET_NOTIFICATION, "%s reached in module '%s' at line %d.",
        trigger_type, module_name, actual_line);
      TTCN_Communication::send_debug_halt_req();
      halt();
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

void TTCN3_Debugger::add_to_result(const char* fmt, ...)
{
  va_list parameters;
  va_start(parameters, fmt);
  command_result = mputprintf_va_list(command_result, fmt, parameters);
  va_end(parameters);
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
  if (snapshots != NULL) {
    snapshots = mputc(snapshots, '\n');
  }
  snapshots = mputstr(snapshots, p_snapshot);
}

#define CHECK_NOF_ARGUMENTS(exp_num) \
  if (exp_num != p_argument_count) { \
    print(DRET_NOTIFICATION, "Invalid number of arguments. Expected %d, got %d.", \
      (int)exp_num, (int)p_argument_count); \
    return; \
  }

#define CHECK_NOF_ARGUMENTS_RANGE(min, max) \
  if ((int)min > p_argument_count || (int)max < p_argument_count) { \
    print(DRET_NOTIFICATION, "Invalid number of arguments. Expected at least %d " \
      "and at most %d, got %d.", (int)min, (int)max, p_argument_count); \
    return; \
  }

#define CHECK_NOF_ARGUMENTS_MIN(min) \
  if ((int)min > p_argument_count) { \
    print(DRET_NOTIFICATION, "Invalid number of arguments. Expected at least %d, got %d.", \
      (int)min, p_argument_count); \
    return; \
  }

#define CHECK_INT_ARGUMENT(arg_idx) \
  { \
    size_t len = strlen(p_arguments[arg_idx]); \
    for (size_t i = 0; i < len; ++i) { \
      if (p_arguments[arg_idx][i] < '0' || p_arguments[arg_idx][i] > '9') { \
        print(DRET_NOTIFICATION, "Argument %d is not an integer.", (int)(arg_idx + 1)); \
        return; \
      } \
    } \
  }

#define CHECK_CALL_STACK \
  if (call_stack.empty()) { \
    print(DRET_NOTIFICATION, "This command can only be used during test execution."); \
    return; \
  }

#define STACK_LEVEL (stack_level >= 0) ? (size_t)stack_level : (call_stack.size() - 1)

void TTCN3_Debugger::execute_command(int p_command, int p_argument_count,
                                     char** p_arguments)
{
  if (!enabled) {
    return;
  }
  Free(command_result);
  command_result = NULL;
  for (int i = 0; i < p_argument_count; ++i) {
    if (p_arguments[i] == NULL) {
      print(DRET_NOTIFICATION, "Argument %d is a null pointer.", i + 1);
      return;
    }
  }
  switch (p_command) {
  case D_SWITCH:
    CHECK_NOF_ARGUMENTS(1)
    switch_state(p_arguments[0]);
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
  case D_SET_OUTPUT:
    CHECK_NOF_ARGUMENTS_RANGE(1, 2)
    set_output(p_arguments[0], (p_argument_count == 2) ? p_arguments[1] : NULL);
    break;
  case D_SET_COMPONENT:
    print(DRET_NOTIFICATION, "Command " D_SET_COMPONENT_TEXT " should have been "
      "sent to the Main Controller.");
    break;
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
    return; // don't print the command result in this case
  case D_LIST_VARIABLES:
    CHECK_CALL_STACK
    CHECK_NOF_ARGUMENTS_RANGE(1, 2)
    call_stack[STACK_LEVEL]->list_variables(p_arguments[0],
      (p_argument_count == 2) ? p_arguments[1] : NULL);
    break;
  case D_PRINT_VARIABLE:
    CHECK_CALL_STACK
    CHECK_NOF_ARGUMENTS_MIN(1)
    for (int i = 0; i < p_argument_count; ++i) {
      const variable_t* var = call_stack[STACK_LEVEL]->find_variable(p_arguments[i]);
      if (var != NULL) {
        print_variable(var);
      }
      else {
        add_to_result("Variable '%s' not found.", p_arguments[i]);
      }
      if (i != p_argument_count - 1) {
        add_to_result("\n");
      }
    }
    break;
  // ...
  case D_PRINT_SNAPSHOTS:
    CHECK_NOF_ARGUMENTS(0)
    add_to_result("%s", snapshots);
    break;
  // ...
  case D_HALT:
    if (TTCN_Runtime::is_mtc()) {
      CHECK_CALL_STACK
    }
    CHECK_NOF_ARGUMENTS(0)
    halt();
    break;
  case D_CONTINUE:
    CHECK_NOF_ARGUMENTS(0)
    resume();
    break;
  case D_EXIT:
    CHECK_NOF_ARGUMENTS(1)
    if (TTCN_Runtime::is_mtc()) {
      CHECK_CALL_STACK
    }
    exit_(p_arguments[0]);
    break;
  case D_SETUP:
    CHECK_NOF_ARGUMENTS_MIN(5)
    if (strlen(p_arguments[0]) > 0) {
      switch_state(p_arguments[0]);
    }
    if (strlen(p_arguments[1]) > 0) {
      set_output(p_arguments[1], p_arguments[2]);
    }
    if (strlen(p_arguments[3]) > 0) {
      set_special_breakpoint(SBP_ERROR_VERDICT, p_arguments[3]);
    }
    if (strlen(p_arguments[4]) > 0) {
      set_special_breakpoint(SBP_FAIL_VERDICT, p_arguments[4]);
    }
    for (int i = 5; i < p_argument_count; i += 2) {
      add_breakpoint(p_arguments[i], str2int(p_arguments[i + 1]));
    }
    break;
  default:
    print(DRET_NOTIFICATION, "Command not implemented.");
    return;
  }
  if (command_result != NULL) {
    print(DRET_DATA, command_result);
  }
}

void TTCN3_Debugger::open_output_file()
{
  if (output_file == NULL && output_file_name != NULL) {
    char* final_file_name = finalize_file_name(output_file_name);
    output_file = fopen(final_file_name, "w");
    if (output_file == NULL) {
      print(DRET_NOTIFICATION, "Failed to open file '%s' for writing.", final_file_name);
    }
    Free(final_file_name);
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

void TTCN3_Debug_Scope::list_variables(regex_t* p_posix_regexp, bool& p_first) const
{
  for (size_t i = 0; i < variables.size(); ++i) {
    if (p_posix_regexp == NULL ||
        regexec(p_posix_regexp, variables[i]->name, 0, NULL, 0) == 0) {
      ttcn3_debugger.add_to_result("%s%s", p_first ? "" : " ", variables[i]->name);
      p_first = false;
    }
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
    snapshot = mputstr(snapshot, ")");
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
  if (!scopes.empty() && scopes[scopes.size() - 1] == p_scope) {
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
  ttcn3_debugger.add_to_result("[%s]\t%s(", function_type, function_name);
  if (parameter_names->size_of() > 0) {
    for (int i = 0; i < parameter_names->size_of(); ++i) {
      if (i > 0) {
        ttcn3_debugger.add_to_result(", ");
      }
      const TTCN3_Debugger::variable_t* parameter = find_variable((*parameter_names)[i]);
      ttcn3_debugger.add_to_result("[%s] %s := %s", (const char*)(*parameter_types)[i],
        (const char*)(*parameter_names)[i], (const char*)parameter->print_function(*parameter));
    }
  }
  ttcn3_debugger.add_to_result(")");
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
  else if (!strcmp(p_scope, "comp")) {
    list_comp = true;
  }
  else if (!strcmp(p_scope, "all")) {
    list_local = true;
    list_global = true;
    list_comp = true;
  }
  else {
    ttcn3_debugger.print(DRET_NOTIFICATION, "Argument 1 is invalid. "
      "Expected 'local', 'global', 'comp' or 'all'.");
    return;
  }
  regex_t* posix_regexp = NULL;
  if (p_filter != NULL) {
    char* posix_str = TTCN_pattern_to_regexp(p_filter);
    if (posix_str == NULL) {
      ttcn3_debugger.print(DRET_NOTIFICATION, "Argument 2 is invalid. "
        "Expected a valid TTCN-3 character pattern.");
      return;
    }
    posix_regexp = new regex_t;
    int ret_val = regcomp(posix_regexp, posix_str, REG_EXTENDED | REG_NOSUB);
    Free(posix_str);
    if (ret_val != 0) {
      char msg[512];
      regerror(ret_val, posix_regexp, msg, sizeof(msg));
      regfree(posix_regexp);
      delete posix_regexp;
      ttcn3_debugger.print(DRET_NOTIFICATION, "Compilation of POSIX regular "
        "expression failed.");
      return;
    }
  }
  if (list_local) {
    for (size_t i = 0; i < variables.size(); ++i) {
      if (posix_regexp == NULL ||
          regexec(posix_regexp, variables[i]->name, 0, NULL, 0) == 0) {
        ttcn3_debugger.add_to_result("%s%s", first ? "" : " ", variables[i]->name);
        first = false;
      }
    }
  }
  if (list_global && global_scope != NULL && global_scope->has_variables()) {
    global_scope->list_variables(posix_regexp, first);
  }
  if (list_comp && component_scope != NULL && component_scope->has_variables()) {
    component_scope->list_variables(posix_regexp, first);
  }
  if (first) {
    ttcn3_debugger.print(DRET_NOTIFICATION, "No variables found.");
  }
  if (posix_regexp != NULL) {
    regfree(posix_regexp);
    delete posix_regexp;
  }
}

