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
// File: ojph_resolution.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>
#include <new>

#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream_local.h"
#include "ojph_resolution.h"
#include "ojph_tile_comp.h"
#include "ojph_tile.h"
#include "ojph_subband.h"
#include "ojph_precinct.h"

#include "../transform/ojph_transform.h"

namespace ojph {

  namespace local
  {
    //////////////////////////////////////////////////////////////////////////
    void resolution::pre_alloc(codestream* codestream, const rect& res_rect,
                               const rect& recon_res_rect, 
                               ui32 comp_num, ui32 res_num)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();
      const param_cod* cdp = codestream->get_coc(comp_num);
      ui32 num_decomps = cdp->get_num_decompositions();
      ui32 t = num_decomps - codestream->get_skipped_res_for_recon();
      bool skipped_res_for_recon = res_num > t;

      const param_atk* atk = cdp->access_atk();
      param_dfs::dfs_dwt_type ds = param_dfs::BIDIR_DWT;
      if (cdp->is_dfs_defined()) {
        const param_dfs* dfs = codestream->access_dfs();
        if (dfs == NULL) {
          OJPH_ERROR(0x00070001, "There is a problem with codestream "
            "marker segments. COD/COC specifies the use of a DFS marker "
            "but there are no DFS markers within the main codestream "
            "headers");
        }
        else {
          ui16 dfs_idx = cdp->get_dfs_index();
          dfs = dfs->get_dfs(dfs_idx);
          if (dfs == NULL) {
            OJPH_ERROR(0x00070002, "There is a problem with codestream "
              "marker segments. COD/COC specifies the use of a DFS marker "
              "with index %d, but there are no such marker within the "
              "main codestream headers", dfs_idx);
          }
          ds = dfs->get_dwt_type(num_decomps - res_num + 1);
        }
      }

      ui32 transform_flags = 0;
      if (res_num > 0)
      {
        if (ds == param_dfs::BIDIR_DWT)
          transform_flags = HORZ_TRX | VERT_TRX;
        else if (ds == param_dfs::HORZ_DWT)
          transform_flags = HORZ_TRX;
        else if (ds == param_dfs::VERT_DWT)
          transform_flags = VERT_TRX;
      }

      //allocate resolution/subbands
      ui32 trx0 = res_rect.org.x;
      ui32 try0 = res_rect.org.y;
      ui32 trx1 = res_rect.org.x + res_rect.siz.w;
      ui32 try1 = res_rect.org.y + res_rect.siz.h;
      allocator->pre_alloc_obj<subband>(4);
      if (res_num > 0)
      {
        if (ds == param_dfs::BIDIR_DWT)
        {
          for (ui32 i = 0; i < 4; ++i)
          {
            ui32 tbx0 = (trx0 - (i & 1) + 1) >> 1;
            ui32 tbx1 = (trx1 - (i & 1) + 1) >> 1;
            ui32 tby0 = (try0 - (i >> 1) + 1) >> 1;
            ui32 tby1 = (try1 - (i >> 1) + 1) >> 1;

            rect re;
            re.org.x = tbx0;
            re.org.y = tby0;
            re.siz.w = tbx1 - tbx0;
            re.siz.h = tby1 - tby0;
            if (i == 0) {
              allocator->pre_alloc_obj<resolution>(1);
              resolution::pre_alloc(codestream, re,
                skipped_res_for_recon ? recon_res_rect : re,
                comp_num, res_num - 1);
            }
            else
              subband::pre_alloc(codestream, re, comp_num, res_num,
                                 transform_flags);
          }
        }
        else if (ds == param_dfs::VERT_DWT)
        {
          ui32 tby0, tby1;
          rect re = res_rect;
          tby0 = (try0 + 1) >> 1;
          tby1 = (try1 + 1) >> 1;
          re.org.y = tby0;
          re.siz.h = tby1 - tby0;
          allocator->pre_alloc_obj<resolution>(1);
          resolution::pre_alloc(codestream, re,
            skipped_res_for_recon ? recon_res_rect : re,
            comp_num, res_num - 1);

          tby0 = try0 >> 1;
          tby1 = try1 >> 1;
          re.org.y = tby0;
          re.siz.h = tby1 - tby0;
          subband::pre_alloc(codestream, re, comp_num, res_num, 
                             transform_flags);
        }
        else if (ds == param_dfs::HORZ_DWT)
        {
          ui32 tbx0, tbx1;
          rect re = res_rect;
          tbx0 = (trx0 + 1) >> 1;
          tbx1 = (trx1 + 1) >> 1;
          re.org.x = tbx0;
          re.siz.w = tbx1 - tbx0;
          allocator->pre_alloc_obj<resolution>(1);
          resolution::pre_alloc(codestream, re,
            skipped_res_for_recon ? recon_res_rect : re,
            comp_num, res_num - 1);

          tbx0 = trx0 >> 1;
          tbx1 = trx1 >> 1;
          re.org.x = tbx0;
          re.siz.w = tbx1 - tbx0;
          subband::pre_alloc(codestream, re, comp_num, res_num, 
                             transform_flags);
        }
        else
        {
          assert(ds == param_dfs::NO_DWT);
          allocator->pre_alloc_obj<resolution>(1);
          resolution::pre_alloc(codestream, res_rect,
            skipped_res_for_recon ? recon_res_rect : res_rect,
            comp_num, res_num - 1);
        }
      }
      else
        subband::pre_alloc(codestream, res_rect, comp_num, res_num, 
                           transform_flags);

      //prealloc precincts
      size log_PP = cdp->get_log_precinct_size(res_num);
      size num_precincts;
      if (trx0 != trx1 && try0 != try1)
      {
        num_precincts.w = (trx1 + (1 << log_PP.w) - 1) >> log_PP.w;
        num_precincts.w -= trx0 >> log_PP.w;
        num_precincts.h = (try1 + (1 << log_PP.h) - 1) >> log_PP.h;
        num_precincts.h -= try0 >> log_PP.h;
        allocator->pre_alloc_obj<precinct>((size_t)num_precincts.area());
      }

      //allocate lines
      if (skipped_res_for_recon == false)
      {
        ui32 num_steps = atk->get_num_steps();
        allocator->pre_alloc_obj<line_buf>(num_steps + 2);
        allocator->pre_alloc_obj<lifting_buf>(num_steps + 2);

        const param_qcd* qp = codestream->access_qcd()->get_qcc(comp_num);
        ui32 precision = qp->propose_precision(cdp);
        const param_atk* atk = cdp->access_atk();
        bool reversible = atk->is_reversible();

        ui32 width = res_rect.siz.w + 1;
        if (reversible)
        {
          if (precision <= 32) {
            for (ui32 i = 0; i < num_steps; ++i)
              allocator->pre_alloc_data<si32>(width, 1);
            allocator->pre_alloc_data<si32>(width, 1);
            allocator->pre_alloc_data<si32>(width, 1);
          }
          else 
          {
            for (ui32 i = 0; i < num_steps; ++i)
              allocator->pre_alloc_data<si64>(width, 1);
            allocator->pre_alloc_data<si64>(width, 1);
            allocator->pre_alloc_data<si64>(width, 1);
          }
        }
        else {
          for (ui32 i = 0; i < num_steps; ++i)
            allocator->pre_alloc_data<float>(width, 1);
          allocator->pre_alloc_data<float>(width, 1);
          allocator->pre_alloc_data<float>(width, 1);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::finalize_alloc(codestream* codestream,
                                    const rect& res_rect,
                                    const rect& recon_res_rect,
                                    ui32 comp_num, ui32 res_num,
                                    point comp_downsamp, point res_downsamp,
                                    tile_comp* parent_tile_comp,
                                    resolution* parent_res)
    {
      mem_fixed_allocator* allocator = codestream->get_allocator();
      elastic = codestream->get_elastic_alloc();
      const param_cod* cdp = codestream->get_coc(comp_num);
      ui32 t, num_decomps = cdp->get_num_decompositions();
      t = num_decomps - codestream->get_skipped_res_for_recon();
      skipped_res_for_recon = res_num > t;
      t = num_decomps - codestream->get_skipped_res_for_read();
      skipped_res_for_read = res_num > t;

      this->comp_downsamp = comp_downsamp;
      this->parent_comp = parent_tile_comp;
      this->parent_res = parent_res;
      this->res_rect = res_rect;
      this->comp_num = comp_num;
      this->res_num = res_num;
      this->num_bytes = 0;
      this->atk = cdp->access_atk();
      param_dfs::dfs_dwt_type ds = param_dfs::BIDIR_DWT;
      if (cdp->is_dfs_defined()) {
        const param_dfs* dfs = codestream->access_dfs();
        if (dfs == NULL) {
          OJPH_ERROR(0x00070011, "There is a problem with codestream "
            "marker segments. COD/COC specifies the use of a DFS marker "
            "but there are no DFS markers within the main codestream "
            "headers");
        }
        else {
          ui16 dfs_idx = cdp->get_dfs_index();
          dfs = dfs->get_dfs(dfs_idx);
          if (dfs == NULL) {
            OJPH_ERROR(0x00070012, "There is a problem with codestream "
              "marker segments. COD/COC specifies the use of a DFS marker "
              "with index %d, but there are no such marker within the "
              "main codestream headers", dfs_idx);
          }
          ui32 num_decomps = cdp->get_num_decompositions();
          ds = dfs->get_dwt_type(num_decomps - res_num + 1);
        }
      }

      transform_flags = 0;
      if (res_num > 0)
      {
        if (ds == param_dfs::BIDIR_DWT)
          transform_flags = HORZ_TRX | VERT_TRX;
        else if (ds == param_dfs::HORZ_DWT)
          transform_flags = HORZ_TRX;
        else if (ds == param_dfs::VERT_DWT)
          transform_flags = VERT_TRX;
      }

      //allocate resolution/subbands
      ui32 trx0 = res_rect.org.x;
      ui32 try0 = res_rect.org.y;
      ui32 trx1 = res_rect.org.x + res_rect.siz.w;
      ui32 try1 = res_rect.org.y + res_rect.siz.h;
      bands = allocator->post_alloc_obj<subband>(4);
      for (int i = 0; i < 4; ++i)
        new (bands + i) subband;
      if (res_num > 0)
      {
        if (ds == param_dfs::BIDIR_DWT)
        {
          for (ui32 i = 0; i < 4; ++i)
          {
            ui32 tbx0 = (trx0 - (i & 1) + 1) >> 1;
            ui32 tbx1 = (trx1 - (i & 1) + 1) >> 1;
            ui32 tby0 = (try0 - (i >> 1) + 1) >> 1;
            ui32 tby1 = (try1 - (i >> 1) + 1) >> 1;

            rect re;
            re.org.x = tbx0;
            re.org.y = tby0;
            re.siz.w = tbx1 - tbx0;
            re.siz.h = tby1 - tby0;
            if (i == 0) {
              point next_res_downsamp;
              next_res_downsamp.x = res_downsamp.x * 2;
              next_res_downsamp.y = res_downsamp.y * 2;

              child_res = allocator->post_alloc_obj<resolution>(1);
              child_res->finalize_alloc(codestream, re,
                skipped_res_for_recon ? recon_res_rect : re, comp_num,
                res_num - 1, comp_downsamp, next_res_downsamp, 
                parent_tile_comp, this);
            }
            else
              bands[i].finalize_alloc(codestream, re, this, res_num, i);
          }
        }
        else if (ds == param_dfs::VERT_DWT)
        {
          ui32 tby0, tby1;
          rect re = res_rect;
          tby0 = (try0 + 1) >> 1;
          tby1 = (try1 + 1) >> 1;
          re.org.y = tby0;
          re.siz.h = tby1 - tby0;

          point next_res_downsamp;
          next_res_downsamp.x = res_downsamp.x;
          next_res_downsamp.y = res_downsamp.y * 2;
          child_res = allocator->post_alloc_obj<resolution>(1);
          child_res->finalize_alloc(codestream, re,
            skipped_res_for_recon ? recon_res_rect : re, comp_num,
            res_num - 1, comp_downsamp, next_res_downsamp,
            parent_tile_comp, this);

          tby0 = try0 >> 1;
          tby1 = try1 >> 1;
          re.org.y = tby0;
          re.siz.h = tby1 - tby0;
          bands[2].finalize_alloc(codestream, re, this, res_num, 2);
        }
        else if (ds == param_dfs::HORZ_DWT)
        {
          ui32 tbx0, tbx1;
          rect re = res_rect;
          tbx0 = (trx0 + 1) >> 1;
          tbx1 = (trx1 + 1) >> 1;
          re.org.x = tbx0;
          re.siz.w = tbx1 - tbx0;

          point next_res_downsamp;
          next_res_downsamp.x = res_downsamp.x * 2;
          next_res_downsamp.y = res_downsamp.y;
          child_res = allocator->post_alloc_obj<resolution>(1);
          child_res->finalize_alloc(codestream, re,
            skipped_res_for_recon ? recon_res_rect : re, comp_num,
            res_num - 1, comp_downsamp, next_res_downsamp,
            parent_tile_comp, this);

          tbx0 = trx0 >> 1;
          tbx1 = trx1 >> 1;
          re.org.x = tbx0;
          re.siz.w = tbx1 - tbx0;
          bands[1].finalize_alloc(codestream, re, this, res_num, 1);
        }
        else
        {
          assert(ds == param_dfs::NO_DWT);
          child_res = allocator->post_alloc_obj<resolution>(1);
          child_res->finalize_alloc(codestream, res_rect,
            skipped_res_for_recon ? recon_res_rect : res_rect, comp_num,
            res_num - 1, comp_downsamp, res_downsamp, parent_tile_comp, this);
        }
      }
      else {
        child_res = NULL;
        bands[0].finalize_alloc(codestream, res_rect, this, res_num, 0);
      }

      //finalize precincts
      log_PP = cdp->get_log_precinct_size(res_num);
      num_precincts = size();
      precincts = NULL;
      if (trx0 != trx1 && try0 != try1)
      {
        num_precincts.w = (trx1 + (1 << log_PP.w) - 1) >> log_PP.w;
        num_precincts.w -= trx0 >> log_PP.w;
        num_precincts.h = (try1 + (1 << log_PP.h) - 1) >> log_PP.h;
        num_precincts.h -= try0 >> log_PP.h;
        precincts = 
          allocator->post_alloc_obj<precinct>((size_t)num_precincts.area());
        ui64 num = num_precincts.area();
        for (ui64 i = 0; i < num; ++i)
          precincts[i] = precinct();
      }
      // precincts will be initialized in full shortly

      ui32 x_lower_bound = (trx0 >> log_PP.w) << log_PP.w;
      ui32 y_lower_bound = (try0 >> log_PP.h) << log_PP.h;

      precinct* pp = precincts;
      point tile_top_left = parent_tile_comp->get_tile()->get_tile_rect().org;
      for (ui32 y = 0; y < num_precincts.h; ++y)
      {
        ui32 ppy0 = y_lower_bound + (y << log_PP.h);
        for (ui32 x = 0; x < num_precincts.w; ++x, ++pp)
        {
          ui32 ppx0 = x_lower_bound + (x << log_PP.w);
          point t(res_downsamp.x * ppx0, res_downsamp.y * ppy0);
          t.x = t.x > tile_top_left.x ? t.x : tile_top_left.x;
          t.y = t.y > tile_top_left.y ? t.y : tile_top_left.y;
          pp->img_point = t;
          pp->bands = bands;
          pp->may_use_sop = cdp->packets_may_use_sop();
          pp->uses_eph = cdp->packets_use_eph();
          pp->scratch = codestream->get_precinct_scratch();
          pp->coded = NULL;
        }
      }
      for (int i = 0; i < 4; ++i)
        if (bands[i].exists())
          bands[i].get_cb_indices(num_precincts, precincts);

      // determine how to divide scratch into multiple levels of
      // tag trees
      size log_cb = cdp->get_log_block_dims();
      log_PP.w -= (transform_flags & HORZ_TRX) ? 1 : 0;
      log_PP.h -= (transform_flags & VERT_TRX) ? 1 : 0;
      size ratio;
      ratio.w = log_PP.w - ojph_min(log_cb.w, log_PP.w);
      ratio.h = log_PP.h - ojph_min(log_cb.h, log_PP.h);
      max_num_levels = ojph_max(ratio.w, ratio.h);
      ui32 val = 1u << (max_num_levels << 1);
      tag_tree_size = (int)((val * 4 + 2) / 3);
      ++max_num_levels;
      level_index[0] = 0;
      for (ui32 i = 1; i <= max_num_levels; ++i, val >>= 2)
        level_index[i] = level_index[i - 1] + val;
      cur_precinct_loc = point(0, 0);

      //allocate lines
      if (skipped_res_for_recon == false)
      {
        this->atk = cdp->access_atk();
        this->reversible = atk->is_reversible();
        this->num_steps = atk->get_num_steps();
        // create line buffers and lifting_bufs
        lines = allocator->post_alloc_obj<line_buf>(num_steps + 2);
        ssp = allocator->post_alloc_obj<lifting_buf>(num_steps + 2);
        sig = ssp + num_steps;
        aug = ssp + num_steps + 1;

        // initiate lifting_bufs
        for (ui32 i = 0; i < num_steps; ++i) {
          new (ssp + i) lifting_buf;
          ssp[i].line = lines + i;
        };
        new (sig) lifting_buf;
        sig->line = lines + num_steps;
        new (aug) lifting_buf;
        aug->line = lines + num_steps + 1;

        const param_qcd* qp = codestream->access_qcd()->get_qcc(comp_num);
        ui32 precision = qp->propose_precision(cdp);

        // initiate storage of line_buf
        ui32 width = res_rect.siz.w + 1;
        if (this->reversible)
        {
          if (precision <= 32)
          {
            for (ui32 i = 0; i < num_steps; ++i)
              ssp[i].line->wrap(
                allocator->post_alloc_data<si32>(width, 1), width, 1);
            sig->line->wrap(
              allocator->post_alloc_data<si32>(width, 1), width, 1);
            aug->line->wrap(
              allocator->post_alloc_data<si32>(width, 1), width, 1);
          }
          else
          {
            for (ui32 i = 0; i < num_steps; ++i)
              ssp[i].line->wrap(
                allocator->post_alloc_data<si64>(width, 1), width, 1);
            sig->line->wrap(
              allocator->post_alloc_data<si64>(width, 1), width, 1);
            aug->line->wrap(
              allocator->post_alloc_data<si64>(width, 1), width, 1);
          }
        }
        else 
        {
            for (ui32 i = 0; i < num_steps; ++i)
              ssp[i].line->wrap(
                allocator->post_alloc_data<float>(width, 1), width, 1);
            sig->line->wrap(
              allocator->post_alloc_data<float>(width, 1), width, 1);
            aug->line->wrap(
              allocator->post_alloc_data<float>(width, 1), width, 1);
        }

        cur_line = 0;
        rows_to_produce = res_rect.siz.h;
        vert_even = (res_rect.org.y & 1) == 0;
        horz_even = (res_rect.org.x & 1) == 0;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf* resolution::get_line()
    { 
      if (vert_even)
      {
        ++cur_line;
        sig->active = true;
        return sig->line;
      }
      else
      {
        ++cur_line;
        aug->active = true;
        return aug->line;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::push_line()
    {
      if (res_num == 0)
      {
        assert(child_res == NULL);
        bands[0].exchange_buf(vert_even ? sig->line : aug->line);
        bands[0].push_line();
        return;
      }

      ui32 width = res_rect.siz.w;
      if (width == 0)
        return;
      if (reversible)
      {
        if (res_rect.siz.h > 1)
        {
          if (!vert_even && cur_line < res_rect.siz.h) {
            vert_even = !vert_even;
            return;
          }

          do
          {
            //vertical transform
            for (ui32 i = 0; i < num_steps; ++i)
            {
              if (aug->active && (sig->active || ssp[i].active))
              {
                line_buf* dp = aug->line;
                line_buf* sp1 = sig->active ? sig->line : ssp[i].line;
                line_buf* sp2 = ssp[i].active ? ssp[i].line : sig->line;
                const lifting_step* s = atk->get_step(num_steps - i - 1);
                rev_vert_step(s, sp1, sp2, dp, width, false);
              }
              lifting_buf t = *aug; *aug = ssp[i]; ssp[i] = *sig; *sig = t;
            }

            if (aug->active) {
              rev_horz_ana(atk, bands[2].get_line(),
                bands[3].get_line(), aug->line, width, horz_even);
              bands[2].push_line();
              bands[3].push_line();
              aug->active = false;
              --rows_to_produce;
            }
            if (sig->active) {
              rev_horz_ana(atk, child_res->get_line(),
                bands[1].get_line(), sig->line, width, horz_even);
              bands[1].push_line();
              child_res->push_line();
              sig->active = false;
              --rows_to_produce;
            };
            vert_even = !vert_even;
          } while (cur_line >= res_rect.siz.h && rows_to_produce > 0);
        }
        else
        {
          if (vert_even) {
            // horizontal transform
            rev_horz_ana(atk, child_res->get_line(),
              bands[1].get_line(), sig->line, width, horz_even);
            bands[1].push_line();
            child_res->push_line();
          }
          else
          {
            // vertical transform
            if (aug->line->flags & line_buf::LFT_32BIT)
            {
              si32* sp = aug->line->i32;
              for (ui32 i = width; i > 0; --i)
                *sp++ <<= 1;
            }
            else
            {
              assert(aug->line->flags & line_buf::LFT_64BIT);
              si64* sp = aug->line->i64;
              for (ui32 i = width; i > 0; --i)
                *sp++ <<= 1;
            }
            // horizontal transform
            rev_horz_ana(atk, bands[2].get_line(),
              bands[3].get_line(), aug->line, width, horz_even);
            bands[2].push_line();
            bands[3].push_line();
          }
        }
      }
      else
      {
        if (res_rect.siz.h > 1)
        {
          if (!vert_even && cur_line < res_rect.siz.h) {
            vert_even = !vert_even;
            return;
          }

          do
          {
            //vertical transform
            for (ui32 i = 0; i < num_steps; ++i)
            {
              if (aug->active && (sig->active || ssp[i].active))
              {
                line_buf* dp = aug->line;
                line_buf* sp1 = sig->active ? sig->line : ssp[i].line;
                line_buf* sp2 = ssp[i].active ? ssp[i].line : sig->line;
                const lifting_step* s = atk->get_step(num_steps - i - 1);
                irv_vert_step(s, sp1, sp2, dp, width, false);
              }
              lifting_buf t = *aug; *aug = ssp[i]; ssp[i] = *sig; *sig = t;
            }

            if (aug->active) {
              const float K = atk->get_K();
              irv_vert_times_K(K, aug->line, width);

              irv_horz_ana(atk, bands[2].get_line(),
                bands[3].get_line(), aug->line, width, horz_even);
              bands[2].push_line();
              bands[3].push_line();
              aug->active = false;
              --rows_to_produce;
            }
            if (sig->active) {
              const float K_inv = 1.0f / atk->get_K();
              irv_vert_times_K(K_inv, sig->line, width);

              irv_horz_ana(atk, child_res->get_line(),
                bands[1].get_line(), sig->line, width, horz_even);
              bands[1].push_line();
              child_res->push_line();
              sig->active = false;
              --rows_to_produce;
            };
            vert_even = !vert_even;
          } while (cur_line >= res_rect.siz.h && rows_to_produce > 0);
        }
        else
        {
          if (vert_even) {
            // horizontal transform
            irv_horz_ana(atk, child_res->get_line(),
              bands[1].get_line(), sig->line, width, horz_even);
            bands[1].push_line();
            child_res->push_line();
          }
          else
          {
            // vertical transform
            float* sp = aug->line->f32;
            for (ui32 i = width; i > 0; --i)
              *sp++ *= 2.0f;
            // horizontal transform
            irv_horz_ana(atk, bands[2].get_line(),
              bands[3].get_line(), aug->line, width, horz_even);
            bands[2].push_line();
            bands[3].push_line();
          }
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf* resolution::pull_line()
    {
      if (res_num == 0)
      {
        assert(child_res == NULL);
        return bands[0].pull_line();
      }

      if (skipped_res_for_recon == true)
        return child_res->pull_line();

      ui32 width = res_rect.siz.w;
      if (width == 0)
        return NULL;

      if (transform_flags & VERT_TRX)
      {
        if (reversible)
        {
          if (res_rect.siz.h > 1)
          {
            if (sig->active) {
              sig->active = false;
              return sig->line;
            };
            for (;;)
            {
              //horizontal transform
              if (cur_line < res_rect.siz.h)
              {
                if (vert_even) { // even
                  if (transform_flags & HORZ_TRX)
                    rev_horz_syn(atk, aug->line, child_res->pull_line(), 
                      bands[1].pull_line(), width, horz_even);
                  else
                    memcpy(aug->line->p, child_res->pull_line()->p,
                      (size_t)width 
                      * (aug->line->flags & line_buf::LFT_SIZE_MASK));
                  aug->active = true;
                  vert_even = !vert_even;
                  ++cur_line;
                  continue;
                }
                else {
                  if (transform_flags & HORZ_TRX)
                    rev_horz_syn(atk, sig->line, bands[2].pull_line(), 
                      bands[3].pull_line(), width, horz_even);
                  else
                    memcpy(sig->line->p, bands[2].pull_line()->p,
                      (size_t)width 
                      * (sig->line->flags & line_buf::LFT_SIZE_MASK));
                  sig->active = true;
                  vert_even = !vert_even;
                  ++cur_line;
                }
              }

              //vertical transform
              for (ui32 i = 0; i < num_steps; ++i)
              {
                if (aug->active && (sig->active || ssp[i].active))
                {
                  line_buf* dp = aug->line;
                  line_buf* sp1 = sig->active ? sig->line : ssp[i].line;
                  line_buf* sp2 = ssp[i].active ? ssp[i].line : sig->line;
                  const lifting_step* s = atk->get_step(i);
                  rev_vert_step(s, sp1, sp2, dp, width, true);
                }
                lifting_buf t = *aug; *aug = ssp[i]; ssp[i] = *sig; *sig = t;
              }

              if (aug->active) {
                aug->active = false;
                return aug->line;
              }
              if (sig->active) {
                sig->active = false;
                return sig->line;
              };
            }
          }
          else
          {
            if (vert_even) {
              if (transform_flags & HORZ_TRX)
                rev_horz_syn(atk, aug->line, child_res->pull_line(),
                  bands[1].pull_line(), width, horz_even);
              else
                memcpy(aug->line->p, child_res->pull_line()->p,
                  (size_t)width 
                  * (aug->line->flags & line_buf::LFT_SIZE_MASK));
            }
            else
            {
              if (transform_flags & HORZ_TRX)
                rev_horz_syn(atk, aug->line, bands[2].pull_line(),
                  bands[3].pull_line(), width, horz_even);
              else
                memcpy(aug->line->p, bands[2].pull_line()->p,
                  (size_t)width 
                  * (aug->line->flags & line_buf::LFT_SIZE_MASK));
              if (aug->line->flags & line_buf::LFT_32BIT)
              {
                si32* sp = aug->line->i32;                
                for (ui32 i = width; i > 0; --i)
                  *sp++ >>= 1;
              }
              else
              {
                assert(aug->line->flags & line_buf::LFT_64BIT);
                si64* sp = aug->line->i64;
                for (ui32 i = width; i > 0; --i)
                  *sp++ >>= 1;
              }
            }
            return aug->line;
          }
        }
        else
        {
          if (res_rect.siz.h > 1)
          {
            if (sig->active) {
              sig->active = false;
              return sig->line;
            };
            for (;;)
            {
              //horizontal transform
              if (cur_line < res_rect.siz.h)
              {
                if (vert_even) { // even
                  if (transform_flags & HORZ_TRX)
                    irv_horz_syn(atk, aug->line, child_res->pull_line(), 
                      bands[1].pull_line(), width, horz_even);
                  else 
                    memcpy(aug->line->f32, child_res->pull_line()->f32,
                      width * sizeof(float));
                  aug->active = true;
                  vert_even = !vert_even;
                  ++cur_line;

                  const float K = atk->get_K();
                  irv_vert_times_K(K, aug->line, width);

                  continue;
                }
                else {
                  if (transform_flags & HORZ_TRX)
                    irv_horz_syn(atk, sig->line, bands[2].pull_line(), 
                      bands[3].pull_line(), width, horz_even);
                  else
                    memcpy(sig->line->f32, bands[2].pull_line()->f32,
                      width * sizeof(float));
                  sig->active = true;
                  vert_even = !vert_even;
                  ++cur_line;

                  const float K_inv = 1.0f / atk->get_K();
                  irv_vert_times_K(K_inv, sig->line, width);
                }
              }

              //vertical transform
              for (ui32 i = 0; i < num_steps; ++i)
              {
                if (aug->active && (sig->active || ssp[i].active))
                {
                  line_buf* dp = aug->line;
                  line_buf* sp1 = sig->active ? sig->line : ssp[i].line;
                  line_buf* sp2 = ssp[i].active ? ssp[i].line : sig->line;
                  const lifting_step* s = atk->get_step(i);
                  irv_vert_step(s, sp1, sp2, dp, width, true);
                }
                lifting_buf t = *aug; *aug = ssp[i]; ssp[i] = *sig; *sig = t;
              }

              if (aug->active) {
                aug->active = false;
                return aug->line;
              }
              if (sig->active) {
                sig->active = false;
                return sig->line;
              };
            }
          }
          else
          {
            if (vert_even) {
              if (transform_flags & HORZ_TRX)
                irv_horz_syn(atk, aug->line, child_res->pull_line(),
                  bands[1].pull_line(), width, horz_even);
              else
                memcpy(aug->line->f32, child_res->pull_line()->f32,
                  width * sizeof(float));
            }
            else
            {
              if (transform_flags & HORZ_TRX)
                irv_horz_syn(atk, aug->line, bands[2].pull_line(),
                  bands[3].pull_line(), width, horz_even);
             else
                memcpy(aug->line->f32, bands[2].pull_line()->f32,
                  width * sizeof(float));
              float* sp = aug->line->f32;
              for (ui32 i = width; i > 0; --i)
                *sp++ *= 0.5f;
            }
            return aug->line;
          }
        }
      }
      else
      { 
        if (reversible)
        {
          if (transform_flags & HORZ_TRX)
            rev_horz_syn(atk, aug->line, child_res->pull_line(),
              bands[1].pull_line(), width, horz_even);
          else
            memcpy(aug->line->p, child_res->pull_line()->p,
              (size_t)width * (aug->line->flags & line_buf::LFT_SIZE_MASK));
          return aug->line;
        }
        else
        {
          if (transform_flags & HORZ_TRX)
            irv_horz_syn(atk, aug->line, child_res->pull_line(),
              bands[1].pull_line(), width, horz_even);
          else
            memcpy(aug->line->f32, child_res->pull_line()->f32,
              width * sizeof(float));
          return aug->line;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 resolution::prepare_precinct()
    {
      ui32 lower_resolutions_bytes = 0;
      if (res_num != 0)
        lower_resolutions_bytes = child_res->prepare_precinct();

      this->num_bytes = 0;
      si32 repeat = (si32)num_precincts.area();
      for (si32 i = 0; i < repeat; ++i)
        this->num_bytes += precincts[i].prepare_precinct(tag_tree_size,
          level_index, elastic);
      return this->num_bytes + lower_resolutions_bytes;
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::write_precincts(outfile_base* file)
    {
      precinct* p = precincts;
      for (si32 i = 0; i < (si32)num_precincts.area(); ++i)
        p[i].write(file);
    }

    //////////////////////////////////////////////////////////////////////////
    bool resolution::get_top_left_precinct(point& top_left)
    {
      ui32 idx = cur_precinct_loc.x + cur_precinct_loc.y * num_precincts.w;
      if (idx < num_precincts.area())
      {
        top_left = precincts[idx].img_point;
        return true;
      }
      return false;
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::write_one_precinct(outfile_base* file)
    {
      ui32 idx = cur_precinct_loc.x + cur_precinct_loc.y * num_precincts.w;
      assert(idx < num_precincts.area());
      precincts[idx].write(file);

      if (++cur_precinct_loc.x >= num_precincts.w)
      {
        cur_precinct_loc.x = 0;
        ++cur_precinct_loc.y;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::parse_all_precincts(ui32& data_left, infile_base* file)
    {
      precinct* p = precincts;
      ui32 idx = cur_precinct_loc.x + cur_precinct_loc.y * num_precincts.w;
      for (ui32 i = idx; i < num_precincts.area(); ++i)
      {
        if (data_left == 0)
          break;
        p[i].parse(tag_tree_size, level_index, elastic, data_left, file,
          skipped_res_for_read);
        if (++cur_precinct_loc.x >= num_precincts.w)
        {
          cur_precinct_loc.x = 0;
          ++cur_precinct_loc.y;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void resolution::parse_one_precinct(ui32& data_left, infile_base* file)
    {
      ui32 idx = cur_precinct_loc.x + cur_precinct_loc.y * num_precincts.w;
      assert(idx < num_precincts.area());

      if (data_left == 0)
        return;
      precinct* p = precincts + idx;
      p->parse(tag_tree_size, level_index, elastic, data_left, file,
        skipped_res_for_read);
      if (++cur_precinct_loc.x >= num_precincts.w)
      {
        cur_precinct_loc.x = 0;
        ++cur_precinct_loc.y;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 resolution::get_num_bytes(ui32 resolution_num) const
    {
      if (this->res_num == resolution_num)
        return get_num_bytes();
      if (resolution_num < this->res_num) {
        assert(child_res);
        return child_res->get_num_bytes(resolution_num);
      }
      return 0;
    }
  }
}