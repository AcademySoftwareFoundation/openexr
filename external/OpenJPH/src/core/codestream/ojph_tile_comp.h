
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
// File: ojph_tile_comp.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_TILE_COMP_H
#define OJPH_TILE_COMP_H

#include "ojph_defs.h"
#include "ojph_base.h"
#include "ojph_file.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //defined elsewhere
  class line_buf;
  class codestream;

  namespace local {

    //////////////////////////////////////////////////////////////////////////
    //defined here
    class tile;
    class resolution;

    //////////////////////////////////////////////////////////////////////////
    class tile_comp
    {
    public:
      static void pre_alloc(codestream *codestream, ui32 comp_num, 
                            const rect& comp_rect,
                            const rect& recon_comp_rect);
      void finalize_alloc(codestream *codestream, tile *parent,
                          ui32 comp_num, const rect& comp_rect,
                          const rect& recon_comp_rect);

      ui32 get_num_resolutions() { return num_decomps + 1; }
      ui32 get_num_decompositions() { return num_decomps; }
      tile* get_tile() { return parent_tile; }
      line_buf* get_line();
      void push_line();
      line_buf* pull_line();

      ui32 prepare_precincts();
      void write_precincts(ui32 res_num, outfile_base *file);
      bool get_top_left_precinct(ui32 res_num, point &top_left);
      void write_one_precinct(ui32 res_num, outfile_base *file);
      void parse_precincts(ui32 res_num, ui32& data_left, infile_base *file);
      void parse_one_precinct(ui32 res_num, ui32& data_left, 
                              infile_base *file);

      ui32 get_num_bytes() const { return num_bytes; }
      ui32 get_num_bytes(ui32 resolution_num) const;

    private:
      tile *parent_tile;
      resolution *res;
      rect comp_rect;
      ojph::point comp_downsamp;
      ui32 num_decomps;
      ui32 comp_num;
      ui32 num_bytes; // number of bytes in this tile component
                      // used for tilepart length
    };

  }
}

#endif // !OJPH_TILE_COMP_H