///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2000-2015 Ericsson Telecom AB
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v1.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v10.html
///////////////////////////////////////////////////////////////////////////////

#ifndef PROFILER_HH
#define PROFILER_HH

#include "Vector.hh"
#include "Types.h"

/** This class performs profiling and code coverage on lines and functions in
  * TTCN-3 code (requires the -z compiler option).
  * Customizable through the configuration file's [PROFILER] section. */
class TTCN3_Profiler {
public:
  
  /** Database entry for one file */
  struct profiler_db_item_t {
    /** Database entry for one line */
    struct profiler_line_data_t {
      /** The line's total execution time */
      double total_time;
      /** The number of times this line was executed */
      int exec_count;
    };
    /** Database entry for one function (including test cases, alt steps, the control part, etc.) */
    struct profiler_function_data_t {
      /** Function name (owned) */
      char* name;
      /** Function starting line */
      int lineno;
      /** The function's total execution time */
      double total_time;
      /** The number of times this function was executed */
      int exec_count;
    };
    /** TTCN-3 File name (relative path, owned) */
    char* filename;
    /** Contains database entries for all the lines in this file (its index is
      * the line number, so there may be empty elements) */
    Vector<profiler_line_data_t> lines;
    /** Contains database entries for all the functions in this file (one entry
      * for each function) */
    Vector<profiler_function_data_t> functions;
  };
  
  /** Constructor */
  TTCN3_Profiler();
  /** Destructor - adds all gathered data to the database file and prints
    * statistics if necessary */
  ~TTCN3_Profiler();
  
  /** Enables or disables profiling - called by the config file parser */
  void set_disable_profiler(boolean p_disable_profiler);
  /** Enables or disables code coverage - called by the config file parser */
  void set_disable_coverage(boolean p_disable_coverage);
  /** Sets the database file name (default is "profiler.db" - called by the config file parser */
  void set_database_filename(const char* p_database_filename);
  /** Enables or disables data aggregation - called by the config file parser */
  void set_aggregate_data(boolean p_aggregate_data);
  /** Sets the statistics file name (default is "profiler.stats" - called by the config file parser */
  void set_stats_filename(const char* p_stats_filename);
  /** Enables or disables the printing of statistics - called by the config file parser */
  void set_disable_stats(boolean p_disable_stats);
  
  /** Returns true if profiling is disabled */
  boolean is_profiler_disabled() const;
  
  /** Deletes the database file if data aggregation is not set */
  void init_data_file();
  /** Adds the data from the database file to the local database */
  void import_data();
  /** Writes the local database to the database file (overwrites the file) */
  void export_data();
  
  /** Calculates and prints statistics from the gathered data */
  void print_stats();
  
  /** Resets data related to the previous location and time (the local database is not changed) */
  void reset();
  /** Returns the current time (in seconds) */
  static double get_time();
  /** Called when a TTCN-3 function starts execution - stores data */
  void enter_function(const char* filename, int lineno, const char* function_name);
  /** Called when a TTCN-3 code line starts execution - stores data */
  void execute_line(const char* filename, int lineno);
  /** Returns the index of a TTCN-3 file's entry in the local database */
  int get_element(const char* filename);
  /** Returns the index of a TTCN-3 function's entry in the database
    * @param element index of the file (where the function is declared)
    * @param lineno function start line */
  int get_function(int element, int lineno);
  /** Creates a new TTCN-3 function entry and inserts it in the database 
    * @param element file entry's index
    * @param lineno function start line
    * @param function_name name of the function */
  void create_function(int element, int lineno, const char* function_name);
  /** Creates TTCN-3 code line entries up to the given line number */
  void create_lines(int element, int lineno);
  /** Adds elapsed time to the specified TTCN-3 code line's total time */
  void add_line_time(double elapsed, int element, int lineno);
  /** Adds elapsed time to the specified TTCN-3 function's total time*/
  void add_function_time(double elapsed, int element, int lineno);
  /** Called when a TTCN-3 function's execution ends - stores data */
  void update_last();
  /** Stores data related to the previous location */
  void set_prev(int stack_len, const char* filename, int lineno);

private:
  /** Profiling is disabled if true */
  boolean disable_profiler;
  /** Code coverage is disabled if true */
  boolean disable_coverage;
  /** Contains the database file name */
  char* database_filename;
  /** If true, data gathered by previous runs will be added to the data gathered
    * in this run */
  boolean aggregate_data;
  /** Contains the statistics file name */
  char* stats_filename;
  /** Statistics will not be calculated and printed if true */
  boolean disable_stats;
  
  /** The time measured at the previous TTCN-3 code line */
  double prev_time;
  /** Name of the TTCN-3 file, where the last executed line is (not owned) */
  const char* prev_file;
  /** The number of the previously executed line */
  int prev_line;
  /** The local database */
  Vector<profiler_db_item_t> profiler_db;
  /** The stack length at the previously executed line */
  int prev_stack_len;
};

/** The global TTCN3_Profiler object
  * 
  * One instance is created in each process (in parallel mode).
  * After construction the configuration file parser may change the profiler's settings.
  * The destructor merges its data with that of other processes (and possibly with previous runs)
  * through the database file. The last destructor (the one in the Host Controller's process)
  * prints the statistics (if enabled). */
extern TTCN3_Profiler ttcn3_prof;

/** Helper class for profiling
  *
  * Its instances depict the current call stack. One instance is created at the start
  * of each TTCN-3 function execution, and it's destroyed at the function's end. */
class TTCN3_Stack_Depth {
public:
  /** Entry for one function call in the call stack */
  struct call_stack_timer_item_t {
    /** Stack length before the function call */
    int stack_len;
    /** File name, where the calling function is declared (not owned) */
    const char* caller_file;
    /** File name, where the called function is declared (not owned)*/
    const char* func_file;
    /** Calling function's start line */
    int caller_line;
    /** Called function's start line */
    int start_line;
    /** Time elapsed in this function call */
    double elapsed;
  };
  
  /** Constructor - increases the stack depth */
  TTCN3_Stack_Depth();
  /** Destructor - decreases the stack depth, updates call times in the profiler */
  ~TTCN3_Stack_Depth();
  
  /** Returns the current stack depth */
  static int depth() { return current_depth; }
  /** Inserts a new function call entry into the call stack database */
  static void add_stack(int stack_len, const char* caller_file, const char* func_file,
    int caller_line, int start_line);
  /** Removes the last entry from the call stack database */
  static void remove_stack();
  /** Adds the elapsed time to all entries in the call stack database */
  static void update_stack_elapsed(double elapsed);
private:
  /** The current stack depth (starts from 0)*/
  static int current_depth;
  /** The call stack database */
  static Vector<call_stack_timer_item_t> call_stack_timer_db;
};

#endif /* PROFILER_HH */

