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

#ifndef DEBUGCOMMANDS_HH
#define DEBUGCOMMANDS_HH

/** list of commands coming from the user interface to the debugger (parameters listed in comments) */

// settings
#define D_SWITCH                  1 // 1, "on" or "off"
#define D_ADD_BREAKPOINT          2 // 2, module name and line number
#define D_REMOVE_BREAKPOINT       3 // 2, module name and line number
#define D_SET_ERROR_BEHAVIOR      4 // 1, "yes" or "no"
#define D_SET_FAIL_BEHAVIOR       5 // 1, "yes" or "no"
#define D_SET_OUTPUT              6 // 1-2, "console", or "file" or "both" + file name
// printing and overwriting data
#define D_SET_COMPONENT           7 // 1, "mtc" or component reference
#define D_PRINT_CALL_STACK        8 // 0
#define D_SET_STACK_LEVEL         9 // 1, stack level
#define D_LIST_VARIABLES         10 // 1-2, "local", "global", "comp" or "all", + optional filter (pattern)
#define D_PRINT_VARIABLE         11 // 1+, list of variable names
#define D_OVERWRITE_VARIABLE     12 // 2, variable name, new value (in module parameter syntax)
#define D_PRINT_SNAPSHOTS        13 // 0
#define D_SET_SNAPSHOT_BEHAVIOR  14 // TBD
// stepping
#define D_STEP_OVER              15 // 0
#define D_STEP_INTO              16 // 0
#define D_STEP_OUT               17 // 0
#define D_RUN_TO_CURSOR          18 // 2, module name and line number
// the halted state
#define D_HALT                   19 // 0
#define D_CONTINUE               20 // 0
#define D_EXIT                   21 // 1, "test" or "all"
// batch files
#define D_BATCH                  22 // 1, batch file name
#define D_SET_HALTING_BATCH_FILE 23 // 1-2, "no", or "yes" + batch file name
// initialization
#define D_SETUP                  24 // 5+, arguments for D_SWITCH, D_SET_OUTPUT, D_ERROR_BEHAVIOR, D_FAIL_BEHAVIOR + any number of D_ADD_BREAKPOINT arguments

#define D_ERROR                   0 // any

/** names of commands in the user interface */

#define D_SWITCH_TEXT "debug"
#define D_ADD_BREAKPOINT_TEXT "daddbp"
#define D_REMOVE_BREAKPOINT_TEXT "drembp"
#define D_SET_ERROR_BEHAVIOR_TEXT "derrcfg"
#define D_SET_FAIL_BEHAVIOR_TEXT "dfailcfg"
#define D_SET_OUTPUT_TEXT "doutput"
#define D_SET_COMPONENT_TEXT "dcomp"
#define D_PRINT_CALL_STACK_TEXT "dprintstack"
#define D_SET_STACK_LEVEL_TEXT "dstacklevel"
#define D_LIST_VARIABLES_TEXT "dlistvar"
#define D_PRINT_VARIABLE_TEXT "dprintvar"
#define D_OVERWRITE_VARIABLE_TEXT "dsetvar"
#define D_PRINT_SNAPSHOTS_TEXT "dprintss"
#define D_SET_SNAPSHOT_BEHAVIOR_TEXT "dsscfg"
#define D_STEP_OVER_TEXT "dstepover"
#define D_STEP_INTO_TEXT "dstepinto"
#define D_STEP_OUT_TEXT "dstepout"
#define D_RUN_TO_CURSOR_TEXT "drunto"
#define D_HALT_TEXT "dhalt"
#define D_CONTINUE_TEXT "dcont"
#define D_EXIT_TEXT "dexit"
#define D_BATCH_TEXT "dbatch"
#define D_SET_HALTING_BATCH_FILE_TEXT "dbatchcfg"

/** debugger return value types */

#define DRET_NOTIFICATION   0
#define DRET_SETTING_CHANGE 1
#define DRET_DATA           2
#define DRET_EXIT_ALL       3

#endif /* DEBUGCOMMANDS_HH */

