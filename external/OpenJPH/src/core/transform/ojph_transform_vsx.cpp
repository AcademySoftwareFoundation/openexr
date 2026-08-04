//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2021, Aous Naman
// Copyright (c) 2021, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2021, The University of New South Wales, Australia
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
// File: ojph_transform_vsx.cpp
// Author: Aous Naman
// Date: 09 February 2021
//***************************************************************************/

#include <cstdio>
#include "ojph_simd_vsx.h"

#include "ojph_defs.h"
#include "ojph_arch.h"
#include "ojph_mem.h"
#include "ojph_params.h"
#include "../codestream/ojph_params_local.h"

#include "ojph_transform.h"
#include "ojph_transform_local.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    static inline
    void vsx_deinterleave32(float* dpl, float* dph, float* sp, int width)
    {
      for (; width > 0; width -= 8, sp += 8, dpl += 4, dph += 4)
      {
        v128_t a = vsx_v128_load(sp);
        v128_t b = vsx_v128_load(sp + 4);
        v128_t c = vsx_i32x4_shuffle(a, b, 0, 2, 4 + 0, 4 + 2);
        v128_t d = vsx_i32x4_shuffle(a, b, 1, 3, 4 + 1, 4 + 3);
        // v128_t c = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0));
        // v128_t d = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1));
        vsx_v128_store(dpl, c);
        vsx_v128_store(dph, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void vsx_interleave32(float* dp, float* spl, float* sph, int width)
    {
      for (; width > 0; width -= 8, dp += 8, spl += 4, sph += 4)
      {
        v128_t a = vsx_v128_load(spl);
        v128_t b = vsx_v128_load(sph);
        v128_t c = vsx_i32x4_shuffle(a, b, 0, 4 + 0, 1, 4 + 1);
        v128_t d = vsx_i32x4_shuffle(a, b, 2, 4 + 2, 3, 4 + 3);
        // v128_t c = _mm_unpacklo_ps(a, b);
        // v128_t d = _mm_unpackhi_ps(a, b);
        vsx_v128_store(dp, c);
        vsx_v128_store(dp + 4, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void vsx_deinterleave64(void* dpl, void* dph, const void* sp, int width)
    {
      for (; width > 0; width -= 4,
             sp = (const char*)sp + 32,
             dpl = (char*)dpl + 16,
             dph = (char*)dph + 16)
      {
        v128_t a = vsx_v128_load(sp);
        v128_t b = vsx_v128_load((const char*)sp + 16);
        v128_t c = vsx_i64x2_shuffle(a, b, 0, 2 + 0);
        v128_t d = vsx_i64x2_shuffle(a, b, 1, 2 + 1);
        vsx_v128_store(dpl, c);
        vsx_v128_store(dph, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void vsx_interleave64(void* dp, const void* spl, const void* sph,
                          int width)
    {
      for (; width > 0; width -= 4,
             dp = (char*)dp + 32,
             spl = (const char*)spl + 16,
             sph = (const char*)sph + 16)
      {
        v128_t a = vsx_v128_load(spl);
        v128_t b = vsx_v128_load(sph);
        v128_t c = vsx_i64x2_shuffle(a, b, 0, 2 + 0);
        v128_t d = vsx_i64x2_shuffle(a, b, 1, 2 + 1);
        vsx_v128_store(dp, c);
        vsx_v128_store((char*)dp + 16, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline void vsx_multiply_const(float* p, float f, int width)
    {
      v128_t factor = vsx_f32x4_splat(f);
      for (; width > 0; width -= 4, p += 4)
      {
        v128_t s = vsx_v128_load(p);
        vsx_v128_store(p, vsx_f32x4_mul(factor, s));
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_irv_vert_step(const lifting_step* s, const line_buf* sig,
                            const line_buf* other, const line_buf* aug,
                            ui32 repeat, bool synthesis)
    {
      float a = s->irv.Aatk;
      if (synthesis)
        a = -a;

      v128_t factor = vsx_f32x4_splat(a);

      float* dst = aug->f32;
      const float* src1 = sig->f32, * src2 = other->f32;
      int i = (int)repeat;
      for ( ; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
      {
        v128_t s1 = vsx_v128_load(src1);
        v128_t s2 = vsx_v128_load(src2);
        v128_t d  = vsx_v128_load(dst);
        d = vsx_f32x4_add(d, vsx_f32x4_mul(factor, vsx_f32x4_add(s1, s2)));
        vsx_v128_store(dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat)
    {
      vsx_multiply_const(aug->f32, K, (int)repeat);
    }

    /////////////////////////////////////////////////////////////////////////
    void vsx_irv_horz_ana(const param_atk* atk, const line_buf* ldst,
                           const line_buf* hdst, const line_buf* src,
                           ui32 width, bool even)
    {
      if (width > 1)
      {
        // split src into ldst and hdst
        {
          float* dpl = even ? ldst->f32 : hdst->f32;
          float* dph = even ? hdst->f32 : ldst->f32;
          float* sp = src->f32;
          int w = (int)width;
          vsx_deinterleave32(dpl, dph, sp, w);
        }

        // the actual horizontal transform
        float* hp = hdst->f32, * lp = ldst->f32;
        ui32 l_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 h_width = (width + (even ? 0 : 1)) >> 1;  // high pass
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = num_steps; j > 0; --j)
        {
          const lifting_step* s = atk->get_step(j - 1);
          const float a = s->irv.Aatk;

          // extension
          lp[-1] = lp[0];
          lp[l_width] = lp[l_width - 1];
          // lifting step
          const float* sp = lp;
          float* dp = hp;
          int i = (int)h_width;
          v128_t f = vsx_f32x4_splat(a);
          if (even)
          {
            for (; i > 0; i -= 4, sp += 4, dp += 4)
            {
              v128_t m = vsx_v128_load(sp);
              v128_t n = vsx_v128_load(sp + 1);
              v128_t p = vsx_v128_load(dp);
              p = vsx_f32x4_add(p, vsx_f32x4_mul(f, vsx_f32x4_add(m, n)));
              vsx_v128_store(dp, p);
            }
          }
          else
          {
            for (; i > 0; i -= 4, sp += 4, dp += 4)
            {
              v128_t m = vsx_v128_load(sp);
              v128_t n = vsx_v128_load(sp - 1);
              v128_t p = vsx_v128_load(dp);
              p = vsx_f32x4_add(p, vsx_f32x4_mul(f, vsx_f32x4_add(m, n)));
              vsx_v128_store(dp, p);
            }
          }

          // swap buffers
          float* t = lp; lp = hp; hp = t;
          even = !even;
          ui32 w = l_width; l_width = h_width; h_width = w;
        }

        { // multiply by K or 1/K
          float K = atk->get_K();
          float K_inv = 1.0f / K;
          vsx_multiply_const(lp, K_inv, (int)l_width);
          vsx_multiply_const(hp, K, (int)h_width);
        }
      }
      else {
        if (even)
          ldst->f32[0] = src->f32[0];
        else
          hdst->f32[0] = src->f32[0] * 2.0f;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_irv_horz_syn(const param_atk* atk, const line_buf* dst,
                           const line_buf* lsrc, const line_buf* hsrc,
                           ui32 width, bool even)
    {
      if (width > 1)
      {
        bool ev = even;
        float* oth = hsrc->f32, * aug = lsrc->f32;
        ui32 aug_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 oth_width = (width + (even ? 0 : 1)) >> 1;  // high pass

        { // multiply by K or 1/K
          float K = atk->get_K();
          float K_inv = 1.0f / K;
          vsx_multiply_const(aug, K, (int)aug_width);
          vsx_multiply_const(oth, K_inv, (int)oth_width);
        }

        // the actual horizontal transform
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = 0; j < num_steps; ++j)
        {
          const lifting_step* s = atk->get_step(j);
          const float a = s->irv.Aatk;

          // extension
          oth[-1] = oth[0];
          oth[oth_width] = oth[oth_width - 1];
          // lifting step
          const float* sp = oth;
          float* dp = aug;
          int i = (int)aug_width;
          v128_t f = vsx_f32x4_splat(a);
          if (ev)
          {
            for ( ; i > 0; i -= 4, sp += 4, dp += 4)
            {
              v128_t m = vsx_v128_load(sp);
              v128_t n = vsx_v128_load(sp - 1);
              v128_t p = vsx_v128_load(dp);
              p = vsx_f32x4_sub(p, vsx_f32x4_mul(f, vsx_f32x4_add(m, n)));
              vsx_v128_store(dp, p);
            }
          }
          else
          {
            for ( ; i > 0; i -= 4, sp += 4, dp += 4)
            {
              v128_t m = vsx_v128_load(sp);
              v128_t n = vsx_v128_load(sp + 1);
              v128_t p = vsx_v128_load(dp);
              p = vsx_f32x4_sub(p, vsx_f32x4_mul(f, vsx_f32x4_add(m, n)));
              vsx_v128_store(dp, p);
            }
          }

          // swap buffers
          float* t = aug; aug = oth; oth = t;
          ev = !ev;
          ui32 w = aug_width; aug_width = oth_width; oth_width = w;
        }

        // combine both lsrc and hsrc into dst
        {
          float* dp = dst->f32;
          float* spl = even ? lsrc->f32 : hsrc->f32;
          float* sph = even ? hsrc->f32 : lsrc->f32;
          int w = (int)width;
          vsx_interleave32(dp, spl, sph, w);
        }
      }
      else {
        if (even)
          dst->f32[0] = lsrc->f32[0];
        else
          dst->f32[0] = hsrc->f32[0] * 0.5f;
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void vsx_rev_vert_step32(const lifting_step* s, const line_buf* sig,
                              const line_buf* other, const line_buf* aug,
                              ui32 repeat, bool synthesis)
    {
      const si32 a = s->rev.Aatk;
      const si32 b = s->rev.Batk;
      const ui8 e = s->rev.Eatk;
      v128_t va = vsx_i32x4_splat(a);
      v128_t vb = vsx_i32x4_splat(b);

      si32* dst = aug->i32;
      const si32* src1 = sig->i32, * src2 = other->i32;
      // The general definition of the wavelet in Part 2 is slightly
      // different to part 2, although they are mathematically equivalent
      // here, we identify the simpler form from Part 1 and employ them
      if (a == 1)
      { // 5/3 update and any case with a == 1
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i32x4_add(s1, s2);
            v128_t v = vsx_i32x4_add(vb, t);
            v128_t w = vsx_i32x4_shr(v, e);
            d = vsx_i32x4_sub(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
        else
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i32x4_add(s1, s2);
            v128_t v = vsx_i32x4_add(vb, t);
            v128_t w = vsx_i32x4_shr(v, e);
            d = vsx_i32x4_add(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
      }
      else if (a == -1 && b == 1 && e == 1)
      { // 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i32x4_add(s1, s2);
            v128_t w = vsx_i32x4_shr(t, e);
            d = vsx_i32x4_add(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
        else
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i32x4_add(s1, s2);
            v128_t w = vsx_i32x4_shr(t, e);
            d = vsx_i32x4_sub(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
      }
      else if (a == -1)
      { // any case with a == -1, which is not 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i32x4_add(s1, s2);
            v128_t v = vsx_i32x4_sub(vb, t);
            v128_t w = vsx_i32x4_shr(v, e);
            d = vsx_i32x4_sub(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
        else
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i32x4_add(s1, s2);
            v128_t v = vsx_i32x4_sub(vb, t);
            v128_t w = vsx_i32x4_shr(v, e);
            d = vsx_i32x4_add(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
      }
      else
      { // general case
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i32x4_add(s1, s2);
            v128_t u = vsx_i32x4_mul(va, t);
            v128_t v = vsx_i32x4_add(vb, u);
            v128_t w = vsx_i32x4_shr(v, e);
            d = vsx_i32x4_sub(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
        else
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i32x4_add(s1, s2);
            v128_t u = vsx_i32x4_mul(va, t);
            v128_t v = vsx_i32x4_add(vb, u);
            v128_t w = vsx_i32x4_shr(v, e);
            d = vsx_i32x4_add(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void vsx_rev_vert_step64(const lifting_step* s, const line_buf* sig,
                              const line_buf* other, const line_buf* aug,
                              ui32 repeat, bool synthesis)
    {
      const si32 a = s->rev.Aatk;
      const si32 b = s->rev.Batk;
      const ui8 e = s->rev.Eatk;
      v128_t va = vsx_i64x2_splat(a);
      v128_t vb = vsx_i64x2_splat(b);

      si64* dst = aug->i64;
      const si64* src1 = sig->i64, * src2 = other->i64;
      // The general definition of the wavelet in Part 2 is slightly
      // different to part 2, although they are mathematically equivalent
      // here, we identify the simpler form from Part 1 and employ them
      if (a == 1)
      { // 5/3 update and any case with a == 1
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 2, dst += 2, src1 += 2, src2 += 2)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i64x2_add(s1, s2);
            v128_t v = vsx_i64x2_add(vb, t);
            v128_t w = vsx_i64x2_shr(v, e);
            d = vsx_i64x2_sub(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
        else
          for (; i > 0; i -= 2, dst += 2, src1 += 2, src2 += 2)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i64x2_add(s1, s2);
            v128_t v = vsx_i64x2_add(vb, t);
            v128_t w = vsx_i64x2_shr(v, e);
            d = vsx_i64x2_add(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
      }
      else if (a == -1 && b == 1 && e == 1)
      { // 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 2, dst += 2, src1 += 2, src2 += 2)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i64x2_add(s1, s2);
            v128_t w = vsx_i64x2_shr(t, e);
            d = vsx_i64x2_add(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
        else
          for (; i > 0; i -= 2, dst += 2, src1 += 2, src2 += 2)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i64x2_add(s1, s2);
            v128_t w = vsx_i64x2_shr(t, e);
            d = vsx_i64x2_sub(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
      }
      else if (a == -1)
      { // any case with a == -1, which is not 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 2, dst += 2, src1 += 2, src2 += 2)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i64x2_add(s1, s2);
            v128_t v = vsx_i64x2_sub(vb, t);
            v128_t w = vsx_i64x2_shr(v, e);
            d = vsx_i64x2_sub(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
        else
          for (; i > 0; i -= 2, dst += 2, src1 += 2, src2 += 2)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i64x2_add(s1, s2);
            v128_t v = vsx_i64x2_sub(vb, t);
            v128_t w = vsx_i64x2_shr(v, e);
            d = vsx_i64x2_add(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
      }
      else
      { // general case
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 2, dst += 2, src1 += 2, src2 += 2)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i64x2_add(s1, s2);
            v128_t u = vsx_i64x2_mul(va, t);
            v128_t v = vsx_i64x2_add(vb, u);
            v128_t w = vsx_i64x2_shr(v, e);
            d = vsx_i64x2_sub(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
        else
          for (; i > 0; i -= 2, dst += 2, src1 += 2, src2 += 2)
          {
            v128_t s1 = vsx_v128_load((v128_t*)src1);
            v128_t s2 = vsx_v128_load((v128_t*)src2);
            v128_t d = vsx_v128_load((v128_t*)dst);
            v128_t t = vsx_i64x2_add(s1, s2);
            v128_t u = vsx_i64x2_mul(va, t);
            v128_t v = vsx_i64x2_add(vb, u);
            v128_t w = vsx_i64x2_shr(v, e);
            d = vsx_i64x2_add(d, w);
            vsx_v128_store((v128_t*)dst, d);
          }
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void vsx_rev_vert_step(const lifting_step* s, const line_buf* sig,
                            const line_buf* other, const line_buf* aug,
                            ui32 repeat, bool synthesis)
    {
      if (((sig != NULL) && (sig->flags & line_buf::LFT_32BIT)) ||
          ((aug != NULL) && (aug->flags & line_buf::LFT_32BIT)) ||
          ((other != NULL) && (other->flags & line_buf::LFT_32BIT)))
      {
        assert((sig == NULL || sig->flags & line_buf::LFT_32BIT) &&
               (other == NULL || other->flags & line_buf::LFT_32BIT) &&
               (aug == NULL || aug->flags & line_buf::LFT_32BIT));
        vsx_rev_vert_step32(s, sig, other, aug, repeat, synthesis);
      }
      else
      {
        assert((sig == NULL || sig->flags & line_buf::LFT_64BIT) &&
               (other == NULL || other->flags & line_buf::LFT_64BIT) &&
               (aug == NULL || aug->flags & line_buf::LFT_64BIT));
        vsx_rev_vert_step64(s, sig, other, aug, repeat, synthesis);
      }
    }

    /////////////////////////////////////////////////////////////////////////
    static
    void vsx_rev_horz_ana32(const param_atk* atk, const line_buf* ldst,
                             const line_buf* hdst, const line_buf* src,
                             ui32 width, bool even)
    {
      if (width > 1)
      {
        // combine both lsrc and hsrc into dst
        {
          float* dpl = even ? ldst->f32 : hdst->f32;
          float* dph = even ? hdst->f32 : ldst->f32;
          float* sp = src->f32;
          int w = (int)width;
          vsx_deinterleave32(dpl, dph, sp, w);
        }

        si32* hp = hdst->i32, * lp = ldst->i32;
        ui32 l_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 h_width = (width + (even ? 0 : 1)) >> 1;  // high pass
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = num_steps; j > 0; --j)
        {
          // first lifting step
          const lifting_step* s = atk->get_step(j - 1);
          const si32 a = s->rev.Aatk;
          const si32 b = s->rev.Batk;
          const ui8 e = s->rev.Eatk;
          v128_t va = vsx_i32x4_splat(a);
          v128_t vb = vsx_i32x4_splat(b);

          // extension
          lp[-1] = lp[0];
          lp[l_width] = lp[l_width - 1];
          // lifting step
          const si32* sp = lp;
          si32* dp = hp;
          if (a == 1)
          { // 5/3 update and any case with a == 1
            int i = (int)h_width;
            if (even)
            {
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t v = vsx_i32x4_add(vb, t);
                v128_t w = vsx_i32x4_shr(v, e);
                d = vsx_i32x4_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t v = vsx_i32x4_add(vb, t);
                v128_t w = vsx_i32x4_shr(v, e);
                d = vsx_i32x4_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t w = vsx_i32x4_shr(t, e);
                d = vsx_i32x4_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t w = vsx_i32x4_shr(t, e);
                d = vsx_i32x4_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t v = vsx_i32x4_sub(vb, t);
                v128_t w = vsx_i32x4_shr(v, e);
                d = vsx_i32x4_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t v = vsx_i32x4_sub(vb, t);
                v128_t w = vsx_i32x4_shr(v, e);
                d = vsx_i32x4_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
          }
          else
          { // general case
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t u = vsx_i32x4_mul(va, t);
                v128_t v = vsx_i32x4_add(vb, u);
                v128_t w = vsx_i32x4_shr(v, e);
                d = vsx_i32x4_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t u = vsx_i32x4_mul(va, t);
                v128_t v = vsx_i32x4_add(vb, u);
                v128_t w = vsx_i32x4_shr(v, e);
                d = vsx_i32x4_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
          }

          // swap buffers
          si32* t = lp; lp = hp; hp = t;
          even = !even;
          ui32 w = l_width; l_width = h_width; h_width = w;
        }
      }
      else {
        if (even)
          ldst->i32[0] = src->i32[0];
        else
          hdst->i32[0] = src->i32[0] << 1;
      }
    }

    /////////////////////////////////////////////////////////////////////////
    static
    void vsx_rev_horz_ana64(const param_atk* atk, const line_buf* ldst,
                             const line_buf* hdst, const line_buf* src,
                             ui32 width, bool even)
    {
      if (width > 1)
      {
        // combine both lsrc and hsrc into dst
        {
          void* dpl = even ? ldst->p : hdst->p;
          void* dph = even ? hdst->p : ldst->p;
          const void* sp = src->p;
          int w = (int)width;
          vsx_deinterleave64(dpl, dph, sp, w);
        }

        si64* hp = hdst->i64, * lp = ldst->i64;
        ui32 l_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 h_width = (width + (even ? 0 : 1)) >> 1;  // high pass
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = num_steps; j > 0; --j)
        {
          // first lifting step
          const lifting_step* s = atk->get_step(j - 1);
          const si32 a = s->rev.Aatk;
          const si32 b = s->rev.Batk;
          const ui8 e = s->rev.Eatk;
          v128_t va = vsx_i64x2_splat(a);
          v128_t vb = vsx_i64x2_splat(b);

          // extension
          lp[-1] = lp[0];
          lp[l_width] = lp[l_width - 1];
          // lifting step
          const si64* sp = lp;
          si64* dp = hp;
          if (a == 1)
          { // 5/3 update and any case with a == 1
            int i = (int)h_width;
            if (even)
            {
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t v = vsx_i64x2_add(vb, t);
                v128_t w = vsx_i64x2_shr(v, e);
                d = vsx_i64x2_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t v = vsx_i64x2_add(vb, t);
                v128_t w = vsx_i64x2_shr(v, e);
                d = vsx_i64x2_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t w = vsx_i64x2_shr(t, e);
                d = vsx_i64x2_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t w = vsx_i64x2_shr(t, e);
                d = vsx_i64x2_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t v = vsx_i64x2_sub(vb, t);
                v128_t w = vsx_i64x2_shr(v, e);
                d = vsx_i64x2_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t v = vsx_i64x2_sub(vb, t);
                v128_t w = vsx_i64x2_shr(v, e);
                d = vsx_i64x2_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
          }
          else
          { // general case
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t u = vsx_i64x2_mul(va, t);
                v128_t v = vsx_i64x2_add(vb, u);
                v128_t w = vsx_i64x2_shr(v, e);
                d = vsx_i64x2_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t u = vsx_i64x2_mul(va, t);
                v128_t v = vsx_i64x2_add(vb, u);
                v128_t w = vsx_i64x2_shr(v, e);
                d = vsx_i64x2_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
          }

          // swap buffers
          si64* t = lp; lp = hp; hp = t;
          even = !even;
          ui32 w = l_width; l_width = h_width; h_width = w;
        }
      }
      else {
        if (even)
          ldst->i64[0] = src->i64[0];
        else
          hdst->i64[0] = src->i64[0] << 1;
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void vsx_rev_horz_ana(const param_atk* atk, const line_buf* ldst,
                           const line_buf* hdst, const line_buf* src,
                           ui32 width, bool even)
    {
      if (src->flags & line_buf::LFT_32BIT)
      {
        assert((ldst == NULL || ldst->flags & line_buf::LFT_32BIT) &&
               (hdst == NULL || hdst->flags & line_buf::LFT_32BIT));
        vsx_rev_horz_ana32(atk, ldst, hdst, src, width, even);
      }
      else
      {
        assert((ldst == NULL || ldst->flags & line_buf::LFT_64BIT) &&
               (hdst == NULL || hdst->flags & line_buf::LFT_64BIT) &&
               (src == NULL || src->flags & line_buf::LFT_64BIT));
        vsx_rev_horz_ana64(atk, ldst, hdst, src, width, even);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_rev_horz_syn32(const param_atk* atk, const line_buf* dst,
                             const line_buf* lsrc, const line_buf* hsrc,
                             ui32 width, bool even)
    {
      if (width > 1)
      {
        bool ev = even;
        si32* oth = hsrc->i32, * aug = lsrc->i32;
        ui32 aug_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 oth_width = (width + (even ? 0 : 1)) >> 1;  // high pass
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = 0; j < num_steps; ++j)
        {
          const lifting_step* s = atk->get_step(j);
          const si32 a = s->rev.Aatk;
          const si32 b = s->rev.Batk;
          const ui8 e = s->rev.Eatk;
          v128_t va = vsx_i32x4_splat(a);
          v128_t vb = vsx_i32x4_splat(b);

          // extension
          oth[-1] = oth[0];
          oth[oth_width] = oth[oth_width - 1];
          // lifting step
          const si32* sp = oth;
          si32* dp = aug;
          if (a == 1)
          { // 5/3 update and any case with a == 1
            int i = (int)aug_width;
            if (ev)
            {
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t v = vsx_i32x4_add(vb, t);
                v128_t w = vsx_i32x4_shr(v, e);
                d = vsx_i32x4_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t v = vsx_i32x4_add(vb, t);
                v128_t w = vsx_i32x4_shr(v, e);
                d = vsx_i32x4_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t w = vsx_i32x4_shr(t, e);
                d = vsx_i32x4_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t w = vsx_i32x4_shr(t, e);
                d = vsx_i32x4_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t v = vsx_i32x4_sub(vb, t);
                v128_t w = vsx_i32x4_shr(v, e);
                d = vsx_i32x4_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t v = vsx_i32x4_sub(vb, t);
                v128_t w = vsx_i32x4_shr(v, e);
                d = vsx_i32x4_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
          }
          else
          { // general case
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t u = vsx_i32x4_mul(va, t);
                v128_t v = vsx_i32x4_add(vb, u);
                v128_t w = vsx_i32x4_shr(v, e);
                d = vsx_i32x4_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i32x4_add(s1, s2);
                v128_t u = vsx_i32x4_mul(va, t);
                v128_t v = vsx_i32x4_add(vb, u);
                v128_t w = vsx_i32x4_shr(v, e);
                d = vsx_i32x4_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
          }

          // swap buffers
          si32* t = aug; aug = oth; oth = t;
          ev = !ev;
          ui32 w = aug_width; aug_width = oth_width; oth_width = w;
        }

        // combine both lsrc and hsrc into dst
        {
          float* dp = dst->f32;
          float* spl = even ? lsrc->f32 : hsrc->f32;
          float* sph = even ? hsrc->f32 : lsrc->f32;
          int w = (int)width;
          vsx_interleave32(dp, spl, sph, w);
        }
      }
      else {
        if (even)
          dst->i32[0] = lsrc->i32[0];
        else
          dst->i32[0] = hsrc->i32[0] >> 1;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_rev_horz_syn64(const param_atk* atk, const line_buf* dst,
                             const line_buf* lsrc, const line_buf* hsrc,
                             ui32 width, bool even)
    {
      if (width > 1)
      {
        bool ev = even;
        si64* oth = hsrc->i64, * aug = lsrc->i64;
        ui32 aug_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 oth_width = (width + (even ? 0 : 1)) >> 1;  // high pass
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = 0; j < num_steps; ++j)
        {
          const lifting_step* s = atk->get_step(j);
          const si32 a = s->rev.Aatk;
          const si32 b = s->rev.Batk;
          const ui8 e = s->rev.Eatk;
          v128_t va = vsx_i64x2_splat(a);
          v128_t vb = vsx_i64x2_splat(b);

          // extension
          oth[-1] = oth[0];
          oth[oth_width] = oth[oth_width - 1];
          // lifting step
          const si64* sp = oth;
          si64* dp = aug;
          if (a == 1)
          { // 5/3 update and any case with a == 1
            int i = (int)aug_width;
            if (ev)
            {
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t v = vsx_i64x2_add(vb, t);
                v128_t w = vsx_i64x2_shr(v, e);
                d = vsx_i64x2_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t v = vsx_i64x2_add(vb, t);
                v128_t w = vsx_i64x2_shr(v, e);
                d = vsx_i64x2_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t w = vsx_i64x2_shr(t, e);
                d = vsx_i64x2_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t w = vsx_i64x2_shr(t, e);
                d = vsx_i64x2_add(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t v = vsx_i64x2_sub(vb, t);
                v128_t w = vsx_i64x2_shr(v, e);
                d = vsx_i64x2_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t v = vsx_i64x2_sub(vb, t);
                v128_t w = vsx_i64x2_shr(v, e);
                d = vsx_i64x2_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
          }
          else
          { // general case
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp - 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t u = vsx_i64x2_mul(va, t);
                v128_t v = vsx_i64x2_add(vb, u);
                v128_t w = vsx_i64x2_shr(v, e);
                d = vsx_i64x2_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
            else
              for (; i > 0; i -= 2, sp += 2, dp += 2)
              {
                v128_t s1 = vsx_v128_load((v128_t*)sp);
                v128_t s2 = vsx_v128_load((v128_t*)(sp + 1));
                v128_t d = vsx_v128_load((v128_t*)dp);
                v128_t t = vsx_i64x2_add(s1, s2);
                v128_t u = vsx_i64x2_mul(va, t);
                v128_t v = vsx_i64x2_add(vb, u);
                v128_t w = vsx_i64x2_shr(v, e);
                d = vsx_i64x2_sub(d, w);
                vsx_v128_store((v128_t*)dp, d);
              }
          }

          // swap buffers
          si64* t = aug; aug = oth; oth = t;
          ev = !ev;
          ui32 w = aug_width; aug_width = oth_width; oth_width = w;
        }

        // combine both lsrc and hsrc into dst
        {
          void* dp = dst->p;
          const void* spl = even ? lsrc->p : hsrc->p;
          const void* sph = even ? hsrc->p : lsrc->p;
          int w = (int)width;
          vsx_interleave64(dp, spl, sph, w);
        }
      }
      else {
        if (even)
          dst->i64[0] = lsrc->i64[0];
        else
          dst->i64[0] = hsrc->i64[0] >> 1;
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void vsx_rev_horz_syn(const param_atk* atk, const line_buf* dst,
                           const line_buf* lsrc, const line_buf* hsrc,
                           ui32 width, bool even)
    {
      if (dst->flags & line_buf::LFT_32BIT)
      {
        assert((lsrc == NULL || lsrc->flags & line_buf::LFT_32BIT) &&
               (hsrc == NULL || hsrc->flags & line_buf::LFT_32BIT));
        vsx_rev_horz_syn32(atk, dst, lsrc, hsrc, width, even);
      }
      else
      {
        assert((dst == NULL || dst->flags & line_buf::LFT_64BIT) &&
               (lsrc == NULL || lsrc->flags & line_buf::LFT_64BIT) &&
               (hsrc == NULL || hsrc->flags & line_buf::LFT_64BIT));
        vsx_rev_horz_syn64(atk, dst, lsrc, hsrc, width, even);
      }
    }

  } // !local
} // !ojph
