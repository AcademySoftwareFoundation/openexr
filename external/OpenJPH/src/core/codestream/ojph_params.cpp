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
// File: ojph_params.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/

#define _USE_MATH_DEFINES
#include <cmath>

#include "ojph_base.h"
#include "ojph_file.h"
#include "ojph_params.h"

#include "ojph_params_local.h"
#include "ojph_message.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void param_siz::set_image_extent(point dims)
  {
    state->Xsiz = dims.x;
    state->Ysiz = dims.y;
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_siz::set_tile_size(size s)
  {
    state->XTsiz = s.w;
    state->YTsiz = s.h;
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_siz::set_image_offset(point offset)
  { // WARNING need to check if these are valid
    state->XOsiz = offset.x;
    state->YOsiz = offset.y;
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_siz::set_tile_offset(point offset)
  { // WARNING need to check if these are valid
    state->XTOsiz = offset.x;
    state->YTOsiz = offset.y;
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_siz::set_num_components(ui32 num_comps)
  {
    state->set_num_components(num_comps);
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_siz::set_component(ui32 comp_num, const point& downsampling,
                                ui32 bit_depth, bool is_signed)
  {
    state->set_comp_info(comp_num, downsampling, bit_depth, is_signed);
  }

  ////////////////////////////////////////////////////////////////////////////
  point param_siz::get_image_extent() const
  {
    return point(state->Xsiz, state->Ysiz);
  }

  ////////////////////////////////////////////////////////////////////////////
  point param_siz::get_image_offset() const
  {
    return point(state->XOsiz, state->YOsiz);
  }

  ////////////////////////////////////////////////////////////////////////////
  size param_siz::get_tile_size() const
  {
    return size(state->XTsiz, state->YTsiz);
  }

  ////////////////////////////////////////////////////////////////////////////
  point param_siz::get_tile_offset() const
  {
    return point(state->XTOsiz, state->YTOsiz);
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 param_siz::get_num_components() const
  {
    return state->Csiz;
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 param_siz::get_bit_depth(ui32 comp_num) const
  {
    return state->get_bit_depth(comp_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_siz::is_signed(ui32 comp_num) const
  {
    return state->is_signed(comp_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  point param_siz::get_downsampling(ui32 comp_num) const
  {
    return state->get_downsampling(comp_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 param_siz::get_recon_width(ui32 comp_num) const
  {
    return state->get_recon_width(comp_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 param_siz::get_recon_height(ui32 comp_num) const
  {
    return state->get_recon_height(comp_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void param_cod::set_num_decomposition(ui32 num_decompositions)
  {
    if (num_decompositions > 32)
      OJPH_ERROR(0x00050001,
        "maximum number of decompositions cannot exceed 32");
    state->SPcod.num_decomp = (ui8)num_decompositions;
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_cod::set_block_dims(ui32 width, ui32 height)
  {
    ui32 log_width = 31 - count_leading_zeros(width);
    ui32 log_height = 31 - count_leading_zeros(height);
    if (width == 0 || width != (1u << log_width)
      || height == 0 || height != (1u << log_height)
      || log_width < 2 || log_height < 2
      || log_width + log_height > 12)
      OJPH_ERROR(0x00050011, "incorrect code block dimensions");
    state->SPcod.block_width = (ui8)(log_width - 2);
    state->SPcod.block_height = (ui8)(log_height - 2);
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_cod::set_precinct_size(int num_levels, size* precinct_size)
  {
    if (num_levels == 0 || precinct_size == NULL)
      state->Scod &= 0xFE;
    else
    {
      state->Scod |= 1;
      for (int i = 0; i <= state->SPcod.num_decomp; ++i)
      {
        size t = precinct_size[i < num_levels ? i : num_levels - 1];

        ui32 PPx = 31 - count_leading_zeros(t.w);
        ui32 PPy = 31 - count_leading_zeros(t.h);
        if (t.w == 0 || t.h == 0)
          OJPH_ERROR(0x00050021, "precinct width or height cannot be 0");
        if (t.w != (1u<<PPx) || t.h != (1u<<PPy))
          OJPH_ERROR(0x00050022,
            "precinct width and height should be a power of 2");
        if (PPx > 15 || PPy > 15)
          OJPH_ERROR(0x00050023, "precinct size is too large");
        if (i > 0 && (PPx == 0 || PPy == 0))
          OJPH_ERROR(0x00050024, "precinct size is too small");
        state->SPcod.precinct_size[i] = (ui8)(PPx | (PPy << 4));
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_cod::set_progression_order(const char *name)
  {
    int prog_order = 0;
    size_t len = strlen(name);
    if (len == 4)
    {
      if (strncmp(name, OJPH_PO_STRING_LRCP, 4) == 0)
        prog_order = OJPH_PO_LRCP;
      else if (strncmp(name, OJPH_PO_STRING_RLCP, 4) == 0)
        prog_order = OJPH_PO_RLCP;
      else if (strncmp(name, OJPH_PO_STRING_RPCL, 4) == 0)
        prog_order = OJPH_PO_RPCL;
      else if (strncmp(name, OJPH_PO_STRING_PCRL, 4) == 0)
        prog_order = OJPH_PO_PCRL;
      else if (strncmp(name, OJPH_PO_STRING_CPRL, 4) == 0)
        prog_order = OJPH_PO_CPRL;
      else
        OJPH_ERROR(0x00050031, "unknown progression order");
    }
    else
      OJPH_ERROR(0x00050032, "improper progression order");


    state->SGCod.prog_order = (ui8)prog_order;
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_cod::set_color_transform(bool color_transform)
  {
    state->employ_color_transform(color_transform ? 1 : 0);
  }

  ////////////////////////////////////////////////////////////////////////////
  void param_cod::set_reversible(bool reversible)
  {
    state->set_reversible(reversible);
  }

  ////////////////////////////////////////////////////////////////////////////
  param_coc param_cod::get_coc(ui32 component_idx)
  {
    local::param_cod *p = state->get_coc(component_idx);
    if (p == state) // no COC segment marker for this component
      p = state->add_coc_object(component_idx);
    return param_coc(p);
  }

  ////////////////////////////////////////////////////////////////////////////
  ui32 param_cod::get_num_decompositions() const
  {
    return state->get_num_decompositions();
  }

  ////////////////////////////////////////////////////////////////////////////
  size param_cod::get_block_dims() const
  {
    return state->get_block_dims();
  }

  ////////////////////////////////////////////////////////////////////////////
  size param_cod::get_log_block_dims() const
  {
    return state->get_log_block_dims();
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_cod::is_reversible() const
  {
    return state->is_reversible();
  }

  ////////////////////////////////////////////////////////////////////////////
  size param_cod::get_precinct_size(ui32 level_num) const
  {
    return state->get_precinct_size(level_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  size param_cod::get_log_precinct_size(ui32 level_num) const
  {
    return state->get_log_precinct_size(level_num);
  }

  ////////////////////////////////////////////////////////////////////////////
  int param_cod::get_progression_order() const
  {
    return state->SGCod.prog_order;
  }

  ////////////////////////////////////////////////////////////////////////////
  const char* param_cod::get_progression_order_as_string() const
  {
    if (state->SGCod.prog_order == OJPH_PO_LRCP)
      return OJPH_PO_STRING_LRCP;
    else if (state->SGCod.prog_order == OJPH_PO_RLCP)
      return OJPH_PO_STRING_RLCP;
    else if (state->SGCod.prog_order == OJPH_PO_RPCL)
      return OJPH_PO_STRING_RPCL;
    else if (state->SGCod.prog_order == OJPH_PO_PCRL)
      return OJPH_PO_STRING_PCRL;
    else if (state->SGCod.prog_order == OJPH_PO_CPRL)
      return OJPH_PO_STRING_CPRL;
    else
      assert(0);
    return "";
  }

  ////////////////////////////////////////////////////////////////////////////
  int param_cod::get_num_layers() const
  {
    return state->SGCod.num_layers;
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_cod::is_using_color_transform() const
  {
    return state->is_employing_color_transform();
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_cod::packets_may_use_sop() const
  {
    return state->packets_may_use_sop();
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_cod::packets_use_eph() const
  {
    return state->packets_use_eph();
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_cod::get_block_vertical_causality() const
  {
    return state->get_block_vertical_causality();
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void param_coc::set_num_decomposition(ui32 num_decompositions)
  { ojph::param_cod(state).set_num_decomposition(num_decompositions); }

  ////////////////////////////////////////////////////////////////////////////
  void param_coc::set_block_dims(ui32 width, ui32 height)
  { ojph::param_cod(state).set_block_dims(width, height); }

  ////////////////////////////////////////////////////////////////////////////
  void param_coc::set_precinct_size(int num_levels, size* precinct_size)
  { ojph::param_cod(state).set_precinct_size(num_levels, precinct_size); }

  ////////////////////////////////////////////////////////////////////////////
  void param_coc::set_reversible(bool reversible)
  { ojph::param_cod(state).set_reversible(reversible); }

  ////////////////////////////////////////////////////////////////////////////
  ui32 param_coc::get_num_decompositions() const
  { return ojph::param_cod(state).get_num_decompositions(); }

  ////////////////////////////////////////////////////////////////////////////
  size param_coc::get_block_dims() const
  { return ojph::param_cod(state).get_block_dims(); }

  ////////////////////////////////////////////////////////////////////////////
  size param_coc::get_log_block_dims() const
  { return ojph::param_cod(state).get_log_block_dims(); }

  ////////////////////////////////////////////////////////////////////////////
  bool param_coc::is_reversible() const
  { return ojph::param_cod(state).is_reversible(); }

  ////////////////////////////////////////////////////////////////////////////
  size param_coc::get_precinct_size(ui32 level_num) const
  { return ojph::param_cod(state).get_precinct_size(level_num); }

  ////////////////////////////////////////////////////////////////////////////
  size param_coc::get_log_precinct_size(ui32 level_num) const
  { return ojph::param_cod(state).get_log_precinct_size(level_num); }

  ////////////////////////////////////////////////////////////////////////////
  bool param_coc::get_block_vertical_causality() const
  { return ojph::param_cod(state).get_block_vertical_causality(); }


  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////
  void param_qcd::set_irrev_quant(float delta)
  {
    state->set_delta(delta);
  }

  //////////////////////////////////////////////////////////////////////////
  void param_qcd::set_irrev_quant(ui32 comp_idx, float delta)
  {
    state->set_delta(comp_idx, delta);
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void param_nlt::set_nonlinear_transform(ui32 comp_num, ui8 nl_type)
  {
    state->set_nonlinear_transform(comp_num, nl_type);
  }

  ////////////////////////////////////////////////////////////////////////////
  bool param_nlt::get_nonlinear_transform(ui32 comp_num, ui8& bit_depth,
                                          bool& is_signed, ui8& nl_type) const
  {
    return state->get_nonlinear_transform(comp_num, bit_depth, is_signed,
                                          nl_type);
  }

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////
  void comment_exchange::set_string(const char* str)
  {
    size_t t = strlen(str);
    if (len > 65531)
      OJPH_ERROR(0x000500C1,
        "COM marker string length cannot be larger than 65531");
    this->data = str;
    this->len = (ui16)t;
    this->Rcom = 1;
  }

  //////////////////////////////////////////////////////////////////////////
  void comment_exchange::set_data(const char* data, ui16 len)
  {
    if (len > 65531)
      OJPH_ERROR(0x000500C2,
        "COM marker string length cannot be larger than 65531");
    this->data = data;
    this->len = len;
    this->Rcom = 0;
  }

  //////////////////////////////////////////////////////////////////////////
  //
  //
  //                                LOCAL
  //
  //
  //////////////////////////////////////////////////////////////////////////

  namespace local {

    //////////////////////////////////////////////////////////////////////////
    static inline
    ui16 swap_byte(ui16 t)
    {
      return (ui16)((t << 8) | (t >> 8));
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    ui32 swap_byte(ui32 t)
    {
      ui32 u = swap_byte((ui16)(t & 0xFFFFu));
      u <<= 16;
      u |= swap_byte((ui16)(t >> 16));
      return u;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    ui64 swap_byte(ui64 t)
    {
      ui64 u = swap_byte((ui32)(t & 0xFFFFFFFFu));
      u <<= 32;
      u |= swap_byte((ui32)(t >> 32));
      return u;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //static
    class sqrt_energy_gains
    {
    public:
      static float get_gain_l(ui32 num_decomp, bool reversible)
      { return reversible ? gain_5x3_l[num_decomp] : gain_9x7_l[num_decomp]; }
      static float get_gain_h(ui32 num_decomp, bool reversible)
      { return reversible ? gain_5x3_h[num_decomp] : gain_9x7_h[num_decomp]; }

    private:
      static const float gain_9x7_l[34];
      static const float gain_9x7_h[34];
      static const float gain_5x3_l[34];
      static const float gain_5x3_h[34];
    };

    //////////////////////////////////////////////////////////////////////////
    const float sqrt_energy_gains::gain_9x7_l[34] = { 1.0000e+00f,
      1.4021e+00f, 2.0304e+00f, 2.9012e+00f, 4.1153e+00f, 5.8245e+00f,
      8.2388e+00f, 1.1652e+01f, 1.6479e+01f, 2.3304e+01f, 3.2957e+01f,
      4.6609e+01f, 6.5915e+01f, 9.3217e+01f, 1.3183e+02f, 1.8643e+02f,
      2.6366e+02f, 3.7287e+02f, 5.2732e+02f, 7.4574e+02f, 1.0546e+03f,
      1.4915e+03f, 2.1093e+03f, 2.9830e+03f, 4.2185e+03f, 5.9659e+03f,
      8.4371e+03f, 1.1932e+04f, 1.6874e+04f, 2.3864e+04f, 3.3748e+04f,
      4.7727e+04f, 6.7496e+04f, 9.5454e+04f };
    const float sqrt_energy_gains::gain_9x7_h[34] = { 1.4425e+00f,
      1.9669e+00f, 2.8839e+00f, 4.1475e+00f, 5.8946e+00f, 8.3472e+00f,
      1.1809e+01f, 1.6701e+01f, 2.3620e+01f, 3.3403e+01f, 4.7240e+01f,
      6.6807e+01f, 9.4479e+01f, 1.3361e+02f, 1.8896e+02f, 2.6723e+02f,
      3.7792e+02f, 5.3446e+02f, 7.5583e+02f, 1.0689e+03f, 1.5117e+03f,
      2.1378e+03f, 3.0233e+03f, 4.2756e+03f, 6.0467e+03f, 8.5513e+03f,
      1.2093e+04f, 1.7103e+04f, 2.4187e+04f, 3.4205e+04f, 4.8373e+04f,
      6.8410e+04f, 9.6747e+04f, 1.3682e+05f };
    const float sqrt_energy_gains::gain_5x3_l[34] = { 1.0000e+00f,
      1.2247e+00f, 1.3229e+00f, 1.5411e+00f, 1.7139e+00f, 1.9605e+00f,
      2.2044e+00f, 2.5047e+00f, 2.8277e+00f, 3.2049e+00f, 3.6238e+00f,
      4.1033e+00f, 4.6423e+00f, 5.2548e+00f, 5.9462e+00f, 6.7299e+00f,
      7.6159e+00f, 8.6193e+00f, 9.7544e+00f, 1.1039e+01f, 1.2493e+01f,
      1.4139e+01f, 1.6001e+01f, 1.8108e+01f, 2.0493e+01f, 2.3192e+01f,
      2.6246e+01f, 2.9702e+01f, 3.3614e+01f, 3.8041e+01f, 4.3051e+01f,
      4.8721e+01f, 5.5138e+01f, 6.2399e+01f };
    const float sqrt_energy_gains::gain_5x3_h[34] = { 1.0458e+00f,
      1.3975e+00f, 1.4389e+00f, 1.7287e+00f, 1.8880e+00f, 2.1841e+00f,
      2.4392e+00f, 2.7830e+00f, 3.1341e+00f, 3.5576e+00f, 4.0188e+00f,
      4.5532e+00f, 5.1494e+00f, 5.8301e+00f, 6.5963e+00f, 7.4663e+00f,
      8.4489e+00f, 9.5623e+00f, 1.0821e+01f, 1.2247e+01f, 1.3860e+01f,
      1.5685e+01f, 1.7751e+01f, 2.0089e+01f, 2.2735e+01f, 2.5729e+01f,
      2.9117e+01f, 3.2952e+01f, 3.7292e+01f, 4.2203e+01f, 4.7761e+01f,
      5.4051e+01f, 6.1170e+01f, 6.9226e+01f };

    //////////////////////////////////////////////////////////////////////////
    //static
    class bibo_gains
    {
    public:
      static float get_bibo_gain_l(ui32 num_decomp, bool reversible)
      { return reversible ? gain_5x3_l[num_decomp] : gain_9x7_l[num_decomp]; }
      static float get_bibo_gain_h(ui32 num_decomp, bool reversible)
      { return reversible ? gain_5x3_h[num_decomp] : gain_9x7_h[num_decomp]; }

    private:
      static const float gain_9x7_l[34];
      static const float gain_9x7_h[34];
      static const float gain_5x3_l[34];
      static const float gain_5x3_h[34];
    };

    //////////////////////////////////////////////////////////////////////////
    const float bibo_gains::gain_9x7_l[34] = { 1.0000e+00f, 1.3803e+00f,
      1.3328e+00f, 1.3067e+00f, 1.3028e+00f, 1.3001e+00f, 1.2993e+00f,
      1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f,
      1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f,
      1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f,
      1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f,
      1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f, 1.2992e+00f,
      1.2992e+00f, 1.2992e+00f };
    const float bibo_gains::gain_9x7_h[34] = { 1.2976e+00f, 1.3126e+00f,
      1.2757e+00f, 1.2352e+00f, 1.2312e+00f, 1.2285e+00f, 1.2280e+00f,
      1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f,
      1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f,
      1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f,
      1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f,
      1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f, 1.2278e+00f,
      1.2278e+00f, 1.2278e+00f };
    const float bibo_gains::gain_5x3_l[34] = { 1.0000e+00f, 1.5000e+00f,
      1.6250e+00f, 1.6875e+00f, 1.6963e+00f, 1.7067e+00f, 1.7116e+00f,
      1.7129e+00f, 1.7141e+00f, 1.7145e+00f, 1.7151e+00f, 1.7152e+00f,
      1.7155e+00f, 1.7155e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f,
      1.7156e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f,
      1.7156e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f,
      1.7156e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f, 1.7156e+00f,
      1.7156e+00f, 1.7156e+00f };
    const float bibo_gains::gain_5x3_h[34] = { 2.0000e+00f, 2.5000e+00f,
      2.7500e+00f, 2.8047e+00f, 2.8198e+00f, 2.8410e+00f, 2.8558e+00f,
      2.8601e+00f, 2.8628e+00f, 2.8656e+00f, 2.8662e+00f, 2.8667e+00f,
      2.8669e+00f, 2.8670e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f,
      2.8671e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f,
      2.8671e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f,
      2.8671e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f, 2.8671e+00f,
      2.8671e+00f, 2.8671e+00f };


    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    bool param_siz::write(outfile_base *file)
    {
      //marker size excluding header
      Lsiz = (ui16)(38 + 3 * Csiz);

      ui8 buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::SIZ;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lsiz);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Rsiz);
      result &= file->write(&buf, 2) == 2;
      *(ui32*)buf = swap_byte(Xsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(Ysiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(XOsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(YOsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(XTsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(YTsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(XTOsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui32*)buf = swap_byte(YTOsiz);
      result &= file->write(&buf, 4) == 4;
      *(ui16*)buf = swap_byte(Csiz);
      result &= file->write(&buf, 2) == 2;
      for (int c = 0; c < Csiz; ++c)
      {
        buf[0] = cptr[c].SSiz;
        buf[1] = cptr[c].XRsiz;
        buf[2] = cptr[c].YRsiz;
        result &= file->write(&buf, 3) == 3;
      }

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    void param_siz::read(infile_base *file)
    {
      if (file->read(&Lsiz, 2) != 2)
        OJPH_ERROR(0x00050041, "error reading SIZ marker");
      Lsiz = swap_byte(Lsiz);
      int num_comps = (Lsiz - 38) / 3;
      if (Lsiz != 38 + 3 * num_comps)
        OJPH_ERROR(0x00050042, "error in SIZ marker length");
      if (file->read(&Rsiz, 2) != 2)
        OJPH_ERROR(0x00050043, "error reading SIZ marker");
      Rsiz = swap_byte(Rsiz);
      if ((Rsiz & 0x4000) == 0)
        OJPH_ERROR(0x00050044,
          "Rsiz bit 14 is not set (this is not a JPH file)");
      if ((Rsiz & 0x8000) != 0 && (Rsiz & 0xD5F) != 0)
        OJPH_WARN(0x00050001, "Rsiz in SIZ has unimplemented fields");
      if (file->read(&Xsiz, 4) != 4)
        OJPH_ERROR(0x00050045, "error reading SIZ marker");
      Xsiz = swap_byte(Xsiz);
      if (file->read(&Ysiz, 4) != 4)
        OJPH_ERROR(0x00050046, "error reading SIZ marker");
      Ysiz = swap_byte(Ysiz);
      if (file->read(&XOsiz, 4) != 4)
        OJPH_ERROR(0x00050047, "error reading SIZ marker");
      XOsiz = swap_byte(XOsiz);
      if (file->read(&YOsiz, 4) != 4)
        OJPH_ERROR(0x00050048, "error reading SIZ marker");
      YOsiz = swap_byte(YOsiz);
      if (file->read(&XTsiz, 4) != 4)
        OJPH_ERROR(0x00050049, "error reading SIZ marker");
      XTsiz = swap_byte(XTsiz);
      if (file->read(&YTsiz, 4) != 4)
        OJPH_ERROR(0x0005004A, "error reading SIZ marker");
      YTsiz = swap_byte(YTsiz);
      if (file->read(&XTOsiz, 4) != 4)
        OJPH_ERROR(0x0005004B, "error reading SIZ marker");
      XTOsiz = swap_byte(XTOsiz);
      if (file->read(&YTOsiz, 4) != 4)
        OJPH_ERROR(0x0005004C, "error reading SIZ marker");
      YTOsiz = swap_byte(YTOsiz);
      if (file->read(&Csiz, 2) != 2)
        OJPH_ERROR(0x0005004D, "error reading SIZ marker");
      Csiz = swap_byte(Csiz);
      if (Csiz != num_comps)
        OJPH_ERROR(0x0005004E, "Csiz does not match the SIZ marker size");
      set_num_components(Csiz);
      for (int c = 0; c < Csiz; ++c)
      {
        if (file->read(&cptr[c].SSiz, 1) != 1)
          OJPH_ERROR(0x00050051, "error reading SIZ marker");
        if (file->read(&cptr[c].XRsiz, 1) != 1)
          OJPH_ERROR(0x00050052, "error reading SIZ marker");
        if (file->read(&cptr[c].YRsiz, 1) != 1)
          OJPH_ERROR(0x00050053, "error reading SIZ marker");
      }

      ws_kern_support_needed = (Rsiz & 0x20) != 0;
      dfs_support_needed = (Rsiz & 0x80) != 0;
    }

    //////////////////////////////////////////////////////////////////////////
    point param_siz::get_recon_downsampling(ui32 comp_num) const
    {
      assert(comp_num < get_num_components());

      point factor(1u << skipped_resolutions, 1u << skipped_resolutions);
      const param_cod* cdp = cod->get_coc(comp_num);
      if (dfs && cdp && cdp->is_dfs_defined()) {
        const param_dfs* d = dfs->get_dfs(cdp->get_dfs_index());
        factor = d->get_res_downsamp(skipped_resolutions);
      }
      factor.x *= (ui32)cptr[comp_num].XRsiz;
      factor.y *= (ui32)cptr[comp_num].YRsiz;
      return factor;
    }

    //////////////////////////////////////////////////////////////////////////
    point param_siz::get_recon_size(ui32 comp_num) const
    {
      assert(comp_num < get_num_components());

      point factor = get_recon_downsampling(comp_num);
      point r;
      r.x = ojph_div_ceil(Xsiz, factor.x) - ojph_div_ceil(XOsiz, factor.x);
      r.y = ojph_div_ceil(Ysiz, factor.y) - ojph_div_ceil(YOsiz, factor.y);
      return r;
    }


    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    bool param_cap::write(outfile_base *file)
    {
      //marker size excluding header
      Lcap = 8;

      char buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::CAP;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lcap);
      result &= file->write(&buf, 2) == 2;
      *(ui32*)buf = swap_byte(Pcap);
      result &= file->write(&buf, 4) == 4;

      *(ui16*)buf = swap_byte(Ccap[0]);
      result &= file->write(&buf, 2) == 2;

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    void param_cap::read(infile_base *file)
    {
      if (file->read(&Lcap, 2) != 2)
        OJPH_ERROR(0x00050061, "error reading CAP marker");
      Lcap = swap_byte(Lcap);
      if (file->read(&Pcap, 4) != 4)
        OJPH_ERROR(0x00050062, "error reading CAP marker");
      Pcap = swap_byte(Pcap);
      ui32 count = population_count(Pcap);
      if (Pcap & 0xFFFDFFFF)
        OJPH_ERROR(0x00050063,
          "error Pcap in CAP has options that are not supported");
      if ((Pcap & 0x00020000) == 0)
        OJPH_ERROR(0x00050064,
          "error Pcap should have its 15th MSB set, Pcap^15. "
          " This is not a JPH file");
      for (ui32 i = 0; i < count; ++i)
        if (file->read(Ccap+i, 2) != 2)
          OJPH_ERROR(0x00050065, "error reading CAP marker");
      if (Lcap != 6 + 2 * count)
        OJPH_ERROR(0x00050066, "error in CAP marker length");
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    bool param_cod::is_reversible() const
    {
      if (SPcod.wavelet_trans <= 1)
        return get_wavelet_kern() == local::param_cod::DWT_REV53;
      else {
        assert(atk != NULL);
        return atk->is_reversible();
      }
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_cod::write(outfile_base *file)
    {
      assert(type == COD_MAIN);

      //marker size excluding header
      Lcod = 12;
      Lcod = (ui16)(Lcod + (Scod & 1 ? 1 + SPcod.num_decomp : 0));

      ui8 buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::COD;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lcod);
      result &= file->write(&buf, 2) == 2;
      *(ui8*)buf = Scod;
      result &= file->write(&buf, 1) == 1;
      *(ui8*)buf = SGCod.prog_order;
      result &= file->write(&buf, 1) == 1;
      *(ui16*)buf = swap_byte(SGCod.num_layers);
      result &= file->write(&buf, 2) == 2;
      *(ui8*)buf = SGCod.mc_trans;
      result &= file->write(&buf, 1) == 1;
      buf[0] = SPcod.num_decomp;
      buf[1] = SPcod.block_width;
      buf[2] = SPcod.block_height;
      buf[3] = SPcod.block_style;
      result &= file->write(&buf, 4) == 4;
      *(ui8*)buf = SPcod.wavelet_trans;
      result &= file->write(&buf, 1) == 1;
      if (Scod & 1)
        for (int i = 0; i <= SPcod.num_decomp; ++i)
        {
          *(ui8*)buf = SPcod.precinct_size[i];
          result &= file->write(&buf, 1) == 1;
        }

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_cod::write_coc(outfile_base *file, ui32 num_comps)
    {
      assert(type == COD_MAIN);
      bool result = true;
      param_cod *p = this->next;
      while (p)
      {
        if (p->comp_idx < num_comps)
          result &= p->internal_write_coc(file, num_comps);
        p = p->next;
      }
      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_cod::internal_write_coc(outfile_base *file, ui32 num_comps)
    {
      assert(type == COC_MAIN);

      //marker size excluding header
      Lcod = num_comps < 257 ? 9 : 10;
      Lcod = (ui16)(Lcod + (Scod & 1 ? 1 + SPcod.num_decomp : 0));

      ui8 buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::COC;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lcod);
      result &= file->write(&buf, 2) == 2;
      if (num_comps < 257)
      {
        *(ui8*)buf = (ui8)comp_idx;
        result &= file->write(&buf, 1) == 1;
      }
      else
      {
        *(ui16*)buf = swap_byte(comp_idx);
        result &= file->write(&buf, 2) == 2;
      }
      *(ui8*)buf = Scod;
      result &= file->write(&buf, 1) == 1;
      buf[0] = SPcod.num_decomp;
      buf[1] = SPcod.block_width;
      buf[2] = SPcod.block_height;
      buf[3] = SPcod.block_style;
      result &= file->write(&buf, 4) == 4;
      *(ui8*)buf = SPcod.wavelet_trans;
      result &= file->write(&buf, 1) == 1;
      if (Scod & 1)
        for (int i = 0; i <= SPcod.num_decomp; ++i)
        {
          *(ui8*)buf = SPcod.precinct_size[i];
          result &= file->write(&buf, 1) == 1;
        }

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    void param_cod::read(infile_base *file)
    {
      assert(type == COD_MAIN);

      if (file->read(&Lcod, 2) != 2)
        OJPH_ERROR(0x00050071, "error reading COD segment");
      Lcod = swap_byte(Lcod);
      if (file->read(&Scod, 1) != 1)
        OJPH_ERROR(0x00050072, "error reading COD segment");
      if (file->read(&SGCod.prog_order, 1) != 1)
        OJPH_ERROR(0x00050073, "error reading COD segment");
      if (file->read(&SGCod.num_layers, 2) != 2)
      { OJPH_ERROR(0x00050074, "error reading COD segment"); }
      else
        SGCod.num_layers = swap_byte(SGCod.num_layers);
      if (file->read(&SGCod.mc_trans, 1) != 1)
        OJPH_ERROR(0x00050075, "error reading COD segment");
      if (file->read(&SPcod.num_decomp, 1) != 1)
        OJPH_ERROR(0x00050076, "error reading COD segment");
      if (file->read(&SPcod.block_width, 1) != 1)
        OJPH_ERROR(0x00050077, "error reading COD segment");
      if (file->read(&SPcod.block_height, 1) != 1)
        OJPH_ERROR(0x00050078, "error reading COD segment");
      if (file->read(&SPcod.block_style, 1) != 1)
        OJPH_ERROR(0x00050079, "error reading COD segment");
      if (file->read(&SPcod.wavelet_trans, 1) != 1)
        OJPH_ERROR(0x0005007A, "error reading COD segment");

      if (get_num_decompositions() > 32
        || SPcod.block_width > 8
        || SPcod.block_height > 8
        || SPcod.block_width + SPcod.block_height > 8
        || (SPcod.block_style & 0x40) != 0x40
        || (SPcod.block_style & 0xB7) != 0x00)
        OJPH_ERROR(0x0005007D, "wrong settings in a COD-SPcod parameter");
      if ((SPcod.block_style & 0x40) != 0x40
        || (SPcod.block_style & 0xB7) != 0x00)
        OJPH_ERROR(0x0005007E, "unsupported settings in a COD-SPcod parameter");

      ui8 num_decompositions =  get_num_decompositions();
      if (Scod & 1)
        for (int i = 0; i <= num_decompositions; ++i)
          if (file->read(&SPcod.precinct_size[i], 1) != 1)
            OJPH_ERROR(0x0005007B, "error reading COD segment");
      if (Lcod != 12 + ((Scod & 1) ? 1 + SPcod.num_decomp : 0))
        OJPH_ERROR(0x0005007C, "error in COD segment length");
    }

    //////////////////////////////////////////////////////////////////////////
    void param_cod::read_coc(infile_base* file, ui32 num_comps,
                             param_cod *top_cod)
    {
      assert(type == COC_MAIN);
      assert(top_cod != NULL);

      this->SGCod = top_cod->SGCod;
      this->top_cod = top_cod;
      if (file->read(&Lcod, 2) != 2)
        OJPH_ERROR(0x00050121, "error reading COC segment");
      Lcod = swap_byte(Lcod);
      if (num_comps < 257) {
        ui8 t;
        if (file->read(&t, 1) != 1)
          OJPH_ERROR(0x00050122, "error reading COC segment");
        comp_idx = t;
      }
      else {
        if (file->read(&comp_idx, 2) != 2)
          OJPH_ERROR(0x00050123, "error reading COC segment");
        comp_idx = swap_byte(comp_idx);
      }
      if (file->read(&Scod, 1) != 1)
        OJPH_ERROR(0x00050124, "error reading COC segment");
      if (Scod & 0xF8)
        OJPH_WARN(0x00050011,
          "Unsupported options in Scoc field of the COC segment");
      if (file->read(&SPcod.num_decomp, 1) != 1)
        OJPH_ERROR(0x00050125, "error reading COC segment");
      if (file->read(&SPcod.block_width, 1) != 1)
        OJPH_ERROR(0x00050126, "error reading COC segment");
      if (file->read(&SPcod.block_height, 1) != 1)
        OJPH_ERROR(0x00050127, "error reading COC segment");
      if (file->read(&SPcod.block_style, 1) != 1)
        OJPH_ERROR(0x00050128, "error reading COC segment");
      if (file->read(&SPcod.wavelet_trans, 1) != 1)
        OJPH_ERROR(0x00050129, "error reading COC segment");

      if (get_num_decompositions() > 32
        || SPcod.block_width > 8
        || SPcod.block_height > 8
        || SPcod.block_width + SPcod.block_height > 8
        || (SPcod.block_style & 0x40) != 0x40
        || (SPcod.block_style & 0xB7) != 0x00)
        OJPH_ERROR(0x0005012C, "wrong settings in a COC-SPcoc parameter");
      if ((SPcod.block_style & 0x40) != 0x40
        || (SPcod.block_style & 0xB7) != 0x00)
        OJPH_ERROR(0x0005012D, "unsupported settings in a COC-SPcoc parameter");

      ui8 num_decompositions =  get_num_decompositions();
      if (Scod & 1)
        for (int i = 0; i <= num_decompositions; ++i)
          if (file->read(&SPcod.precinct_size[i], 1) != 1)
            OJPH_ERROR(0x0005012A, "error reading COC segment");
      ui32 t = 9;
      t += num_comps < 257 ? 0 : 1;
      t += (Scod & 1) ? 1 + num_decompositions : 0;
      if (Lcod != t)
        OJPH_ERROR(0x0005012B, "error in COC segment length");
    }

    //////////////////////////////////////////////////////////////////////////
    void param_cod::update_atk(param_atk* atk)
    {
      assert(type == COD_MAIN);
      this->atk = atk->get_atk(SPcod.wavelet_trans);
      if (this->atk == NULL)
        OJPH_ERROR(0x00050131, "A COD segment employs the DWT kernel "
          "atk = %d, but a corresponding ATK segment cannot be found.",
          SPcod.wavelet_trans);
      param_cod *p = next;
      while (p)
      {
        p->atk = atk->get_atk(p->SPcod.wavelet_trans);
        if (p->atk == NULL)
          OJPH_ERROR(0x00050132, "A COC segment employs the DWT kernel "
            "atk = %d, but a corresponding ATK segment cannot be found",
            SPcod.wavelet_trans);
        p = p->next;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    const param_cod* param_cod::get_coc(ui32 comp_idx) const
    {
      assert(this->type == COD_MAIN || this->top_cod->type == COD_MAIN);
      const param_cod *p, *q;
      if (this->type == COD_MAIN)
        q = p = this;
      else
        q = p = this->top_cod;
      while (p && p->comp_idx != comp_idx)
        p = p->next;
      return p ? p : q;
    }

    ////////////////////////////////////////
    param_cod* param_cod::get_coc(ui32 comp_idx)
    {
      // cast object to constant
      const param_cod* const_p = const_cast<const param_cod*>(this);
      // call using the constant object, then cast to non-const
      return const_cast<param_cod*>(const_p->get_coc(comp_idx));
    }

    ////////////////////////////////////////
    param_cod* param_cod::add_coc_object(ui32 comp_idx)
    {
      assert(type == COD_MAIN);
      param_cod *p = this;
      while (p->next != NULL)
        p = p->next;
      if (avail)
      {
        p->next = avail;
        avail = avail->next;
        p->next->init(this, (ui16)comp_idx);
      }
      else
        p->next = new param_cod(this, (ui16)comp_idx);
      return p->next;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void param_qcd::check_validity(const param_siz& siz, const param_cod& cod)
    {
      ui32 num_comps = siz.get_num_components();
      trim_non_existing_components(num_comps);

      // first check that all the component captured by QCD have the same
      // bit_depth and signedness
      bool all_same = true;
      bool other_comps_exist = false;
      ui32 first_comp = 0xFFFF; // an impossible component
      {
        ui32 num_decompositions = 0;
        ui32 bit_depth = 0;
        bool is_signed = false;
        ui32 wavelet_kern = param_cod::DWT_IRV97;

        for (ui32 c = 0; c < num_comps; ++c)
        {
          if (get_qcc(c) == this) // no qcc defined for component c
          {
            const param_cod *p = cod.get_coc(c);
            if (bit_depth == 0) // first component captured by QCD
            {
              num_decompositions = p->get_num_decompositions();
              bit_depth = siz.get_bit_depth(c);
              is_signed = siz.is_signed(c);
              wavelet_kern = p->get_wavelet_kern();
              first_comp = c;
            }
            else
            {
              all_same = all_same
                && (num_decompositions == p->get_num_decompositions())
                && (bit_depth == siz.get_bit_depth(c))
                && (is_signed == siz.is_signed(c))
                && (wavelet_kern == p->get_wavelet_kern());
            }
          }
          else
            other_comps_exist = true;
        }
      }

      // configure QCD according COD
      ui32 qcd_num_decompositions;
      ui32 qcd_bit_depth;
      bool qcd_is_signed;
      ui32 qcd_wavelet_kern;
      {
        ui32 qcd_component = first_comp != 0xFFFF ? first_comp : 0;
        bool employing_color_transform = cod.is_employing_color_transform();
        qcd_num_decompositions = cod.get_num_decompositions();
        qcd_bit_depth = siz.get_bit_depth(qcd_component);
        qcd_is_signed = siz.is_signed(qcd_component);
        qcd_wavelet_kern = cod.get_wavelet_kern();
        this->num_subbands = 1 + 3 * qcd_num_decompositions;
        if (qcd_wavelet_kern == param_cod::DWT_REV53)
          set_rev_quant(qcd_num_decompositions, qcd_bit_depth,
            qcd_component < 3 ? employing_color_transform : false);
        else if (qcd_wavelet_kern == param_cod::DWT_IRV97)
        {
          if (this->base_delta == -1.0f)
            this->base_delta = 1.0f / (float)(1 << qcd_bit_depth);
          set_irrev_quant(qcd_num_decompositions);
        }
        else
          assert(0);
      }

      // if not all the same and captured by QCD, then create QCC for them
      if (!all_same)
      {
        bool employing_color_transform = cod.is_employing_color_transform();
        for (ui32 c = 0; c < num_comps; ++c)
        {
          const param_cod *cp = cod.get_coc(c);
          if (qcd_num_decompositions == cp->get_num_decompositions()
              && qcd_bit_depth == siz.get_bit_depth(c)
              && qcd_is_signed == siz.is_signed(c)
              && qcd_wavelet_kern == cp->get_wavelet_kern())
            continue; // captured by QCD

          // Does not match QCD, must have QCC
          param_qcd *qp = get_qcc(c);
          if (qp == this) // no QCC was defined, create QCC
            qp = this->add_qcc_object(c);

          ui32 num_decompositions = cp->get_num_decompositions();
          qp->num_subbands = 1 + 3 * num_decompositions;
          ui32 bit_depth = siz.get_bit_depth(c);
          if (cp->get_wavelet_kern() == param_cod::DWT_REV53)
            qp->set_rev_quant(num_decompositions, bit_depth,
              c < 3 ? employing_color_transform : false);
          else if (cp->get_wavelet_kern() == param_cod::DWT_IRV97)
          {
            if (qp->base_delta == -1.0f)
              qp->base_delta = 1.0f / (float)(1 << bit_depth);
            qp->set_irrev_quant(num_decompositions);
          }
          else
            assert(0);
        }
      }
      else if (other_comps_exist) // Some are captured by QCD
      {
        bool employing_color_transform = cod.is_employing_color_transform();
        for (ui32 c = 0; c < num_comps; ++c)
        {
          param_qcd *qp = get_qcc(c);
          if (qp == this) // if captured by QCD continue
            continue;
          const param_cod *cp = cod.get_coc(c);
          ui32 num_decompositions = cp->get_num_decompositions();
          qp->num_subbands = 1 + 3 * num_decompositions;
          ui32 bit_depth = siz.get_bit_depth(c);
          if (cp->get_wavelet_kern() == param_cod::DWT_REV53)
            qp->set_rev_quant(num_decompositions, bit_depth,
              c < 3 ? employing_color_transform : false);
          else if (cp->get_wavelet_kern() == param_cod::DWT_IRV97)
          {
            if (qp->base_delta == -1.0f)
              qp->base_delta = 1.0f / (float)(1 << bit_depth);
            qp->set_irrev_quant(num_decompositions);
          }
          else
            assert(0);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void param_qcd::set_delta(ui32 comp_idx, float delta)
    {
      assert(type == QCD_MAIN);
      param_qcd *p = get_qcc(comp_idx);
      if (p == NULL)
        p = add_qcc_object(comp_idx);
      p->set_delta(delta);
    }

    //////////////////////////////////////////////////////////////////////////
    void param_qcd::set_rev_quant(ui32 num_decomps, ui32 bit_depth,
                                  bool is_employing_color_transform)
    {
      ui32 B = bit_depth;
      B += is_employing_color_transform ? 1 : 0; //1 bit for RCT
      int s = 0;
      double bibo_l = bibo_gains::get_bibo_gain_l(num_decomps, true);
      ui32 X = (ui32) ceil(log(bibo_l * bibo_l) / M_LN2);
      SPqcd.u8[s++] = (ui8)(B + X);
      ui32 max_B_plus_X = (ui32)(B + X);
      for (ui32 d = num_decomps; d > 0; --d)
      {
        double bibo_l = bibo_gains::get_bibo_gain_l(d, true);
        double bibo_h = bibo_gains::get_bibo_gain_h(d - 1, true);
        X = (ui32) ceil(log(bibo_h * bibo_l) / M_LN2);
        SPqcd.u8[s++] = (ui8)(B + X);
        max_B_plus_X = ojph_max(max_B_plus_X, B + X);
        SPqcd.u8[s++] = (ui8)(B + X);
        max_B_plus_X = ojph_max(max_B_plus_X, B + X);
        X = (ui32) ceil(log(bibo_h * bibo_h) / M_LN2);
        SPqcd.u8[s++] = (ui8)(B + X);
        max_B_plus_X = ojph_max(max_B_plus_X, B + X);
      }

      if (max_B_plus_X > 38)
        OJPH_ERROR(0x00050151, "The specified combination of bit_depth, "
         "colour transform, and type of wavelet transform requires more than "
         "38 bits; it requires %d bits. This is beyond what is allowed in "
         "the JPEG2000 image coding format.", max_B_plus_X);

      int guard_bits = ojph_max(1, (si32)max_B_plus_X - 31);
      Sqcd = (ui8)(guard_bits << 5);
      s = 0;
      SPqcd.u8[s] = encode_SPqcd((ui8)(SPqcd.u8[s] - guard_bits));
      s++;
      for (ui32 d = num_decomps; d > 0; --d)
      {
        SPqcd.u8[s] = encode_SPqcd((ui8)(SPqcd.u8[s] - guard_bits));
        s++;
        SPqcd.u8[s] = encode_SPqcd((ui8)(SPqcd.u8[s] - guard_bits));
        s++;
        SPqcd.u8[s] = encode_SPqcd((ui8)(SPqcd.u8[s] - guard_bits));
        s++;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void param_qcd::set_irrev_quant(ui32 num_decomps)
    {
      int guard_bits = 1;
      Sqcd = (ui8)((guard_bits<<5)|0x2);//one guard bit, scalar quantization
      int s = 0;
      float gain_l = sqrt_energy_gains::get_gain_l(num_decomps, false);
      float delta_b = base_delta / (gain_l * gain_l);
      int exp = 0, mantissa;
      while (delta_b < 1.0f)
      { exp++; delta_b *= 2.0f; }
      //with rounding, there is a risk of becoming equal to 1<<12
      // but that should not happen in reality
      mantissa = (int)round(delta_b * (float)(1<<11)) - (1<<11);
      mantissa = mantissa < (1<<11) ? mantissa : 0x7FF;
      SPqcd.u16[s++] = (ui16)((exp << 11) | mantissa);
      for (ui32 d = num_decomps; d > 0; --d)
      {
        float gain_l = sqrt_energy_gains::get_gain_l(d, false);
        float gain_h = sqrt_energy_gains::get_gain_h(d - 1, false);

        delta_b = base_delta / (gain_l * gain_h);

        int exp = 0, mantissa;
        while (delta_b < 1.0f)
        { exp++; delta_b *= 2.0f; }
        mantissa = (int)round(delta_b * (float)(1<<11)) - (1<<11);
        mantissa = mantissa < (1<<11) ? mantissa : 0x7FF;
        SPqcd.u16[s++] = (ui16)((exp << 11) | mantissa);
        SPqcd.u16[s++] = (ui16)((exp << 11) | mantissa);

        delta_b = base_delta / (gain_h * gain_h);

        exp = 0;
        while (delta_b < 1)
        { exp++; delta_b *= 2.0f; }
        mantissa = (int)round(delta_b * (float)(1<<11)) - (1<<11);
        mantissa = mantissa < (1<<11) ? mantissa : 0x7FF;
        SPqcd.u16[s++] = (ui16)((exp << 11) | mantissa);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 param_qcd::get_MAGB() const
    {
      ui32 B = 0;

      const param_qcd *p = this;
      while (p)
      {
        //this can be written better, but it is only executed once
        // this assumes a bi-directional wavelet (conventional DWT)
        ui32 num_decomps = (p->num_subbands - 1) / 3;

        int irrev = p->Sqcd & 0x1F;
        if (irrev == 0) //reversible
          for (ui32 i = 0; i < p->num_subbands; ++i) {
            ui32 t = p->decode_SPqcd(p->SPqcd.u8[i]);
            t += p->get_num_guard_bits() - 1u;
            B = ojph_max(B, t);
          }
        else if (irrev == 2) //scalar expounded
          for (ui32 i = 0; i < p->num_subbands; ++i)
          {
            ui32 nb = num_decomps - (i ? (i - 1) / 3 : 0); //decompsition level
            ui32 t = (p->SPqcd.u16[i] >> 11) + p->get_num_guard_bits() - nb;
            B = ojph_max(B, t);
          }
        else
          assert(0);

        p = p->next;
      }

      return B;
    }

    //////////////////////////////////////////////////////////////////////////
    float param_qcd::get_irrev_delta(const param_dfs* dfs,
                                     ui32 num_decompositions,
                                     ui32 resolution, ui32 subband) const
    {
      float arr[] = { 1.0f, 2.0f, 2.0f, 4.0f };
      assert((Sqcd & 0x1F) == 2);

      ui32 idx;
      if (dfs != NULL && dfs->exists())
        idx = dfs->get_subband_idx(num_decompositions, resolution, subband);
      else
        idx = resolution ? (resolution - 1) * 3 + subband : 0;
      if (idx >= num_subbands) {
        OJPH_INFO(0x00050101, "Trying to access quantization step size for "
          "subband %d when the QCD/QCC marker segment specifies "
          "quantization step sizes for %d subbands only.  To continue "
          "decoding, we are using the step size for subband %d, which can "
          "produce incorrect results",
          idx + 1, num_subbands, num_subbands - 1);
        idx = num_subbands - 1;
      }
      int eps = SPqcd.u16[idx] >> 11;
      float mantissa;
      mantissa = (float)((SPqcd.u16[idx] & 0x7FF) | 0x800) * arr[subband];
      mantissa /= (float)(1 << 11);
      mantissa /= (float)(1u << eps);
      return mantissa;
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 param_qcd::propose_precision(const param_cod* cod) const
    {
      ui32 comp_idx = cod->get_comp_idx();
      ui32 precision = 0;
      const param_cod *main =
        cod->get_coc(param_cod::OJPH_COD_DEFAULT);
      if (main->is_employing_color_transform() && comp_idx < 3)
      {
        for (ui32 i = 0; i < 3; ++i) {
          const param_qcd* p = this->get_qcc(i);
          precision = ojph_max(precision, p->get_largest_Kmax());
        }
      }
      else {
        precision = get_largest_Kmax();
      }
      // ``precision'' now holds the largest K_max, which excludes the sign
      // bit.
      // + 1 for the sign bit
      // + 1 because my block decoder/encoder does not supports up to 30
      //     bits (not 31), so we bump it by one more bit.
      return precision + 1 + 1;
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 param_qcd::get_num_guard_bits() const
    {
      return (Sqcd >> 5);
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 param_qcd::get_Kmax(const param_dfs* dfs, ui32 num_decompositions,
                             ui32 resolution, ui32 subband) const
    {
      ui32 idx;
      if (dfs != NULL && dfs->exists())
        idx = dfs->get_subband_idx(num_decompositions, resolution, subband);
      else
        idx = resolution ? (resolution - 1) * 3 + subband : 0;
      if (idx >= num_subbands) {
        OJPH_INFO(0x00050111, "Trying to access quantization step size for "
          "subband %d when the QCD/QCC marker segment specifies "
          "quantization step sizes for %d subbands only.  To continue "
          "decoding, we are using the step size for subband %d, which can "
          "produce incorrect results",
          idx + 1, num_subbands, num_subbands - 1);
        idx = num_subbands - 1;
      }

      int irrev = Sqcd & 0x1F;
      ui32 num_bits = 0;
      if (irrev == 0) // reversible; this is (10.22) from the J2K book
      {
        num_bits = decode_SPqcd(SPqcd.u8[idx]);
        num_bits = num_bits == 0 ? 0 : num_bits - 1;
      }
      else if (irrev == 1)
        assert(0);
      else if (irrev == 2) //scalar expounded
        num_bits = (SPqcd.u16[idx] >> 11) - 1;
      else
        assert(0);

      return num_bits + get_num_guard_bits();
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 param_qcd::get_largest_Kmax() const
    {
      int irrev = Sqcd & 0x1F;
      ui32 num_bits = 0;
      if (irrev == 0) // reversible; this is (10.22) from the J2K book
      {
        for (ui32 i = 0; i < num_subbands; ++i) {
          ui32 t = decode_SPqcd(SPqcd.u8[i]);
          num_bits = ojph_max(num_bits, t == 0 ? 0 : t - 1);
        }
      }
      else if (irrev == 1)
        assert(0);
      else if (irrev == 2) //scalar expounded
      {
        for (ui32 i = 0; i < num_subbands; ++i) {
          ui32 t = (SPqcd.u16[i] >> 11) - 1;
          num_bits = ojph_max(num_bits, t);
        }
      }
      else
        assert(0);

      return num_bits + get_num_guard_bits();
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_qcd::write(outfile_base *file)
    {
      int irrev = Sqcd & 0x1F;

      //marker size excluding header
      Lqcd = 3;
      if (irrev == 0)
        Lqcd = (ui16)(Lqcd + num_subbands);
      else if (irrev == 2)
        Lqcd = (ui16)(Lqcd + 2 * num_subbands);
      else
        assert(0);

      char buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::QCD;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lqcd);
      result &= file->write(&buf, 2) == 2;
      *(ui8*)buf = Sqcd;
      result &= file->write(&buf, 1) == 1;

      if (irrev == 0)
        for (ui32 i = 0; i < num_subbands; ++i)
        {
          *(ui8*)buf = SPqcd.u8[i];
          result &= file->write(&buf, 1) == 1;
        }
      else if (irrev == 2)
        for (ui32 i = 0; i < num_subbands; ++i)
        {
          *(ui16*)buf = swap_byte(SPqcd.u16[i]);
          result &= file->write(&buf, 2) == 2;
        }
      else
        assert(0);

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_qcd::write_qcc(outfile_base *file, ui32 num_comps)
    {
      assert(type == QCD_MAIN);
      bool result = true;
      param_qcd *p = this->next;
      while (p)
      {
        if (p->enabled)
          result &= p->internal_write_qcc(file, num_comps);
        p = p->next;
      }
      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_qcd::internal_write_qcc(outfile_base *file, ui32 num_comps)
    {
      int irrev = Sqcd & 0x1F;

      //marker size excluding header
      Lqcd = (ui16)(4 + (num_comps < 257 ? 0 : 1));
      if (irrev == 0)
        Lqcd = (ui16)(Lqcd + num_subbands);
      else if (irrev == 2)
        Lqcd = (ui16)(Lqcd + 2 * num_subbands);
      else
        assert(0);

      char buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::QCC;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lqcd);
      result &= file->write(&buf, 2) == 2;
      if (num_comps < 257)
      {
        *(ui8*)buf = (ui8)comp_idx;
        result &= file->write(&buf, 1) == 1;
      }
      else
      {
        *(ui16*)buf = swap_byte(comp_idx);
        result &= file->write(&buf, 2) == 2;
      }
      *(ui8*)buf = Sqcd;
      result &= file->write(&buf, 1) == 1;
      if (irrev == 0)
        for (ui32 i = 0; i < num_subbands; ++i)
        {
          *(ui8*)buf = SPqcd.u8[i];
          result &= file->write(&buf, 1) == 1;
        }
      else if (irrev == 2)
        for (ui32 i = 0; i < num_subbands; ++i)
        {
          *(ui16*)buf = swap_byte(SPqcd.u16[i]);
          result &= file->write(&buf, 2) == 2;
        }
      else
        assert(0);

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    void param_qcd::trim_non_existing_components(ui32 num_comps)
    {
      assert(type == QCD_MAIN && comp_idx == OJPH_QCD_DEFAULT);
      param_qcd *p = this->next;
      while (p)
      {
        assert(p->type == QCC_MAIN);
        p->enabled = p->comp_idx < num_comps;
        p = p->next;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void param_qcd::read(infile_base *file)
    {
      if (file->read(&Lqcd, 2) != 2)
        OJPH_ERROR(0x00050081, "error reading QCD marker");
      Lqcd = swap_byte(Lqcd);
      if (file->read(&Sqcd, 1) != 1)
        OJPH_ERROR(0x00050082, "error reading QCD marker");
      if ((Sqcd & 0x1F) == 0)
      {
        num_subbands = (Lqcd - 3);
        if (num_subbands > 97 || Lqcd != 3 + num_subbands)
          OJPH_ERROR(0x00050083, "wrong Lqcd value of %d in QCD marker", Lqcd);
        for (ui32 i = 0; i < num_subbands; ++i)
          if (file->read(&SPqcd.u8[i], 1) != 1)
            OJPH_ERROR(0x00050084, "error reading QCD marker");
      }
      else if ((Sqcd & 0x1F) == 1)
      {
        num_subbands = 0;
        OJPH_ERROR(0x00050089,
          "Scalar derived quantization is not supported yet in QCD marker");
        if (Lqcd != 5)
          OJPH_ERROR(0x00050085, "wrong Lqcd value in QCD marker");
      }
      else if ((Sqcd & 0x1F) == 2)
      {
        num_subbands = (Lqcd - 3) / 2;
        if (num_subbands > 97 || Lqcd != 3 + 2 * num_subbands)
          OJPH_ERROR(0x00050086, "wrong Lqcd value of %d in QCD marker", Lqcd);
        for (ui32 i = 0; i < num_subbands; ++i)
        {
          if (file->read(&SPqcd.u16[i], 2) != 2)
            OJPH_ERROR(0x00050087, "error reading QCD marker");
          SPqcd.u16[i] = swap_byte(SPqcd.u16[i]);
        }
      }
      else
        OJPH_ERROR(0x00050088, "wrong Sqcd value in QCD marker");
    }

    //////////////////////////////////////////////////////////////////////////
    void param_qcd::read_qcc(infile_base *file, ui32 num_comps)
    {
      if (file->read(&Lqcd, 2) != 2)
        OJPH_ERROR(0x000500A1, "error reading QCC marker");
      Lqcd = swap_byte(Lqcd);
      if (num_comps < 257)
      {
        ui8 v;
        if (file->read(&v, 1) != 1)
          OJPH_ERROR(0x000500A2, "error reading QCC marker");
        comp_idx = v;
      }
      else
      {
        if (file->read(&comp_idx, 2) != 2)
          OJPH_ERROR(0x000500A3, "error reading QCC marker");
        comp_idx = swap_byte(comp_idx);
      }
      if (file->read(&Sqcd, 1) != 1)
        OJPH_ERROR(0x000500A4, "error reading QCC marker");
      ui32 offset = num_comps < 257 ? 4 : 5;
      if ((Sqcd & 0x1F) == 0)
      {
        num_subbands = (Lqcd - offset);
        if (num_subbands > 97 || Lqcd != offset + num_subbands)
          OJPH_ERROR(0x000500A5, "wrong Lqcd value of %d in QCC marker", Lqcd);
        for (ui32 i = 0; i < num_subbands; ++i)
          if (file->read(&SPqcd.u8[i], 1) != 1)
            OJPH_ERROR(0x000500A6, "error reading QCC marker");
      }
      else if ((Sqcd & 0x1F) == 1)
      {
        num_subbands = 0;
        OJPH_ERROR(0x000500AB,
          "Scalar derived quantization is not supported yet in QCC marker");
        if (Lqcd != offset)
          OJPH_ERROR(0x000500A7, "wrong Lqcc value in QCC marker");
      }
      else if ((Sqcd & 0x1F) == 2)
      {
        num_subbands = (Lqcd - offset) / 2;
        if (num_subbands > 97 || Lqcd != offset + 2 * num_subbands)
          OJPH_ERROR(0x000500A8, "wrong Lqcc value of %d in QCC marker", Lqcd);
        for (ui32 i = 0; i < num_subbands; ++i)
        {
          if (file->read(&SPqcd.u16[i], 2) != 2)
            OJPH_ERROR(0x000500A9, "error reading QCC marker");
          SPqcd.u16[i] = swap_byte(SPqcd.u16[i]);
        }
      }
      else
        OJPH_ERROR(0x000500AA, "wrong Sqcc value in QCC marker");
    }

    //////////////////////////////////////////////////////////////////////////
    param_qcd* param_qcd::get_qcc(ui32 comp_idx)
    {
      // cast object to constant
      const param_qcd* const_p = const_cast<const param_qcd*>(this);
      // call using the constant object, then cast to non-const
      return const_cast<param_qcd*>(const_p->get_qcc(comp_idx));
    }

    //////////////////////////////////////////////////////////////////////////
    const param_qcd* param_qcd::get_qcc(ui32 comp_idx) const
    {
      assert(this->type == QCD_MAIN || this->top_qcd->type == QCD_MAIN);
      const param_qcd *p, *q;
      if (this->type == QCD_MAIN)
        q = p = this;
      else
        q = p = this->top_qcd;
      while (p && p->comp_idx != comp_idx)
        p = p->next;
      return p ? p : q;
    }

    //////////////////////////////////////////////////////////////////////////
    param_qcd* param_qcd::add_qcc_object(ui32 comp_idx)
    {
      assert(type == QCD_MAIN);
      param_qcd *p = this;
      while (p->next != NULL)
        p = p->next;
      if (avail)
      {
        p->next = avail;
        avail = avail->next;
        p->next->init(this, (ui16)comp_idx);
      }
      else
        p->next = new param_qcd(this, (ui16)comp_idx);
      return p->next;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void param_nlt::check_validity(param_siz& siz)
    {
      if (is_any_enabled() == false)
        return;

      if (this->enabled && this->Tnlt == nonlinearity::OJPH_NLT_NO_NLT)
        this->enabled = false;

      if (this->enabled &&
          this->Tnlt == nonlinearity::OJPH_NLT_BINARY_COMPLEMENT_NLT)
      {
        bool all_same = true;
        ui32 num_comps = siz.get_num_components();

        // first stage; find out if all components captured by the default
        // entry (ALL_COMPS) has the same bit_depth/signedness,
        // while doing this, set the BDnlt for components not captured by the
        // default entry (ALL_COMPS)
        ui32 bit_depth = 0;      // unknown yet
        bool is_signed = false;  // unknown yet
        for (ui32 c = 0; c < num_comps; ++c)
        { // captured by ALL_COMPS
          param_nlt* p = get_nlt_object(c);
          if (p == NULL || !p->enabled)
          {
            if (bit_depth != 0)
            {
              // we have seen an undefined component previously
              all_same = all_same && (bit_depth == siz.get_bit_depth(c));
              all_same = all_same && (is_signed == siz.is_signed(c));
            }
            else
            {
              // this is the first component which has not type 3 nlt definition
              bit_depth = siz.get_bit_depth(c);
              is_signed = siz.is_signed(c);
            }
          }
          else
          { // can be type 0 or type 3
            p->BDnlt = (ui8)(siz.get_bit_depth(c) - 1);
            p->BDnlt = (ui8)(p->BDnlt | (siz.is_signed(c) ? 0x80 : 0));
          }
        }

        if (all_same && bit_depth != 0)
        { // all the same, and some components are captured by ALL_COMPS
          this->BDnlt = (ui8)(bit_depth - 1);
          this->BDnlt = (ui8)(this->BDnlt | (is_signed ? 0x80 : 0));
        }
        else if (!all_same)
        { // have different settings or no component is captured by ALL_COMPS
          this->enabled = false;
          for (ui32 c = 0; c < num_comps; ++c)
          {
            param_nlt* p = get_nlt_object(c);
            if (p == NULL || !p->enabled)
            { // captured by ALL_COMPS
              if (p == NULL)
                p = add_object(c);
              p->enabled = true;
              p->Tnlt = nonlinearity::OJPH_NLT_BINARY_COMPLEMENT_NLT;
              p->BDnlt = (ui8)(siz.get_bit_depth(c) - 1);
              p->BDnlt = (ui8)(p->BDnlt | (siz.is_signed(c) ? 0x80 : 0));
            }
          }
        }
      }
      else {
        // fill NLT segment markers with correct information
        ui32 num_comps = siz.get_num_components();
        for (ui32 c = 0; c < num_comps; ++c)
        { // captured by ALL_COMPS
          param_nlt* p = get_nlt_object(c);
          if (p != NULL && p->enabled)
          { // can be type 0 or type 3
            p->BDnlt = (ui8)(siz.get_bit_depth(c) - 1);
            p->BDnlt = (ui8)(p->BDnlt | (siz.is_signed(c) ? 0x80 : 0));
          }
        }
      }

      trim_non_existing_components(siz.get_num_components());

      if (is_any_enabled() == true)
        siz.set_Rsiz_flag(param_siz::RSIZ_EXT_FLAG | param_siz::RSIZ_NLT_FLAG);
    }

    //////////////////////////////////////////////////////////////////////////
    void param_nlt::set_nonlinear_transform(ui32 comp_num, ui8 nl_type)
    {
      if (nl_type != ojph::param_nlt::OJPH_NLT_NO_NLT &&
          nl_type != ojph::param_nlt::OJPH_NLT_BINARY_COMPLEMENT_NLT)
      OJPH_ERROR(0x00050171, "Nonliearities other than type 0 "
        "(No Nonlinearity) or type  3 (Binary Binary Complement to Sign "
        "Magnitude Conversion) are not supported yet");
      param_nlt* p = get_nlt_object(comp_num);
      if (p == NULL)
        p = add_object(comp_num);
      p->Tnlt = nl_type;
      p->enabled = true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool
    param_nlt::get_nonlinear_transform(ui32 comp_num, ui8& bit_depth,
                                       bool& is_signed, ui8& nl_type) const
    {
      assert(Cnlt == special_comp_num::ALL_COMPS);
      const param_nlt* p = get_nlt_object(comp_num);
      p = (p && p->enabled) ? p : this;
      if (p->enabled)
      {
        bit_depth = (ui8)((p->BDnlt & 0x7F) + 1);
        bit_depth = bit_depth <= 38 ? bit_depth : 38;
        is_signed = (p->BDnlt & 0x80) == 0x80;
        nl_type = (nonlinearity)p->Tnlt;
        return true;
      }
      return false;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_nlt::write(outfile_base* file) const
    {
      if (is_any_enabled() == false)
        return true;

      char buf[2];
      bool result = true;
      const param_nlt* p = this;
      while (p)
      {
        if (p->enabled)
        {
          *(ui16*)buf = JP2K_MARKER::NLT;
          *(ui16*)buf = swap_byte(*(ui16*)buf);
          result &= file->write(&buf, 2) == 2;
          *(ui16*)buf = swap_byte(p->Lnlt);
          result &= file->write(&buf, 2) == 2;
          *(ui16*)buf = swap_byte(p->Cnlt);
          result &= file->write(&buf, 2) == 2;
          result &= file->write(&p->BDnlt, 1) == 1;
          result &= file->write(&p->Tnlt, 1) == 1;
        }
        p = p->next;
      }
      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    void param_nlt::read(infile_base* file)
    {
      ui8 buf[6];

      if (file->read(buf, 6) != 6)
        OJPH_ERROR(0x00050141, "error reading NLT marker segment");

      ui16 length = swap_byte(*(ui16*)buf);
      if (length != 6 || (buf[5] != 3 && buf[5] != 0)) // wrong length or type
        OJPH_ERROR(0x00050142, "Unsupported NLT type %d\n", buf[5]);

      ui16 comp = swap_byte(*(ui16*)(buf + 2));
      param_nlt* p = get_nlt_object(comp);
      if (p == NULL)
        p = add_object(comp);
      p->enabled = true;
      p->Cnlt = comp;
      p->BDnlt = buf[4];
      p->Tnlt = buf[5];
    }

    //////////////////////////////////////////////////////////////////////////
    param_nlt* param_nlt::get_nlt_object(ui32 comp_num)
    {
      // cast object to constant
      const param_nlt* const_p = const_cast<const param_nlt*>(this);
      // call using the constant object, then cast to non-const
      return const_cast<param_nlt*>(const_p->get_nlt_object(comp_num));
    }

    //////////////////////////////////////////////////////////////////////////
    const param_nlt* param_nlt::get_nlt_object(ui32 comp_num) const
    {
      const param_nlt* p = this;
      while (p && p->Cnlt != comp_num)
        p = p->next;
      return p;
    }

    //////////////////////////////////////////////////////////////////////////
    param_nlt* param_nlt::add_object(ui32 comp_num)
    {
      assert(comp_num != special_comp_num::ALL_COMPS);
      assert(Cnlt == special_comp_num::ALL_COMPS);
      param_nlt* p = this;
      while (p->next != NULL) {
        assert(p->Cnlt != comp_num);
        p = p->next;
      }
      if (avail)
      {
        p->next = avail;
        avail = avail->next;
        p->next->init();
      }
      else
        p->next = new param_nlt;
      p = p->next;
      p->Cnlt = (ui16)comp_num;
      return p;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_nlt::is_any_enabled() const
    {
      // check if any field is enabled
      const param_nlt* p = this;
      while (p && p->enabled == false)
        p = p->next;
      return (p != NULL);
    }

    //////////////////////////////////////////////////////////////////////////
    void param_nlt::trim_non_existing_components(ui32 num_comps)
    {
      param_nlt* p = this->next;
      while (p) {
          if (p->enabled == true && p->Cnlt >= num_comps) {
            p->enabled = false;
            OJPH_INFO(0x00050161, "The NLT marker segment for the "
              "non-existing component %d has been removed.", p->Cnlt);
          }
        p = p->next;
      }
    }


    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    bool param_sot::write(outfile_base *file, ui32 payload_len)
    {
      char buf[4];
      bool result = true;

      this->Psot = payload_len + 14; //inc. SOT marker, field & SOD

      *(ui16*)buf = JP2K_MARKER::SOT;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lsot);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Isot);
      result &= file->write(&buf, 2) == 2;
      *(ui32*)buf = swap_byte(Psot);
      result &= file->write(&buf, 4) == 4;
      result &= file->write(&TPsot, 1) == 1;
      result &= file->write(&TNsot, 1) == 1;

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_sot::write(outfile_base *file, ui32 payload_len,
                          ui8 TPsot, ui8 TNsot)
    {
      char buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::SOT;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Lsot);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Isot);
      result &= file->write(&buf, 2) == 2;
      *(ui32*)buf = swap_byte(payload_len + 14);
      result &= file->write(&buf, 4) == 4;
      result &= file->write(&TPsot, 1) == 1;
      result &= file->write(&TNsot, 1) == 1;

      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_sot::read(infile_base *file, bool resilient)
    {
      if (resilient)
      {
        if (file->read(&Lsot, 2) != 2)
        {
          OJPH_INFO(0x00050091, "error reading SOT marker");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
        Lsot = swap_byte(Lsot);
        if (Lsot != 10)
        {
          OJPH_INFO(0x00050092, "error in SOT length");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
        if (file->read(&Isot, 2) != 2)
        {
          OJPH_INFO(0x00050093, "error reading tile index");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
        Isot = swap_byte(Isot);
        if (Isot == 0xFFFF)
        {
          OJPH_INFO(0x00050094, "tile index in SOT marker cannot be 0xFFFF");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
        if (file->read(&Psot, 4) != 4)
        {
          OJPH_INFO(0x00050095, "error reading SOT marker");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
        Psot = swap_byte(Psot);
        if (file->read(&TPsot, 1) != 1)
        {
          OJPH_INFO(0x00050096, "error reading SOT marker");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
        if (file->read(&TNsot, 1) != 1)
        {
          OJPH_INFO(0x00050097, "error reading SOT marker");
          Lsot = 0; Isot = 0; Psot = 0; TPsot = 0; TNsot = 0;
          return false;
        }
      }
      else
      {
        if (file->read(&Lsot, 2) != 2)
          OJPH_ERROR(0x00050091, "error reading SOT marker");
        Lsot = swap_byte(Lsot);
        if (Lsot != 10)
          OJPH_ERROR(0x00050092, "error in SOT length");
        if (file->read(&Isot, 2) != 2)
          OJPH_ERROR(0x00050093, "error reading SOT tile index");
        Isot = swap_byte(Isot);
        if (Isot == 0xFFFF)
          OJPH_ERROR(0x00050094, "tile index in SOT marker cannot be 0xFFFF");
        if (file->read(&Psot, 4) != 4)
          OJPH_ERROR(0x00050095, "error reading SOT marker");
        Psot = swap_byte(Psot);
        if (file->read(&TPsot, 1) != 1)
          OJPH_ERROR(0x00050096, "error reading SOT marker");
        if (file->read(&TNsot, 1) != 1)
          OJPH_ERROR(0x00050097, "error reading SOT marker");
      }
      return true;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void param_tlm::init(ui32 num_pairs, Ttlm_Ptlm_pair *store)
    {
      if (4 + 6 * num_pairs > 65535)
        OJPH_ERROR(0x000500B1, "Trying to allocate more than 65535 bytes for "
                   "a TLM marker; this can be resolved by having more than "
                   "one TLM marker, but the code does not support this. "
                   "In any case, this limit means that we have 10922 "
                   "tileparts or more, which is a huge number.");
      this->num_pairs = num_pairs;
      pairs = store;
      Ltlm = (ui16)(4 + 6 * num_pairs);
      Ztlm = 0;
      Stlm = 0x60;
    }

    //////////////////////////////////////////////////////////////////////////
    void param_tlm::set_next_pair(ui16 Ttlm, ui32 Ptlm)
    {
      assert(next_pair_index < num_pairs);
      pairs[next_pair_index].Ttlm = Ttlm;
      pairs[next_pair_index].Ptlm = Ptlm + 14;
      ++next_pair_index;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_tlm::write(outfile_base *file)
    {
      assert(next_pair_index == num_pairs);
      char buf[4];
      bool result = true;

      *(ui16*)buf = JP2K_MARKER::TLM;
      *(ui16*)buf = swap_byte(*(ui16*)buf);
      result &= file->write(&buf, 2) == 2;
      *(ui16*)buf = swap_byte(Ltlm);
      result &= file->write(&buf, 2) == 2;
      result &= file->write(&Ztlm, 1) == 1;
      result &= file->write(&Stlm, 1) == 1;
      for (ui32 i = 0; i < num_pairs; ++i)
      {
        *(ui16*)buf = swap_byte(pairs[i].Ttlm);
        result &= file->write(&buf, 2) == 2;
        *(ui32*)buf = swap_byte(pairs[i].Ptlm);
        result &= file->write(&buf, 4) == 4;
      }
      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    const param_dfs* param_dfs::get_dfs(int index) const
    {
      const param_dfs* p = this;
      while (p && p->Sdfs != index)
        p = p->next;
      return p;
    }

    //////////////////////////////////////////////////////////////////////////
    param_dfs::dfs_dwt_type param_dfs::get_dwt_type(ui32 decomp_level) const
    {
      decomp_level = ojph_min(decomp_level, Ids);
      ui32 d = decomp_level - 1;          // decomp_level starts from 1
      ui32 idx = d >> 2;                  // complete bytes
      ui32 bits = d & 0x3;                // bit within the bytes
      ui32 val = (Ddfs[idx] >> (6 - 2 * bits)) & 0x3;
      return (dfs_dwt_type)val;
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 param_dfs::get_subband_idx(ui32 num_decompositions, ui32 resolution,
                                    ui32 subband) const
    {
      assert((resolution == 0 && subband == 0) ||
              (resolution > 0 && subband > 0 && subband < 4));

      ui32 ns[4] = { 0, 3, 1, 1 };

      ui32 idx = 0;
      if (resolution > 0)
      {
        idx = 0;
        ui32 i = 1;
        for (; i < resolution; ++i)
          idx += ns[get_dwt_type(num_decompositions - i + 1)];
        dfs_dwt_type t = get_dwt_type(num_decompositions - i + 1);
        idx += subband;
        if (t == VERT_DWT && subband == 2)
          --idx;
      }

      return idx;
    }

    //////////////////////////////////////////////////////////////////////////
    point param_dfs::get_res_downsamp(ui32 skipped_resolutions) const
    {
      point factor(1, 1);
      ui32 decomp_level = 1;
      while (skipped_resolutions > 0)
      {
        param_dfs::dfs_dwt_type type = get_dwt_type(decomp_level);
        if (type == BIDIR_DWT)
        { factor.x *= 2; factor.y *= 2; }
        else if (type == HORZ_DWT)
          factor.x *= 2;
        else if (type == VERT_DWT)
          factor.y *= 2;

        ++decomp_level;
        --skipped_resolutions;
      }
      return factor;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_dfs::read(infile_base *file)
    {
      if (Ldfs != 0) { // this param_dfs is used
        param_dfs* p = this;
        while (p->next != NULL)
          p = p->next;
        if (avail)
        {
          p->next = avail;
          avail = avail->next;
          p->next->init();
        }
        else
          p->next = new param_dfs;
        p = p->next;
        return p->read(file);
      }

      if (file->read(&Ldfs, 2) != 2)
        OJPH_ERROR(0x000500D1, "error reading DFS-Ldfs parameter");
      Ldfs = swap_byte(Ldfs);
      if (file->read(&Sdfs, 2) != 2)
        OJPH_ERROR(0x000500D2, "error reading DFS-Sdfs parameter");
      Sdfs = swap_byte(Sdfs);
      if (Sdfs > 15)
        OJPH_ERROR(0x000500D3, "The DFS-Sdfs parameter is %d, which is "
          "larger than the permissible 15", Sdfs);
      ui8 t, l_Ids = 0;
      if (file->read(&l_Ids, 1) != 1)
        OJPH_ERROR(0x000500D4, "error reading DFS-Ids parameter");
      constexpr int max_Ddfs = sizeof(Ddfs) * 4;
      if (l_Ids > max_Ddfs)
        OJPH_INFO(0x000500D5, "The DFS-Ids parameter is %d; while this is "
          "valid, the number is unnessarily large -- you do not need more "
          "than %d.  Please contact me regarding this issue.",
          l_Ids, max_Ddfs);
      Ids = l_Ids < max_Ddfs ? l_Ids : max_Ddfs;
      for (int i = 0; i < Ids; i += 4)
        if (file->read(&Ddfs[i / 4], 1) != 1)
          OJPH_ERROR(0x000500D6, "error reading DFS-Ddfs parameters");
      for (int i = Ids; i < l_Ids; i += 4)
        if (file->read(&t, 1) != 1)
          OJPH_ERROR(0x000500D7, "error reading DFS-Ddfs parameters");
      return true;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    param_atk* param_atk::get_atk(int index)
    {
      assert(top_atk == NULL);

      if (Latk == 0)
      {
        // This atk object is not used, initialize it to either 0 (irv97)
        // or 1 (rev53), and use it.  If index is not 0 nor 1, then index
        // must have been read from file previously, otherwise it is an
        // error.
        if (index == 0) { this->init_irv97(); return this; }
        else if (index == 1) { this->init_rev53(); return this; }
      }

      param_atk* p = this;
      while (p && p->get_index() != index)
        p = p->next;

      if (p == NULL && (index == 0 || index == 1))
      {
        // The index was not found, add an atk object only if the index is
        // either 0 or 1
        p = add_object();
        if (index == 0)
          p->init_irv97();
        else if (index == 1)
          p->init_rev53();
      }

      return p;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_atk::read_coefficient(infile_base *file, float &K, si32& bytes)
    {
      int coeff_type = get_coeff_type();
      if (coeff_type == 0) { // 8bit
        ui8 v;
        if (file->read(&v, 1) != 1) return false;
        bytes -= 1;
        K = v;
      }
      else if (coeff_type == 1) { // 16bit
        ui16 v;
        if (file->read(&v, 2) != 2) return false;
        bytes -= 2;
        K = swap_byte(v);
      }
      else if (coeff_type == 2) { // float
        union {
          float f;
          ui32 i;
        } v;
        if (file->read(&v.i, 4) != 4) return false;
        bytes -= 4;
        v.i = swap_byte(v.i);
        K = v.f;
      }
      else if (coeff_type == 3) { // double
        union {
          double d;
          ui64 i;
        } v;
        if (file->read(&v.i, 8) != 8) return false;
        bytes -= 8;
        v.i = swap_byte(v.i);
        K = (float)v.d;
      }
      else if (coeff_type == 4) { // 128 bit float
        ui64 v, v1;
        if (file->read(&v, 8) != 8) return false;
        bytes -= 8;
        if (file->read(&v1, 8) != 8) return false; // v1 not needed
        bytes -= 8;
        v = swap_byte(v);

        union {
          float f;
          ui32 i;
        } s;
        // convert the MSB of 128b float to 32b float
        // 32b float has 1 sign bit, 8 exponent (offset 127), 23 mantissa
        // 128b float has 1 sign bit, 15 exponent (offset 16383), 112 mantissa
        si32 e = (si32)((v >> 48) & 0x7FFF);   // exponent
        e -= 16383;
        e += 127;
        e = e & 0xFF;                          // removes MSBs if negative
        e <<= 23;                              // move bits to their location
        s.i = 0;
        s.i |= ((ui32)(v >> 32) & 0x80000000); // copy sign bit
        s.i |= (ui32)e;                        // copy exponent
        s.i |= (ui32)((v >> 25) & 0x007FFFFF); // copy 23 mantissa
        K = s.f;
      }
      return true;
    }


    //////////////////////////////////////////////////////////////////////////
    bool param_atk::read_coefficient(infile_base *file, si16 &K, si32& bytes)
    {
      int coeff_type = get_coeff_type();
      if (coeff_type == 0) {
        si8 v;
        if (file->read(&v, 1) != 1) return false;
        bytes -= 1;
        K = v;
      }
      else if (coeff_type == 1) {
        si16 v;
        if (file->read(&v, 2) != 2) return false;
        bytes -= 2;
        K = (si16)swap_byte((ui16)v);
      }
      else
        return false;
      return true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool param_atk::read(infile_base *file)
    {
      if (Latk != 0) // this param_atk is used
        return add_object()->read(file);

      if (file->read(&Latk, 2) != 2)
        OJPH_ERROR(0x000500E1, "error reading ATK-Latk parameter");
      Latk = swap_byte(Latk);
      si32 bytes = Latk - 2;
      ojph::ui16 temp_Satk;
      if (file->read(&temp_Satk, 2) != 2)
        OJPH_ERROR(0x000500E2, "error reading ATK-Satk parameter");
      bytes -= 2;
      temp_Satk = swap_byte(temp_Satk);
      int tmp_idx = temp_Satk & 0xFF;
      if ((top_atk && top_atk->get_atk(tmp_idx) != NULL)
        || tmp_idx == 0 || tmp_idx == 1)
        OJPH_ERROR(0x000500F3, "ATK-Satk parameter sets ATK marker index to "
          "the illegal value of %d. ATK-Satk should be in (2-255) and, I "
          "believe, must not be repeated; otherwise, it would be unclear "
          "what marker segment must be employed when an index is repeated.",
          tmp_idx);
      Satk = temp_Satk;
      if (is_m_init0() == false)  // only even-indexed is supported
        OJPH_ERROR(0x000500E3, "ATK-Satk parameter sets m_init to 1, "
          "requiring odd-indexed subsequence in first reconstruction step, "
          "which is not supported yet.");
      if (is_whole_sample() == false)  // ARB filter not supported
        OJPH_ERROR(0x000500E4, "ATK-Satk parameter specified ARB filter, "
          "which is not supported yet.");
      if (is_reversible() && get_coeff_type() >= 2) // reversible & float
        OJPH_ERROR(0x000500E5, "ATK-Satk parameter does not make sense. "
          "It employs floats with reversible filtering.");
      if (is_using_ws_extension() == false)  // only sym. ext is supported
        OJPH_ERROR(0x000500E6, "ATK-Satk parameter requires constant "
          "boundary extension, which is not supported yet.");
      if (is_reversible() == false)
        if (read_coefficient(file, Katk, bytes) == false)
          OJPH_ERROR(0x000500E7, "error reading ATK-Katk parameter");
      if (file->read(&Natk, 1) != 1)
        OJPH_ERROR(0x000500E8, "error reading ATK-Natk parameter");
      bytes -= 1;
      if (Natk > max_steps) {
        if (d != d_store) // was this allocated -- very unlikely
          delete[] d;
        d = new lifting_step[Natk];
        max_steps = Natk;
      }

      if (is_reversible())
      {
        for (int s = 0; s < Natk; ++s)
        {
          if (file->read(&d[s].rev.Eatk, 1) != 1)
            OJPH_ERROR(0x000500E9, "error reading ATK-Eatk parameter");
          bytes -= 1;
          if (file->read(&d[s].rev.Batk, 2) != 2)
            OJPH_ERROR(0x000500EA, "error reading ATK-Batk parameter");
          bytes -= 2;
          d[s].rev.Batk = (si16)swap_byte((ui16)d[s].rev.Batk);
          ui8 LCatk;
          if (file->read(&LCatk, 1) != 1)
            OJPH_ERROR(0x000500EB, "error reading ATK-LCatk parameter");
          bytes -= 1;
          if (LCatk == 0)
            OJPH_ERROR(0x000500EC, "Encountered a ATK-LCatk value of zero; "
              "something is wrong.");
          if (LCatk > 1)
            OJPH_ERROR(0x000500ED, "ATK-LCatk value greater than 1; "
              "that is, a multitap filter is not supported");
          if (read_coefficient(file, d[s].rev.Aatk, bytes) == false)
            OJPH_ERROR(0x000500EE, "Error reding ATK-Aatk parameter");
        }
      }
      else
      {
        for (int s = 0; s < Natk; ++s)
        {
          ui8 LCatk;
          if (file->read(&LCatk, 1) != 1)
            OJPH_ERROR(0x000500EF, "error reading ATK-LCatk parameter");
          bytes -= 1;
          if (LCatk == 0)
            OJPH_ERROR(0x000500F0, "Encountered a ATK-LCatk value of zero; "
              "something is wrong.");
          if (LCatk > 1)
            OJPH_ERROR(0x000500F1, "ATK-LCatk value greater than 1; "
              "that is, a multitap filter is not supported.");
          if (read_coefficient(file, d[s].irv.Aatk, bytes) == false)
            OJPH_ERROR(0x000500F2, "Error reding ATK-Aatk parameter");
        }
      }
      if (bytes != 0)
        OJPH_ERROR(0x000500F3, "The length of an ATK marker segment "
          "(ATK-Latk) is not correct");

      return true;
    }

    //////////////////////////////////////////////////////////////////////////
    void param_atk::init_irv97()
    {
      Satk = 0x4a00;     // illegal because ATK = 0
      Katk = (float)1.230174104914001;
      Natk = 4;
      // next is (A-4) in T.801 second line
      Latk = (ui16)(5 + Natk + sizeof(float) * (1 + Natk));
      d[0].irv.Aatk = (float)0.443506852043971;
      d[1].irv.Aatk = (float)0.882911075530934;
      d[2].irv.Aatk = (float)-0.052980118572961;
      d[3].irv.Aatk = (float)-1.586134342059924;
    }

    //////////////////////////////////////////////////////////////////////////
    void param_atk::init_rev53()
    {
      Satk = 0x5801;     // illegal because ATK = 1
      Natk = 2;
      // next is (A-4) in T.801 fourth line
      Latk = (ui16)(5 + 2 * Natk + sizeof(ui8) * (Natk + Natk));
      d[0].rev.Aatk = 1;
      d[0].rev.Batk = 2;
      d[0].rev.Eatk = 2;
      d[1].rev.Aatk = -1;
      d[1].rev.Batk = 1;
      d[1].rev.Eatk = 1;
    }

    //////////////////////////////////////////////////////////////////////////
    param_atk* param_atk::add_object()
    {
      assert(top_atk = NULL);
      param_atk *p = this;
      while (p->next != NULL)
        p = p->next;
      if (avail)
      {
        p->next = avail;
        avail = avail->next;
      }
      else
        p->next = new param_atk;
      p = p->next;
      p->init(this);
      return p;
    }

  } // !local namespace
}  // !ojph namespace
