//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019-2024, Aous Naman 
// Copyright (c) 2019-2024, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2019-2024, The University of New South Wales, Australia
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
// File: ojph_transform_avx512.cpp
// Author: Aous Naman
// Date: 13 April 2024
//***************************************************************************/

#include "ojph_arch.h"
#if defined(OJPH_ARCH_X86_64)

#include <cstdio>

#include "ojph_defs.h"
#include "ojph_mem.h"
#include "ojph_params.h"
#include "../codestream/ojph_params_local.h"

#include "ojph_transform.h"
#include "ojph_transform_local.h"

#include <immintrin.h>

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    // We split multiples of 32 followed by multiples of 16, because
    // we assume byte_alignment == 64
    static 
    void avx512_deinterleave32(float* dpl, float* dph, float* sp, int width)
    {
      __m512i idx1 = _mm512_set_epi32(
        0x1E, 0x1C, 0x1A, 0x18, 0x16, 0x14, 0x12, 0x10,
        0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02, 0x00
      );
      __m512i idx2 = _mm512_set_epi32(
        0x1F, 0x1D, 0x1B, 0x19, 0x17, 0x15, 0x13, 0x11,
        0x0F, 0x0D, 0x0B, 0x09, 0x07, 0x05, 0x03, 0x01
      );
      for (; width > 16; width -= 32, sp += 32, dpl += 16, dph += 16)
      {
        __m512 a = _mm512_load_ps(sp);
        __m512 b = _mm512_load_ps(sp + 16);
        __m512 c = _mm512_permutex2var_ps(a, idx1, b);
        __m512 d = _mm512_permutex2var_ps(a, idx2, b);
        _mm512_store_ps(dpl, c);
        _mm512_store_ps(dph, d);
      }
      for (; width > 0; width -= 16, sp += 16, dpl += 8, dph += 8)
      {
        __m256 a = _mm256_load_ps(sp);
        __m256 b = _mm256_load_ps(sp + 8);
        __m256 c = _mm256_permute2f128_ps(a, b, (2 << 4) | (0));
        __m256 d = _mm256_permute2f128_ps(a, b, (3 << 4) | (1));
        __m256 e = _mm256_shuffle_ps(c, d, _MM_SHUFFLE(2, 0, 2, 0));
        __m256 f = _mm256_shuffle_ps(c, d, _MM_SHUFFLE(3, 1, 3, 1));
        _mm256_store_ps(dpl, e);
        _mm256_store_ps(dph, f);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    // We split multiples of 32 followed by multiples of 16, because
    // we assume byte_alignment == 64
    static 
    void avx512_interleave32(float* dp, float* spl, float* sph, int width)
    {
      __m512i idx1 = _mm512_set_epi32(
        0x17, 0x7, 0x16, 0x6, 0x15, 0x5, 0x14, 0x4,
        0x13, 0x3, 0x12, 0x2, 0x11, 0x1, 0x10, 0x0
      );
      __m512i idx2 = _mm512_set_epi32(
        0x1F, 0xF, 0x1E, 0xE, 0x1D, 0xD, 0x1C, 0xC,
        0x1B, 0xB, 0x1A, 0xA, 0x19, 0x9, 0x18, 0x8
      );
      for (; width > 16; width -= 32, dp += 32, spl += 16, sph += 16)
      {
        __m512 a = _mm512_load_ps(spl);
        __m512 b = _mm512_load_ps(sph);
        __m512 c = _mm512_permutex2var_ps(a, idx1, b);
        __m512 d = _mm512_permutex2var_ps(a, idx2, b);
        _mm512_store_ps(dp, c);
        _mm512_store_ps(dp + 16, d);
      }
      for (; width > 0; width -= 16, dp += 16, spl += 8, sph += 8)
      {
        __m256 a = _mm256_load_ps(spl);
        __m256 b = _mm256_load_ps(sph);
        __m256 c = _mm256_unpacklo_ps(a, b);
        __m256 d = _mm256_unpackhi_ps(a, b);
        __m256 e = _mm256_permute2f128_ps(c, d, (2 << 4) | (0));
        __m256 f = _mm256_permute2f128_ps(c, d, (3 << 4) | (1));
        _mm256_store_ps(dp, e);
        _mm256_store_ps(dp + 8, f);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    // We split multiples of 32 followed by multiples of 16, because
    // we assume byte_alignment == 64
    static void avx512_deinterleave64(double* dpl, double* dph, double* sp, 
                                      int width)
    {
      __m512i idx1 = _mm512_set_epi64(
        0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02, 0x00
      );
      __m512i idx2 = _mm512_set_epi64(
        0x0F, 0x0D, 0x0B, 0x09, 0x07, 0x05, 0x03, 0x01
      );
      for (; width > 8; width -= 16, sp += 16, dpl += 8, dph += 8)
      {
        __m512d a = _mm512_load_pd(sp);
        __m512d b = _mm512_load_pd(sp + 16);
        __m512d c = _mm512_permutex2var_pd(a, idx1, b);
        __m512d d = _mm512_permutex2var_pd(a, idx2, b);
        _mm512_store_pd(dpl, c);
        _mm512_store_pd(dph, d);
      }
      for (; width > 0; width -= 8, sp += 8, dpl += 4, dph += 4)
      {
        __m256d a = _mm256_load_pd(sp);
        __m256d b = _mm256_load_pd(sp + 4);
        __m256d c = _mm256_permute2f128_pd(a, b, (2 << 4) | (0));
        __m256d d = _mm256_permute2f128_pd(a, b, (3 << 4) | (1));
        __m256d e = _mm256_shuffle_pd(c, d, 0x0);
        __m256d f = _mm256_shuffle_pd(c, d, 0xF);
        _mm256_store_pd(dpl, e);
        _mm256_store_pd(dph, f);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    // We split multiples of 32 followed by multiples of 16, because
    // we assume byte_alignment == 64
    static void avx512_interleave64(double* dp, double* spl, double* sph, 
                                    int width)
    {
      __m512i idx1 = _mm512_set_epi64(
        0xB, 0x3, 0xA, 0x2, 0x9, 0x1, 0x8, 0x0
      );
      __m512i idx2 = _mm512_set_epi64(
        0xF, 0x7, 0xE, 0x6, 0xD, 0x5, 0xC, 0x4
      );
      for (; width > 8; width -= 16, dp += 16, spl += 8, sph += 8)
      {
        __m512d a = _mm512_load_pd(spl);
        __m512d b = _mm512_load_pd(sph);
        __m512d c = _mm512_permutex2var_pd(a, idx1, b);
        __m512d d = _mm512_permutex2var_pd(a, idx2, b);
        _mm512_store_pd(dp, c);
        _mm512_store_pd(dp + 16, d);
      }
      for (; width > 0; width -= 8, dp += 8, spl += 4, sph += 4)
      {
        __m256d a = _mm256_load_pd(spl);
        __m256d b = _mm256_load_pd(sph);
        __m256d c = _mm256_unpacklo_pd(a, b);
        __m256d d = _mm256_unpackhi_pd(a, b);
        __m256d e = _mm256_permute2f128_pd(c, d, (2 << 4) | (0));
        __m256d f = _mm256_permute2f128_pd(c, d, (3 << 4) | (1));
        _mm256_store_pd(dp, e);
        _mm256_store_pd(dp + 4, f);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline void avx512_multiply_const(float* p, float f, int width)
    {
      __m512 factor = _mm512_set1_ps(f);
      for (; width > 0; width -= 16, p += 16)
      {
        __m512 s = _mm512_load_ps(p);
        _mm512_store_ps(p, _mm512_mul_ps(factor, s));
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx512_irv_vert_step(const lifting_step* s, const line_buf* sig, 
                              const line_buf* other, const line_buf* aug, 
                              ui32 repeat, bool synthesis)
    {
      float a = s->irv.Aatk;
      if (synthesis)
        a = -a;

      __m512 factor = _mm512_set1_ps(a);

      float* dst = aug->f32;
      const float* src1 = sig->f32, * src2 = other->f32;
      int i = (int)repeat;
      for ( ; i > 0; i -= 16, dst += 16, src1 += 16, src2 += 16)
      {
        __m512 s1 = _mm512_load_ps(src1);
        __m512 s2 = _mm512_load_ps(src2);
        __m512 d = _mm512_load_ps(dst);
        d = _mm512_add_ps(d, _mm512_mul_ps(factor, _mm512_add_ps(s1, s2)));
        _mm512_store_ps(dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx512_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat)
    {
      avx512_multiply_const(aug->f32, K, (int)repeat);
    }

    /////////////////////////////////////////////////////////////////////////
    void avx512_irv_horz_ana(const param_atk* atk, const line_buf* ldst, 
                             const line_buf* hdst, const line_buf* src, 
                             ui32 width, bool even)
    {
      if (width > 1)
      {
        // split src into ldst and hdst
        {
          float* dpl = even ? ldst->f32 : hdst->f32;
          float* dph = even ? hdst->f32 : ldst->f32;
          float* sp  = src->f32;
          int w = (int)width;
          avx512_deinterleave32(dpl, dph, sp, w);
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
          __m512 f = _mm512_set1_ps(a);
          if (even)
          {
            for (; i > 0; i -= 16, sp += 16, dp += 16)
            {
              __m512 m = _mm512_load_ps(sp);
              __m512 n = _mm512_loadu_ps(sp + 1);
              __m512 p = _mm512_load_ps(dp);
              p = _mm512_add_ps(p, _mm512_mul_ps(f, _mm512_add_ps(m, n)));
              _mm512_store_ps(dp, p);
            }
          }
          else
          {
            for (; i > 0; i -= 16, sp += 16, dp += 16)
            {
              __m512 m = _mm512_load_ps(sp);
              __m512 n = _mm512_loadu_ps(sp - 1);
              __m512 p = _mm512_load_ps(dp);
              p = _mm512_add_ps(p, _mm512_mul_ps(f, _mm512_add_ps(m, n)));
              _mm512_store_ps(dp, p);
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
          avx512_multiply_const(lp, K_inv, (int)l_width);
          avx512_multiply_const(hp, K, (int)h_width);
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
    void avx512_irv_horz_syn(const param_atk* atk, const line_buf* dst, 
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
          avx512_multiply_const(aug, K, (int)aug_width);
          avx512_multiply_const(oth, K_inv, (int)oth_width);
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
          __m512 f = _mm512_set1_ps(a);
          if (ev)
          {
            for (; i > 0; i -= 16, sp += 16, dp += 16)
            {
              __m512 m = _mm512_load_ps(sp);
              __m512 n = _mm512_loadu_ps(sp - 1);
              __m512 p = _mm512_load_ps(dp);
              p = _mm512_sub_ps(p, _mm512_mul_ps(f, _mm512_add_ps(m, n)));
              _mm512_store_ps(dp, p);
            }
          }
          else
          {
            for (; i > 0; i -= 16, sp += 16, dp += 16)
            {
              __m512 m = _mm512_load_ps(sp);
              __m512 n = _mm512_loadu_ps(sp + 1);
              __m512 p = _mm512_load_ps(dp);
              p = _mm512_sub_ps(p, _mm512_mul_ps(f, _mm512_add_ps(m, n)));
              _mm512_store_ps(dp, p);
            }
          }

          // swap buffers
          float* t = aug; aug = oth; oth = t;
          ev = !ev;
          ui32 w = aug_width; aug_width = oth_width; oth_width = w;
        }

        // combine both lsrc and hsrc into dst
        {
          float* dp  = dst->f32;
          float* spl = even ? lsrc->f32 : hsrc->f32;
          float* sph = even ? hsrc->f32 : lsrc->f32;
          int w = (int)width;
          avx512_interleave32(dp, spl, sph, w);
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
    void avx512_rev_vert_step32(const lifting_step* s, const line_buf* sig, 
                                const line_buf* other, const line_buf* aug, 
                                ui32 repeat, bool synthesis)
    {
      const si32 a = s->rev.Aatk;
      const si32 b = s->rev.Batk;
      const ui8 e = s->rev.Eatk;
      __m512i va = _mm512_set1_epi32(a);
      __m512i vb = _mm512_set1_epi32(b);

      si32* dst = aug->i32;
      const si32* src1 = sig->i32, * src2 = other->i32;
      // The general definition of the wavelet in Part 2 is slightly 
      // different to part 2, although they are mathematically equivalent
      // here, we identify the simpler form from Part 1 and employ them
      if (a == 1)
      { // 5/3 update and any case with a == 1
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 16, dst += 16, src1 += 16, src2 += 16)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi32(s1, s2);
            __m512i v = _mm512_add_epi32(vb, t);
            __m512i w = _mm512_srai_epi32(v, e);
            d = _mm512_sub_epi32(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
        else
          for (; i > 0; i -= 16, dst += 16, src1 += 16, src2 += 16)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi32(s1, s2);
            __m512i v = _mm512_add_epi32(vb, t);
            __m512i w = _mm512_srai_epi32(v, e);
            d = _mm512_add_epi32(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
      }
      else if (a == -1 && b == 1 && e == 1)
      { // 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 16, dst += 16, src1 += 16, src2 += 16)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi32(s1, s2);
            __m512i w = _mm512_srai_epi32(t, e);
            d = _mm512_add_epi32(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
        else
          for (; i > 0; i -= 16, dst += 16, src1 += 16, src2 += 16)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi32(s1, s2);
            __m512i w = _mm512_srai_epi32(t, e);
            d = _mm512_sub_epi32(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
      }
      else if (a == -1)
      { // any case with a == -1, which is not 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 16, dst += 16, src1 += 16, src2 += 16)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi32(s1, s2);
            __m512i v = _mm512_sub_epi32(vb, t);
            __m512i w = _mm512_srai_epi32(v, e);
            d = _mm512_sub_epi32(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
        else
          for (; i > 0; i -= 16, dst += 16, src1 += 16, src2 += 16)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi32(s1, s2);
            __m512i v = _mm512_sub_epi32(vb, t);
            __m512i w = _mm512_srai_epi32(v, e);
            d = _mm512_add_epi32(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
      }
      else { // general case
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 16, dst += 16, src1 += 16, src2 += 16)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi32(s1, s2);
            __m512i u = _mm512_mullo_epi32(va, t);
            __m512i v = _mm512_add_epi32(vb, u);
            __m512i w = _mm512_srai_epi32(v, e);
            d = _mm512_sub_epi32(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
        else
          for (; i > 0; i -= 16, dst += 16, src1 += 16, src2 += 16)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi32(s1, s2);
            __m512i u = _mm512_mullo_epi32(va, t);
            __m512i v = _mm512_add_epi32(vb, u);
            __m512i w = _mm512_srai_epi32(v, e);
            d = _mm512_add_epi32(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void avx512_rev_vert_step64(const lifting_step* s, const line_buf* sig, 
                                const line_buf* other, const line_buf* aug, 
                                ui32 repeat, bool synthesis)
    {
      const si32 a = s->rev.Aatk;
      const si32 b = s->rev.Batk;
      const ui8 e = s->rev.Eatk;
      __m512i vb = _mm512_set1_epi64(b);

      si64* dst = aug->i64;
      const si64* src1 = sig->i64, * src2 = other->i64;
      // The general definition of the wavelet in Part 2 is slightly 
      // different to part 2, although they are mathematically equivalent
      // here, we identify the simpler form from Part 1 and employ them
      if (a == 1)
      { // 5/3 update and any case with a == 1
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi64(s1, s2);
            __m512i v = _mm512_add_epi64(vb, t);
            __m512i w = _mm512_srai_epi64(v, e);
            d = _mm512_sub_epi64(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
        else
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi64(s1, s2);
            __m512i v = _mm512_add_epi64(vb, t);
            __m512i w = _mm512_srai_epi64(v, e);
            d = _mm512_add_epi64(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
      }
      else if (a == -1 && b == 1 && e == 1)
      { // 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi64(s1, s2);
            __m512i w = _mm512_srai_epi64(t, e);
            d = _mm512_add_epi64(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
        else
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi64(s1, s2);
            __m512i w = _mm512_srai_epi64(t, e);
            d = _mm512_sub_epi64(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
      }
      else if (a == -1)
      { // any case with a == -1, which is not 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi64(s1, s2);
            __m512i v = _mm512_sub_epi64(vb, t);
            __m512i w = _mm512_srai_epi64(v, e);
            d = _mm512_sub_epi64(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
        else
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m512i s1 = _mm512_load_si512((__m512i*)src1);
            __m512i s2 = _mm512_load_si512((__m512i*)src2);
            __m512i d = _mm512_load_si512((__m512i*)dst);
            __m512i t = _mm512_add_epi64(s1, s2);
            __m512i v = _mm512_sub_epi64(vb, t);
            __m512i w = _mm512_srai_epi64(v, e);
            d = _mm512_add_epi64(d, w);
            _mm512_store_si512((__m512i*)dst, d);
          }
      }
      else { 
        // general case
        // 64bit multiplication is not supported in AVX512F + AVX512CD;
        // in particular, _mm256_mullo_epi64.
        if (synthesis)
          for (ui32 i = repeat; i > 0; --i)
            *dst++ -= (b + a * (*src1++ + *src2++)) >> e;
        else
          for (ui32 i = repeat; i > 0; --i)
            *dst++ += (b + a * (*src1++ + *src2++)) >> e;
      }

      // This can only be used if you have AVX512DQ
      // { // general case
      //   __m512i va = _mm512_set1_epi64(a);
      //   int i = (int)repeat;
      //   if (synthesis)
      //     for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
      //     {
      //       __m512i s1 = _mm512_load_si512((__m512i*)src1);
      //       __m512i s2 = _mm512_load_si512((__m512i*)src2);
      //       __m512i d = _mm512_load_si512((__m512i*)dst);
      //       __m512i t = _mm512_add_epi64(s1, s2);
      //       __m512i u = _mm512_mullo_epi64(va, t);
      //       __m512i v = _mm512_add_epi64(vb, u);
      //       __m512i w = _mm512_srai_epi64(v, e);
      //       d = _mm512_sub_epi64(d, w);
      //       _mm512_store_si512((__m512i*)dst, d);
      //     }
      //   else
      //     for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
      //     {
      //       __m512i s1 = _mm512_load_si512((__m512i*)src1);
      //       __m512i s2 = _mm512_load_si512((__m512i*)src2);
      //       __m512i d = _mm512_load_si512((__m512i*)dst);
      //       __m512i t = _mm512_add_epi64(s1, s2);
      //       __m512i u = _mm512_mullo_epi64(va, t);
      //       __m512i v = _mm512_add_epi64(vb, u);
      //       __m512i w = _mm512_srai_epi64(v, e);
      //       d = _mm512_add_epi64(d, w);
      //       _mm512_store_si512((__m512i*)dst, d);
      //     }
      // }
    }

    /////////////////////////////////////////////////////////////////////////
    void avx512_rev_vert_step(const lifting_step* s, const line_buf* sig, 
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
        avx512_rev_vert_step32(s, sig, other, aug, repeat, synthesis);
      }
      else 
      {
        assert((sig == NULL || sig->flags & line_buf::LFT_64BIT) &&
               (other == NULL || other->flags & line_buf::LFT_64BIT) && 
               (aug == NULL || aug->flags & line_buf::LFT_64BIT));
        avx512_rev_vert_step64(s, sig, other, aug, repeat, synthesis);
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void avx512_rev_horz_ana32(const param_atk* atk, const line_buf* ldst, 
                               const line_buf* hdst, const line_buf* src, 
                               ui32 width, bool even)
    {
      if (width > 1)
      {
        // split src into ldst and hdst
        {
          float* dpl = even ? ldst->f32 : hdst->f32;
          float* dph = even ? hdst->f32 : ldst->f32;
          float* sp  = src->f32;
          int w = (int)width;
          avx512_deinterleave32(dpl, dph, sp, w);
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
          __m512i va = _mm512_set1_epi32(a);
          __m512i vb = _mm512_set1_epi32(b);

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
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i v = _mm512_add_epi32(vb, t);
                __m512i w = _mm512_srai_epi32(v, e);
                d = _mm512_add_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i v = _mm512_add_epi32(vb, t);
                __m512i w = _mm512_srai_epi32(v, e);
                d = _mm512_add_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i w = _mm512_srai_epi32(t, e);
                d = _mm512_sub_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            else
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i w = _mm512_srai_epi32(t, e);
                d = _mm512_sub_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i v = _mm512_sub_epi32(vb, t);
                __m512i w = _mm512_srai_epi32(v, e);
                d = _mm512_add_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            else
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i v = _mm512_sub_epi32(vb, t);
                __m512i w = _mm512_srai_epi32(v, e);
                d = _mm512_add_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
          }
          else {
            // general case
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i u = _mm512_mullo_epi32(va, t);
                __m512i v = _mm512_add_epi32(vb, u);
                __m512i w = _mm512_srai_epi32(v, e);
                d = _mm512_add_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            else
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i u = _mm512_mullo_epi32(va, t);
                __m512i v = _mm512_add_epi32(vb, u);
                __m512i w = _mm512_srai_epi32(v, e);
                d = _mm512_add_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
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
    void avx512_rev_horz_ana64(const param_atk* atk, const line_buf* ldst, 
                               const line_buf* hdst, const line_buf* src, 
                               ui32 width, bool even)
    {
      if (width > 1)
      {
        // split src into ldst and hdst
        {
          double* dpl = (double*)(even ? ldst->p : hdst->p);
          double* dph = (double*)(even ? hdst->p : ldst->p);
          double* sp  = (double*)(src->p);
          int w = (int)width;
          avx512_deinterleave64(dpl, dph, sp, w);
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
          __m512i vb = _mm512_set1_epi64(b);

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
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi64(s1, s2);
                __m512i v = _mm512_add_epi64(vb, t);
                __m512i w = _mm512_srai_epi64(v, e);
                d = _mm512_add_epi64(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi64(s1, s2);
                __m512i v = _mm512_add_epi64(vb, t);
                __m512i w = _mm512_srai_epi64(v, e);
                d = _mm512_add_epi64(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi64(s1, s2);
                __m512i w = _mm512_srai_epi64(t, e);
                d = _mm512_sub_epi64(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi64(s1, s2);
                __m512i w = _mm512_srai_epi64(t, e);
                d = _mm512_sub_epi64(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi64(s1, s2);
                __m512i v = _mm512_sub_epi64(vb, t);
                __m512i w = _mm512_srai_epi64(v, e);
                d = _mm512_add_epi64(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi64(s1, s2);
                __m512i v = _mm512_sub_epi64(vb, t);
                __m512i w = _mm512_srai_epi64(v, e);
                d = _mm512_add_epi64(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
          }
          else 
          {
            // general case
            // 64bit multiplication is not supported in AVX512F + AVX512CD;
            // in particular, _mm256_mullo_epi64.
            if (even)
              for (ui32 i = h_width; i > 0; --i, sp++, dp++)
                *dp += (b + a * (sp[0] + sp[1])) >> e;
            else
              for (ui32 i = h_width; i > 0; --i, sp++, dp++)
                *dp += (b + a * (sp[-1] + sp[0])) >> e;
          }

          // This can only be used if you have AVX512DQ
          // {
          //   // general case
          //   __m512i va = _mm512_set1_epi64(a);
          //   int i = (int)h_width;
          //   if (even)
          //     for (; i > 0; i -= 8, sp += 8, dp += 8)
          //     {
          //       __m512i s1 = _mm512_load_si512((__m512i*)sp);
          //       __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
          //       __m512i d = _mm512_load_si512((__m512i*)dp);
          //       __m512i t = _mm512_add_epi64(s1, s2);
          //       __m512i u = _mm512_mullo_epi64(va, t);
          //       __m512i v = _mm512_add_epi64(vb, u);
          //       __m512i w = _mm512_srai_epi64(v, e);
          //       d = _mm512_add_epi64(d, w);
          //       _mm512_store_si512((__m512i*)dp, d);
          //     }
          //   else
          //     for (; i > 0; i -= 8, sp += 8, dp += 8)
          //     {
          //       __m512i s1 = _mm512_load_si512((__m512i*)sp);
          //       __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
          //       __m512i d = _mm512_load_si512((__m512i*)dp);
          //       __m512i t = _mm512_add_epi64(s1, s2);
          //       __m512i u = _mm512_mullo_epi64(va, t);
          //       __m512i v = _mm512_add_epi64(vb, u);
          //       __m512i w = _mm512_srai_epi64(v, e);
          //       d = _mm512_add_epi64(d, w);
          //       _mm512_store_si512((__m512i*)dp, d);
          //     }
          // }

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
    void avx512_rev_horz_ana(const param_atk* atk, const line_buf* ldst, 
                             const line_buf* hdst, const line_buf* src, 
                             ui32 width, bool even)
    {
      if (src->flags & line_buf::LFT_32BIT) 
      {
        assert((ldst == NULL || ldst->flags & line_buf::LFT_32BIT) &&
               (hdst == NULL || hdst->flags & line_buf::LFT_32BIT));
        avx512_rev_horz_ana32(atk, ldst, hdst, src, width, even);
      }
      else 
      {
        assert((ldst == NULL || ldst->flags & line_buf::LFT_64BIT) &&
               (hdst == NULL || hdst->flags & line_buf::LFT_64BIT) && 
               (src == NULL || src->flags & line_buf::LFT_64BIT));
        avx512_rev_horz_ana64(atk, ldst, hdst, src, width, even);
      }
    } 

    //////////////////////////////////////////////////////////////////////////
    void avx512_rev_horz_syn32(const param_atk* atk, const line_buf* dst, 
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
          __m512i va = _mm512_set1_epi32(a);
          __m512i vb = _mm512_set1_epi32(b);

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
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i v = _mm512_add_epi32(vb, t);
                __m512i w = _mm512_srai_epi32(v, e);
                d = _mm512_sub_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i v = _mm512_add_epi32(vb, t);
                __m512i w = _mm512_srai_epi32(v, e);
                d = _mm512_sub_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i w = _mm512_srai_epi32(t, e);
                d = _mm512_add_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            else
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i w = _mm512_srai_epi32(t, e);
                d = _mm512_add_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i v = _mm512_sub_epi32(vb, t);
                __m512i w = _mm512_srai_epi32(v, e);
                d = _mm512_sub_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            else
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i v = _mm512_sub_epi32(vb, t);
                __m512i w = _mm512_srai_epi32(v, e);
                d = _mm512_sub_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
          }
          else {
            // general case
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i u = _mm512_mullo_epi32(va, t);
                __m512i v = _mm512_add_epi32(vb, u);
                __m512i w = _mm512_srai_epi32(v, e);
                d = _mm512_sub_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            else
              for (; i > 0; i -= 16, sp += 16, dp += 16)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi32(s1, s2);
                __m512i u = _mm512_mullo_epi32(va, t);
                __m512i v = _mm512_add_epi32(vb, u);
                __m512i w = _mm512_srai_epi32(v, e);
                d = _mm512_sub_epi32(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
          }

          // swap buffers
          si32* t = aug; aug = oth; oth = t;
          ev = !ev;
          ui32 w = aug_width; aug_width = oth_width; oth_width = w;
        }

        // combine both lsrc and hsrc into dst
        {
          float* dp  = dst->f32;
          float* spl = even ? lsrc->f32 : hsrc->f32;
          float* sph = even ? hsrc->f32 : lsrc->f32;
          int w = (int)width;
          avx512_interleave32(dp, spl, sph, w);
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
    void avx512_rev_horz_syn64(const param_atk* atk, const line_buf* dst, 
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
          __m512i vb = _mm512_set1_epi64(b);

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
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi64(s1, s2);
                __m512i v = _mm512_add_epi64(vb, t);
                __m512i w = _mm512_srai_epi64(v, e);
                d = _mm512_sub_epi64(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi64(s1, s2);
                __m512i v = _mm512_add_epi64(vb, t);
                __m512i w = _mm512_srai_epi64(v, e);
                d = _mm512_sub_epi64(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi64(s1, s2);
                __m512i w = _mm512_srai_epi64(t, e);
                d = _mm512_add_epi64(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi64(s1, s2);
                __m512i w = _mm512_srai_epi64(t, e);
                d = _mm512_add_epi64(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi64(s1, s2);
                __m512i v = _mm512_sub_epi64(vb, t);
                __m512i w = _mm512_srai_epi64(v, e);
                d = _mm512_sub_epi64(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m512i s1 = _mm512_load_si512((__m512i*)sp);
                __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
                __m512i d = _mm512_load_si512((__m512i*)dp);
                __m512i t = _mm512_add_epi64(s1, s2);
                __m512i v = _mm512_sub_epi64(vb, t);
                __m512i w = _mm512_srai_epi64(v, e);
                d = _mm512_sub_epi64(d, w);
                _mm512_store_si512((__m512i*)dp, d);
              }
          }
          else 
           {
            // general case
            // 64bit multiplication is not supported in AVX512F + AVX512CD;
            // in particular, _mm256_mullo_epi64.            
            if (ev)
              for (ui32 i = aug_width; i > 0; --i, sp++, dp++)
                *dp -= (b + a * (sp[-1] + sp[0])) >> e;
            else
              for (ui32 i = aug_width; i > 0; --i, sp++, dp++)
                *dp -= (b + a * (sp[0] + sp[1])) >> e;
          }

          // This can only be used if you have AVX512DQ
          // {
          //   // general case
          //   __m512i va = _mm512_set1_epi64(a);
          //   int i = (int)aug_width;
          //   if (ev)
          //     for (; i > 0; i -= 8, sp += 8, dp += 8)
          //     {
          //       __m512i s1 = _mm512_load_si512((__m512i*)sp);
          //       __m512i s2 = _mm512_loadu_si512((__m512i*)(sp - 1));
          //       __m512i d = _mm512_load_si512((__m512i*)dp);
          //       __m512i t = _mm512_add_epi64(s1, s2);
          //       __m512i u = _mm512_mullo_epi64(va, t);
          //       __m512i v = _mm512_add_epi64(vb, u);
          //       __m512i w = _mm512_srai_epi64(v, e);
          //       d = _mm512_sub_epi64(d, w);
          //       _mm512_store_si512((__m512i*)dp, d);
          //     }
          //   else
          //     for (; i > 0; i -= 8, sp += 8, dp += 8)
          //     {
          //       __m512i s1 = _mm512_load_si512((__m512i*)sp);
          //       __m512i s2 = _mm512_loadu_si512((__m512i*)(sp + 1));
          //       __m512i d = _mm512_load_si512((__m512i*)dp);
          //       __m512i t = _mm512_add_epi64(s1, s2);
          //       __m512i u = _mm512_mullo_epi64(va, t);
          //       __m512i v = _mm512_add_epi64(vb, u);
          //       __m512i w = _mm512_srai_epi64(v, e);
          //       d = _mm512_sub_epi64(d, w);
          //       _mm512_store_si512((__m512i*)dp, d);
          //     }
          // }

          // swap buffers
          si64* t = aug; aug = oth; oth = t;
          ev = !ev;
          ui32 w = aug_width; aug_width = oth_width; oth_width = w;
        }

        // combine both lsrc and hsrc into dst
        {
          double* dp  = (double*)(dst->p);
          double* spl = (double*)(even ? lsrc->p : hsrc->p);
          double* sph = (double*)(even ? hsrc->p : lsrc->p);
          int w = (int)width;
          avx512_interleave64(dp, spl, sph, w);
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
    void avx512_rev_horz_syn(const param_atk* atk, const line_buf* dst, 
                             const line_buf* lsrc, const line_buf* hsrc, 
                             ui32 width, bool even)
    {
      if (dst->flags & line_buf::LFT_32BIT) 
      {
        assert((lsrc == NULL || lsrc->flags & line_buf::LFT_32BIT) && 
               (hsrc == NULL || hsrc->flags & line_buf::LFT_32BIT));
        avx512_rev_horz_syn32(atk, dst, lsrc, hsrc, width, even);
      }
      else 
      {
        assert((dst == NULL || dst->flags & line_buf::LFT_64BIT) &&
               (lsrc == NULL || lsrc->flags & line_buf::LFT_64BIT) && 
               (hsrc == NULL || hsrc->flags & line_buf::LFT_64BIT));
        avx512_rev_horz_syn64(atk, dst, lsrc, hsrc, width, even);
      }
    }

  } // !local
} // !ojph

#endif
