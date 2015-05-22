///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2000-2015 Ericsson Telecom AB
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v1.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v10.html
///////////////////////////////////////////////////////////////////////////////

#include "Profiler.hh"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "JSON_Tokenizer.hh"
#include "memory.h"
#include "Runtime.hh"
#include <unistd.h>
#include "Component.hh"

////////////////////////////////////
//////// timeval operations ////////
////////////////////////////////////

/** Reads a timeval value from the given string. The parameter must contain the
  * string representation of a real number with 6 digits after the decimal dot. */
static timeval string2timeval(const char* str)
{
  // read and store the first part (atoi will read until the decimal dot)
  long int sec = atoi(str);
  timeval tv;
  tv.tv_sec = sec;

  do {
    // step over each digit
    sec /= 10;
    ++str;
  }
  while (sec > 9);

  // step over the decimal dot and read the second part of the number
  tv.tv_usec = atoi(str + 1);
  return tv;
}

/** Returns the string representation of a real number (with 6 digits after the
  * decimal dot) equivalent to the timeval parameter. 
  * The returned character pointer needs to be freed. */
static char* timeval2string(timeval tv)
{
  // convert the first part and set the second part to all zeros
  char* str = mprintf("%ld.000000", tv.tv_sec);
  
  // go through each digit of the second part and add them to the zeros in the string
  size_t pos = mstrlen(str) - 1;
  while (tv.tv_usec > 0) {
    str[pos] += tv.tv_usec % 10;
    tv.tv_usec /= 10;
    --pos;
  }
  return str;
}

/** Adds the two timeval parameters together and returns the result. */
static timeval add_timeval(const timeval operand1, const timeval operand2)
{
  timeval tv;
  tv.tv_usec = operand1.tv_usec + operand2.tv_usec;
  tv.tv_sec = operand1.tv_sec + operand2.tv_sec;
  if (tv.tv_usec >= 1000000) {
    ++tv.tv_sec;
    tv.tv_usec -= 1000000;
  }
  return tv;
}

/** Subtracts the second timeval parameter from the first one and returns the result. */
static timeval subtract_timeval(const timeval operand1, const timeval operand2)
{
  timeval tv;
  tv.tv_usec = operand1.tv_usec - operand2.tv_usec;
  tv.tv_sec = operand1.tv_sec - operand2.tv_sec;
  if (tv.tv_usec < 0) {
    --tv.tv_sec;
    tv.tv_usec += 1000000;
  }
  return tv;
}

////////////////////////////////////
////////// TTCN3_Profiler //////////
////////////////////////////////////

TTCN3_Profiler ttcn3_prof;

TTCN3_Profiler::TTCN3_Profiler()
: stopped(FALSE), disable_profiler(FALSE), disable_coverage(FALSE)
, aggregate_data(FALSE), disable_stats(FALSE), stats_flags(STATS_ALL)
{
  database_filename = mcopystr("profiler.db");
  stats_filename = mcopystr("profiler.stats");
  reset();
}

TTCN3_Profiler::~TTCN3_Profiler()
{
  if (!profiler_db.empty() && (!disable_profiler || !disable_coverage)) {
    if (aggregate_data && (TTCN_Runtime::is_single() || TTCN_Runtime::is_hc())) {
      // import the data from the previous run
      import_data();
    }
    if (TTCN_Runtime::is_hc()) {
      // import the data gathered by the other processes (the import function
      // waits for them to finish exporting)
      import_data(MTC_COMPREF);
      for (size_t i = 0; i < ptc_list.size(); ++i) {
        import_data(ptc_list[i]);
      }
    }
    export_data();
    if (!disable_stats && (TTCN_Runtime::is_single() || TTCN_Runtime::is_hc())) {
      print_stats();
    }
  }
  for (size_t i = 0; i < profiler_db.size(); ++i) {
    Free(profiler_db[i].filename);
    for (size_t j = 0; j < profiler_db[i].functions.size(); ++j) {
      Free(profiler_db[i].functions[j].name);
    }
  }
  Free(database_filename);
  Free(stats_filename);
}

void TTCN3_Profiler::start()
{
  if (stopped) {
    set_prev(disable_profiler ? -1 : TTCN3_Stack_Depth::depth(), NULL, -1);
    stopped = FALSE;
  }
}

void TTCN3_Profiler::stop()
{
  if (!stopped) {
    if (NULL != prev_file) {
      // update the previous line's time
      timeval elapsed = subtract_timeval(get_time(), prev_time);
      add_line_time(elapsed, get_element(prev_file), prev_line);
      TTCN3_Stack_Depth::update_stack_elapsed(elapsed);
    }
    stopped = TRUE;
  }
}

void TTCN3_Profiler::set_disable_profiler(boolean p_disable_profiler)
{
  disable_profiler = p_disable_profiler;
}

void TTCN3_Profiler::set_disable_coverage(boolean p_disable_coverage)
{
  disable_coverage = p_disable_coverage;
}

void TTCN3_Profiler::set_database_filename(const char* p_database_filename)
{
  Free(database_filename);
  database_filename = mcopystr(p_database_filename);
}

void TTCN3_Profiler::set_aggregate_data(boolean p_aggregate_data)
{
  aggregate_data = p_aggregate_data;
}

void TTCN3_Profiler::set_stats_filename(const char* p_stats_filename)
{
  Free(stats_filename);
  stats_filename = mcopystr(p_stats_filename);
}

void TTCN3_Profiler::set_disable_stats(boolean p_disable_stats)
{
  disable_stats = p_disable_stats;
}

void TTCN3_Profiler::reset_stats_flags()
{
  stats_flags = 0;
}

void TTCN3_Profiler::add_stats_flags(unsigned int p_flags)
{
  stats_flags |= p_flags;
}

boolean TTCN3_Profiler::is_profiler_disabled() const
{
  return disable_profiler;
}

boolean TTCN3_Profiler::is_running() const
{
  return !stopped;
}

void TTCN3_Profiler::add_ptc(component p_comp_ref)
{
  ptc_list.push_back(p_comp_ref);
}

#define IMPORT_FORMAT_ERROR(cond) \
  if (cond) { \
    TTCN_warning("Database format is invalid. Profiler and/or code coverage data will not be loaded."); \
    return; \
  }

void TTCN3_Profiler::import_data(component p_comp_ref /* = NULL_COMPREF */)
{
  char* file_name = NULL;
  if (NULL_COMPREF == p_comp_ref) {
    // this is the main database file (from the previous run), no suffix needed
    file_name = database_filename;
  }
  else if (MTC_COMPREF == p_comp_ref) {
    // this is the database for the MTC, suffix the file name with "mtc"
    file_name = mprintf("%s.mtc", database_filename);
  }
  else {
    // this is the database for one of the PTCs, suffix the file name with the
    // component reference
    file_name = mprintf("%s.%d", database_filename, p_comp_ref);
  }
  
  // open the file, if it exists
  int file_size = 0;
  FILE* file = fopen(file_name, "r");
  if (NULL != file) {
    // get the file size
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
  }
  while (0 == file_size) {
    if (NULL_COMPREF == p_comp_ref) {
      // no data from the previous run
      return;
    }
    // as for the process-specific database files: keep reading until it appears
    if (NULL != file) {
      fclose(file);
    }
    usleep(1000);
    file = fopen(file_name, "r");
    if (NULL != file) {
      // refresh the file size
      fseek(file, 0, SEEK_END);
      file_size = ftell(file);
    }
  }
  
  // rewind the file (the file pointer has been moved to the end of the file to
  // calculate its size)
  rewind(file);
  
  // read the entire file into a character buffer
  char* buffer = (char*)Malloc(file_size);
  fread(buffer, 1, file_size, file);
  fclose(file);
  
  if (NULL_COMPREF != p_comp_ref) {
    // the process-specific database file is no longer needed
    remove(file_name);
    Free(file_name);
  }
  
  // initialize a JSON tokenizer with the buffer
  JSON_Tokenizer json(buffer, file_size);
  Free(buffer);
  
  // attempt to read tokens from the buffer
  // if the format is invalid, abort the importing process
  json_token_t token = JSON_TOKEN_NONE;
  char* value = NULL;
  size_t value_len = 0;
  
  // start of main array
  json.get_next_token(&token, NULL, NULL);
  IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_START != token);
  
  // read objects (one for each TTCN-3 file), until the main array end mark is reached
  json.get_next_token(&token, NULL, NULL);
  while (JSON_TOKEN_OBJECT_START == token) {
    size_t file_index = 0;
    
    // file name:
    json.get_next_token(&token, &value, &value_len);
    IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 4 ||
      0 != strncmp(value, "file", value_len));
    
    // read the file name and see if its record already exists
    json.get_next_token(&token, &value, &value_len);
    IMPORT_FORMAT_ERROR(JSON_TOKEN_STRING != token);
    for (file_index = 0; file_index < profiler_db.size(); ++file_index) {
      if (strlen(profiler_db[file_index].filename) == value_len - 2 &&
          0 == strncmp(profiler_db[file_index].filename, value + 1, value_len - 2)) {
        break;
      }
    }
    
    // insert a new element if the file was not found
    if (profiler_db.size() == file_index) {
      profiler_db_item_t item;
      item.filename = mcopystrn(value + 1, value_len - 2);
      profiler_db.push_back(item);
    }
    
    // functions:
    json.get_next_token(&token, &value, &value_len);
    IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 9 ||
      0 != strncmp(value, "functions", value_len));
    
    // read and store the functions (an array of objects, same as before)
    json.get_next_token(&token, NULL, NULL);
    IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_START != token);
    json.get_next_token(&token, NULL, NULL);
    while (JSON_TOKEN_OBJECT_START == token) {
      size_t function_index = 0;
      
      // function name:
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 4 ||
        0 != strncmp(value, "name", value_len));
      
      // read the function name, it will be checked later
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_STRING != token);
      char* function_name = mcopystrn(value + 1, value_len - 2);
      
      // function start line:
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 10 ||
        0 != strncmp(value, "start line", value_len));
      
      // read the start line and check if the function already exists
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
      int start_line = atoi(value);
      for (function_index = 0; function_index < profiler_db[file_index].functions.size(); ++function_index) {
        if (profiler_db[file_index].functions[function_index].lineno == start_line &&
            0 == strcmp(profiler_db[file_index].functions[function_index].name, function_name)) {
          break;
        }
      }
      
      // insert a new element if the function was not found
      if (profiler_db[file_index].functions.size() == function_index) {
        profiler_db_item_t::profiler_function_data_t func_data;
        func_data.name = function_name;
        func_data.lineno = start_line;
        func_data.exec_count = 0;
        func_data.total_time.tv_sec = 0;
        func_data.total_time.tv_usec = 0;
        profiler_db[file_index].functions.push_back(func_data);
      }
      
      // function execution count:
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 15 ||
        0 != strncmp(value, "execution count", value_len));

      // read the execution count and add it to the current data
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
      profiler_db[file_index].functions[function_index].exec_count += atoi(value);

      // total function execution time:
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 10 ||
        0 != strncmp(value, "total time", value_len));

      // read the total time and add it to the current data
      // note: the database contains a real number, this needs to be split into 2 integers
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
      profiler_db[file_index].functions[function_index].total_time = add_timeval(
        profiler_db[file_index].functions[function_index].total_time, string2timeval(value));

      // end of the function's object
      json.get_next_token(&token, NULL, NULL);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token);
      
      // read the next token (either the start of another object or the function array end)
      json.get_next_token(&token, NULL, NULL);
    }
    
    // function array end
    IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_END != token);
    
    // lines:
    json.get_next_token(&token, &value, &value_len);
    IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 5 ||
      0 != strncmp(value, "lines", value_len));
    
    // read and store the lines (an array of objects, same as before)
    json.get_next_token(&token, NULL, NULL);
    IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_START != token);
    json.get_next_token(&token, NULL, NULL);
    while (JSON_TOKEN_OBJECT_START == token) {
      int line_index = 0;
      
      // line number:
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 6 ||
        0 != strncmp(value, "number", value_len));
      
      // read the line number and check if the line already exists
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
      int lineno = atoi(value);
      IMPORT_FORMAT_ERROR(lineno < 0);
      line_index = get_line(file_index, lineno);
      if (-1 == line_index) {
        create_line(file_index, lineno);
        line_index = profiler_db[file_index].lines.size() - 1;
      }
      
      // line execution count:
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 15 ||
        0 != strncmp(value, "execution count", value_len));

      // read the execution count and add it to the current data
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
      profiler_db[file_index].lines[line_index].exec_count += atoi(value);
      
      // total line execution time:
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 10 ||
        0 != strncmp(value, "total time", value_len));

      // read the total time and add it to the current data
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
      profiler_db[file_index].lines[line_index].total_time = add_timeval(
        profiler_db[file_index].lines[line_index].total_time, string2timeval(value));

      // end of the line's object
      json.get_next_token(&token, NULL, NULL);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token);
      
      // read the next token (either the start of another object or the line array end)
      json.get_next_token(&token, NULL, NULL);
    }
    
    // line array end
    IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_END != token);
    
    // end of the file's object
    json.get_next_token(&token, NULL, NULL);
    IMPORT_FORMAT_ERROR(JSON_TOKEN_OBJECT_END != token);
    
    // read the next token (either the start of another object or the main array end)
    json.get_next_token(&token, NULL, NULL);
  }
  
  // main array end
  IMPORT_FORMAT_ERROR(JSON_TOKEN_ARRAY_END != token);
}

void TTCN3_Profiler::export_data()
{
  char* file_name = NULL;
  if (TTCN_Runtime::is_single() || TTCN_Runtime::is_hc()) {
    // this is the main database file, no suffix needed
    file_name = database_filename;
  }
  else if (TTCN_Runtime::is_mtc()) {
    // this is the database for the MTC, suffix the file name with "mtc"
    file_name = mprintf("%s.mtc", database_filename);
  }
  else {
    // this is the database for one of the PTCs, suffix the file name with the
    // component reference
    file_name = mprintf("%s.%d", database_filename, (component)self);
  }
  
  // check whether the file can be opened for writing
  FILE* file = fopen(file_name, "w");
  if (NULL == file) {
    TTCN_warning("Could not open file '%s' for writing. Profiler and/or code coverage "
      "data will not be saved.", file_name);
    if (file_name != database_filename) {
      Free(file_name);
    }
    return;
  }
  
  if (file_name != database_filename) {
    Free(file_name);
  }
  
  // use the JSON tokenizer to create a JSON document from the database
  JSON_Tokenizer json(true);
  
  // main array, contains an element for each file
  json.put_next_token(JSON_TOKEN_ARRAY_START, NULL);
  for (size_t i = 0; i < profiler_db.size(); ++i) {
    
    // each file's data is stored in an object
    json.put_next_token(JSON_TOKEN_OBJECT_START, NULL);
    
    // store the file name
    json.put_next_token(JSON_TOKEN_NAME, "file");
    char* file_name_str = mprintf("\"%s\"", profiler_db[i].filename);
    json.put_next_token(JSON_TOKEN_STRING, file_name_str);
    Free(file_name_str);
    
    // store the function data in an array (one element for each function)
    json.put_next_token(JSON_TOKEN_NAME, "functions");
    json.put_next_token(JSON_TOKEN_ARRAY_START, NULL);
    for (size_t j = 0; j < profiler_db[i].functions.size(); ++j) {
      // only store functions with actual data
      if ((0 != profiler_db[i].functions[j].total_time.tv_sec &&
           0 != profiler_db[i].functions[j].total_time.tv_usec) ||
          0 != profiler_db[i].functions[j].exec_count) {
      
        // the data is stored in an object for each function
        json.put_next_token(JSON_TOKEN_OBJECT_START, NULL);

        // store the function name
        json.put_next_token(JSON_TOKEN_NAME, "name");
        char* func_name_str = mprintf("\"%s\"", profiler_db[i].functions[j].name);
        json.put_next_token(JSON_TOKEN_STRING, func_name_str);
        Free(func_name_str);

        // store the function start line
        json.put_next_token(JSON_TOKEN_NAME, "start line");
        char* start_line_str = mprintf("%d", profiler_db[i].functions[j].lineno);
        json.put_next_token(JSON_TOKEN_NUMBER, start_line_str);
        Free(start_line_str);

        // store the function execution count
        json.put_next_token(JSON_TOKEN_NAME, "execution count");
        char* exec_count_str = mprintf("%d", disable_coverage ? 0 :
          profiler_db[i].functions[j].exec_count);
        json.put_next_token(JSON_TOKEN_NUMBER, exec_count_str);
        Free(exec_count_str);

        // store the function's total execution time
        json.put_next_token(JSON_TOKEN_NAME, "total time");
        if (disable_profiler) {
          json.put_next_token(JSON_TOKEN_NUMBER, "0.000000");
        }
        else {
          char* total_time_str = timeval2string(profiler_db[i].functions[j].total_time);
          json.put_next_token(JSON_TOKEN_NUMBER, total_time_str);
          Free(total_time_str);
        }

        // end of function object
        json.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
      }
    }
    
    // end of function data array
    json.put_next_token(JSON_TOKEN_ARRAY_END, NULL);
    
    // store the line data in an array (one element for each line with useful data)
    json.put_next_token(JSON_TOKEN_NAME, "lines");
    json.put_next_token(JSON_TOKEN_ARRAY_START, NULL);
    for (size_t j = 0; j < profiler_db[i].lines.size(); ++j) {
      // only store lines with actual data
      if ((0 != profiler_db[i].lines[j].total_time.tv_sec &&
           0 != profiler_db[i].lines[j].total_time.tv_usec) ||
          0 != profiler_db[i].lines[j].exec_count) {

        // store line data in an object
        json.put_next_token(JSON_TOKEN_OBJECT_START, NULL);

        // store the line number
        json.put_next_token(JSON_TOKEN_NAME, "number");
        char* line_number_str = mprintf("%d", profiler_db[i].lines[j].lineno);
        json.put_next_token(JSON_TOKEN_NUMBER, line_number_str);
        Free(line_number_str);

        // store the line execution count
        json.put_next_token(JSON_TOKEN_NAME, "execution count");
        char* exec_count_str = mprintf("%d", disable_coverage ? 0 :
          profiler_db[i].lines[j].exec_count);
        json.put_next_token(JSON_TOKEN_NUMBER, exec_count_str);
        Free(exec_count_str);

        // store the line's total execution time
        json.put_next_token(JSON_TOKEN_NAME, "total time");
        if (disable_profiler) {
          json.put_next_token(JSON_TOKEN_NUMBER, "0.000000");
        }
        else {
          char* total_time_str = timeval2string(profiler_db[i].lines[j].total_time);
          json.put_next_token(JSON_TOKEN_NUMBER, total_time_str);
          Free(total_time_str);
        }

        // end of this line's object
        json.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
      }
    }
    
    // end of line data array
    json.put_next_token(JSON_TOKEN_ARRAY_END, NULL);
    
    // end of this file's object
    json.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
  }
  
  // end of main array
  json.put_next_token(JSON_TOKEN_ARRAY_END, NULL);
  
  // write the JSON document into the file
  fprintf(file, "%s\n", json.get_buffer());
  fclose(file);
}

// Structure for one code line or function, used by print_stats for sorting
struct stats_data_t {
  const char* filename; // not owned
  const char* funcname; // not owned, NULL for code lines that don't start a function
  int lineno;
  timeval total_time;
  int exec_count;
};

// Compare function for sorting stats data based on total execution time (descending)
int stats_data_cmp_time(const void* p_left, const void* p_right) {
  const stats_data_t* p_left_data = (stats_data_t*)p_left;
  const stats_data_t* p_right_data = (stats_data_t*)p_right;
  if (p_left_data->total_time.tv_sec > p_right_data->total_time.tv_sec) return -1;
  if (p_left_data->total_time.tv_sec < p_right_data->total_time.tv_sec) return 1;
  if (p_left_data->total_time.tv_usec > p_right_data->total_time.tv_usec) return -1;
  if (p_left_data->total_time.tv_usec < p_right_data->total_time.tv_usec) return 1;
  return 0;
}

// Compare function for sorting stats data based on execution count (descending)
int stats_data_cmp_count(const void* p_left, const void* p_right) {
  return ((stats_data_t*)p_right)->exec_count - ((stats_data_t*)p_left)->exec_count;
}

// Compare function for sorting stats data based on total time per execution count (descending)
int stats_data_cmp_avg(const void* p_left, const void* p_right) {
  const stats_data_t* p_left_data = (stats_data_t*)p_left;
  const stats_data_t* p_right_data = (stats_data_t*)p_right;
  double left_time = p_left_data->total_time.tv_sec + p_left_data->total_time.tv_usec / 1000000.0;
  double right_time = p_right_data->total_time.tv_sec + p_right_data->total_time.tv_usec / 1000000.0;
  double diff = (right_time / p_right_data->exec_count) - (left_time / p_left_data->exec_count);
  if (diff < 0) return -1;
  if (diff > 0) return 1;
  return 0;
}

void TTCN3_Profiler::print_stats() 
{
  if (profiler_db.empty()) {
    return;
  }
  
  // title
  char* title_str = mprintf(
    "##################################################\n"
    "%s## TTCN-3 %s%s%sstatistics ##%s\n"
    "##################################################\n\n\n"
    , disable_profiler ? "#######" : (disable_coverage ? "#########" : "")
    , disable_profiler ? "" : "profiler "
    , (disable_profiler || disable_coverage) ? "" : "and "
    , disable_coverage ? "" : "code coverage "
    , disable_profiler ? "######" : (disable_coverage ? "#########" : ""));
  
  char* line_func_count_str = NULL;
  if (stats_flags & STATS_NUMBER_OF_LINES) {
    line_func_count_str = mcopystr(
      "--------------------------------------\n"
      "- Number of code lines and functions -\n"
      "--------------------------------------\n");
  }
  
  // line data
  char* line_data_str = NULL;
  if (stats_flags & STATS_LINE_DATA_RAW) {
    line_data_str = mprintf(
      "-------------------------------------------------\n"
      "%s- Code line data (%s%s%s) -%s\n"
      "-------------------------------------------------\n"
      , disable_profiler ? "-------" : (disable_coverage ? "---------" : "")
      , disable_profiler ? "" : "total time"
      , (disable_profiler || disable_coverage) ? "" : " / "
      , disable_coverage ? "" : "execution count"
      , disable_profiler ? "------" : (disable_coverage ? "---------" : ""));
  }
  
  // average time / exec count for lines
  char* line_avg_str = NULL;
  if (!disable_coverage && !disable_profiler && (stats_flags & STATS_LINE_AVG_RAW)) {
    line_avg_str = mcopystr(
      "-------------------------------------------\n"
      "- Average time / execution for code lines -\n"
      "-------------------------------------------\n");
  }
  
  // function data
  char* func_data_str = NULL;
  if (stats_flags & STATS_FUNC_DATA_RAW) {
    func_data_str = mprintf(
      "------------------------------------------------\n"
      "%s- Function data (%s%s%s) -%s\n"
      "------------------------------------------------\n"
      , disable_profiler ? "-------" : (disable_coverage ? "---------" : "")
      , disable_profiler ? "" : "total time"
      , (disable_profiler || disable_coverage) ? "" : " / "
      , disable_coverage ? "" : "execution count"
      , disable_profiler ? "------" : (disable_coverage ? "---------" : ""));
  }
  
  // average time / exec count for functions
  char* func_avg_str = NULL;
  if (!disable_coverage && !disable_profiler && (stats_flags & STATS_FUNC_AVG_RAW)) {
    func_avg_str = mcopystr(
      "------------------------------------------\n"
      "- Average time / execution for functions -\n"
      "------------------------------------------\n");
  }
  
  char* line_time_sorted_mod_str = NULL;
  if (!disable_profiler && (stats_flags & STATS_LINE_TIMES_SORTED_BY_MOD)) {
    line_time_sorted_mod_str = mcopystr(
      "------------------------------------------------\n"
      "- Total time of code lines, sorted, per module -\n"
      "------------------------------------------------\n");
  }
  
  char* line_count_sorted_mod_str = NULL;
  if (!disable_coverage && (stats_flags & STATS_LINE_COUNT_SORTED_BY_MOD)) {
    line_count_sorted_mod_str = mcopystr(
      "-----------------------------------------------------\n"
      "- Execution count of code lines, sorted, per module -\n"
      "-----------------------------------------------------\n");
  }
  
  char* line_avg_sorted_mod_str = NULL;
  if (!disable_profiler && !disable_coverage && (stats_flags & STATS_LINE_AVG_SORTED_BY_MOD)) {
    line_avg_sorted_mod_str = mcopystr(
      "--------------------------------------------------------------\n"
      "- Average time / execution of code lines, sorted, per module -\n"
      "--------------------------------------------------------------\n");
  }
  
  char* line_time_sorted_tot_str = NULL;
  if (!disable_profiler && (stats_flags & STATS_LINE_TIMES_SORTED_TOTAL)) {
    line_time_sorted_tot_str = mcopystr(
      "-------------------------------------------\n"
      "- Total time of code lines, sorted, total -\n"
      "-------------------------------------------\n");
  }
  
  char* line_count_sorted_tot_str = NULL;
  if (!disable_coverage && (stats_flags & STATS_LINE_COUNT_SORTED_TOTAL)) {
    line_count_sorted_tot_str = mcopystr(
      "------------------------------------------------\n"
      "- Execution count of code lines, sorted, total -\n"
      "------------------------------------------------\n");
  }
  
  char* line_avg_sorted_tot_str = NULL;
  if (!disable_profiler && !disable_coverage && (stats_flags & STATS_LINE_AVG_SORTED_TOTAL)) {
    line_avg_sorted_tot_str = mcopystr(
      "---------------------------------------------------------\n"
      "- Average time / execution of code lines, sorted, total -\n"
      "---------------------------------------------------------\n");
  }
  
  char* func_time_sorted_mod_str = NULL;
  if (!disable_profiler && (stats_flags & STATS_FUNC_TIMES_SORTED_BY_MOD)) {
    func_time_sorted_mod_str = mcopystr(
      "-----------------------------------------------\n"
      "- Total time of functions, sorted, per module -\n"
      "-----------------------------------------------\n");
  }
  
  char* func_count_sorted_mod_str = NULL;
  if (!disable_coverage && (stats_flags & STATS_FUNC_COUNT_SORTED_BY_MOD)) {
    func_count_sorted_mod_str = mcopystr(
      "----------------------------------------------------\n"
      "- Execution count of functions, sorted, per module -\n"
      "----------------------------------------------------\n");
  }
  
  char* func_avg_sorted_mod_str = NULL;
  if (!disable_profiler && !disable_coverage && (stats_flags & STATS_FUNC_AVG_SORTED_BY_MOD)) {
    func_avg_sorted_mod_str = mcopystr(
      "-------------------------------------------------------------\n"
      "- Average time / execution of functions, sorted, per module -\n"
      "-------------------------------------------------------------\n");
  }
  
  char* func_time_sorted_tot_str = NULL;
  if (!disable_profiler && (stats_flags & STATS_FUNC_TIMES_SORTED_TOTAL)) {
    func_time_sorted_tot_str = mcopystr(
      "------------------------------------------\n"
      "- Total time of functions, sorted, total -\n"
      "------------------------------------------\n");
  }
  
  char* func_count_sorted_tot_str = NULL;
  if (!disable_coverage && (stats_flags & STATS_FUNC_COUNT_SORTED_TOTAL)) {
    func_count_sorted_tot_str = mcopystr(
      "-----------------------------------------------\n"
      "- Execution count of functions, sorted, total -\n"
      "-----------------------------------------------\n");
  }
  
  char* func_avg_sorted_tot_str = NULL;
  if (!disable_profiler && !disable_coverage && (stats_flags & STATS_FUNC_AVG_SORTED_TOTAL)) {
    func_avg_sorted_tot_str = mcopystr(
      "--------------------------------------------------------\n"
      "- Average time / execution of functions, sorted, total -\n"
      "--------------------------------------------------------\n");
  }
  
  char* line_time_sorted_top10_str = NULL;
  if (!disable_profiler && (stats_flags & STATS_TOP10_LINE_TIMES)) {
    line_time_sorted_top10_str = mcopystr(
      "------------------------------------\n"
      "- Total time of code lines, top 10 -\n"
      "------------------------------------\n");
  }
  
  char* line_count_sorted_top10_str = NULL;
  if (!disable_coverage && (stats_flags & STATS_TOP10_LINE_COUNT)) {
    line_count_sorted_top10_str = mcopystr(
      "-----------------------------------------\n"
      "- Execution count of code lines, top 10 -\n"
      "-----------------------------------------\n");
  }
  
  char* line_avg_sorted_top10_str = NULL;
  if (!disable_profiler && !disable_coverage && (stats_flags & STATS_TOP10_LINE_AVG)) {
    line_avg_sorted_top10_str = mcopystr(
      "--------------------------------------------------\n"
      "- Average time / execution of code lines, top 10 -\n"
      "--------------------------------------------------\n");
  }
  
  char* func_time_sorted_top10_str = NULL;
  if (!disable_profiler && (stats_flags & STATS_TOP10_FUNC_TIMES)) {
    func_time_sorted_top10_str = mcopystr(
      "-----------------------------------\n"
      "- Total time of functions, top 10 -\n"
      "-----------------------------------\n");
  }
  
  char* func_count_sorted_top10_str = NULL;
  if (!disable_coverage && (stats_flags & STATS_TOP10_FUNC_COUNT)) {
    func_count_sorted_top10_str = mcopystr(
      "----------------------------------------\n"
      "- Execution count of functions, top 10 -\n"
      "----------------------------------------\n");
  }
  
  char* func_avg_sorted_top10_str = NULL;
  if (!disable_profiler && !disable_coverage && (stats_flags & STATS_TOP10_FUNC_AVG)) {
    func_avg_sorted_top10_str = mcopystr(
      "-------------------------------------------------\n"
      "- Average time / execution of functions, top 10 -\n"
      "-------------------------------------------------\n");
  }
  
  char* unused_lines_str = NULL;
  char* unused_func_str = NULL;
  if (!disable_coverage && (stats_flags & STATS_UNUSED_LINES)) {
    unused_lines_str = mcopystr(
      "---------------------\n"
      "- Unused code lines -\n"
      "---------------------\n");
  }
  if (!disable_coverage && (stats_flags & STATS_UNUSED_FUNC)) {
    unused_func_str = mcopystr(
      "--------------------\n"
      "- Unused functions -\n"
      "--------------------\n");
  }
  
  // variables for counting totals, and for determining the amount of unused lines/functions
  size_t total_code_lines = 0;
  size_t total_functions = 0;
  size_t used_code_lines = 0;
  size_t used_functions = 0;
  
  // cached sizes of statistics data segments, needed to determine whether a separator
  // is needed or not
  size_t line_data_str_len = mstrlen(line_data_str);
  size_t func_data_str_len = mstrlen(func_data_str);
  size_t unused_lines_str_len = mstrlen(unused_lines_str);
  size_t unused_func_str_len = mstrlen(unused_func_str);
  size_t line_avg_str_len = mstrlen(line_avg_str);
  size_t func_avg_str_len = mstrlen(func_avg_str);
  
  // cycle through the database and gather the necessary data
  for (size_t i = 0; i < profiler_db.size(); ++i) {
    if (i > 0) {
      // add separators between files (only add them if the previous file actually added something)
      if ((stats_flags & STATS_LINE_DATA_RAW) && line_data_str_len != mstrlen(line_data_str)) {
        line_data_str = mputstr(line_data_str, "-------------------------------------------------\n");
        line_data_str_len = mstrlen(line_data_str);
      }
      if ((stats_flags & STATS_FUNC_DATA_RAW) && func_data_str_len != mstrlen(func_data_str)) {
        func_data_str = mputstr(func_data_str, "------------------------------------------------\n");
        func_data_str_len = mstrlen(func_data_str);
      }
      if (!disable_coverage) {
        if ((stats_flags & STATS_UNUSED_LINES) && unused_lines_str_len != mstrlen(unused_lines_str)) {
          unused_lines_str = mputstr(unused_lines_str, "---------------------\n");
          unused_lines_str_len = mstrlen(unused_lines_str);
        }
        if ((stats_flags & STATS_UNUSED_FUNC) && unused_func_str_len != mstrlen(unused_func_str)) {
          unused_func_str = mputstr(unused_func_str, "--------------------\n");
          unused_func_str_len = mstrlen(unused_func_str);
        }
        if (!disable_profiler) {
          if ((stats_flags & STATS_LINE_AVG_RAW) && line_avg_str_len != mstrlen(line_avg_str)) {
            line_avg_str = mputstr(line_avg_str, "-------------------------------------------\n");
            line_avg_str_len = mstrlen(line_avg_str);
          }
          if ((stats_flags & STATS_FUNC_AVG_RAW) && func_avg_str_len != mstrlen(func_avg_str)) {
            func_avg_str = mputstr(func_avg_str, "------------------------------------------\n");
            func_avg_str_len = mstrlen(func_avg_str);
          }
        }
      }
    }
    
    // lines
    for (size_t j = 0; j < profiler_db[i].lines.size(); ++j) {
      // line specification (including function name for the function's start line)
      char* line_spec_str = mprintf("%s:%d", profiler_db[i].filename,
        profiler_db[i].lines[j].lineno);
      int func = get_function(i, profiler_db[i].lines[j].lineno);
      if (-1 != func) {
        line_spec_str = mputprintf(line_spec_str, " [%s]", profiler_db[i].functions[func].name);
      }
      line_spec_str = mputstrn(line_spec_str, "\n", 1);
      
      if (disable_coverage || 0 != profiler_db[i].lines[j].exec_count) {
        if (!disable_profiler) {
          if (stats_flags & STATS_LINE_DATA_RAW) {
            char* total_time_str = timeval2string(profiler_db[i].lines[j].total_time);
            line_data_str = mputprintf(line_data_str, "%ss", total_time_str);
            Free(total_time_str);
          }
          if (!disable_coverage) {
            if (stats_flags & STATS_LINE_DATA_RAW) {
              line_data_str = mputstrn(line_data_str, "\t/\t", 3);
            }
            if (stats_flags & STATS_LINE_AVG_RAW) {
              double avg = (profiler_db[i].lines[j].total_time.tv_sec +
                profiler_db[i].lines[j].total_time.tv_usec / 1000000.0) /
                profiler_db[i].lines[j].exec_count;
              char* total_time_str = timeval2string(profiler_db[i].lines[j].total_time);
              line_avg_str = mputprintf(line_avg_str, "%.6lfs\t(%ss / %d)", 
                avg, total_time_str, profiler_db[i].lines[j].exec_count);
              Free(total_time_str);
            }
          }
        }
        if (!disable_coverage && (stats_flags & STATS_LINE_DATA_RAW)) {
          line_data_str = mputprintf(line_data_str, "%d", profiler_db[i].lines[j].exec_count);
        }

        // add the line spec string to the other strings
        if (stats_flags & STATS_LINE_DATA_RAW) {
          line_data_str = mputprintf(line_data_str, "\t%s", line_spec_str);
        }
        if (!disable_profiler && !disable_coverage && (stats_flags & STATS_LINE_AVG_RAW)) {
          line_avg_str = mputprintf(line_avg_str, "\t%s", line_spec_str);
        }
        ++used_code_lines;
      }
      else if (stats_flags & STATS_UNUSED_LINES) {
        // unused line
        unused_lines_str = mputstr(unused_lines_str, line_spec_str);
      }
      Free(line_spec_str);
    }
    
    // functions
    for (size_t j = 0; j < profiler_db[i].functions.size(); ++j) {
      // functions specification
      char* func_spec_str = mprintf("%s:%d [%s]\n", profiler_db[i].filename,
        profiler_db[i].functions[j].lineno, profiler_db[i].functions[j].name);

      if (disable_coverage || 0 != profiler_db[i].functions[j].exec_count) {
        if (!disable_profiler) {
          if (stats_flags & STATS_FUNC_DATA_RAW) {
            char* total_time_str = timeval2string(profiler_db[i].functions[j].total_time);
            func_data_str = mputprintf(func_data_str, "%ss", total_time_str);
            Free(total_time_str);
          }
          if (!disable_coverage) {
            if (stats_flags & STATS_FUNC_DATA_RAW) {
              func_data_str = mputstrn(func_data_str, "\t/\t", 3);
            }
            if (stats_flags & STATS_FUNC_AVG_RAW) {
              double avg = (profiler_db[i].functions[j].total_time.tv_sec +
                profiler_db[i].functions[j].total_time.tv_usec / 1000000.0) /
                profiler_db[i].functions[j].exec_count;
              char* total_time_str = timeval2string(profiler_db[i].functions[j].total_time);
              func_avg_str = mputprintf(func_avg_str, "%.6lfs\t(%ss / %d)", 
                avg, total_time_str, profiler_db[i].functions[j].exec_count);
              Free(total_time_str);
            }
          }
        }
        if (!disable_coverage && (stats_flags & STATS_FUNC_DATA_RAW)) {
          func_data_str = mputprintf(func_data_str, "%d", profiler_db[i].functions[j].exec_count);
        }

        // add the line spec string to the other strings
        if (stats_flags & STATS_FUNC_DATA_RAW) {
          func_data_str = mputprintf(func_data_str, "\t%s", func_spec_str);
        }
        if (!disable_profiler && !disable_coverage && (stats_flags & STATS_FUNC_AVG_RAW)) {
          func_avg_str = mputprintf(func_avg_str, "\t%s", func_spec_str);
        }
        
        ++used_functions;
      }
      else if (stats_flags & STATS_UNUSED_FUNC) {
        // unused function
        unused_func_str = mputprintf(unused_func_str, func_spec_str);
      }
      Free(func_spec_str);
    }
    
    // number of lines and functions
    if (stats_flags & STATS_NUMBER_OF_LINES) {
      line_func_count_str = mputprintf(line_func_count_str, "%s:\t%lu lines,\t%lu functions\n",
         profiler_db[i].filename, profiler_db[i].lines.size(), profiler_db[i].functions.size());
    }
    total_code_lines += profiler_db[i].lines.size();
    total_functions += profiler_db[i].functions.size();
  }
  if (stats_flags & STATS_NUMBER_OF_LINES) {
    line_func_count_str = mputprintf(line_func_count_str,
      "--------------------------------------\n"
      "Total:\t%lu lines,\t%lu functions\n", total_code_lines, total_functions);
  }
  
  if (stats_flags & (STATS_TOP10_ALL_DATA | STATS_ALL_DATA_SORTED)) {
    // copy code line and function info into stats_data_t containers for sorting
    stats_data_t* code_line_stats = (stats_data_t*)Malloc(used_code_lines * sizeof(stats_data_t));
    stats_data_t* function_stats = (stats_data_t*)Malloc(used_functions * sizeof(stats_data_t));
    int line_index = 0;
    int func_index = 0;

    for (size_t i = 0; i < profiler_db.size(); ++i) {
      for (size_t j = 0; j < profiler_db[i].lines.size(); ++j) {
        if (disable_coverage || 0 != profiler_db[i].lines[j].exec_count) {
          code_line_stats[line_index].filename = profiler_db[i].filename;
          code_line_stats[line_index].funcname = NULL;
          code_line_stats[line_index].lineno = profiler_db[i].lines[j].lineno;
          code_line_stats[line_index].total_time = profiler_db[i].lines[j].total_time;
          code_line_stats[line_index].exec_count = profiler_db[i].lines[j].exec_count;
          int func = get_function(i, profiler_db[i].lines[j].lineno);
          if (-1 != func) {
            code_line_stats[line_index].funcname = profiler_db[i].functions[func].name;
          }
          ++line_index;
        }
      }
      for (size_t j = 0; j < profiler_db[i].functions.size(); ++j) {
        if (disable_coverage || 0 != profiler_db[i].functions[j].exec_count) {
          function_stats[func_index].filename = profiler_db[i].filename;
          function_stats[func_index].funcname = profiler_db[i].functions[j].name;
          function_stats[func_index].lineno = profiler_db[i].functions[j].lineno;
          function_stats[func_index].total_time = profiler_db[i].functions[j].total_time;
          function_stats[func_index].exec_count = profiler_db[i].functions[j].exec_count;
          ++func_index;
        }
      }
    }

    if (!disable_profiler) {
      // sort the code lines and functions by total time
      qsort(code_line_stats, used_code_lines, sizeof(stats_data_t), &stats_data_cmp_time);
      qsort(function_stats, used_functions, sizeof(stats_data_t), &stats_data_cmp_time);

      if (stats_flags & (STATS_LINE_TIMES_SORTED_TOTAL | STATS_TOP10_LINE_TIMES)) {
        // cycle through the sorted code lines and gather the necessary data
        for (size_t i = 0; i < used_code_lines; ++i) {
          char* total_time_str = timeval2string(code_line_stats[i].total_time);
          char* the_data = mprintf("%ss\t%s:%d", total_time_str,
            code_line_stats[i].filename, code_line_stats[i].lineno);
          Free(total_time_str);
          if (NULL != code_line_stats[i].funcname) {
            the_data = mputprintf(the_data, " [%s]", code_line_stats[i].funcname);
          }
          the_data = mputstrn(the_data, "\n", 1);
          if (stats_flags & STATS_LINE_TIMES_SORTED_TOTAL) {
            line_time_sorted_tot_str = mputstr(line_time_sorted_tot_str, the_data);
          }
          if (i < 10 && (stats_flags & STATS_TOP10_LINE_TIMES)) {
            line_time_sorted_top10_str = mputprintf(line_time_sorted_top10_str,
              "%2lu.\t%s", i + 1, the_data);
          }
          Free(the_data);
        }
      }

      if (stats_flags & (STATS_FUNC_TIMES_SORTED_TOTAL | STATS_TOP10_FUNC_TIMES)) {
        // cycle through the sorted functions and gather the necessary data
        for (size_t i = 0; i < used_functions; ++i) {
          char* total_time_str = timeval2string(function_stats[i].total_time);
          char* the_data = mprintf("%ss\t%s:%d [%s]\n", total_time_str,
            function_stats[i].filename, function_stats[i].lineno, function_stats[i].funcname);
          Free(total_time_str);
          if (stats_flags & STATS_FUNC_TIMES_SORTED_TOTAL) {
            func_time_sorted_tot_str = mputstr(func_time_sorted_tot_str, the_data);
          }
          if (i < 10 && (stats_flags & STATS_TOP10_FUNC_TIMES)) {
            func_time_sorted_top10_str = mputprintf(func_time_sorted_top10_str,
              "%2lu.\t%s", i + 1, the_data);
          }
          Free(the_data);
        }
      }

      if (stats_flags & (STATS_LINE_TIMES_SORTED_BY_MOD | STATS_FUNC_TIMES_SORTED_BY_MOD)) {
        // cached string lengths, to avoid multiple separators after each other
        size_t line_time_sorted_mod_str_len = mstrlen(line_time_sorted_mod_str);
        size_t func_time_sorted_mod_str_len = mstrlen(func_time_sorted_mod_str);

        // cycle through the sorted statistics and gather the necessary data per module
        for (size_t i = 0; i < profiler_db.size(); ++i) {
          if (i > 0) {
            if ((stats_flags & STATS_LINE_TIMES_SORTED_BY_MOD) &&
                line_time_sorted_mod_str_len != mstrlen(line_time_sorted_mod_str)) {
              line_time_sorted_mod_str = mputstr(line_time_sorted_mod_str,
                "------------------------------------------------\n");
              line_time_sorted_mod_str_len = mstrlen(line_time_sorted_mod_str);
            }
            if ((stats_flags & STATS_FUNC_TIMES_SORTED_BY_MOD) &&
                func_time_sorted_mod_str_len != mstrlen(func_time_sorted_mod_str)) {
              func_time_sorted_mod_str = mputstr(func_time_sorted_mod_str,
                "-----------------------------------------------\n");
              func_time_sorted_mod_str_len = mstrlen(func_time_sorted_mod_str);
            }
          }
          if (stats_flags & STATS_LINE_TIMES_SORTED_BY_MOD) {
            for (size_t j = 0; j < used_code_lines; ++j) {
              if (0 == strcmp(code_line_stats[j].filename, profiler_db[i].filename)) {
                char* total_time_str = timeval2string(code_line_stats[j].total_time);
                line_time_sorted_mod_str = mputprintf(line_time_sorted_mod_str,
                  "%ss\t%s:%d", total_time_str, code_line_stats[j].filename,
                  code_line_stats[j].lineno);
                Free(total_time_str);
                if (NULL != code_line_stats[j].funcname) {
                  line_time_sorted_mod_str = mputprintf(line_time_sorted_mod_str,
                    " [%s]", code_line_stats[j].funcname);
                }
                line_time_sorted_mod_str = mputstrn(line_time_sorted_mod_str, "\n", 1);
              }
            }
          }
          if (stats_flags & STATS_FUNC_TIMES_SORTED_BY_MOD) {
            for (size_t j = 0; j < used_functions; ++j) {
              if (0 == strcmp(function_stats[j].filename, profiler_db[i].filename)) {
                char* total_time_str = timeval2string(function_stats[j].total_time);
                func_time_sorted_mod_str = mputprintf(func_time_sorted_mod_str,
                  "%ss\t%s:%d [%s]\n", total_time_str, function_stats[j].filename,
                  function_stats[j].lineno, function_stats[j].funcname);
                Free(total_time_str);
              }
            }
          }
        }
      }
    }

    if (!disable_coverage) {
      // sort the code lines and functions by execution count
      qsort(code_line_stats, used_code_lines, sizeof(stats_data_t), &stats_data_cmp_count);
      qsort(function_stats, used_functions, sizeof(stats_data_t), &stats_data_cmp_count);

      if (stats_flags & (STATS_LINE_COUNT_SORTED_TOTAL | STATS_TOP10_LINE_COUNT)) {
        // cycle through the sorted code lines and gather the necessary data
        for (size_t i = 0; i < used_code_lines; ++i) {
          char* the_data = mprintf("%d\t%s:%d", code_line_stats[i].exec_count,
            code_line_stats[i].filename, code_line_stats[i].lineno);
          if (NULL != code_line_stats[i].funcname) {
            the_data = mputprintf(the_data, " [%s]", code_line_stats[i].funcname);
          }
          the_data = mputstrn(the_data, "\n", 1);
          if (stats_flags & STATS_LINE_COUNT_SORTED_TOTAL) {
            line_count_sorted_tot_str = mputstr(line_count_sorted_tot_str, the_data);
          }
          if (i < 10 && (stats_flags & STATS_TOP10_LINE_COUNT)) {
            line_count_sorted_top10_str = mputprintf(line_count_sorted_top10_str,
              "%2lu.\t%s", i + 1, the_data);
          }
          Free(the_data);
        }
      }

      if (stats_flags & (STATS_FUNC_COUNT_SORTED_TOTAL | STATS_TOP10_FUNC_COUNT)) {
        // cycle through the sorted functions and gather the necessary data
        for (size_t i = 0; i < used_functions; ++i) {
          char* the_data = mprintf("%d\t%s:%d [%s]\n",
            function_stats[i].exec_count, function_stats[i].filename,
            function_stats[i].lineno, function_stats[i].funcname);
          if (stats_flags & STATS_FUNC_COUNT_SORTED_TOTAL) {
            func_count_sorted_tot_str = mputstr(func_count_sorted_tot_str, the_data);
          }
          if (i < 10 && (stats_flags & STATS_TOP10_FUNC_COUNT)) {
            func_count_sorted_top10_str = mputprintf(func_count_sorted_top10_str,
              "%2lu.\t%s", i + 1, the_data);
          }
          Free(the_data);
        }
      }

      if (stats_flags & (STATS_LINE_COUNT_SORTED_BY_MOD | STATS_FUNC_COUNT_SORTED_BY_MOD)) {
        // cached string lengths, to avoid multiple separators after each other
        size_t line_count_sorted_mod_str_len = mstrlen(line_count_sorted_mod_str);
        size_t func_count_sorted_mod_str_len = mstrlen(func_count_sorted_mod_str);

        // cycle through the sorted statistics and gather the necessary data per module
        for (size_t i = 0; i < profiler_db.size(); ++i) {
          if (i > 0) {
            if ((stats_flags & STATS_LINE_COUNT_SORTED_BY_MOD) &&
                line_count_sorted_mod_str_len != mstrlen(line_count_sorted_mod_str)) {
              line_count_sorted_mod_str = mputstr(line_count_sorted_mod_str,
                "-----------------------------------------------------\n");
              line_count_sorted_mod_str_len = mstrlen(line_count_sorted_mod_str);
            }
            if ((stats_flags & STATS_FUNC_COUNT_SORTED_BY_MOD) &&
                func_count_sorted_mod_str_len != mstrlen(func_count_sorted_mod_str)) {
              func_count_sorted_mod_str = mputstr(func_count_sorted_mod_str,
                "----------------------------------------------------\n");
              func_count_sorted_mod_str_len = mstrlen(func_count_sorted_mod_str);
            }
          }
          if (stats_flags & STATS_LINE_COUNT_SORTED_BY_MOD) {
            for (size_t j = 0; j < used_code_lines; ++j) {
              if (0 == strcmp(code_line_stats[j].filename, profiler_db[i].filename)) {
                line_count_sorted_mod_str = mputprintf(line_count_sorted_mod_str,
                  "%d\t%s:%d", code_line_stats[j].exec_count, code_line_stats[j].filename,
                  code_line_stats[j].lineno);
                if (NULL != code_line_stats[j].funcname) {
                  line_count_sorted_mod_str = mputprintf(line_count_sorted_mod_str,
                    " [%s]", code_line_stats[j].funcname);
                }
                line_count_sorted_mod_str = mputstrn(line_count_sorted_mod_str, "\n", 1);
              }
            }
          }
          if (stats_flags & STATS_FUNC_COUNT_SORTED_BY_MOD) {
            for (size_t j = 0; j < used_functions; ++j) {
              if (0 == strcmp(function_stats[j].filename, profiler_db[i].filename)) {
                func_count_sorted_mod_str = mputprintf(func_count_sorted_mod_str,
                  "%d\t%s:%d [%s]\n", function_stats[j].exec_count, function_stats[j].filename,
                  function_stats[j].lineno, function_stats[j].funcname);
              }
            }
          }
        }
      }
    }

    if (!disable_profiler && !disable_coverage) {
      // sort the code lines and functions by average time / execution
      qsort(code_line_stats, used_code_lines, sizeof(stats_data_t), &stats_data_cmp_avg);
      qsort(function_stats, used_functions, sizeof(stats_data_t), &stats_data_cmp_avg);

      if (stats_flags & (STATS_LINE_AVG_SORTED_TOTAL | STATS_TOP10_LINE_AVG)) {
        // cycle through the sorted code lines and gather the necessary data
        for (size_t i = 0; i < used_code_lines; ++i) {
          double avg = (code_line_stats[i].total_time.tv_sec +
            code_line_stats[i].total_time.tv_usec / 1000000.0) /
            code_line_stats[i].exec_count;
          char* total_time_str = timeval2string(code_line_stats[i].total_time);
          char* the_data = mprintf("%.6lfs\t(%ss / %d)\t%s:%d",
            avg, total_time_str, code_line_stats[i].exec_count,
            code_line_stats[i].filename, code_line_stats[i].lineno);
          Free(total_time_str);
          if (NULL != code_line_stats[i].funcname) {
            the_data = mputprintf(the_data, " [%s]", code_line_stats[i].funcname);
          }
          the_data = mputstrn(the_data, "\n", 1);
          if (stats_flags & STATS_LINE_AVG_SORTED_TOTAL) {
            line_avg_sorted_tot_str = mputstr(line_avg_sorted_tot_str, the_data);
          }
          if (i < 10 && (stats_flags & STATS_TOP10_LINE_AVG)) {
            line_avg_sorted_top10_str = mputprintf(line_avg_sorted_top10_str,
              "%2lu.\t%s", i + 1, the_data);
          }
          Free(the_data);
        }
      }

      if (stats_flags & (STATS_FUNC_AVG_SORTED_TOTAL | STATS_TOP10_FUNC_AVG)) {
        // cycle through the sorted functions and gather the necessary data
        for (size_t i = 0; i < used_functions; ++i) {
          double avg = (function_stats[i].total_time.tv_sec +
            function_stats[i].total_time.tv_usec / 1000000.0) /
            function_stats[i].exec_count;
          char* total_time_str = timeval2string(function_stats[i].total_time);
          char* the_data = mprintf("%.6lfs\t(%ss / %d)\t%s:%d [%s]\n",
            avg, total_time_str, function_stats[i].exec_count,
            function_stats[i].filename, function_stats[i].lineno, function_stats[i].funcname);
          Free(total_time_str);
          if (stats_flags & STATS_FUNC_AVG_SORTED_TOTAL) {
            func_avg_sorted_tot_str = mputstr(func_avg_sorted_tot_str, the_data);
          }
          if (i < 10 && (stats_flags & STATS_TOP10_FUNC_AVG)) {
            func_avg_sorted_top10_str = mputprintf(func_avg_sorted_top10_str,
              "%2lu.\t%s", i + 1, the_data);
          }
          Free(the_data);
        }
      }

      if (stats_flags & (STATS_LINE_AVG_SORTED_BY_MOD | STATS_FUNC_AVG_SORTED_BY_MOD)) {
        // cached string lengths, to avoid multiple separators after each other
        size_t line_avg_sorted_mod_str_len = mstrlen(line_avg_sorted_mod_str);
        size_t func_avg_sorted_mod_str_len = mstrlen(func_avg_sorted_mod_str);

        // cycle through the sorted statistics and gather the necessary data per module
        for (size_t i = 0; i < profiler_db.size(); ++i) {
          if (i > 0) {
            if ((stats_flags & STATS_LINE_AVG_SORTED_BY_MOD) &&
                line_avg_sorted_mod_str_len != mstrlen(line_avg_sorted_mod_str)) {
              line_avg_sorted_mod_str = mputstr(line_avg_sorted_mod_str,
                "--------------------------------------------------------------\n");
              line_avg_sorted_mod_str_len = mstrlen(line_avg_sorted_mod_str);
            }
            if ((stats_flags & STATS_FUNC_AVG_SORTED_BY_MOD) &&
                func_avg_sorted_mod_str_len != mstrlen(func_avg_sorted_mod_str)) {
              func_avg_sorted_mod_str = mputstr(func_avg_sorted_mod_str,
                "-------------------------------------------------------------\n");
              func_avg_sorted_mod_str_len = mstrlen(func_avg_sorted_mod_str);
            }
          }
          if (stats_flags & STATS_LINE_AVG_SORTED_BY_MOD) {
            for (size_t j = 0; j < used_code_lines; ++j) {
              if (0 == strcmp(code_line_stats[j].filename, profiler_db[i].filename)) {
                double avg = (code_line_stats[j].total_time.tv_sec +
                  code_line_stats[j].total_time.tv_usec / 1000000.0) /
                  code_line_stats[j].exec_count;
                char* total_time_str = timeval2string(code_line_stats[j].total_time);
                line_avg_sorted_mod_str = mputprintf(line_avg_sorted_mod_str,
                  "%.6lfs\t(%ss / %d)\t%s:%d",
                  avg, total_time_str, code_line_stats[j].exec_count,
                  code_line_stats[j].filename, code_line_stats[j].lineno);
                Free(total_time_str);
                if (NULL != code_line_stats[j].funcname) {
                  line_avg_sorted_mod_str = mputprintf(line_avg_sorted_mod_str,
                    " [%s]", code_line_stats[j].funcname);
                }
                line_avg_sorted_mod_str = mputstrn(line_avg_sorted_mod_str, "\n", 1);
              }
            }
          }
          if (stats_flags & STATS_FUNC_AVG_SORTED_BY_MOD) {
            for (size_t j = 0; j < used_functions; ++j) {
              if (0 == strcmp(function_stats[j].filename, profiler_db[i].filename)) {
                double avg = (function_stats[j].total_time.tv_sec +
                  function_stats[j].total_time.tv_usec / 1000000.0) /
                  function_stats[j].exec_count;
                char* total_time_str = timeval2string(function_stats[j].total_time);
                func_avg_sorted_mod_str = mputprintf(func_avg_sorted_mod_str,
                  "%.6lfs\t(%ss / %d)\t%s:%d [%s]\n",
                  avg, total_time_str, function_stats[j].exec_count,
                  function_stats[j].filename, function_stats[j].lineno, function_stats[j].funcname);
                Free(total_time_str);
              }
            }
          }
        }
      }
    }

    // free the stats data
    Free(code_line_stats);
    Free(function_stats);
  }
  
  // add new lines at the end of each segment
  if (stats_flags & STATS_NUMBER_OF_LINES) {
    line_func_count_str = mputstrn(line_func_count_str, "\n", 1);
  }
  if (stats_flags & STATS_LINE_DATA_RAW) {
    line_data_str = mputstrn(line_data_str, "\n", 1);
  }
  if (stats_flags & STATS_FUNC_DATA_RAW) {
    func_data_str = mputstrn(func_data_str, "\n", 1);
  }
  if (!disable_profiler) {
    if (stats_flags & STATS_LINE_TIMES_SORTED_BY_MOD) {
      line_time_sorted_mod_str = mputstrn(line_time_sorted_mod_str, "\n", 1);
    }
    if (stats_flags & STATS_LINE_TIMES_SORTED_TOTAL) {
      line_time_sorted_tot_str = mputstrn(line_time_sorted_tot_str, "\n", 1);
    }
    if (stats_flags & STATS_FUNC_TIMES_SORTED_BY_MOD) {
      func_time_sorted_mod_str = mputstrn(func_time_sorted_mod_str, "\n", 1);
    }
    if (stats_flags & STATS_FUNC_TIMES_SORTED_TOTAL) {
      func_time_sorted_tot_str = mputstrn(func_time_sorted_tot_str, "\n", 1);
    }
    if (stats_flags & STATS_TOP10_LINE_TIMES) {
      line_time_sorted_top10_str = mputstrn(line_time_sorted_top10_str, "\n", 1);
    }
    if (stats_flags & STATS_TOP10_FUNC_TIMES) {
      func_time_sorted_top10_str = mputstrn(func_time_sorted_top10_str, "\n", 1);
    }
    if (!disable_coverage) {
      if (stats_flags & STATS_LINE_AVG_RAW) {
        line_avg_str = mputstrn(line_avg_str, "\n", 1);
      }
      if (stats_flags & STATS_LINE_AVG_RAW) {
        func_avg_str = mputstrn(func_avg_str, "\n", 1);
      }
      if (stats_flags & STATS_LINE_AVG_SORTED_BY_MOD) {
        line_avg_sorted_mod_str = mputstrn(line_avg_sorted_mod_str, "\n", 1);
      }
      if (stats_flags & STATS_LINE_AVG_SORTED_TOTAL) {
        line_avg_sorted_tot_str = mputstrn(line_avg_sorted_tot_str, "\n", 1);
      }
      if (stats_flags & STATS_FUNC_AVG_SORTED_BY_MOD) {
        func_avg_sorted_mod_str = mputstrn(func_avg_sorted_mod_str, "\n", 1);
      }
      if (stats_flags & STATS_FUNC_AVG_SORTED_TOTAL) {
        func_avg_sorted_tot_str = mputstrn(func_avg_sorted_tot_str, "\n", 1);
      }
      if (stats_flags & STATS_TOP10_LINE_AVG) {
        line_avg_sorted_top10_str = mputstrn(line_avg_sorted_top10_str, "\n", 1);
      }
      if (stats_flags & STATS_TOP10_FUNC_AVG) {
        func_avg_sorted_top10_str = mputstrn(func_avg_sorted_top10_str, "\n", 1);
      }
    }
  }
  if (!disable_coverage) {
    if (stats_flags & STATS_LINE_COUNT_SORTED_BY_MOD) {
      line_count_sorted_mod_str = mputstrn(line_count_sorted_mod_str, "\n", 1);
    }
    if (stats_flags & STATS_LINE_COUNT_SORTED_TOTAL) {
      line_count_sorted_tot_str = mputstrn(line_count_sorted_tot_str, "\n", 1);
    }
    if (stats_flags & STATS_FUNC_COUNT_SORTED_BY_MOD) {
      func_count_sorted_mod_str = mputstrn(func_count_sorted_mod_str, "\n", 1);
    }
    if (stats_flags & STATS_FUNC_COUNT_SORTED_TOTAL) {
      func_count_sorted_tot_str = mputstrn(func_count_sorted_tot_str, "\n", 1);
    }
    if (stats_flags & STATS_TOP10_LINE_COUNT) {
      line_count_sorted_top10_str = mputstrn(line_count_sorted_top10_str, "\n", 1);
    }
    if (stats_flags & STATS_TOP10_FUNC_COUNT) {
      func_count_sorted_top10_str = mputstrn(func_count_sorted_top10_str, "\n", 1);
    }
    if (stats_flags & STATS_UNUSED_LINES) {
      unused_lines_str = mputstrn(unused_lines_str, "\n", 1);
    }
    if (stats_flags & STATS_UNUSED_FUNC) {
      unused_func_str = mputstrn(unused_func_str, "\n", 1);
    }
  }
  
  // write the statistics to the specified file
  FILE* file = fopen(stats_filename, "w");
  if (NULL == file) {
    TTCN_warning("Could not open file '%s' for writing. Profiler and/or code coverage "
      "statistics will not be saved.", stats_filename);
    return;
  }
  // by now the strings for all disabled statistics entries should be null
  fprintf(file, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"
    , title_str
    , (NULL != line_func_count_str) ? line_func_count_str : ""
    , (NULL != line_data_str) ? line_data_str : ""
    , (NULL != line_avg_str) ? line_avg_str : ""
    , (NULL != func_data_str) ? func_data_str : ""
    , (NULL != func_avg_str) ? func_avg_str : ""
    , (NULL != line_time_sorted_mod_str) ? line_time_sorted_mod_str : ""
    , (NULL != line_time_sorted_tot_str) ? line_time_sorted_tot_str : ""
    , (NULL != func_time_sorted_mod_str) ? func_time_sorted_mod_str : ""
    , (NULL != func_time_sorted_tot_str) ? func_time_sorted_tot_str : ""
    , (NULL != line_count_sorted_mod_str) ? line_count_sorted_mod_str : ""
    , (NULL != line_count_sorted_tot_str) ? line_count_sorted_tot_str : ""
    , (NULL != func_count_sorted_mod_str) ? func_count_sorted_mod_str : ""
    , (NULL != func_count_sorted_tot_str) ? func_count_sorted_tot_str : ""
    , (NULL != line_avg_sorted_mod_str) ? line_avg_sorted_mod_str : ""
    , (NULL != line_avg_sorted_tot_str) ? line_avg_sorted_tot_str : ""
    , (NULL != func_avg_sorted_mod_str) ? func_avg_sorted_mod_str : ""
    , (NULL != func_avg_sorted_tot_str) ? func_avg_sorted_tot_str : ""
    , (NULL != line_time_sorted_top10_str) ? line_time_sorted_top10_str : ""
    , (NULL != func_time_sorted_top10_str) ? func_time_sorted_top10_str : ""
    , (NULL != line_count_sorted_top10_str) ? line_count_sorted_top10_str : ""
    , (NULL != func_count_sorted_top10_str) ? func_count_sorted_top10_str : ""
    , (NULL != line_avg_sorted_top10_str) ? line_avg_sorted_top10_str : ""
    , (NULL != func_avg_sorted_top10_str) ? func_avg_sorted_top10_str : ""
    , (NULL != unused_lines_str) ? unused_lines_str : ""
    , (NULL != unused_func_str) ? unused_func_str : "");
  
  fclose(file);
  
  // free the strings
  Free(title_str);
  Free(line_func_count_str);
  Free(line_data_str);
  Free(line_avg_str);
  Free(func_data_str);
  Free(func_avg_str);
  Free(line_time_sorted_mod_str);
  Free(line_time_sorted_tot_str);
  Free(func_time_sorted_mod_str);
  Free(func_time_sorted_tot_str);
  Free(line_count_sorted_mod_str);
  Free(line_count_sorted_tot_str);
  Free(func_count_sorted_mod_str);
  Free(func_count_sorted_tot_str);
  Free(line_avg_sorted_mod_str);
  Free(line_avg_sorted_tot_str);
  Free(func_avg_sorted_mod_str);
  Free(func_avg_sorted_tot_str);
  Free(line_time_sorted_top10_str);
  Free(func_time_sorted_top10_str);
  Free(line_count_sorted_top10_str);
  Free(func_count_sorted_top10_str);
  Free(line_avg_sorted_top10_str);
  Free(func_avg_sorted_top10_str);
  Free(unused_lines_str);
  Free(unused_func_str);
}

void TTCN3_Profiler::reset()
{
  prev_time.tv_sec = 0;
  prev_time.tv_usec = 0;
  prev_file = NULL;
  prev_line = -1;
  prev_stack_len = -1;
}

timeval TTCN3_Profiler::get_time() 
{
  timeval tv;
  gettimeofday(&tv, NULL);
  return tv;
}

void TTCN3_Profiler::enter_function(const char* filename, int lineno)
{
  if (disable_profiler && disable_coverage) {
    return;
  }
  
  // Note that the execution time of the last line in a function
  // is measured by using the stack depth.
  execute_line(filename, lineno);
  
  if (!stopped) {
    int element = get_element(filename);

    // store function data
    if (!disable_coverage) {
      ++profiler_db[element].functions[get_function(element, lineno)].exec_count;
    }
  }
}

void TTCN3_Profiler::execute_line(const char* filename, int lineno)
{
  if (disable_profiler && disable_coverage) {
    return;
  }
  
  if (!disable_profiler && TTCN3_Stack_Depth::depth() > prev_stack_len) {
    // this line is in a different function than the last one, don't measure anything
    TTCN3_Stack_Depth::add_stack(prev_stack_len, prev_file, filename, prev_line, lineno);
  }
  
  if (!stopped) {
    if (!disable_profiler && NULL != prev_file) {
      // this line is in the same function as the previous one, measure the time difference
      timeval elapsed = subtract_timeval(get_time(), prev_time);

      // add the elapsed time to the total time of the previous line
      add_line_time(elapsed, get_element(prev_file), prev_line);

      TTCN3_Stack_Depth::update_stack_elapsed(elapsed);
    }

    // functions starting at line 0 are: pre_init_module and post_init_module,
    // don't include them in the database (as they don't appear in the TTCN-3 code),
    // but include any actual code lines they may contain
    // also, several instructions could be in the same line, only count the line once
    if (0 != lineno && !disable_coverage && (lineno != prev_line || NULL == prev_file || 
                                             0 != strcmp(prev_file, filename))) {
      int element = get_element(filename);

      // increase line execution count
      ++profiler_db[element].lines[get_line(element, lineno)].exec_count;
    }
  }
  
  // store the current location as previous for the next call
  set_prev(disable_profiler ? -1 : TTCN3_Stack_Depth::depth(), filename, lineno);
}

int TTCN3_Profiler::get_element(const char* filename) 
{
  for (size_t i = 0; i < profiler_db.size(); ++i) {
    if (0 == strcmp(profiler_db[i].filename, filename)) {
      return i;
    }
  }
  
  profiler_db_item_t item;
  item.filename = mcopystr(filename);
  profiler_db.push_back(item);
  return profiler_db.size() - 1;
}

int TTCN3_Profiler::get_function(int element, int lineno)
{
  for (size_t i = 0; i < profiler_db[element].functions.size(); ++i) {
    if (profiler_db[element].functions[i].lineno == lineno) {
      return i;
    }
  }
  return -1;
}

void TTCN3_Profiler::create_function(int element, int lineno, const char* function_name)
{
  profiler_db_item_t::profiler_function_data_t func_data;
  func_data.lineno = lineno;
  func_data.total_time.tv_sec = 0;
  func_data.total_time.tv_usec = 0;
  func_data.exec_count = 0;
  func_data.name = mcopystr(function_name);
  profiler_db[element].functions.push_back(func_data);
}

int TTCN3_Profiler::get_line(int element, int lineno)
{
  for (size_t i = 0; i < profiler_db[element].lines.size(); ++i) {
    if (profiler_db[element].lines[i].lineno == lineno) {
      return i;
    }
  }
  return -1;
}

void TTCN3_Profiler::create_line(int element, int lineno)
{
  profiler_db_item_t::profiler_line_data_t line_data;
  line_data.lineno = lineno;
  line_data.total_time.tv_sec = 0;
  line_data.total_time.tv_usec = 0;
  line_data.exec_count = 0;
  profiler_db[element].lines.push_back(line_data);
}

void TTCN3_Profiler::add_line_time(timeval elapsed, int element, int lineno) 
{
  if (0 == lineno) {
    return;
  }
  profiler_db[element].lines[get_line(element, lineno)].total_time = add_timeval(
    profiler_db[element].lines[get_line(element, lineno)].total_time, elapsed);
}

void TTCN3_Profiler::add_function_time(timeval elapsed, int element, int lineno)
{
  int func = get_function(element, lineno);
  if (-1 == func) {
    return;
  }
  profiler_db[element].functions[func].total_time = add_timeval(
    profiler_db[element].functions[func].total_time, elapsed);
}

void TTCN3_Profiler::update_last()
{
  if (stopped || (0 == prev_time.tv_sec && 0 == prev_time.tv_usec) || NULL == prev_file) {
    return;
  }

  timeval elapsed = subtract_timeval(get_time(), prev_time);

  int element = get_element(prev_file);
  
  // add the elapsed time to the total time of the previous line
  add_line_time(elapsed, element, prev_line);
  TTCN3_Stack_Depth::update_stack_elapsed(elapsed);

  // reset measurement
  prev_time.tv_sec = 0;
  prev_time.tv_usec = 0;
}

void TTCN3_Profiler::set_prev(int stack_len, const char* filename, int lineno)
{
  prev_file = filename;
  prev_line = lineno;
  if (!disable_profiler) {
    prev_time = get_time();
    prev_stack_len = stack_len;
  }
}

/////////////////////////////////////
///////// TTCN3_Stack_Depth /////////
/////////////////////////////////////

int TTCN3_Stack_Depth::current_depth = -1;
Vector<TTCN3_Stack_Depth::call_stack_timer_item_t> TTCN3_Stack_Depth::call_stack_timer_db;
boolean TTCN3_Stack_Depth::net_line_times = FALSE;
boolean TTCN3_Stack_Depth::net_func_times = FALSE;

TTCN3_Stack_Depth::TTCN3_Stack_Depth() 
{
  if (ttcn3_prof.is_profiler_disabled()) {
    return;
  }
  ++current_depth;
}

TTCN3_Stack_Depth::~TTCN3_Stack_Depth() 
{
  if (ttcn3_prof.is_profiler_disabled()) {
    return;
  }
  ttcn3_prof.update_last();
  remove_stack();
  if (0 == current_depth) {
    ttcn3_prof.reset();
  }
  --current_depth;
}

void TTCN3_Stack_Depth::set_net_line_times(boolean p_net_line_times)
{
  net_line_times = p_net_line_times;
}

void TTCN3_Stack_Depth::set_net_func_times(boolean p_net_func_times)
{
  net_func_times = p_net_func_times;
}

void TTCN3_Stack_Depth::add_stack(int stack_len, const char* caller_file, const char* func_file,
                                  int caller_line, int start_line) 
{
  call_stack_timer_item_t item;
  item.stack_len = stack_len;
  item.caller_file = caller_file;
  item.func_file = func_file;
  item.caller_line = caller_line;
  item.start_line = start_line;
  item.elapsed.tv_sec = 0;
  item.elapsed.tv_usec = 0;
  item.first_call = true;
  item.recursive_call = false;

  if (!net_line_times || !net_func_times) {
    // check if it's a recursive function
    for (int i = current_depth - 1; i >= 0 ; --i) {
      if (call_stack_timer_db[i].start_line == start_line &&
          0 == strcmp(call_stack_timer_db[i].func_file, func_file)) {
        item.recursive_call = true;

        // check if the caller is new
        if (call_stack_timer_db[i].caller_line == caller_line &&
            ((NULL == call_stack_timer_db[i].caller_file && NULL == caller_file) ||
             (NULL != call_stack_timer_db[i].caller_file && NULL != caller_file &&
             0 == strcmp(call_stack_timer_db[i].caller_file, caller_file)))) {
          item.first_call = false;
          break;
        }
      }
    }
  }
  
  call_stack_timer_db.push_back(item);
}

void TTCN3_Stack_Depth::remove_stack()
{
  // add the time gathered for this stack level to the appropriate line and function
  // except for functions starting at line 0 (pre_init_module and post_init_module)
  if (0 != call_stack_timer_db[current_depth].start_line) {
    timeval elapsed = call_stack_timer_db[current_depth].elapsed;
    if (!net_line_times && NULL != call_stack_timer_db[current_depth].caller_file &&
        call_stack_timer_db[current_depth].first_call) {
      // add the elapsed time to the caller line, if it exists
      // (only add it once for recursive functions, at the first call)
      ttcn3_prof.add_line_time(elapsed,
        ttcn3_prof.get_element(call_stack_timer_db[current_depth].caller_file),
        call_stack_timer_db[current_depth].caller_line);
    }
    if (!net_func_times && !call_stack_timer_db[current_depth].recursive_call) {
      // add the elapsed time to the called function, if it's not recursive
      // (in case of net function times this has already been done in update_stack_elapsed)
      ttcn3_prof.add_function_time(elapsed,
        ttcn3_prof.get_element(call_stack_timer_db[current_depth].func_file),
        call_stack_timer_db[current_depth].start_line);
    }
  }

  ttcn3_prof.set_prev(call_stack_timer_db[current_depth].stack_len, 
    call_stack_timer_db[current_depth].caller_file,
    call_stack_timer_db[current_depth].caller_line);
  
  call_stack_timer_db.erase_at(current_depth);
}

void TTCN3_Stack_Depth::update_stack_elapsed(timeval elapsed) 
{
  // if function times are net times, only add the elapsed time to the current function
  if (net_func_times) {
    ttcn3_prof.add_function_time(elapsed,
      ttcn3_prof.get_element(call_stack_timer_db[current_depth].func_file),
      call_stack_timer_db[current_depth].start_line);
  }
  if (!net_line_times || !net_func_times) {
    // cycle through the stack and add the elapsed time to the entries where 
    // the function/caller pair appears for the first time (marked by 'first_call')
    for(int i = 0; i <= current_depth; ++i) {
      if (call_stack_timer_db[i].first_call) {
        call_stack_timer_db[i].elapsed = add_timeval(call_stack_timer_db[i].elapsed, elapsed);
      }
    }
  }
}
