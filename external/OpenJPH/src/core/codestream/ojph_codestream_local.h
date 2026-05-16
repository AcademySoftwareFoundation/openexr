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
// File: ojph_codestream_local.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_CODESTREAM_LOCAL_H
#define OJPH_CODESTREAM_LOCAL_H

#include "ojph_defs.h"
#include "ojph_params_local.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //defined elsewhere
  class line_buf;
  class mem_fixed_allocator;
  class mem_elastic_allocator;
  class codestream;

  namespace local {

    /////////////////////////////////////////////////////////////////////////
    static inline
    ui16 swap_byte(ui16 t)
    {
      return (ui16)((t << 8) | (t >> 8));
    }

    //////////////////////////////////////////////////////////////////////////
    //defined elsewhere
    class tile;

    //////////////////////////////////////////////////////////////////////////
    class codestream
    {
      friend ::ojph::codestream;

    public:
      codestream();
      ~codestream();

      void restart();

      void pre_alloc();
      void finalize_alloc();

      ojph::param_siz access_siz()            // returns externally wrapped siz
      { return ojph::param_siz(&siz); }
      const param_siz* get_siz()              // returns internal siz
      { return &siz; }
      ojph::param_cod access_cod()            // returns externally wrapped cod
      { return ojph::param_cod(&cod); }
      const param_cod* get_cod()              // returns internal cod
      { return &cod; }
      const param_cod* get_coc(ui32 comp_num) // returns internal cod
      { return cod.get_coc(comp_num); }
      const param_qcd* access_qcd()
      { return &qcd; }
      const param_dfs* access_dfs()
      { if (dfs.exists()) return &dfs; else return NULL; }
      const param_nlt* get_nlt()
      { return &nlt; }
      mem_fixed_allocator* get_allocator() { return allocator; }
      mem_elastic_allocator* get_elastic_alloc() { return elastic_alloc; }
      outfile_base* get_file() { return outfile; }

      line_buf* exchange(line_buf* line, ui32& next_component);
      void write_headers(outfile_base *file, const comment_exchange* comments,
                         ui32 num_comments);
      void enable_resilience();
      bool is_resilient() { return resilient; }
      void read_headers(infile_base *file);
      void restrict_input_resolution(ui32 skipped_res_for_data,
        ui32 skipped_res_for_recon);
      void read();
      void set_planar(int planar);
      void set_profile(const char *s);
      void set_tilepart_divisions(ui32 value);
      void request_tlm_marker(bool needed);
      line_buf* pull(ui32 &comp_num);
      void flush();
      void close();

      bool is_planar() const { return planar != 0; }
      si32 get_profile() const { return profile; };
      ui32 get_tilepart_div() const { return tilepart_div; };
      bool is_tlm_needed() const { return need_tlm; };

      void check_imf_validity();
      void check_broadcast_validity();

      ui8* get_precinct_scratch() { return precinct_scratch; }
      ui32 get_skipped_res_for_recon()
      { return skipped_res_for_recon; }
      ui32 get_skipped_res_for_read()
      { return skipped_res_for_read; }

    private:
      ui32 precinct_scratch_needed_bytes;
      ui8* precinct_scratch;

    private:
      ui32 cur_line;
      ui32 cur_comp;
      ui32 cur_tile_row;
      bool resilient;
      ui32 skipped_res_for_read, skipped_res_for_recon;

    private:
      size num_tiles;
      tile *tiles;
      line_buf* lines;
      ui32 num_comps;
      size *comp_size;       //stores full resolution no. of lines and width
      size *recon_comp_size; //stores number of lines and width of each comp
      bool employ_color_transform;
      int planar;
      int profile;
      ui32 tilepart_div;     // tilepart division value
      bool need_tlm;         // true if tlm markers are needed

    private:
      param_siz siz;         // image and tile size
      param_cod cod;         // coding style default
      param_cap cap;         // extended capabilities
      param_qcd qcd;         // quantization default
      param_tlm tlm;         // tile-part lengths
      param_nlt nlt;         // non-linearity point transformation

    private:  // these are from Part 2 of the standard
      param_dfs dfs;         // downsmapling factor styles
      param_atk atk;         // wavelet structure and coefficients

    private:
      mem_fixed_allocator *allocator;
      mem_elastic_allocator *elastic_alloc;
      outfile_base *outfile;
      infile_base *infile;
    };

  }
}

#endif // !OJPH_CODESTREAM_LOCAL_H
