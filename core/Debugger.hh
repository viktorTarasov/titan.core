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

#ifndef DEBUGGER_HH
#define DEBUGGER_HH

#include "Vector.hh"
#include "Basetype.hh"
#include "Charstring.hh"
#ifdef TITAN_RUNTIME_2
#include "RT2/PreGenRecordOf.hh"
#else
#include "RT1/PreGenRecordOf.hh"
#endif

/** alias for record of charstring */
typedef PreGenRecordOf::PREGEN__RECORD__OF__CHARSTRING charstring_list;

// forward declarations
class TTCN3_Debug_Scope;
class TTCN3_Debug_Function;


//////////////////////////////////////////////////////
////////////////// TTCN3_Debugger ////////////////////
//////////////////////////////////////////////////////

/** main debugger class
  * 
  * instantiated once per process at the beginning of the current process,
  * destroyed at the end of the current process */
class TTCN3_Debugger {
public:
  
  /** type for keeping track of a variable */
  struct variable_t {
    /** pointer to the variable object, not owned */
    const void* value;
    /** variable name (used for looking up variables), not owned */
    const char* name;
    /** name of the variable's type, not owned */
    const char* type_name;
    /** variable printing function (using the variable object's log() function) */
    CHARSTRING (*print_function)(const variable_t&);
  };
  
  /** this type pairs a debug scope with a name, used for storing global and
    * component scopes */
  struct named_scope_t {
    /** scope name (module name for global scopes, or component type name for
      * component scopes), not owned*/
    const char* name;
    /** scope pointer, owned */
    TTCN3_Debug_Scope* scope;
  };
  
  /** type for storing breakpoints */
  struct breakpoint_t {
    /** module name, owned */
    char* module;
    /** line number */
    int line;
    // const char* batch_file;
  };
  
  /** list of commands coming from the user interface (parameters listed in comments) */
  enum debug_command_t {
    // on/off switch
    D_SWITCH_OFF, // 0
    D_SWITCH_ON, // 0
    // breakpoints
    D_ADD_BREAKPOINT, // 2, module name and line number
    D_REMOVE_BREAKPOINT, // 2, module name and line number
    D_SET_ERROR_BEHAVIOR, // 1, "yes" or "no"
    D_SET_FAIL_BEHAVIOR, // 1, "yes" or "no"
    // printing and overwriting data
    D_SET_OUTPUT, // 1-2, "stdout", "stderr" or "file" + file name
    D_SET_PROCESS, // 1, 'mtc' or component reference
    D_PRINT_CALL_STACK, // 0
    D_SET_STACK_LEVEL, // 1, stack level
    D_LIST_VARIABLES, // 1-2, "local", "global", "comp", "component" or "all", + optional filter (pattern)
    D_PRINT_VARIABLE, // 1+, list of variable names
    D_OVERWRITE_VARIABLE, // 2, variable name, new value (in module parameter syntax)
    D_PRINT_SNAPSHOTS, // 0
    D_SET_SNAPSHOT_BEHAVIOR, // TBD
    // stepping
    D_STEP_OVER, // 0
    D_STEP_INTO, // 0
    D_STEP_OUT, // 0
    D_RUN_TO_CURSOR, // 2, module name and line number
    // ending the halted state
    D_CONTINUE, // 0
    D_EXIT, // 0
    // batch files
    D_SET_HALTING_BATCH_FILE // TBD
  };
  
  /** special breakpoint types, passed to breakpoint_entry() as the line parameter,
    * so these need to be 0 or negative, to never conflict with any line number */
  enum special_breakpoint_t {
    /** triggered when the local verdict is set to ERROR (by dynamic test case errors) */
    SBP_ERROR_VERDICT = 0,
    /** triggered when the local verdict is set to FAIL (by the user) */
    SBP_FAIL_VERDICT = -1
  };
  
private:
  
  /** the debugger's on/off switch */
  bool active;
  
  /** the debugger's output file handler */
  FILE* output;
  
  /** list of all global and component variables, elements are owned */
  Vector<variable_t*> variables;
  
  /** list of global scopes */
  Vector<named_scope_t> global_scopes;
  
  /** list of component scopes */
  Vector<named_scope_t> component_scopes;
  
  /** pointers to debug function objects (resembling a call stack), elements are not owned
    * the current function is always the last element in the array (the top element in the stack) */
  Vector<TTCN3_Debug_Function*> call_stack;
  
  /** list of breakpoints */
  Vector<breakpoint_t> breakpoints;
  
  /** string containing function call snapshots, owned */
  char* snapshots;
  
  /** stores the last line hit by breakpoint_entry() */
  breakpoint_t last_breakpoint_entry;
  
  /** current stack level (reset whenever a breakpoint is reached) */
  int stack_level;
  
  /** behavior triggered by setting the local verdict to FAIL
    * (a breakpoint is activated if set to true) */
  bool fail_behavior;
  
  /** behavior triggered by setting the local verdict to ERROR
    * (a breakpoint is activated if set to true) */
  bool error_behavior;
  
  //////////////////////////////////////////////////////
  ///////////////// internal functions /////////////////
  //////////////////////////////////////////////////////
  
  /** switches the debugger off
    * handles the D_SWITCH_OFF command */
  void switch_off();
  
  /** switches the debugger on 
    * handles the D_SWITCH_ON command */
  void switch_on();
  
  /** adds a new breakpoint at the specified module and line 
    * handles the D_ADD_BREAKPOINT command */
  void add_breakpoint(const char* p_module, int p_line /*const char* batch_file*/);
  
  /** removes the breakpoint from the specified module/line, if it exists
    * handles the D_REMOVE_BREAKPOINT command  */
  void remove_breakpoint(const char* p_module, int p_line);
  
  /** sets the behavior of a special breakpoint type 
    * @param p_state_str "yes" turns the special breakpoint on (as if it was an
    * actual breakpoint), "no" turns it off 
    * handles the D_SET_ERROR_BEHAVIOR and D_SET_FAIL_BEHAVIOR commands */
  void set_special_breakpoint(special_breakpoint_t p_type, const char* p_state_str);
  
  /** prints the current call stack
    * handles the D_PRINT_CALL_STACK command */
  void print_call_stack();
  
  /** sets the current stack level to the specified level
    * handles the D_SET_STACK_LEVEL command */
  void set_stack_level(int new_level);
  
  /** prints the specified variable
    * handles (one parameter of) the D_PRINT_VARIABLE command */
  void print_variable(const variable_t* p_var) const;
  
  /** sets the debugger's output to a different stream 
    * handles the D_SET_OUTPUT command
    * @param p_output_type "stdout", "stderr" or "file"
    * @param p_file_name output file name, if the output is a file, or NULL */
  void set_output(const char* p_output_type, const char* p_file_name);
  
  /** returns the index of the specified breakpoint, if found,
    * otherwise returns breakpoints.size() */
  size_t find_breakpoint(const char* p_module, int p_line) const;
  
  /** returns the specified variable, if found, otherwise returns NULL */
  TTCN3_Debugger::variable_t* find_variable(const void* p_value) const;

public:
  /** constructor - called once per process (at the beginning) */
  TTCN3_Debugger();
  
  /** destructor - called once per process (at the end) */
  ~TTCN3_Debugger();
  
  //////////////////////////////////////////////////////
  ////// methods called from TITAN generated code //////
  //////////////////////////////////////////////////////
  
  /** creates, stores and returns a new global scope for the specified module */
  TTCN3_Debug_Scope* add_global_scope(const char* p_module);
  
  /** creates, stores and returns a new global scope for the specified module */
  TTCN3_Debug_Scope* add_component_scope(const char* p_component);
  
  /** stores the string representation of the current function's return value
    * (only if the debugger is switched on) */
  void set_return_value(const CHARSTRING& p_value);
  
  /** activates a breakpoint if the specified line and the current function's module
    * match any of the breakpoints stored in the debugger 
    * the special parameter values (SBP_ERROR_VERDICT and SBP_FAIL_VERDICT) only
    * trigger a breakpoint if their respective behaviors have been set to do so
    * (does nothing if the debugger is switched off) */
  void breakpoint_entry(int p_line /*bool p_stepping_helper*/);
  
  /** variable printing function for base types */
  static CHARSTRING print_base_var(const variable_t& p_var);
  
  /** variable printing function for value arrays */
  template <typename T_type, unsigned int array_size, int index_offset>
  static CHARSTRING print_value_array(const variable_t& p_var)
  {
    TTCN_Logger::begin_event_log2str();
    ((VALUE_ARRAY<T_type, array_size, index_offset>*)p_var.value)->log();
    return TTCN_Logger::end_event_log2str();
  }
  
  /** variable printing function for template arrays */
  template <typename T_value_type, typename T_template_type,
    unsigned int array_size, int index_offset>
  static CHARSTRING print_template_array(const variable_t& p_var)
  {
    TTCN_Logger::begin_event_log2str();
    ((TEMPLATE_ARRAY<T_value_type, T_template_type, array_size,
      index_offset>*)p_var.value)->log();
    return TTCN_Logger::end_event_log2str();
  }
  
  /** variable printing function for port arrays */
  template <typename T_type, unsigned int array_size, int index_offset>
  static CHARSTRING print_port_array(const variable_t& p_var)
  {
    TTCN_Logger::begin_event_log2str();
    ((PORT_ARRAY<T_type, array_size, index_offset>*)p_var.value)->log();
    return TTCN_Logger::end_event_log2str();
  }
  
  /** variable printing function for timer arrays */
  template <typename T_type, unsigned int array_size, int index_offset>
  static CHARSTRING print_timer_array(const variable_t& p_var)
  {
    TTCN_Logger::begin_event_log2str();
    ((TIMER_ARRAY<T_type, array_size, index_offset>*)p_var.value)->log();
    return TTCN_Logger::end_event_log2str();
  }
  
  /** variable printing function for lazy parameters */
  template <typename EXPR_TYPE>
  static CHARSTRING print_lazy_param(const variable_t& p_var)
  {
    TTCN_Logger::begin_event_log2str();
    ((Lazy_Param<EXPR_TYPE>*)p_var.value)->log();
    return TTCN_Logger::end_event_log2str();
  }
  
  //////////////////////////////////////////////////////
  ////// methods called by other debugger classes //////
  //////////////////////////////////////////////////////
  
  /** returns true if the debugger is switched on */
  bool is_on() const { return active; }
  
  /** prints formatted string to the debugger's output stream */
  void print(const char* fmt, ...) const;
  
  /** adds the specified function object pointer to the call stack
    * (only if the debugger is switched on) */
  void add_function(TTCN3_Debug_Function* p_function);
  
  /** adds the specified scope object pointer to the current function's scope list
    * (only if the debugger is switched on and the call stack is not empty) */
  void add_scope(TTCN3_Debug_Scope* p_scope);
  
  /** removes the specified function object pointer from the call stack, if it is
    * the function at the top of the stack */
  void remove_function(TTCN3_Debug_Function* p_function);
  
  /** removes the specified scope object pointer from the current function's scope list
    * (only if the call stack is not empty) */
  void remove_scope(TTCN3_Debug_Scope* p_scope);
  
  /** finds or creates, and returns the variable entry specified by the parameters
    *
    * if the call stack is empty, an entry for a global or component variable is
    * created and stored in the main debugger object (if it doesn't already exist);
    * if the call stack is not empty (and if the debugger is switched on), the 
    * variable entry for a local variable is created and stored by the current function*/
  const variable_t* add_variable(const void* p_value, const char* p_name, const char* p_type,
    CHARSTRING (*p_print_function)(const variable_t&));
  
  /** removes the variable entry for the specified local variable in the current
    * function (only if the call stack is not empty) */
  void remove_variable(const variable_t* p_var);
  
  /** returns the global scope object associated with the specified module */
  const TTCN3_Debug_Scope* get_global_scope(const char* p_module) const;
  
  /** returns the component scope object associated with the specified component type */
  const TTCN3_Debug_Scope* get_component_scope(const char* p_component) const;
  
  /** appends the specified function call snapshot to the end of the snapshot string */
  void add_snapshot(const char* p_snapshot);
  
  /** executes a command received from the user interface */
  void execute_command(debug_command_t p_command, const charstring_list& p_arguments);
};

/** the main debugger object */
extern TTCN3_Debugger ttcn3_debugger;


//////////////////////////////////////////////////////
//////////////// TTCN3_Debug_Scope ///////////////////
//////////////////////////////////////////////////////

/** debugger scope class
  *
  * instantiated at the beginning of every code block in the TTCN-3 code (except
  * for the code blocks of functions), plus one (global scope) instance is created
  * for every module and one (component scope) for every component type 
  * 
  * the class' main purpose is to track which local variables were created in the
  * current code block or to track which of the main debugger object's variables
  * belong to which global or component scope */
class TTCN3_Debug_Scope {
  
  /** list of pointers to local variable entries from the current function object or
    * global or component variable entries from the main debugger object
    * (the elements are not owned)*/
  Vector<const TTCN3_Debugger::variable_t*> variables;
  
public:
  
  /** constructor - lets the current function know of this new scope */
  TTCN3_Debug_Scope();
  
  /** destructor - tells the current function to delete the variable entries listed
    * in this instance */
  ~TTCN3_Debug_Scope();
  
  //////////////////////////////////////////////////////
  ////// methods called from TITAN generated code //////
  //////////////////////////////////////////////////////
  
  /** passes the parameters to the main debugger or current function object to 
    * create and store a variable entry from them, and tracks this new variable
    * by storing a pointer to it
    * (local variables are only created and stored if the debugger is switched on) */
  void add_variable(const void* p_value, const char* p_name, const char* p_type,
    CHARSTRING (*p_print_function)(const TTCN3_Debugger::variable_t&));
  
  //////////////////////////////////////////////////////
  ////// methods called by other debugger classes //////
  //////////////////////////////////////////////////////
  
  /** returns true if there is at least one variable in the scope object */
  bool has_variables() const { return !variables.empty(); }
  
  /** returns the specified variable, if found, otherwise returns NULL */
  const TTCN3_Debugger::variable_t* find_variable(const char* p_name) const;
  
  /** prints the names of variables in this scope that match the specified pattern
    * @param p_filter the mentioned pattern
    * @param p_first true if no variables have been printed yet */
  void list_variables(const char* p_filter, bool& p_first) const;
};


//////////////////////////////////////////////////////
/////////////// TTCN3_Debug_Function /////////////////
//////////////////////////////////////////////////////

/** debugger function class
  *
  * instantiated at the beginning of every function, destroyed when function execution ends
  *
  * tracks all variables created during the function's execution (local variables),
  * including the function's parameters, and stores the function's return value */
class TTCN3_Debug_Function {
  
  /** name of the function, not owned */
  const char* function_name;
  
  /** the TTCN-3 keyword(s) used to define the function ("function", "testcase",
    * "altstep", "template" or "external function"), not owned */
  const char* function_type;
  
  /** name of the module this function is defined in, not owned */
  const char* module_name;
  
  /** names of the function's parameters (in the order of their declaration), owned */
  charstring_list* parameter_names;
  
  /** types (directions) of the function's parameters ("in", "inout" or "out"), owned */
  charstring_list* parameter_types;
  
  /** list of local variables tracked by this object, the array elements are owned */
  Vector<TTCN3_Debugger::variable_t*> variables;
  
  /** list of pointers to the scope objects of code blocks in the function,
    * the elements are not owned 
    * (currently not used for anything) */
  Vector<TTCN3_Debug_Scope*> scopes;
  
  /** pointer to the global scope object, not owned
    * (may be NULL, if the module's global scope is empty) */
  const TTCN3_Debug_Scope* global_scope;
  
  /** pointer to the runs-on component's scope object, not owned
    * (may be NULL, if the component's scope is empty or if the function has no runs-on clause) */
  const TTCN3_Debug_Scope* component_scope;
  
  /** the function's return value (unbound if the return value has not been set yet,
    * or if the function doesn't return anything)
    * 
    * since this is only set right before the function ends, it is only accessible
    * from the destructor */
  CHARSTRING return_value;
  
public:
  
  /** constructor - initializes the instance with the specified parameters,
    * retrieves the global scope and component scope from the main debugger object */
  TTCN3_Debug_Function(const char* p_name, const char* p_type, const char* p_module,
    const charstring_list& p_parameter_names, const charstring_list& p_parameter_types, const char* p_component_name);
  
  /** destructor - frees resources and saves the function's ending snapshot
    * (including the values of 'out' and 'inout' parameters and the return value)
    * in the main debugger object (only if the debugger is switched on) */
  ~TTCN3_Debug_Function();
  
  //////////////////////////////////////////////////////
  ////// methods called from TITAN generated code //////
  //////////////////////////////////////////////////////
  
  /** creates, stores and returns the variable entry of the local variable
    * specified by the parameters (only if the debugger is switched on) */
  const TTCN3_Debugger::variable_t* add_variable(const void* p_value, const char* p_name,
    const char* p_type, CHARSTRING (*p_print_function)(const TTCN3_Debugger::variable_t&));
  
  /** stores the string representation of the value returned by the function */
  void set_return_value(const CHARSTRING& p_value);
  
  /** saves the function's initial snapshot (including the values on 'in' and
    * 'inout' parameters) in the main debugger object
    * (only if the debugger is switched on) */
  void initial_snapshot() const;
  
  //////////////////////////////////////////////////////
  ////// methods called by other debugger classes //////
  //////////////////////////////////////////////////////
  
  /** adds the specified scope object pointer to the function's scope list */
  void add_scope(TTCN3_Debug_Scope* p_scope);
  
  /** removes the specified scope object pointer from the function's scope list,
    * if it is the last scope in the list */
  void remove_scope(TTCN3_Debug_Scope* p_scope);
  
  /** removes the specified variable from the variable list */
  void remove_variable(const TTCN3_Debugger::variable_t* p_var);
  
  /** searches for the variable entry with the specified name in the function's
    * local variable list, the global scope (if any) and the component scope (if any),
    * returns NULL, if the variable was not found */
  const TTCN3_Debugger::variable_t* find_variable(const char* p_name) const;
  
  /** prints the function's type, name and current values of parameters */
  void print_function() const;
  
  /** returns the name of the module the function was defined in */
  const char* get_module_name() const { return module_name; }
  
  /** prints the names of variables specified by the parameters (separated by spaces)
    * handles the D_LIST_VARIABLES debugger command 
    * @param p_scope specifies which scope to print variables from:
    * - "local" - the function's local variables (including variables in code blocks)
    * - "global" - variables in the module's global scope
    * - "comp" or "component" - variables in the function's runs-on component scope
    * - "all" - all variables visible in the function (i.e. all of the above)
    * @param p_filter a pattern to filter variable names further */
  void list_variables(const char* p_scope, const char* p_filter) const;
};

/** This macro stores a function's return value in the current function.
  * The returned value might be an expression, so it is copied to a temporary first,
  * to avoid being evaluated twice. */
#define DEBUGGER_STORE_RETURN_VALUE(tmp, ret_val) \
  (tmp = (ret_val), \
   ttcn3_debugger.set_return_value((TTCN_Logger::begin_event_log2str(), \
                                    tmp.log(), \
                                    TTCN_Logger::end_event_log2str())), \
   tmp)

#endif /* DEBUGGER_HH */

