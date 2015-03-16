///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2000-2015 Ericsson Telecom AB
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v1.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v10.html
///////////////////////////////////////////////////////////////////////////////

#include "Profiler.hh"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "JSON_Tokenizer.hh"
#include "memory.h"
#include "Runtime.hh"

////////////////////////////////////
////////// TTCN3_Profiler //////////
////////////////////////////////////

TTCN3_Profiler ttcn3_prof;

TTCN3_Profiler::TTCN3_Profiler()
: disable_profiler(FALSE), disable_coverage(FALSE), aggregate_data(FALSE)
, disable_stats(FALSE)
{
  database_filename = mcopystr("profiler.db");
  stats_filename = mcopystr("profiler.stats");
  reset();
}

TTCN3_Profiler::~TTCN3_Profiler()
{
  if (!disable_profiler || !disable_coverage) {
    import_data();
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

boolean TTCN3_Profiler::is_profiler_disabled() const
{
  return disable_profiler;
}

#define IMPORT_FORMAT_ERROR(cond) \
  if (cond) { \
    TTCN_warning("Database format is invalid. Profiler and/or code coverage data will not be loaded."); \
    return; \
  }

void TTCN3_Profiler::init_data_file()
{
  // delete the database file (from the previous run) if data aggregation is not set
  if (!aggregate_data && (!disable_profiler || !disable_coverage)) {
    remove(database_filename);
  }
}

void TTCN3_Profiler::import_data()
{
  // open the file, if it exists
  FILE* file = fopen(database_filename, "r");
  if (NULL == file) {
    return;
  }
  
  // get the file size
  fseek(file, 0, SEEK_END);
  int file_size = ftell(file);
  rewind(file);
  
  // read the entire file into a character buffer
  char* buffer = (char*)Malloc(file_size);
  fread(buffer, 1, file_size, file);
  
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
        func_data.total_time = 0.0;
        profiler_db[file_index].functions.push_back(func_data);
      }
      
      if (!disable_coverage) {
        // function execution count:
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 15 ||
          0 != strncmp(value, "execution count", value_len));

        // read the execution count and add it to the current data
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
        profiler_db[file_index].functions[function_index].exec_count += atoi(value);
      }
      
      if (!disable_profiler) {
        // total function execution time:
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 10 ||
          0 != strncmp(value, "total time", value_len));

        // read the total time and add it to the current data
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
        profiler_db[file_index].functions[function_index].total_time += atof(value);
      }

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
      
      // line number:
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 6 ||
        0 != strncmp(value, "number", value_len));
      
      // read the line number and check if the line already exists
      json.get_next_token(&token, &value, &value_len);
      IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
      int lineno = atoi(value);
      IMPORT_FORMAT_ERROR(lineno < 0);
      size_t line_no = lineno;
      if (line_no >= profiler_db[file_index].lines.size()) {
        for (size_t i = profiler_db[file_index].lines.size(); i <= line_no; ++i) {
          profiler_db_item_t::profiler_line_data_t line_data;
          line_data.total_time = 0.0;
          line_data.exec_count = 0;
          profiler_db[file_index].lines.push_back(line_data);
        }
      }
      
      if (!disable_coverage) {
        // line execution count:
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 15 ||
          0 != strncmp(value, "execution count", value_len));

        // read the execution count and add it to the current data
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
        profiler_db[file_index].lines[line_no].exec_count += atoi(value);
      }
      
      if (!disable_profiler) {
        // total line execution time:
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NAME != token || value_len != 10 ||
          0 != strncmp(value, "total time", value_len));

        // read the total time and add it to the current data
        json.get_next_token(&token, &value, &value_len);
        IMPORT_FORMAT_ERROR(JSON_TOKEN_NUMBER != token);
        profiler_db[file_index].lines[line_no].total_time += atof(value);
      }

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
  // nothing to export if the database is empty
  if (profiler_db.empty()) {
    return;
  }
  
  // check whether the file can be opened for writing
  FILE* file = fopen(database_filename, "w");
  if (NULL == file) {
    TTCN_warning("Could not open file '%s' for writing. Profiler and/or code coverage "
      "data will not be saved.", database_filename);
    return;
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
      
      if (!disable_coverage) {
        // store the function execution count
        json.put_next_token(JSON_TOKEN_NAME, "execution count");
        char* exec_count_str = mprintf("%d", profiler_db[i].functions[j].exec_count);
        json.put_next_token(JSON_TOKEN_NUMBER, exec_count_str);
        Free(exec_count_str);
      }
      
      if (!disable_profiler) {
        // store the function's total execution time
        json.put_next_token(JSON_TOKEN_NAME, "total time");
        char* exec_count_str = mprintf("%.6lf", profiler_db[i].functions[j].total_time);
        json.put_next_token(JSON_TOKEN_NUMBER, exec_count_str);
        Free(exec_count_str);
      }
      
      // end of function object
      json.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
    }
    
    // end of function data array
    json.put_next_token(JSON_TOKEN_ARRAY_END, NULL);
    
    // store the line data in an array (one element for each line with useful data)
    json.put_next_token(JSON_TOKEN_NAME, "lines");
    json.put_next_token(JSON_TOKEN_ARRAY_START, NULL);
    for (size_t j = 0; j < profiler_db[i].lines.size(); ++j) {
      if (0.0 != profiler_db[i].lines[j].total_time ||
          0 != profiler_db[i].lines[j].exec_count) {
        
        // store line data in an object
        json.put_next_token(JSON_TOKEN_OBJECT_START, NULL);
        
        // store the line number
        json.put_next_token(JSON_TOKEN_NAME, "number");
        char* line_number_str = mprintf("%lu", j);
        json.put_next_token(JSON_TOKEN_NUMBER, line_number_str);
        Free(line_number_str);
        
        if (!disable_coverage) {
          // store the line execution count
          json.put_next_token(JSON_TOKEN_NAME, "execution count");
          char* exec_count_str = mprintf("%d", profiler_db[i].lines[j].exec_count);
          json.put_next_token(JSON_TOKEN_NUMBER, exec_count_str);
          Free(exec_count_str);
        }

        if (!disable_profiler) {
          // store the line's total execution time
          json.put_next_token(JSON_TOKEN_NAME, "total time");
          char* exec_count_str = mprintf("%.6lf", profiler_db[i].lines[j].total_time);
          json.put_next_token(JSON_TOKEN_NUMBER, exec_count_str);
          Free(exec_count_str);
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
  
  // line data
  char* line_data_str = mprintf(
    "-------------------------------------------------\n"
    "%s- Code line data (%s%s%s) -%s\n"
    "-------------------------------------------------\n"
    , disable_profiler ? "-------" : (disable_coverage ? "---------" : "")
    , disable_profiler ? "" : "total time"
    , (disable_profiler || disable_coverage) ? "" : " / "
    , disable_coverage ? "" : "execution count"
    , disable_profiler ? "------" : (disable_coverage ? "---------" : ""));
  
  // average time / exec count for lines
  char* line_avg_str = NULL;
  if (!disable_coverage && !disable_profiler) {
    line_avg_str = mcopystr(
      "-------------------------------------------------\n"
      "- Average time / execution count for code lines -\n"
      "-------------------------------------------------\n");
  }
  
  // function data
  char* func_data_str = mprintf(
    "------------------------------------------------\n"
    "%s- Function data (%s%s%s) -%s\n"
    "------------------------------------------------\n"
    , disable_profiler ? "-------" : (disable_coverage ? "---------" : "")
    , disable_profiler ? "" : "total time"
    , (disable_profiler || disable_coverage) ? "" : " / "
    , disable_coverage ? "" : "execution count"
    , disable_profiler ? "------" : (disable_coverage ? "---------" : ""));
  
  // average time / exec count for functions
  char* func_avg_str = NULL;
  if (!disable_coverage && !disable_profiler) {
    func_avg_str = mcopystr(
      "------------------------------------------------\n"
      "- Average time / execution count for functions -\n"
      "------------------------------------------------\n");
  }
  
  // cycle through the database and gather the necessary data
  for (size_t i = 0; i < profiler_db.size(); ++i) {
    if (i > 0) {
      // add separators between files
      line_data_str = mputstr(line_data_str, "-------------------------------------------------\n");
      func_data_str = mputstr(func_data_str, "------------------------------------------------\n");
      if (!disable_profiler && !disable_coverage) {
        line_avg_str = mputstr(line_avg_str, "-------------------------------------------------\n");
        func_avg_str = mputstr(func_avg_str, "------------------------------------------------\n");
      }
    }
    
    // lines
    for (size_t j = 0; j < profiler_db[i].lines.size(); ++j) {
      if (0.0 != profiler_db[i].lines[j].total_time ||
          0 != profiler_db[i].lines[j].exec_count) {
        if (!disable_profiler) {
          line_data_str = mputprintf(line_data_str, "%.6lfs", profiler_db[i].lines[j].total_time);
          if (!disable_coverage) {
            line_data_str = mputstrn(line_data_str, "\t/\t", 3);
            line_avg_str = mputprintf(line_avg_str, "%.6lfs", 
              profiler_db[i].lines[j].total_time / profiler_db[i].lines[j].exec_count);
          }
        }
        if (!disable_coverage) {
          line_data_str = mputprintf(line_data_str, "%d", profiler_db[i].lines[j].exec_count);
        }
        
        // line specification (including function name for the function's start line)
        char* line_spec_str = mprintf("\t%s:%lu", profiler_db[i].filename, j);
        int func = get_function(i, j);
        if (-1 != func) {
          line_spec_str = mputprintf(line_spec_str, " [%s]", profiler_db[i].functions[func].name);
        }
        line_spec_str = mputstrn(line_spec_str, "\n", 1);
        
        // add the line spec string to the other strings
        line_data_str = mputstr(line_data_str, line_spec_str);
        if (!disable_profiler && !disable_coverage) {
          line_avg_str = mputstr(line_avg_str, line_spec_str);
        }
      }
    }
    
    // functions
    for (size_t j = 0; j < profiler_db[i].functions.size(); ++j) {
      if (!disable_profiler) {
        func_data_str = mputprintf(func_data_str, "%.6lfs", profiler_db[i].functions[j].total_time);
        if (!disable_coverage) {
          func_data_str = mputstrn(func_data_str, "\t/\t", 3);
          func_avg_str = mputprintf(func_avg_str, "%.6lfs", 
            profiler_db[i].functions[j].total_time / profiler_db[i].functions[j].exec_count);
        }
      }
      if (!disable_coverage) {
        func_data_str = mputprintf(func_data_str, "%d", profiler_db[i].functions[j].exec_count);
      }

      // functions specification
      char* func_spec_str = mprintf("\t%s:%d [%s]\n", profiler_db[i].filename,
        profiler_db[i].functions[j].lineno, profiler_db[i].functions[j].name);

      // add the line spec string to the other strings
      func_data_str = mputstr(func_data_str, func_spec_str);
      if (!disable_profiler && !disable_coverage) {
        func_avg_str = mputstr(func_avg_str, func_spec_str);
      }
    }
  }
  
  // add new lines at the end of each segment
  line_data_str = mputstrn(line_data_str, "\n", 1);
  func_data_str = mputstrn(func_data_str, "\n", 1);
  if (!disable_profiler && !disable_coverage) {
    line_avg_str = mputstrn(line_avg_str, "\n", 1);
    func_avg_str = mputstrn(func_avg_str, "\n", 1);
  }
  
  // write the statistics to the specified file
  FILE* file = fopen(stats_filename, "w");
  if (NULL == file) {
    TTCN_warning("Could not open file '%s' for writing. Profiler and/or code coverage "
      "statistics will not be saved.", stats_filename);
    return;
  }
  fprintf(file, "%s%s%s%s%s"
    , title_str, line_data_str
    , (disable_profiler || disable_coverage) ? "" : line_avg_str
    , func_data_str, (disable_profiler || disable_coverage) ? "" : func_avg_str);
  fclose(file);
}

void TTCN3_Profiler::reset()
{
  prev_time = 0.0;
  prev_file = NULL;
  prev_line = -1;
  prev_stack_len = 0;
}

double TTCN3_Profiler::get_time() 
{
  timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void TTCN3_Profiler::enter_function(const char* filename, int lineno, const char* function_name)
{
  if (disable_profiler && disable_coverage) {
    return;
  }
  
  // Note that the execution time of the last line in a function
  // is measured by using the stack depth.
  execute_line(filename, lineno);
  
  int element = get_element(filename);
  
  // store function data
  int func = get_function(element, lineno);
  if (-1 == func) {
    create_function(element, lineno, function_name);
    func = profiler_db[element].functions.size() - 1;
  }
  
  if (!disable_coverage) {
    ++profiler_db[element].functions[func].exec_count;
  }
}

void TTCN3_Profiler::execute_line(const char* filename, int lineno)
{
  if (disable_profiler && disable_coverage) {
    return;
  }
  
  if (!disable_profiler) {
    double currentTime = get_time();

    // prev line should not be measured, because it is still running: we are in longer stack level
    if (0.0 == prev_time || TTCN3_Stack_Depth::depth() > prev_stack_len) {
      // add prev timer to call stack:
      TTCN3_Stack_Depth::add_stack(prev_stack_len, prev_file, filename, prev_line, lineno);
    }
    else {
      // if stack level is the same or higher: current line should be measured:
      double elapsed = currentTime - prev_time;

      // add the elapsed time to the time of the previous line:
      add_line_time(elapsed, get_element(prev_file), prev_line);

      TTCN3_Stack_Depth::update_stack_elapsed(elapsed);
    }
  }
  
  // several instructions could be in the same line, only count the line once
  if (!disable_coverage && (lineno != prev_line || NULL == prev_file || 
                            0 != strcmp(prev_file, filename))) {
    int element = get_element(filename);
    
    // make sure the line exists in the database
    create_lines(element, lineno);
    
    // increase line execution count
    ++profiler_db[element].lines[lineno].exec_count;
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
  func_data.total_time = 0.0;
  func_data.exec_count = 0;
  func_data.name = mcopystr(function_name);
  profiler_db[element].functions.push_back(func_data);
}

void TTCN3_Profiler::create_lines(int element, int lineno)
{
  // set 0 for the unknown lines
  size_t line_no = lineno;
  if (profiler_db[element].lines.size() <= line_no) {
    for (size_t i = profiler_db[element].lines.size(); i <= line_no; ++i) {
      profiler_db_item_t::profiler_line_data_t line_data;
      line_data.total_time = 0.0;
      line_data.exec_count = 0;
      profiler_db[element].lines.push_back(line_data);
    }
  }
}

void TTCN3_Profiler::add_line_time(double elapsed, int element, int lineno) 
{
  if (-1 == lineno) {
    return;
  }

  // ensure the line data exists
  create_lines(element, lineno);
  
  // increase the time of the line in the current file:
  profiler_db[element].lines[lineno].total_time += elapsed;
}

void TTCN3_Profiler::add_function_time(double elapsed, int element, int lineno)
{
  int func = get_function(element, lineno);
  if (-1 == func) {
    return;
  }
  profiler_db[element].functions[func].total_time += elapsed;
}

void TTCN3_Profiler::update_last()
{
  if (0.0 == prev_time) {
    return;
  }
  
  double currentTime = get_time();
  double elapsed = currentTime - prev_time;

  int element = get_element(prev_file);
  
  // add the elapsed time to the time of the previous line:
  add_line_time(elapsed, element, prev_line);
  TTCN3_Stack_Depth::update_stack_elapsed(elapsed);

  // reset measurement:
  prev_time = 0.0;
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

void TTCN3_Stack_Depth::add_stack(int stack_len, const char* caller_file, const char* func_file,
                                  int caller_line, int start_line) 
{
  call_stack_timer_item_t item;
  item.stack_len = stack_len;
  item.caller_file = caller_file;
  item.func_file = func_file;
  item.caller_line = caller_line;
  item.start_line = start_line;
  item.elapsed = 0.0;
  call_stack_timer_db.push_back(item);
}

void TTCN3_Stack_Depth::remove_stack()
{
  // if stack level is the same or higher: measure the time:
  double elapsed = call_stack_timer_db[current_depth].elapsed;

  int element = ttcn3_prof.get_element(call_stack_timer_db[current_depth].func_file);

  // add elapsed time to the total execution time of the previous line:
  ttcn3_prof.add_function_time(elapsed, element, call_stack_timer_db[current_depth].start_line);

  ttcn3_prof.set_prev(call_stack_timer_db[current_depth].stack_len, 
    call_stack_timer_db[current_depth].caller_file,
    call_stack_timer_db[current_depth].caller_line);
  
  call_stack_timer_db.erase_at(current_depth);
}

void TTCN3_Stack_Depth::update_stack_elapsed(double elapsed) 
{
  for(int i = 0; i <= current_depth; i++) {
    call_stack_timer_db[i].elapsed += elapsed;
  }
}