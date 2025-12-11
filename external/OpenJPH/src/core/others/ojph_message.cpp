//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman
// Copyright (c) 2019, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2019, The University of New South Wales, Australia
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//***************************************************************************/
// This file is part of the OpenJPH software implementation.
// File: ojph_message.cpp
// Author: Aous Naman
// Date: 29 August 2019
//***************************************************************************/

#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <stdexcept>

#include "ojph_message.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  FILE* info_stream = stdout;

  ////////////////////////////////////////////////////////////////////////////
  message_info info;
  message_info* local_info = &info;
  OJPH_MSG_LEVEL message_level = OJPH_MSG_LEVEL::ALL_MSG;

  ////////////////////////////////////////////////////////////////////////////
  void configure_info(message_info* info)
  {
    local_info = info;
  }

  ////////////////////////////////////////////////////////////////////////////
  message_info* get_info()
  {
    return local_info;
  }

  ////////////////////////////////////////////////////////////////////////////
  void set_info_stream(FILE* s)
  {
    info_stream = s;
  }

  ////////////////////////////////////////////////////////////////////////////
  void message_info::operator()(int info_code, const char* file_name,
    int line_num, const char* fmt, ...)
  {
    if (info_stream == NULL || message_level > OJPH_MSG_LEVEL::INFO)
      return;
    
    fprintf(info_stream, "ojph info 0x%08X at %s:%d: ",
      info_code, file_name, line_num);
    va_list args;
    va_start(args, fmt);
    vfprintf(info_stream, fmt, args);
    fprintf(info_stream, "\n");
    va_end(args);
  }

  ////////////////////////////////////////////////////////////////////////////
  FILE *warning_stream = stdout;

  ////////////////////////////////////////////////////////////////////////////
  message_warning warn;
  message_warning* local_warn = &warn;

  ////////////////////////////////////////////////////////////////////////////
  void configure_warning(message_warning* warn)
  {
    local_warn = warn;
  }

  ////////////////////////////////////////////////////////////////////////////
  message_warning* get_warning()
  {
    return local_warn;
  }

  ////////////////////////////////////////////////////////////////////////////
  void set_warning_stream(FILE *s)
  {
    warning_stream = s;
  }

  ////////////////////////////////////////////////////////////////////////////
  void message_warning::operator()(int warn_code, const char* file_name,
    int line_num, const char *fmt, ...)
  {
    if (warning_stream == NULL || message_level > OJPH_MSG_LEVEL::WARN)
      return;

    fprintf(warning_stream, "ojph warning 0x%08X at %s:%d: ",
      warn_code, file_name, line_num);
    va_list args;
    va_start(args, fmt);
    vfprintf(warning_stream, fmt, args);
    fprintf(warning_stream, "\n");
    va_end(args);
  }

  ////////////////////////////////////////////////////////////////////////////
  FILE *error_stream = stderr;

  ////////////////////////////////////////////////////////////////////////////
  message_error error;
  message_error* local_error = &error;

  ////////////////////////////////////////////////////////////////////////////
  void configure_error(message_error* error)
  {
    local_error = error;
  }

  ////////////////////////////////////////////////////////////////////////////
  message_error* get_error()
  {
    return local_error;
  }

  ////////////////////////////////////////////////////////////////////////////
  void set_error_stream(FILE *s)
  {
    error_stream = s;
  }

  ////////////////////////////////////////////////////////////////////////////
  void message_error::operator()(int error_code, const char* file_name,
    int line_num, const char *fmt, ...)
  {
    if (error_stream != NULL && message_level <= OJPH_MSG_LEVEL::ERROR)
    {
      fprintf(error_stream, "ojph error 0x%08X at %s:%d: ",
        error_code, file_name, line_num);
      va_list args;
      va_start(args, fmt);
      vfprintf(error_stream, fmt, args);
      fprintf(error_stream, "\n");
      va_end(args);
    }

    throw std::runtime_error("ojph error");
  }

  ////////////////////////////////////////////////////////////////////////////
  void set_message_level(OJPH_MSG_LEVEL level)
  {
    assert(level >= OJPH_MSG_LEVEL::ALL_MSG && 
           level <= OJPH_MSG_LEVEL::NO_MSG);
    message_level = level;
  }

}
