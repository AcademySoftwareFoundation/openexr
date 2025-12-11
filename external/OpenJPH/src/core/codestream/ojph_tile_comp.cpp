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
// File: ojph_tile_comp.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>

#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream_local.h"
#include "ojph_tile_comp.h"
#include "ojph_resolution.h"

namespace ojph {

  namespace local
  {

    //////////////////////////////////////////////////////////////////////////
    void tile_comp::pre_alloc(codestream *codestream, ui32 comp_num, 
                              const rect& comp_rect,
                              const rect& recon_comp_rect)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();

      //allocate a resolution
      ui32 num_decomps;
      num_decomps = codestream->get_coc(comp_num)->get_num_decompositions();
      allocator->pre_alloc_obj<resolution>(1);

      resolution::pre_alloc(codestream, comp_rect, recon_comp_rect, comp_num, 
                            num_decomps);
    }

    //////////////////////////////////////////////////////////////////////////
    void tile_comp::finalize_alloc(codestream *codestream, tile *parent,
                                  ui32 comp_num, const rect& comp_rect,
                                  const rect& recon_comp_rect)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();

      //allocate a resolution
      num_decomps = codestream->get_coc(comp_num)->get_num_decompositions();

      comp_downsamp = codestream->get_siz()->get_downsampling(comp_num);
      this->comp_rect = comp_rect;
      this->parent_tile = parent;

      this->comp_num = comp_num;
      this->num_bytes = 0;
      res = allocator->post_alloc_obj<resolution>(1);
      res->finalize_alloc(codestream, comp_rect, recon_comp_rect, comp_num,
                          num_decomps, comp_downsamp, comp_downsamp, this, 
                          NULL);
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf* tile_comp::get_line()
    {
      return res->get_line();
    }

    //////////////////////////////////////////////////////////////////////////
    void tile_comp::push_line()
    {
      res->push_line();
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf* tile_comp::pull_line()
    {
      return res->pull_line();
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 tile_comp::prepare_precincts()
    {
      this->num_bytes = res->prepare_precinct();
      return this->num_bytes;
    }

    //////////////////////////////////////////////////////////////////////////
    void tile_comp::write_precincts(ui32 res_num, outfile_base *file)
    {
      assert(res_num <= num_decomps);
      res_num = num_decomps - res_num; //how many levels to go down
      resolution *r = res;
      while (res_num > 0 && r != NULL)
      {
        r = r->next_resolution();
        --res_num;
      }
      if (r) //resolution does not exist if r is NULL
        r->write_precincts(file);
    }

    //////////////////////////////////////////////////////////////////////////
    bool tile_comp::get_top_left_precinct(ui32 res_num, point &top_left)
    {
      int resolution_num = (int)num_decomps - (int)res_num;
      resolution *r = res;
       while (resolution_num > 0 && r != NULL)
      {
        r = r->next_resolution();
        --resolution_num;
      }
      if (r) //resolution does not exist if r is NULL
        return r->get_top_left_precinct(top_left);
      else
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    void tile_comp::write_one_precinct(ui32 res_num, outfile_base *file)
    {
      int resolution_num = (int)num_decomps - (int)res_num;
      resolution *r = res;
      while (resolution_num > 0 && r != NULL)
      {
        r = r->next_resolution();
        --resolution_num;
      }
      if (r) //resolution does not exist if r is NULL
        r->write_one_precinct(file);
    }

    //////////////////////////////////////////////////////////////////////////
    void tile_comp::parse_precincts(ui32 res_num, ui32& data_left,
                                    infile_base *file)
    {
      assert(res_num <= num_decomps);
      res_num = num_decomps - res_num; //how many levels to go down
      resolution *r = res;
      while (res_num > 0 && r != NULL)
      {
        r = r->next_resolution();
        --res_num;
      }
      if (r) //resolution does not exist if r is NULL
        r->parse_all_precincts(data_left, file);
    }


    //////////////////////////////////////////////////////////////////////////
    void tile_comp::parse_one_precinct(ui32 res_num, ui32& data_left,
                                       infile_base *file)
    {
      assert(res_num <= num_decomps);
      res_num = num_decomps - res_num;
      resolution *r = res;
      while (res_num > 0 && r != NULL)
      {
        r = r->next_resolution();
        --res_num;
      }
      if (r) //resolution does not exist if r is NULL
        r->parse_one_precinct(data_left, file);
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 tile_comp::get_num_bytes(ui32 resolution_num) const
    {
      return res->get_num_bytes(resolution_num);
    }
  }
}
