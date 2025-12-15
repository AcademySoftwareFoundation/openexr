
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
// File: ojph_subband.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>

#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream_local.h"
#include "ojph_subband.h"
#include "ojph_resolution.h"
#include "ojph_codeblock.h"
#include "ojph_precinct.h"

namespace ojph {

  namespace local
  {

    //////////////////////////////////////////////////////////////////////////
    void subband::pre_alloc(codestream *codestream, const rect &band_rect,
                            ui32 comp_num, ui32 res_num, ui32 transform_flags)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();

      bool empty = ((band_rect.siz.w == 0) || (band_rect.siz.h == 0));
      if (empty)
        return;

      const param_cod* cdp = codestream->get_coc(comp_num);
      size log_cb = cdp->get_log_block_dims();
      size log_PP = cdp->get_log_precinct_size(res_num);

      ui32 x_off = ((transform_flags & resolution::HORZ_TRX) ? 1 : 0);
      ui32 y_off = ((transform_flags & resolution::VERT_TRX) ? 1 : 0);

      ui32 xcb_prime = ojph_min(log_cb.w, log_PP.w - x_off);
      ui32 ycb_prime = ojph_min(log_cb.h, log_PP.h - y_off);

      size nominal(1 << xcb_prime, 1 << ycb_prime);

      ui32 tbx0 = band_rect.org.x;
      ui32 tby0 = band_rect.org.y;
      ui32 tbx1 = band_rect.org.x + band_rect.siz.w;
      ui32 tby1 = band_rect.org.y + band_rect.siz.h;

      size num_blocks;
      num_blocks.w = (tbx1 + (1 << xcb_prime) - 1) >> xcb_prime;
      num_blocks.w -= tbx0 >> xcb_prime;
      num_blocks.h = (tby1 + (1 << ycb_prime) - 1) >> ycb_prime;
      num_blocks.h -= tby0 >> ycb_prime;

      allocator->pre_alloc_obj<codeblock>(num_blocks.w);
      //allocate codeblock headers
      allocator->pre_alloc_obj<coded_cb_header>((size_t)num_blocks.area());

      const param_qcd* qp = codestream->access_qcd()->get_qcc(comp_num);
      ui32 precision = qp->propose_precision(cdp);
      const param_atk* atk = cdp->access_atk();
      bool reversible = atk->is_reversible();

      for (ui32 i = 0; i < num_blocks.w; ++i)
        codeblock::pre_alloc(codestream, nominal, precision);

      //allocate lines
      allocator->pre_alloc_obj<line_buf>(1);
      //allocate line_buf
      ui32 width = band_rect.siz.w + 1;
      if (reversible)
      {
        if (precision <= 32)
          allocator->pre_alloc_data<si32>(width, 1);
        else
          allocator->pre_alloc_data<si64>(width, 1);
      }
      else
        allocator->pre_alloc_data<float>(width, 1);
    }

    //////////////////////////////////////////////////////////////////////////
    void subband::finalize_alloc(codestream *codestream,
                                 const rect &band_rect,
                                 resolution* res, ui32 res_num,
                                 ui32 subband_num)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();
      elastic = codestream->get_elastic_alloc();

      this->res_num = res_num;
      this->band_num = subband_num;
      this->band_rect = band_rect;
      this->parent = res;

      const param_cod* cdp = codestream->get_coc(parent->get_comp_num());
      this->reversible = cdp->access_atk()->is_reversible();
      size log_cb = cdp->get_log_block_dims();
      log_PP = cdp->get_log_precinct_size(res_num);

      ui32 x_off = ((parent->has_horz_transform()) ? 1 : 0);
      ui32 y_off = ((parent->has_vert_transform()) ? 1 : 0);

      xcb_prime = ojph_min(log_cb.w, log_PP.w - x_off);
      ycb_prime = ojph_min(log_cb.h, log_PP.h - y_off);

      size nominal(1 << xcb_prime, 1 << ycb_prime);

      cur_cb_row = 0;
      cur_line = 0;
      cur_cb_height = 0;
      const param_dfs* dfs = NULL;
      if (cdp->is_dfs_defined()) {
        dfs = codestream->access_dfs();
        if (dfs != NULL)
          dfs = dfs->get_dfs(cdp->get_dfs_index());
      }
      ui32 comp_num = parent->get_comp_num();
      const param_qcd* qcd = codestream->access_qcd()->get_qcc(comp_num);
      ui32 num_decomps = cdp->get_num_decompositions();
      this->K_max = qcd->get_Kmax(dfs, num_decomps, this->res_num, band_num);
      if (!reversible)
      {
        float d = 
          qcd->get_irrev_delta(dfs, num_decomps, res_num, subband_num);
        d /= (float)(1u << (31 - this->K_max));
        delta = d;
        delta_inv = (1.0f/d);
      }
      ui32 precision = qcd->propose_precision(cdp);

      this->empty = ((band_rect.siz.w == 0) || (band_rect.siz.h == 0));
      if (this->empty)
        return;

      ui32 tbx0 = band_rect.org.x;
      ui32 tby0 = band_rect.org.y;
      ui32 tbx1 = band_rect.org.x + band_rect.siz.w;
      ui32 tby1 = band_rect.org.y + band_rect.siz.h;

      num_blocks = size();
      num_blocks.w = (tbx1 + (1 << xcb_prime) - 1) >> xcb_prime;
      num_blocks.w -= tbx0 >> xcb_prime;
      num_blocks.h = (tby1 + (1 << ycb_prime) - 1) >> ycb_prime;
      num_blocks.h -= tby0 >> ycb_prime;

      blocks = allocator->post_alloc_obj<codeblock>(num_blocks.w);
      //allocate codeblock headers
      coded_cb_header *cp = coded_cbs =
        allocator->post_alloc_obj<coded_cb_header>((size_t)num_blocks.area());
      memset(coded_cbs, 0, sizeof(coded_cb_header) * (size_t)num_blocks.area());
      for (int i = (int)num_blocks.area(); i > 0; --i, ++cp)
        cp->Kmax = K_max;

      ui32 x_lower_bound = (tbx0 >> xcb_prime) << xcb_prime;
      ui32 y_lower_bound = (tby0 >> ycb_prime) << ycb_prime;

      size cb_size;
      cb_size.h = ojph_min(tby1, y_lower_bound + nominal.h) - tby0;
      cur_cb_height = (si32)cb_size.h;
      int line_offset = 0;
      for (ui32 i = 0; i < num_blocks.w; ++i)
      {
        ui32 cbx0 = ojph_max(tbx0, x_lower_bound + i * nominal.w);
        ui32 cbx1 = ojph_min(tbx1, x_lower_bound + (i + 1) * nominal.w);
        cb_size.w = cbx1 - cbx0;
        blocks[i].finalize_alloc(codestream, this, nominal, cb_size,
                                 coded_cbs + i, K_max, line_offset, 
                                 precision, comp_num);
        line_offset += cb_size.w;
      }

      //allocate lines
      lines = allocator->post_alloc_obj<line_buf>(1);
      //allocate line_buf
      ui32 width = band_rect.siz.w + 1;
      if (reversible)
      {
        if (precision <= 32)      
          lines->wrap(allocator->post_alloc_data<si32>(width, 1), width, 1);
        else
          lines->wrap(allocator->post_alloc_data<si64>(width, 1), width, 1);
      }
      else
        lines->wrap(allocator->post_alloc_data<float>(width, 1), width, 1);
    }

    //////////////////////////////////////////////////////////////////////////
    void subband::get_cb_indices(const size& num_precincts,
                                 precinct *precincts)
    {
      if (empty)
        return;

      rect res_rect = parent->get_rect();
      ui32 trx0 = res_rect.org.x;
      ui32 try0 = res_rect.org.y;
      ui32 trx1 = res_rect.org.x + res_rect.siz.w;
      ui32 try1 = res_rect.org.y + res_rect.siz.h;

      ui32 pc_lft = (res_rect.org.x >> log_PP.w) << log_PP.w;
      ui32 pc_top = (res_rect.org.y >> log_PP.h) << log_PP.h;

      ui32 pcx0, pcx1, pcy0, pcy1;
      ui32 x_shift = parent->has_horz_transform() ? 1 : 0;
      ui32 y_shift = parent->has_vert_transform() ? 1 : 0;
      ui32 yb, xb, coly = 0, colx = 0;
      for (ui32 y = 0; y < num_precincts.h; ++y)
      {
        pcy0 = ojph_max(try0, pc_top + (y << log_PP.h));
        pcy1 = ojph_min(try1, pc_top + ((y + 1) << log_PP.h));
        pcy0 = (pcy0 - (band_num >> 1) + (1 << y_shift) - 1) >> y_shift;
        pcy1 = (pcy1 - (band_num >> 1) + (1 << y_shift) - 1) >> y_shift;

        precinct *p = precincts + y * num_precincts.w;
        yb = ((pcy1 + (1<<ycb_prime) - 1) >> ycb_prime);
        yb -= (pcy0 >> ycb_prime);
        colx = 0;

        for (ui32 x = 0; x < num_precincts.w; ++x, ++p)
        {
          pcx0 = ojph_max(trx0, pc_lft + (x << log_PP.w));
          pcx1 = ojph_min(trx1, pc_lft + ((x + 1) << log_PP.w));
          pcx0 = (pcx0 - (band_num & 1) + (1 << x_shift) - 1) >> x_shift;
          pcx1 = (pcx1 - (band_num & 1) + (1 << x_shift) - 1) >> x_shift;

          rect *bp = p->cb_idxs + band_num;
          xb = ((pcx1 + (1<<xcb_prime) - 1) >> xcb_prime);
          xb -= (pcx0 >> xcb_prime);

          bp->org.x = colx;
          bp->org.y = coly;
          bp->siz.w = xb;
          bp->siz.h = yb;

          colx += xb;
        }
        coly += yb;
      }
      assert(colx == num_blocks.w && coly == num_blocks.h);
    }

    //////////////////////////////////////////////////////////////////////////
    void subband::exchange_buf(line_buf *l)
    {
      if (empty)
        return;

      assert(l->pre_size == lines[0].pre_size && l->size == lines[0].size &&
             l->flags == lines[0].flags);
      void* p = lines[0].p;
      lines[0].p = l->p;
      l->p = p;
    }

    //////////////////////////////////////////////////////////////////////////
    void subband::push_line()
    {
      if (empty)
        return;

      //push to codeblocks
      for (ui32 i = 0; i < num_blocks.w; ++i)
        blocks[i].push(lines + 0);
      if (++cur_line >= cur_cb_height)
      {
        for (ui32 i = 0; i < num_blocks.w; ++i)
          blocks[i].encode(elastic);

        if (++cur_cb_row < num_blocks.h)
        {
          cur_line = 0;

          ui32 tbx0 = band_rect.org.x;
          ui32 tby0 = band_rect.org.y;
          ui32 tbx1 = band_rect.org.x + band_rect.siz.w;
          ui32 tby1 = band_rect.org.y + band_rect.siz.h;
          size nominal(1 << xcb_prime, 1 << ycb_prime);

          ui32 x_lower_bound = (tbx0 >> xcb_prime) << xcb_prime;
          ui32 y_lower_bound = (tby0 >> ycb_prime) << ycb_prime;
          ui32 cby0 = y_lower_bound + cur_cb_row * nominal.h;
          ui32 cby1 = ojph_min(tby1, cby0 + nominal.h);

          size cb_size;
          cb_size.h = cby1 - ojph_max(tby0, cby0);
          cur_cb_height = (int)cb_size.h;
          for (ui32 i = 0; i < num_blocks.w; ++i)
          {
            ui32 cbx0 = ojph_max(tbx0, x_lower_bound + i * nominal.w);
            ui32 cbx1 = ojph_min(tbx1, x_lower_bound + (i + 1) * nominal.w);
            cb_size.w = cbx1 - cbx0;
            blocks[i].recreate(cb_size,
                               coded_cbs + i + cur_cb_row * num_blocks.w);
          }
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf *subband::pull_line()
    {
      if (empty)
        return lines;

      //pull from codeblocks
      if (--cur_line <= 0)
      {
        if (cur_cb_row < num_blocks.h)
        {
          ui32 tbx0 = band_rect.org.x;
          ui32 tby0 = band_rect.org.y;
          ui32 tbx1 = band_rect.org.x + band_rect.siz.w;
          ui32 tby1 = band_rect.org.y + band_rect.siz.h;
          size nominal(1 << xcb_prime, 1 << ycb_prime);

          ui32 x_lower_bound = (tbx0 >> xcb_prime) << xcb_prime;
          ui32 y_lower_bound = (tby0 >> ycb_prime) << ycb_prime;
          ui32 cby0 = ojph_max(tby0, y_lower_bound + cur_cb_row * nominal.h);
          ui32 cby1 = ojph_min(tby1, y_lower_bound+(cur_cb_row+1)*nominal.h);

          size cb_size;
          cb_size.h = cby1 - cby0;
          cur_line = cur_cb_height = (int)cb_size.h;
          for (ui32 i = 0; i < num_blocks.w; ++i)
          {
            ui32 cbx0 = ojph_max(tbx0, x_lower_bound + i * nominal.w);
            ui32 cbx1 = ojph_min(tbx1, x_lower_bound + (i + 1) * nominal.w);
            cb_size.w = cbx1 - cbx0;
            blocks[i].recreate(cb_size,
                               coded_cbs + i + cur_cb_row * num_blocks.w);
            blocks[i].decode();
          }
          ++cur_cb_row;
        }
      }

      assert(cur_line >= 0);

      //pull from codeblocks
      for (ui32 i = 0; i < num_blocks.w; ++i)
        blocks[i].pull_line(lines + 0);

      return lines;
    }

  }
}