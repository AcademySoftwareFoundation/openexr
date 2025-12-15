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
// File: ojph_arg.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_ARG_H
#define OJPH_ARG_H

#include <cstdlib>
#include <cassert>
#include <cstring>

#include "ojph_defs.h"

namespace ojph {

  /////////////////////////////////////////////////////////////////////////////
  //
  /////////////////////////////////////////////////////////////////////////////
  class argument {
    friend class cli_interpreter;
  public:
    argument() : arg(NULL), index(0) {}
    char *arg;
    bool is_valid() { return (arg != NULL); }
  private:
    int index;
  };

  /////////////////////////////////////////////////////////////////////////////
  //
  /////////////////////////////////////////////////////////////////////////////
  class cli_interpreter {
  public:
    cli_interpreter() : argv(NULL), argc(0), avail(avail_store) { }
    ~cli_interpreter()
    { if (avail != avail_store) delete[] avail; }

    ///////////////////////////////////////////////////////////////////////////
    void init(int argc, char *argv[]) {
      assert(avail == avail_store);
      if (argc > 128)
        avail = new ui8[((ui32)argc + 7u) >> 3];
      memset(avail, 0, 
        ojph_max(sizeof(avail_store), (size_t)((argc + 7) >> 3)));
      this->argv = argv;
      this->argc = argc;
      for (int i = 0; i < argc; ++i)
        avail[i >> 3] = (ui8)(avail[i >> 3] | (ui8)(1 << (i & 7)));
    }

    ///////////////////////////////////////////////////////////////////////////
    argument get_next_value(const argument& current) {
      argument t;
      int idx = current.index + 1;
      if (idx < argc && (avail[idx >> 3] & (1 << (idx & 0x7)))) {
        t.arg = argv[idx];
        t.index = idx;
      }
      return t;
    }

    ///////////////////////////////////////////////////////////////////////////
    argument find_argument(const char *str) {
      argument t;
      for (int index = 1; index < argc; ++index)
        if (avail[index >> 3] & (1 << (index & 0x7)))
          if (strcmp(str, argv[index]) == 0) {
            t.arg = argv[index];
            t.index = index;
            return t;
          }
      return t;
    }

    ///////////////////////////////////////////////////////////////////////////
    void release_argument(const argument& arg) {
      if (arg.index != 0) {
        assert(arg.index < argc);
        avail[arg.index >> 3] &= (ui8)(~(1 << (arg.index & 0x7)));
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    bool is_exhausted() {
      for (int i = 1; i < argc; ++i)
        if (avail[i >> 3] & (1 << (i & 0x7)))
          return false;
      return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    argument get_argument_zero() {
      argument t;
      t.arg = argv[0];
      return t;
    }

    ///////////////////////////////////////////////////////////////////////////
    argument get_next_avail_argument(const argument& arg) {
      argument t;
      int idx = arg.index + 1;
      while (idx < argc && (avail[idx >> 3] & (1 << (idx & 0x7))) == 0)
        ++idx;
      if (idx < argc) {
        t.arg = argv[idx];
        t.index = idx;
      }
      return t;
    }

    ///////////////////////////////////////////////////////////////////////////
    void reinterpret(const char *str, int& val) {
      argument t = find_argument(str);
      if (t.is_valid()) {
        argument t2 = get_next_value(t);
        if (t2.is_valid()) {
          val = atoi(t2.arg);
          release_argument(t);
          release_argument(t2);
        }
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    void reinterpret(const char* str, ui32& val) {
      argument t = find_argument(str);
      if (t.is_valid()) {
        argument t2 = get_next_value(t);
        if (t2.is_valid()) {
          val = (ui32)strtoul(t2.arg, NULL, 10);
          release_argument(t);
          release_argument(t2);
        }
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    void reinterpret(const char *str, float& val) {
      argument t = find_argument(str);
      if (t.is_valid()) {
        argument t2 = get_next_value(t);
        if (t2.is_valid()) {
          val = strtof(t2.arg, NULL);
          release_argument(t);
          release_argument(t2);
        }
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    void reinterpret(const char *str, bool& val) {
      argument t = find_argument(str);
      if (t.is_valid()) {
        argument t2 = get_next_value(t);
        if (t2.is_valid()) {
          if (strcmp(t2.arg, "false") == 0) {
            val = false;
            release_argument(t);
            release_argument(t2);
          }
          else if (strcmp(t2.arg, "true") == 0) {
            val = true;
            release_argument(t);
            release_argument(t2);
          }
        }
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    bool reinterpret(const char *str) {
      argument t = find_argument(str);
      if (t.is_valid()) {
        release_argument(t);
        return true;
      }
      else
        return false;
    }    

    ///////////////////////////////////////////////////////////////////////////
    void reinterpret_to_bool(const char *str, int& val) {
      argument t = find_argument(str);
      if (t.is_valid()) {
        argument t2 = get_next_value(t);
        if (t2.is_valid()) {
          if (strcmp(t2.arg, "false") == 0) {
            val = 0;
            release_argument(t);
            release_argument(t2);
          }
          else if (strcmp(t2.arg, "true") == 0) {
            val = 1;
            release_argument(t);
            release_argument(t2);
          }
        }
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    void reinterpret(const char *str, char *& val) {
      argument t = find_argument(str);
      if (t.is_valid()) {
        argument t2 = get_next_value(t);
        if (t2.is_valid()) {
          val = t2.arg;
          release_argument(t);
          release_argument(t2);
        }
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    struct arg_inter_base { virtual void operate(const char *) = 0; };

    ///////////////////////////////////////////////////////////////////////////
    void reinterpret(const char *str, arg_inter_base* fun) {
      argument t = find_argument(str);
      if (t.is_valid()) {
        argument t2 = get_next_value(t);
        if (t2.is_valid()) {
          fun->operate(t2.arg);
          release_argument(t);
          release_argument(t2);
        }
      }
    }

  private:
    char **argv;
    int argc;
    ui8 avail_store[16];//this should be enough for 128 command line arguments
    ui8 *avail;
  };
}

#endif // !OJPH_ARG_H
