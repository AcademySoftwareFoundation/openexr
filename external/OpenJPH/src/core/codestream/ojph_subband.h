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
// File: ojph_subband.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_SUBBAND_H
#define OJPH_SUBBAND_H

#include "ojph_defs.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //defined elsewhere
  class line_buf;
  class mem_elastic_allocator;
  class codestream;

  namespace local {

    //////////////////////////////////////////////////////////////////////////
    //defined here
    class resolution;
    struct precinct;
    class codeblock;
    struct coded_cb_header;
  
  //////////////////////////////////////////////////////////////////////////
    class subband
    {
      friend struct precinct;
    public:
      subband() { 
        res_num = band_num = 0;
        reversible = false;
        empty = true;             // <---- true
        lines = NULL;
        parent = NULL;
        blocks = NULL;
        xcb_prime = ycb_prime = 0;
        cur_cb_row = 0;
        cur_line = 0;
        cur_cb_height = 0;
        delta = delta_inv = 0.0f;
        K_max = 0;
        coded_cbs = NULL;
        elastic = NULL;
      }

      static void pre_alloc(codestream *codestream, const rect& band_rect,
                            ui32 comp_num, ui32 res_num, ui32 transform_flags);
      void finalize_alloc(codestream *codestream, const rect& band_rect,
                          resolution* res, ui32 res_num, ui32 subband_num);

      void exchange_buf(line_buf* l);
      line_buf* get_line() { return lines; }
      void push_line();

      void get_cb_indices(const size& num_precincts, precinct *precincts);
      float get_delta() { return delta; }
      bool exists() { return !empty; }

      line_buf* pull_line();
      resolution* get_parent() { return parent; }
      const resolution* get_parent() const { return parent; }

    private:
      bool empty;                  // true if the subband has no pixels or
                                   // the subband is NOT USED
      ui32 res_num, band_num;
      bool reversible;
      rect band_rect;
      line_buf *lines;
      resolution* parent;
      codeblock* blocks;
      size num_blocks;
      size log_PP;
      ui32 xcb_prime, ycb_prime;
      ui32 cur_cb_row;
      int cur_line;
      int cur_cb_height;
      float delta, delta_inv;
      ui32 K_max;
      coded_cb_header *coded_cbs;
      mem_elastic_allocator *elastic;
    };

  }
}

#endif // !OJPH_SUBBAND_H