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
// File: ojph_resolution.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_RESOLUTION_H
#define OJPH_RESOLUTION_H

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
    class tile_comp;
    struct precinct;
    class subband;

    //////////////////////////////////////////////////////////////////////////
    class resolution
    {
    public:
      enum : ui32 {
        HORZ_TRX = 0x01,   // horizontal transform
        VERT_TRX = 0x02,   // vertical transform
      };

    public:
      static void pre_alloc(codestream *codestream, const rect& res_rect,
                            const rect& recon_res_rect, 
                            ui32 comp_num, ui32 res_num);
      void finalize_alloc(codestream *codestream, const rect& res_rect,
                          const rect& recon_res_rect, ui32 comp_num,
                          ui32 res_num, point comp_downsamp, 
                          point res_downsamp, tile_comp *parent_tile_comp,
                          resolution *parent_res);

      line_buf* get_line();
      void push_line();
      line_buf* pull_line();
      rect get_rect() { return res_rect; }
      ui32 get_comp_num() { return comp_num; }
      bool has_horz_transform() { return (transform_flags & HORZ_TRX) != 0; }
      bool has_vert_transform() { return (transform_flags & VERT_TRX) != 0; }

      ui32 prepare_precinct();
      void write_precincts(outfile_base *file);
      bool get_top_left_precinct(point &top_left);
      void write_one_precinct(outfile_base *file);
      resolution *next_resolution() { return child_res; }
      void parse_all_precincts(ui32& data_left, infile_base *file);
      void parse_one_precinct(ui32& data_left, infile_base *file);

      ui32 get_num_bytes() const { return num_bytes; }
      ui32 get_num_bytes(ui32 resolution_num) const;

    private:
      bool reversible, skipped_res_for_read, skipped_res_for_recon;
      ui32 num_steps;
      ui32 res_num;
      ui32 comp_num;
      ui32 num_bytes; // number of bytes in this resolution 
                      // used for tilepart length
      point comp_downsamp;
      rect res_rect;                             // resolution rectangle
      line_buf* lines;                           // used to store lines
      lifting_buf *ssp;                          // step state pointer
      lifting_buf *aug, *sig;
      subband *bands;
      tile_comp *parent_comp;
      resolution *parent_res, *child_res;
      //precincts stuff
      precinct *precincts;
      size num_precincts;
      size log_PP;
      ui32 max_num_levels;
      int tag_tree_size;
      ui32 level_index[20]; //more than enough
      point cur_precinct_loc; //used for progressing spatial modes (2, 3, 4)
      const param_atk* atk;
      ui32 transform_flags;
      //wavelet machinery
      ui32 cur_line;
      ui32 rows_to_produce;
      bool vert_even, horz_even;
      mem_elastic_allocator *elastic;
    };

  }
}

#endif // !OJPH_RESOLUTION_H