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
// File: ojph_codestream_local.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>

#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream_local.h"
#include "ojph_tile.h"

#include "../transform/ojph_colour.h"
#include "../transform/ojph_transform.h"

namespace ojph {

  namespace local
  {

    //////////////////////////////////////////////////////////////////////////
    codestream::codestream()
    : precinct_scratch(NULL), allocator(NULL), elastic_alloc(NULL)
    {
      allocator = new mem_fixed_allocator;
      elastic_alloc = new mem_elastic_allocator(1048576); // 1 megabyte

      init_colour_transform_functions();
      init_wavelet_transform_functions();

      restart();
    }

    //////////////////////////////////////////////////////////////////////////
    codestream::~codestream()
    {
      if (allocator)
        delete allocator;
      if (elastic_alloc)
        delete elastic_alloc;
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::restart()
    {
      tiles = NULL;
      lines = NULL;
      comp_size = NULL;
      recon_comp_size = NULL;
      outfile = NULL;
      infile = NULL;

      num_comps = 0;
      employ_color_transform = false;
      planar = -1;
      profile = OJPH_PN_UNDEFINED;
      tilepart_div = OJPH_TILEPART_NO_DIVISIONS;
      need_tlm = false;

      cur_comp = 0;
      cur_line = 0;
      cur_tile_row = 0;
      resilient = false;
      skipped_res_for_read = skipped_res_for_recon = 0;

      precinct_scratch_needed_bytes = 0;

      cod.restart();
      qcd.restart();
      nlt.restart();
      dfs.restart();
      atk.restart();

      allocator->restart();
      elastic_alloc->restart();
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::pre_alloc()
    {
      ojph::param_siz sz = access_siz();
      num_tiles.w = sz.get_image_extent().x - sz.get_tile_offset().x;
      num_tiles.w = ojph_div_ceil(num_tiles.w, sz.get_tile_size().w);
      num_tiles.h = sz.get_image_extent().y - sz.get_tile_offset().y;
      num_tiles.h = ojph_div_ceil(num_tiles.h, sz.get_tile_size().h);
      if (num_tiles.area() > 65535)
        OJPH_ERROR(0x00030011, "number of tiles cannot exceed 65535");

      //allocate tiles
      allocator->pre_alloc_obj<tile>((size_t)num_tiles.area());

      ui32 num_tileparts = 0;
      point index;
      rect tile_rect, recon_tile_rect;
      ui32 ds = 1 << skipped_res_for_recon;
      for (index.y = 0; index.y < num_tiles.h; ++index.y)
      {
        ui32 y0 = sz.get_tile_offset().y
                + index.y * sz.get_tile_size().h;
        ui32 y1 = y0 + sz.get_tile_size().h; //end of tile

        tile_rect.org.y = ojph_max(y0, sz.get_image_offset().y);
        tile_rect.siz.h =
          ojph_min(y1, sz.get_image_extent().y) - tile_rect.org.y;

        recon_tile_rect.org.y = ojph_max(ojph_div_ceil(y0, ds),
          ojph_div_ceil(sz.get_image_offset().y, ds));
        recon_tile_rect.siz.h = ojph_min(ojph_div_ceil(y1, ds),
          ojph_div_ceil(sz.get_image_extent().y, ds))
          - recon_tile_rect.org.y;

        for (index.x = 0; index.x < num_tiles.w; ++index.x)
        {
          ui32 x0 = sz.get_tile_offset().x
                  + index.x * sz.get_tile_size().w;
          ui32 x1 = x0 + sz.get_tile_size().w;

          tile_rect.org.x = ojph_max(x0, sz.get_image_offset().x);
          tile_rect.siz.w =
            ojph_min(x1, sz.get_image_extent().x) - tile_rect.org.x;

          recon_tile_rect.org.x = ojph_max(ojph_div_ceil(x0, ds),
            ojph_div_ceil(sz.get_image_offset().x, ds));
          recon_tile_rect.siz.w = ojph_min(ojph_div_ceil(x1, ds),
            ojph_div_ceil(sz.get_image_extent().x, ds))
            - recon_tile_rect.org.x;

          ui32 tps = 0; // number of tileparts for this tile
          tile::pre_alloc(this, tile_rect, recon_tile_rect, tps);
          num_tileparts += tps;
        }
      }

      //allocate lines
      //These lines are used by codestream to exchange data with external
      // world
      ui32 num_comps = sz.get_num_components();
      allocator->pre_alloc_obj<line_buf>(num_comps);
      allocator->pre_alloc_obj<size>(num_comps); //for *comp_size
      allocator->pre_alloc_obj<size>(num_comps); //for *recon_comp_size
      for (ui32 i = 0; i < num_comps; ++i)
        allocator->pre_alloc_data<si32>(siz.get_recon_width(i), 0);

      //allocate tlm
      if (outfile != NULL && need_tlm)
        allocator->pre_alloc_obj<param_tlm::Ttlm_Ptlm_pair>(num_tileparts);

      //precinct scratch buffer
      ui32 num_decomps = cod.get_num_decompositions();
      size log_cb = cod.get_log_block_dims();

      size ratio;
      for (ui32 r = 0; r <= num_decomps; ++r)
      {
        size log_PP = cod.get_log_precinct_size(r);
        ratio.w = ojph_max(ratio.w, log_PP.w - ojph_min(log_cb.w, log_PP.w));
        ratio.h = ojph_max(ratio.h, log_PP.h - ojph_min(log_cb.h, log_PP.h));
      }
      ui32 max_ratio = ojph_max(ratio.w, ratio.h);
      max_ratio = 1 << max_ratio;
      // assuming that we have a hierarchy of n levels.
      // This needs 4/3 times the area, rounded up
      // (rounding up leaves one extra entry).
      // This exta entry is necessary
      // We need 4 such tables. These tables store
      // 1. missing msbs and 2. their flags,
      // 3. number of layers and 4. their flags
      precinct_scratch_needed_bytes =
        4 * ((max_ratio * max_ratio * 4 + 2) / 3);

      allocator->pre_alloc_obj<ui8>(precinct_scratch_needed_bytes);
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::finalize_alloc()
    {
      allocator->alloc();

      //precinct scratch buffer
      precinct_scratch =
        allocator->post_alloc_obj<ui8>(precinct_scratch_needed_bytes);

      //get tiles
      tiles = this->allocator->post_alloc_obj<tile>((size_t)num_tiles.area());

      ui32 num_tileparts = 0;
      point index;
      rect tile_rect;
      ojph::param_siz sz = access_siz();
      for (index.y = 0; index.y < num_tiles.h; ++index.y)
      {
        ui32 y0 = sz.get_tile_offset().y
                + index.y * sz.get_tile_size().h;
        ui32 y1 = y0 + sz.get_tile_size().h; //end of tile

        tile_rect.org.y = ojph_max(y0, sz.get_image_offset().y);
        tile_rect.siz.h =
          ojph_min(y1, sz.get_image_extent().y) - tile_rect.org.y;

        ui32 offset = 0;
        for (index.x = 0; index.x < num_tiles.w; ++index.x)
        {
          ui32 x0 = sz.get_tile_offset().x
                  + index.x * sz.get_tile_size().w;
          ui32 x1 = x0 + sz.get_tile_size().w;

          tile_rect.org.x = ojph_max(x0, sz.get_image_offset().x);
          tile_rect.siz.w =
            ojph_min(x1, sz.get_image_extent().x) - tile_rect.org.x;

          ui32 tps = 0; // number of tileparts for this tile
          ui32 idx = index.y * num_tiles.w + index.x;
          tiles[idx].finalize_alloc(this, tile_rect, idx, offset, tps);
          num_tileparts += tps;
        }
      }

      //allocate lines
      //These lines are used by codestream to exchange data with external
      // world
      this->num_comps = sz.get_num_components();
      lines = allocator->post_alloc_obj<line_buf>(this->num_comps);
      comp_size = allocator->post_alloc_obj<size>(this->num_comps);
      recon_comp_size = allocator->post_alloc_obj<size>(this->num_comps);
      employ_color_transform = cod.is_employing_color_transform();
      for (ui32 i = 0; i < this->num_comps; ++i)
      {
        comp_size[i].w = siz.get_width(i);
        comp_size[i].h = siz.get_height(i);
        ui32 cw = siz.get_recon_width(i);
        recon_comp_size[i].w = cw;
        recon_comp_size[i].h = siz.get_recon_height(i);
        lines[i].wrap(allocator->post_alloc_data<si32>(cw, 0), cw, 0);
      }

      cur_comp = 0;
      cur_line = 0;

      //allocate tlm
      if (outfile != NULL && need_tlm)
        tlm.init(num_tileparts,
          allocator->post_alloc_obj<param_tlm::Ttlm_Ptlm_pair>(num_tileparts));
    }


    //////////////////////////////////////////////////////////////////////////
    void codestream::check_imf_validity()
    {
      //two possibilities lossy single tile or lossless
      //For the following code, we use the least strict profile
      ojph::param_siz sz(&siz);
      ojph::param_cod cd(&cod);
      bool reversible = cd.is_reversible();
      bool imf2k = !reversible, imf4k = !reversible, imf8k = !reversible;
      bool imf2kls = reversible, imf4kls = reversible, imf8kls = reversible;

      if (reversible)
      {
        point ext = sz.get_image_extent();
        if (ext.x <= 2048 && ext.y <= 1556)
          imf2kls &= true;
        if (ext.x <= 4096 && ext.y <= 3112)
          imf4kls &= true;
        if (ext.x <= 8192 && ext.y <= 6224)
          imf8kls &= true;

        if (!imf2kls && !imf4kls && !imf8kls)
          OJPH_ERROR(0x000300C1,
            "Image dimensions do not meet any of the lossless IMF profiles");
      }
      else
      {
        point ext = sz.get_image_extent();
        if (ext.x <= 2048 && ext.y <= 1556)
          imf2k &= true;
        if (ext.x <= 4096 && ext.y <= 3112)
          imf4k &= true;
        if (ext.x <= 8192 && ext.y <= 6224)
          imf8k &= true;

        if (!imf2k && !imf4k && !imf8k)
          OJPH_ERROR(0x000300C2,
            "Image dimensions do not meet any of the lossy IMF profiles");
      }


      if (sz.get_image_offset().x != 0 || sz.get_image_offset().y != 0)
        OJPH_ERROR(0x000300C3,
          "For IMF profile, image offset (XOsiz, YOsiz) has to be 0.");
      if (sz.get_tile_offset().x != 0 || sz.get_tile_offset().y != 0)
        OJPH_ERROR(0x000300C4,
          "For IMF profile, tile offset (XTOsiz, YTOsiz) has to be 0.");
      if (sz.get_num_components() > 3)
        OJPH_ERROR(0x000300C5,
          "For IMF profile, the number of components has to be less "
          " or equal to 3");
      bool test_ds1 = true, test_ds2 = true;
      for (ojph::ui32 i = 0; i < sz.get_num_components(); ++i)
      {
        point downsamping = sz.get_downsampling(i);
        test_ds1 &= downsamping.y == 1;
        test_ds2 &= downsamping.y == 1;

        test_ds1 &= downsamping.x == 1;
        if (i == 1 || i == 2)
          test_ds2 &= downsamping.x == 2;
        else
          test_ds2 &= downsamping.x == 1;
      }
      if (!test_ds1 && !test_ds2)
        OJPH_ERROR(0x000300C6,
          "For IMF profile, either no component downsampling is used,"
          " or the x-dimension of the 2nd and 3rd components is downsampled"
          " by 2.");

      bool test_bd = true;
      for (ojph::ui32 i = 0; i < sz.get_num_components(); ++i)
      {
        ui32 bit_depth = sz.get_bit_depth(i);
        bool is_signed = sz.is_signed(i);
        test_bd &= bit_depth >= 8 && bit_depth <= 16 && is_signed == false;
      }
      if (!test_bd)
        OJPH_ERROR(0x000300C7,
          "For IMF profile, compnent bit_depth has to be between"
          " 8 and 16 bits inclusively, and the samples must be unsigned");

      if (cd.get_log_block_dims().w != 5 || cd.get_log_block_dims().h != 5)
        OJPH_ERROR(0x000300C8,
          "For IMF profile, codeblock dimensions are restricted."
          " Use \"-block_size {32,32}\" at the commandline");

      ui32 num_decomps = cd.get_num_decompositions();
      bool test_pz = cd.get_log_precinct_size(0).w == 7
                  && cd.get_log_precinct_size(0).h == 7;
      for (ui32 i = 1; i <= num_decomps; ++i)
        test_pz = cd.get_log_precinct_size(i).w == 8
               && cd.get_log_precinct_size(i).h == 8;
      if (!test_pz)
        OJPH_ERROR(0x000300C9,
          "For IMF profile, precinct sizes are restricted."
          " Use \"-precincts {128,128},{256,256}\" at the commandline");

      if (cd.get_progression_order() != OJPH_PO_CPRL)
        OJPH_ERROR(0x000300CA,
          "For IMF profile, the CPRL progression order must be used."
          " Use \"-prog_order CPRL\".");

      imf2k &= num_decomps <= 5;
      imf2kls &= num_decomps <= 5;
      imf4k &= num_decomps <= 6;
      imf4kls &= num_decomps <= 6;
      imf8k &= num_decomps <= 7;
      imf8kls &= num_decomps <= 7;

      if (num_decomps == 0 ||
        (!imf2k && !imf4k && !imf8k && !imf2kls && !imf4kls && !imf8kls))
        OJPH_ERROR(0x000300CB,
          "Number of decompositions does not match the IMF profile"
          " dictated by wavelet reversibility and image dimensions.");

      ui32 tiles_w = sz.get_image_extent().x;
      tiles_w = ojph_div_ceil(tiles_w, sz.get_tile_size().w);
      ui32 tiles_h = sz.get_image_extent().y;
      tiles_h = ojph_div_ceil(tiles_h, sz.get_tile_size().h);
      ui32 total_tiles = tiles_w * tiles_h;

      if (total_tiles > 1)
      {
        if (!reversible)
          OJPH_ERROR(0x000300CC,
            "Lossy IMF profile must have one tile.");

        size tt = sz.get_tile_size();
        imf2kls &= (tt.w == 1024 && tt.h == 1024);
        imf2kls &= (tt.w >= 1024 && num_decomps <= 4)
                || (tt.w >= 2048 && num_decomps <= 5);
        imf4kls &= (tt.w == 1024 && tt.h == 1024)
                || (tt.w == 2048 && tt.h == 2048);
        imf4kls &= (tt.w >= 1024 && num_decomps <= 4)
                || (tt.w >= 2048 && num_decomps <= 5)
                || (tt.w >= 4096 && num_decomps <= 6);
        imf8kls &= (tt.w == 1024 && tt.h == 1024)
                || (tt.w == 2048 && tt.h == 2048)
                || (tt.w == 4096 && tt.h == 4096);
        imf8kls &= (tt.w >= 1024 && num_decomps <= 4)
                || (tt.w >= 2048 && num_decomps <= 5)
                || (tt.w >= 4096 && num_decomps <= 6)
                || (tt.w >= 8192 && num_decomps <= 7);
        if (!imf2kls && !imf4kls && !imf8kls)
          OJPH_ERROR(0x000300CD,
            "Number of decompositions does not match the IMF profile"
            " dictated by wavelet reversibility and image dimensions and"
            " tiles.");
      }

      need_tlm = true;
      tilepart_div |= OJPH_TILEPART_COMPONENTS;
      if (tilepart_div != OJPH_TILEPART_COMPONENTS)
      {
        tilepart_div = OJPH_TILEPART_COMPONENTS;
        OJPH_WARN(0x000300C1,
          "In IMF profile, tile part divisions at the component level must be "
          "employed, while at the resolution level is not allowed. "
          "This has been corrected.");
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::check_broadcast_validity()
    {
      ojph::param_siz sz(&siz);
      ojph::param_cod cd(&cod);

      if (sz.get_image_offset().x != 0 || sz.get_image_offset().y != 0)
        OJPH_ERROR(0x000300B1,
          "For broadcast profile, image offset (XOsiz, YOsiz) has to be 0.");
      if (sz.get_tile_offset().x != 0 || sz.get_tile_offset().y != 0)
        OJPH_ERROR(0x000300B2,
          "For broadcast profile, tile offset (XTOsiz, YTOsiz) has to be 0.");
      if (sz.get_num_components() > 4)
        OJPH_ERROR(0x000300B3,
          "For broadcast profile, the number of components has to be less "
          " or equal to 4");
      bool test_ds1 = true, test_ds2 = true;
      for (ojph::ui32 i = 0; i < sz.get_num_components(); ++i)
      {
        point downsamping = sz.get_downsampling(i);
        test_ds1 &= downsamping.y == 1;
        test_ds2 &= downsamping.y == 1;

        test_ds1 &= downsamping.x == 1;
        if (i == 1 || i == 2)
          test_ds2 &= downsamping.x == 2;
        else
          test_ds2 &= downsamping.x == 1;
      }
      if (!test_ds1 && !test_ds2)
        OJPH_ERROR(0x000300B4,
          "For broadcast profile, either no component downsampling is used,"
          " or the x-dimension of the 2nd and 3rd components is downsampled"
          " by 2.");

      bool test_bd = true;
      for (ojph::ui32 i = 0; i < sz.get_num_components(); ++i)
      {
        ui32 bit_depth = sz.get_bit_depth(i);
        bool is_signed = sz.is_signed(i);
        test_bd &= bit_depth >= 8 && bit_depth <= 12 && is_signed == false;
      }
      if (!test_bd)
        OJPH_ERROR(0x000300B5,
          "For broadcast profile, compnent bit_depth has to be between"
          " 8 and 12 bits inclusively, and the samples must be unsigned");

      ui32 num_decomps = cd.get_num_decompositions();
      if (num_decomps == 0 || num_decomps > 5)
        OJPH_ERROR(0x000300B6,
          "For broadcast profile, number of decompositions has to be between"
          "1 and 5 inclusively.");

      if (cd.get_log_block_dims().w < 5 || cd.get_log_block_dims().w > 7)
        OJPH_ERROR(0x000300B7,
          "For broadcast profile, codeblock dimensions are restricted such"
          " that codeblock width has to be either 32, 64, or 128.");

      if (cd.get_log_block_dims().h < 5 || cd.get_log_block_dims().h > 7)
        OJPH_ERROR(0x000300B8,
          "For broadcast profile, codeblock dimensions are restricted such"
          " that codeblock height has to be either 32, 64, or 128.");

      bool test_pz = cd.get_log_precinct_size(0).w == 7
                  && cd.get_log_precinct_size(0).h == 7;
      for (ui32 i = 1; i <= num_decomps; ++i)
        test_pz = cd.get_log_precinct_size(i).w == 8
               && cd.get_log_precinct_size(i).h == 8;
      if (!test_pz)
        OJPH_ERROR(0x000300B9,
          "For broadcast profile, precinct sizes are restricted."
          " Use \"-precincts {128,128},{256,256}\" at the commandline");

      if (cd.get_progression_order() != OJPH_PO_CPRL)
        OJPH_ERROR(0x000300BA,
          "For broadcast profile, the CPRL progression order must be used."
          " Use \"-prog_order CPRL\".");

      ui32 tiles_w = sz.get_image_extent().x;
      tiles_w = ojph_div_ceil(tiles_w, sz.get_tile_size().w);
      ui32 tiles_h = sz.get_image_extent().y;
      tiles_h = ojph_div_ceil(tiles_h, sz.get_tile_size().h);
      ui32 total_tiles = tiles_w * tiles_h;

      if (total_tiles != 1 && total_tiles != 4)
        OJPH_ERROR(0x000300BB,
          "The broadcast profile can only have 1 or 4 tiles");

      need_tlm = true;
      tilepart_div |= OJPH_TILEPART_COMPONENTS;
      if (tilepart_div != OJPH_TILEPART_COMPONENTS)
      {
        tilepart_div = OJPH_TILEPART_COMPONENTS;
        OJPH_WARN(0x000300B1,
          "In BROADCAST profile, tile part divisions at the component level "
          "must be employed, while at the resolution level is not allowed. "
          "This has been corrected.");
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::write_headers(outfile_base *file,
                                   const comment_exchange* comments,
                                   ui32 num_comments)
    {
      //finalize
      siz.check_validity(cod);
      cod.check_validity(siz);
      cod.update_atk(&atk);
      qcd.check_validity(siz, cod);
      cap.check_validity(cod, qcd);
      nlt.check_validity(siz);
      if (profile == OJPH_PN_IMF)
        check_imf_validity();
      else if (profile == OJPH_PN_BROADCAST)
        check_broadcast_validity();

      int po = ojph::param_cod(&cod).get_progression_order();
      if ((po == OJPH_PO_LRCP || po == OJPH_PO_RLCP) &&
           tilepart_div == OJPH_TILEPART_COMPONENTS)
      {
        tilepart_div |= OJPH_TILEPART_RESOLUTIONS;
        OJPH_INFO(0x00030021,
          "For LRCP and RLCP progression orders, tilepart divisions at the "
          "component level, means that we have a tilepart for every "
          "resolution and component.\n");
      }
      if (po == OJPH_PO_RPCL && (tilepart_div & OJPH_TILEPART_COMPONENTS) != 0)
      {
        tilepart_div &= ~OJPH_TILEPART_COMPONENTS;
        OJPH_WARN(0x00030021,
          "For RPCL progression, having tilepart divisions at the component "
          "level means a tilepart for every precinct, which does not "
          "make sense, since we can have no more than 255 tile parts. This "
          "has been corrected by removing tilepart divisions at the component "
          "level.");
      }
      if (po == OJPH_PO_PCRL && tilepart_div != 0)
      {
        tilepart_div = 0;
        OJPH_WARN(0x00030022,
          "For PCRL progression, having tilepart divisions at the component "
          "level or the resolution level means a tile part for every "
          "precinct, which does not make sense, since we can have no more "
          "than 255 tile parts.  This has been corrected by removing tilepart "
          "divisions; use another progression if you want tileparts.");
      }
      if (po == OJPH_PO_CPRL && (tilepart_div & OJPH_TILEPART_RESOLUTIONS) != 0)
      {
        tilepart_div &= ~OJPH_TILEPART_RESOLUTIONS;
        OJPH_WARN(0x00030023,
          "For CPRL progression, having tilepart divisions at the resolution "
          "level means a tile part for every precinct, which does not "
          "make sense, since we can have no more than 255 tile parts. This "
          "has been corrected by removing tilepart divisions at the "
          "resolution level.");
      }

      if (planar == -1) //not initialized
        planar = cod.is_employing_color_transform() ? 1 : 0;
      else if (planar == 0) //interleaved is chosen
      {
      }
      else if (planar == 1) //plannar is chosen
      {
        if (cod.is_employing_color_transform() == true)
          OJPH_ERROR(0x00030021,
            "the planar interface option cannot be used when colour "
            "transform is employed");
      }
      else
        assert(0);

      assert(this->outfile == NULL);
      this->outfile = file;
      this->pre_alloc();
      this->finalize_alloc();

      ui16 t = swap_byte(JP2K_MARKER::SOC);
      if (file->write(&t, 2) != 2)
        OJPH_ERROR(0x00030022, "Error writing to file");

      if (!siz.write(file))
        OJPH_ERROR(0x00030023, "Error writing to file");

      if (!cap.write(file))
        OJPH_ERROR(0x00030024, "Error writing to file");

      if (!cod.write(file))
        OJPH_ERROR(0x00030025, "Error writing to file");

      if (!cod.write_coc(file, num_comps))
        OJPH_ERROR(0x0003002E, "Error writing to file");

      if (!qcd.write(file))
        OJPH_ERROR(0x00030026, "Error writing to file");

      if (!qcd.write_qcc(file, num_comps))
        OJPH_ERROR(0x0003002D, "Error writing to file");

      if (!nlt.write(file))
        OJPH_ERROR(0x00030027, "Error writing to file");

      char buf[] = "      OpenJPH Ver "
        OJPH_INT_TO_STRING(OPENJPH_VERSION_MAJOR) "."
        OJPH_INT_TO_STRING(OPENJPH_VERSION_MINOR) "."
        OJPH_INT_TO_STRING(OPENJPH_VERSION_PATCH) ".";
      size_t len = strlen(buf);
      *(ui16*)buf = swap_byte(JP2K_MARKER::COM);
      *(ui16*)(buf + 2) = swap_byte((ui16)(len - 2));
      //1 for General use (IS 8859-15:1999 (Latin) values)
      *(ui16*)(buf + 4) = swap_byte((ui16)(1));
      if (file->write(buf, len) != len)
        OJPH_ERROR(0x00030028, "Error writing to file");

      if (comments != NULL) {
        for (ui32 i = 0; i < num_comments; ++i)
        {
          t = swap_byte(JP2K_MARKER::COM);
          if (file->write(&t, 2) != 2)
            OJPH_ERROR(0x00030029, "Error writing to file");
          t = swap_byte((ui16)(comments[i].len + 4));
          if (file->write(&t, 2) != 2)
            OJPH_ERROR(0x0003002A, "Error writing to file");
          //1 for General use (IS 8859-15:1999 (Latin) values)
          t = swap_byte(comments[i].Rcom);
          if (file->write(&t, 2) != 2)
            OJPH_ERROR(0x0003002B, "Error writing to file");
          if (file->write(comments[i].data, comments[i].len)!=comments[i].len)
            OJPH_ERROR(0x0003002C, "Error writing to file");
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static
    int find_marker(infile_base *f, const ui16* char_list, int list_len)
    {
      //returns the marker index in char_list, or -1
      while (!f->eof())
      {
        ui8 new_char;
        size_t num_bytes = f->read(&new_char, 1);
        if (num_bytes != 1)
            return -1;
        if (new_char == 0xFF)
        {
          size_t num_bytes = f->read(&new_char, 1);

          if (num_bytes != 1)
              return -1;

          for (int i = 0; i < list_len; ++i)
            if (new_char == (char_list[i] & 0xFF))
              return i;
        }
      }
      return -1;
    }

    //////////////////////////////////////////////////////////////////////////
    static
    int skip_marker(infile_base *file, const char *marker,
                     const char *msg, int msg_level, bool resilient)
    {
      ojph_unused(marker);
      ui16 com_len;
      if (file->read(&com_len, 2) != 2)
      {
        if (resilient)
          return -1;
        else
          OJPH_ERROR(0x00030041, "error reading marker");
      }
      com_len = swap_byte(com_len);
      file->seek(com_len - 2, infile_base::OJPH_SEEK_CUR);
      if (msg != NULL && msg_level != OJPH_MSG_LEVEL::NO_MSG)
      {
        if (msg_level == OJPH_MSG_LEVEL::INFO)
        {
          OJPH_INFO(0x00030001, "%s", msg);
        }
        else if (msg_level == OJPH_MSG_LEVEL::WARN)
        {
          OJPH_WARN(0x00030001, "%s", msg);
        }
        else if (msg_level == OJPH_MSG_LEVEL::ERROR)
        {
          OJPH_ERROR(0x00030001, "%s", msg);
        }
        else // there is the option of ALL_MSG but it should not be used here
          assert(0);
      }
      return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::read_headers(infile_base *file)
    {
      ui16 marker_list[20] = { SOC, SIZ, CAP, PRF, CPF, COD, COC, QCD, QCC,
        RGN, POC, PPM, TLM, PLM, CRG, COM, DFS, ATK, NLT, SOT };
      find_marker(file, marker_list, 1); //find SOC
      find_marker(file, marker_list + 1, 1); //find SIZ
      siz.read(file);
      int marker_idx = 0;
      int received_markers = 0; //check that COD, & QCD received
      while (true)
      {
        marker_idx = find_marker(file, marker_list + 2, 18);
        if (marker_idx == 0)
          cap.read(file);
        else if (marker_idx == 1)
          //Skipping PRF marker segment; this should not cause any issues
          skip_marker(file, "PRF", NULL, OJPH_MSG_LEVEL::NO_MSG, false);
        else if (marker_idx == 2)
          //Skipping CPF marker segment; this should not cause any issues
          skip_marker(file, "CPF", NULL, OJPH_MSG_LEVEL::NO_MSG, false);
        else if (marker_idx == 3)
        {
          cod.read(file);
          received_markers |= 1;
          ojph::param_cod c(&cod);
          int num_qlayers = c.get_num_layers();
          if (num_qlayers != 1)
            OJPH_ERROR(0x00030053, "The current implementation supports "
              "1 quality layer only.  This codestream has %d quality layers",
              num_qlayers);
        }
        else if (marker_idx == 4)
        {
          param_cod* p = cod.add_coc_object(param_cod::OJPH_COD_UNKNOWN);
          p->read_coc(file, siz.get_num_components(), &cod);
          if (p->get_comp_idx() >= siz.get_num_components())
            OJPH_INFO(0x00030056, "The codestream carries a COC marker "
              "segment for a component indexed by %d, which is more than the "
              "allowed index number, since the codestream has %d components",
              p->get_comp_idx(), num_comps);
          param_cod *q = cod.get_coc(p->get_comp_idx());
          if (p != q && p->get_comp_idx() == q->get_comp_idx())
            OJPH_ERROR(0x00030057, "The codestream has two COC marker "
              "segments for one component of index %d",  p->get_comp_idx());
        }
        else if (marker_idx == 5)
        {
          qcd.read(file);
          received_markers |= 2;
        }
        else if (marker_idx == 6)
        {
          param_qcd* p = qcd.add_qcc_object(param_qcd::OJPH_QCD_UNKNOWN);
          p->read_qcc(file, siz.get_num_components());
          if (p->get_comp_idx() >= siz.get_num_components())
            OJPH_ERROR(0x00030054, "The codestream carries a QCC marker "
              "segment for a component indexed by %d, which is more than the "
              "allowed index number, since the codestream has %d components",
              p->get_comp_idx(), num_comps);
          param_qcd *q = qcd.get_qcc(p->get_comp_idx());
          if (p != q && p->get_comp_idx() == q->get_comp_idx())
            OJPH_ERROR(0x00030055, "The codestream has two QCC marker "
              "segments for one component of index %d", p->get_comp_idx());
        }
        else if (marker_idx == 7)
          skip_marker(file, "RGN", "RGN is not supported yet",
            OJPH_MSG_LEVEL::WARN, false);
        else if (marker_idx == 8)
          skip_marker(file, "POC", "POC is not supported yet",
            OJPH_MSG_LEVEL::WARN, false);
        else if (marker_idx == 9)
          skip_marker(file, "PPM", "PPM is not supported yet",
            OJPH_MSG_LEVEL::WARN, false);
        else if (marker_idx == 10)
          //Skipping TLM marker segment; this should not cause any issues
          skip_marker(file, "TLM", NULL, OJPH_MSG_LEVEL::NO_MSG, false);
        else if (marker_idx == 11)
          //Skipping PLM marker segment; this should not cause any issues
          skip_marker(file, "PLM", NULL, OJPH_MSG_LEVEL::NO_MSG, false);
        else if (marker_idx == 12)
          //Skipping CRG marker segment;
          skip_marker(file, "CRG", "CRG has been ignored; CRG is related to"
            " where the Cb and Cr colour components are co-sited or located"
            " with respect to the Y' luma component. Perhaps, it is better"
            " to get the individual components and assemble the samples"
            " according to your needs",
            OJPH_MSG_LEVEL::INFO, false);
        else if (marker_idx == 13)
          skip_marker(file, "COM", NULL, OJPH_MSG_LEVEL::NO_MSG, false);
        else if (marker_idx == 14)
          dfs.read(file);
        else if (marker_idx == 15)
          atk.read(file);
        else if (marker_idx == 16)
          nlt.read(file);
        else if (marker_idx == 17)
          break;
        else
          OJPH_ERROR(0x00030051, "File ended before finding a tile segment");
      }

      cod.update_atk(&atk);
      siz.link(&cod);
      if (dfs.exists())
        siz.link(&dfs);

      if (received_markers != 3)
        OJPH_ERROR(0x00030052, "markers error, COD and QCD are required");

      this->infile = file;
      planar = cod.is_employing_color_transform() ? 0 : 1;
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::restrict_input_resolution(ui32 skipped_res_for_read,
      ui32 skipped_res_for_recon)
    {
      if (skipped_res_for_read < skipped_res_for_recon)
        OJPH_ERROR(0x000300A1,
          "skipped_resolution for data %d must be equal or smaller than "
          " skipped_resolution for reconstruction %d\n",
          skipped_res_for_read, skipped_res_for_recon);
      if (skipped_res_for_read > cod.get_num_decompositions())
        OJPH_ERROR(0x000300A2,
          "skipped_resolution for data %d must be smaller than "
          " the number of decomposition levels %d\n",
          skipped_res_for_read, cod.get_num_decompositions());

      this->skipped_res_for_read = skipped_res_for_read;
      this->skipped_res_for_recon = skipped_res_for_recon;
      siz.set_skipped_resolutions(skipped_res_for_recon);
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::enable_resilience()
    {
      if (infile != NULL)
        OJPH_ERROR(0x000300A3, "Codestream resilience must be enabled before"
          " reading file headers.\n");
      this->resilient = true;
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::read()
    {
      this->pre_alloc();
      this->finalize_alloc();

      while (true)
      {
        param_sot sot;
        if (sot.read(infile, resilient))
        {
          ui64 tile_start_location = (ui64)infile->tell();

          if (sot.get_tile_index() > (int)num_tiles.area())
          {
            if (resilient)
              OJPH_INFO(0x00030061, "wrong tile index")
            else
              OJPH_ERROR(0x00030061, "wrong tile index")
          }

          if (sot.get_tile_part_index())
          { //tile part
            if (sot.get_num_tile_parts() &&
              sot.get_tile_part_index() >= sot.get_num_tile_parts())
            {
              if (resilient)
                OJPH_INFO(0x00030062,
                  "error in tile part number, should be smaller than total"
                  " number of tile parts")
              else
                OJPH_ERROR(0x00030062,
                  "error in tile part number, should be smaller than total"
                  " number of tile parts")
            }

            bool sod_found = false;
            ui16 other_tile_part_markers[7] = { SOT, POC, PPT, PLT, COM,
              NLT, SOD };
            while (true)
            {
              int marker_idx = 0;
              int result = 0;
              marker_idx = find_marker(infile, other_tile_part_markers + 1, 6);
              if (marker_idx == 0)
                result = skip_marker(infile, "POC",
                  "POC marker segment in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 1)
                result = skip_marker(infile, "PPT",
                  "PPT marker segment in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 2)
                //Skipping PLT marker segment;this should not cause any issues
                result = skip_marker(infile, "PLT", NULL,
                  OJPH_MSG_LEVEL::NO_MSG, resilient);
              else if (marker_idx == 3)
                result = skip_marker(infile, "COM", NULL,
                  OJPH_MSG_LEVEL::NO_MSG, resilient);
              else if (marker_idx == 4)
                result = skip_marker(infile, "NLT",
                  "NLT marker in tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 5)
              {
                sod_found = true;
                break;
              }

              if (marker_idx == -1) //marker not found
              {
                if (resilient)
                  OJPH_INFO(0x00030063,
                    "File terminated early before start of data is found"
                    " for tile indexed %d and tile part %d",
                    sot.get_tile_index(), sot.get_tile_part_index())
                else
                  OJPH_ERROR(0x00030063,
                    "File terminated early before start of data is found"
                    " for tile indexed %d and tile part %d",
                    sot.get_tile_index(), sot.get_tile_part_index())
                break;
              }
              if (result == -1) //file terminated during marker seg. skipping
              {
                if (resilient)
                  OJPH_INFO(0x00030064,
                    "File terminated during marker segment skipping")
                else
                  OJPH_ERROR(0x00030064,
                    "File terminated during marker segment skipping")
                break;
              }
            }
            if (sod_found)
              tiles[sot.get_tile_index()].parse_tile_header(sot, infile,
                tile_start_location);
          }
          else
          { //first tile part
            bool sod_found = false;
            ui16 first_tile_part_markers[12] = { SOT, COD, COC, QCD, QCC, RGN,
              POC, PPT, PLT, COM, NLT, SOD };
            while (true)
            {
              int marker_idx = 0;
              int result = 0;
              marker_idx = find_marker(infile, first_tile_part_markers+1, 11);
              if (marker_idx == 0)
                result = skip_marker(infile, "COD",
                  "COD marker segment in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 1)
                result = skip_marker(infile, "COC",
                  "COC marker segment in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 2)
                result = skip_marker(infile, "QCD",
                  "QCD marker segment in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 3)
                result = skip_marker(infile, "QCC",
                  "QCC marker segment in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 4)
                result = skip_marker(infile, "RGN",
                  "RGN marker segment in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 5)
                result = skip_marker(infile, "POC",
                  "POC marker segment in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 6)
                result = skip_marker(infile, "PPT",
                  "PPT marker segment in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 7)
                //Skipping PLT marker segment;this should not cause any issues
                result = skip_marker(infile, "PLT", NULL,
                  OJPH_MSG_LEVEL::NO_MSG, resilient);
              else if (marker_idx == 8)
                result = skip_marker(infile, "COM", NULL,
                  OJPH_MSG_LEVEL::NO_MSG, resilient);
              else if (marker_idx == 9)
                result = skip_marker(infile, "NLT",
                  "PPT marker segment in a tile is not supported yet",
                  OJPH_MSG_LEVEL::WARN, resilient);
              else if (marker_idx == 10)
              {
                sod_found = true;
                break;
              }

              if (marker_idx == -1) //marker not found
              {
                if (resilient)
                  OJPH_INFO(0x00030065,
                    "File terminated early before start of data is found"
                    " for tile indexed %d and tile part %d",
                    sot.get_tile_index(), sot.get_tile_part_index())
                else
                  OJPH_ERROR(0x00030065,
                    "File terminated early before start of data is found"
                    " for tile indexed %d and tile part %d",
                    sot.get_tile_index(), sot.get_tile_part_index())
                break;
              }
              if (result == -1) //file terminated during marker seg. skipping
              {
                if (resilient)
                  OJPH_INFO(0x00030066,
                    "File terminated during marker segment skipping")
                else
                  OJPH_ERROR(0x00030066,
                    "File terminated during marker segment skipping")
                break;
              }
            }
            if (sod_found)
              tiles[sot.get_tile_index()].parse_tile_header(sot, infile,
                tile_start_location);
          }
        }

        // check the next marker; either SOT or EOC,
        // if something is broken, just an end of file
        ui16 next_markers[2] = { SOT, EOC };
        int marker_idx = find_marker(infile, next_markers, 2);
        if (marker_idx == -1)
        {
          OJPH_INFO(0x00030067, "File terminated early");
          break;
        }
        else if (marker_idx == 0)
          ;
        else if (marker_idx == 1)
          break;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::set_planar(int planar)
    {
      this->planar = planar;
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::set_profile(const char *s)
    {
      size_t len = strlen(s);
      if (len == 9 && strncmp(s, OJPH_PN_STRING_BROADCAST, 9) == 0)
        profile = OJPH_PN_BROADCAST;
      else if (len == 3 && strncmp(s, OJPH_PN_STRING_IMF, 3) == 0)
        profile = OJPH_PN_IMF;
      else
        OJPH_ERROR(0x000300A1, "unkownn or unsupported profile");
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::set_tilepart_divisions(ui32 value)
    {
      tilepart_div = value;
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::request_tlm_marker(bool needed)
    {
      need_tlm = needed;
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::flush()
    {
      si32 repeat = (si32)num_tiles.area();
      for (si32 i = 0; i < repeat; ++i)
        tiles[i].prepare_for_flush();
      if (need_tlm)
      { //write tlm
        for (si32 i = 0; i < repeat; ++i)
          tiles[i].fill_tlm(&tlm);
        tlm.write(outfile);
      }
      for (si32 i = 0; i < repeat; ++i)
        tiles[i].flush(outfile);
      ui16 t = swap_byte(JP2K_MARKER::EOC);
      if (!outfile->write(&t, 2))
        OJPH_ERROR(0x00030071, "Error writing to file");
    }

    //////////////////////////////////////////////////////////////////////////
    void codestream::close()
    {
      if (infile)
        infile->close();
      if (outfile)
        outfile->close();
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf* codestream::exchange(line_buf *line, ui32 &next_component)
    {
      if (line)
      {
        bool success = false;
        while (!success)
        {
          success = true;
          for (ui32 i = 0; i < num_tiles.w; ++i)
          {
            ui32 idx = i + cur_tile_row * num_tiles.w;
            if ((success &= tiles[idx].push(line, cur_comp)) == false)
              break;
          }
          cur_tile_row += success == false ? 1 : 0;
          if (cur_tile_row >= num_tiles.h)
            cur_tile_row = 0;
        }

        if (planar) //process one component at a time
        {
          if (++cur_line >= comp_size[cur_comp].h)
          {
            cur_line = 0;
            cur_tile_row = 0;
            if (++cur_comp >= num_comps)
            {
              next_component = 0;
              return NULL;
            }
          }
        }
        else //process all component for a line
        {
          if (++cur_comp >= num_comps)
          {
            cur_comp = 0;
            if (++cur_line >= comp_size[cur_comp].h)
            {
              next_component = 0;
              return NULL;
            }
          }
        }
      }

      next_component = cur_comp;
      return this->lines + cur_comp;
    }

    //////////////////////////////////////////////////////////////////////////
    line_buf* codestream::pull(ui32 &comp_num)
    {
      bool success = false;
      while (!success)
      {
        success = true;
        for (ui32 i = 0; i < num_tiles.w; ++i)
        {
          ui32 idx = i + cur_tile_row * num_tiles.w;
          if ((success &= tiles[idx].pull(lines + cur_comp, cur_comp)) == false)
            break;
        }
        cur_tile_row += success == false ? 1 : 0;
        if (cur_tile_row >= num_tiles.h)
          cur_tile_row = 0;
      }
      comp_num = cur_comp;

      if (planar) //process one component at a time
      {
        if (++cur_line >= recon_comp_size[cur_comp].h)
        {
          cur_line = 0;
          cur_tile_row = 0;
          if (cur_comp++ >= num_comps)
          {
            comp_num = 0;
            return NULL;
          }
        }
      }
      else //process all component for a line
      {
        if (++cur_comp >= num_comps)
        {
          cur_comp = 0;
          if (cur_line++ >= recon_comp_size[cur_comp].h)
          {
            comp_num = 0;
            return NULL;
          }
        }
      }

      return lines + comp_num;
    }

  }
}
