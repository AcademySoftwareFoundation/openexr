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
// File: ojph_transform_avx2.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/

#include "ojph_arch.h"
#if defined(OJPH_ARCH_I386) || defined(OJPH_ARCH_X86_64)

#include <climits>
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

    /////////////////////////////////////////////////////////////////////////
    // https://github.com/seung-lab/dijkstra3d/blob/master/libdivide.h
    static inline 
    __m256i avx2_mm256_srai_epi64(__m256i a, int amt, __m256i m) 
    {
      // note than m must be obtained using
      // __m256i m = _mm256_set1_epi64x(1ULL << (63 - amt));
      __m256i x = _mm256_srli_epi64(a, amt);
      x = _mm256_xor_si256(x, m);
      __m256i result = _mm256_sub_epi64(x, m);
      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void avx2_deinterleave32(float* dpl, float* dph, float* sp, int width)
    {
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
    static inline 
    void avx2_interleave32(float* dp, float* spl, float* sph, int width)
    {
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
    static inline 
    void avx2_deinterleave64(double* dpl, double* dph, double* sp, int width)
    {
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
    static inline 
    void avx2_interleave64(double* dp, double* spl, double* sph, int width)
    {
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

    /////////////////////////////////////////////////////////////////////////
    static
    void avx2_rev_vert_step32(const lifting_step* s, const line_buf* sig, 
                              const line_buf* other, const line_buf* aug, 
                              ui32 repeat, bool synthesis)
    {
      const si32 a = s->rev.Aatk;
      const si32 b = s->rev.Batk;
      const ui8 e = s->rev.Eatk;
      __m256i va = _mm256_set1_epi32(a);
      __m256i vb = _mm256_set1_epi32(b);

      si32* dst = aug->i32;
      const si32* src1 = sig->i32, * src2 = other->i32;
      // The general definition of the wavelet in Part 2 is slightly 
      // different to part 2, although they are mathematically equivalent
      // here, we identify the simpler form from Part 1 and employ them
      if (a == 1)
      { // 5/3 update and any case with a == 1
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i v = _mm256_add_epi32(vb, t);
            __m256i w = _mm256_srai_epi32(v, e);
            d = _mm256_sub_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
        else
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i v = _mm256_add_epi32(vb, t);
            __m256i w = _mm256_srai_epi32(v, e);
            d = _mm256_add_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
      }
      else if (a == -1 && b == 1 && e == 1)
      { // 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i w = _mm256_srai_epi32(t, e);
            d = _mm256_add_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
        else
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i w = _mm256_srai_epi32(t, e);
            d = _mm256_sub_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
      }
      else if (a == -1)
      { // any case with a == -1, which is not 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i v = _mm256_sub_epi32(vb, t);
            __m256i w = _mm256_srai_epi32(v, e);
            d = _mm256_sub_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
        else
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i v = _mm256_sub_epi32(vb, t);
            __m256i w = _mm256_srai_epi32(v, e);
            d = _mm256_add_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
      }
      else { // general case
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i u = _mm256_mullo_epi32(va, t);
            __m256i v = _mm256_add_epi32(vb, u);
            __m256i w = _mm256_srai_epi32(v, e);
            d = _mm256_sub_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
        else
          for (; i > 0; i -= 8, dst += 8, src1 += 8, src2 += 8)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi32(s1, s2);
            __m256i u = _mm256_mullo_epi32(va, t);
            __m256i v = _mm256_add_epi32(vb, u);
            __m256i w = _mm256_srai_epi32(v, e);
            d = _mm256_add_epi32(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
      }
    }

    /////////////////////////////////////////////////////////////////////////
    static
    void avx2_rev_vert_step64(const lifting_step* s, const line_buf* sig, 
                              const line_buf* other, const line_buf* aug, 
                              ui32 repeat, bool synthesis)
    {
      const si32 a = s->rev.Aatk;
      const si32 b = s->rev.Batk;
      const ui8 e = s->rev.Eatk;
      __m256i vb = _mm256_set1_epi64x(b);
      __m256i ve = _mm256_set1_epi64x(1LL << (63 - e));      

      si64* dst = aug->i64;
      const si64* src1 = sig->i64, * src2 = other->i64;
      // The general definition of the wavelet in Part 2 is slightly 
      // different to part 2, although they are mathematically equivalent
      // here, we identify the simpler form from Part 1 and employ them
      if (a == 1)
      { // 5/3 update and any case with a == 1
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi64(s1, s2);
            __m256i v = _mm256_add_epi64(vb, t);
            __m256i w = avx2_mm256_srai_epi64(v, e, ve);
            d = _mm256_sub_epi64(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
        else
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi64(s1, s2);
            __m256i v = _mm256_add_epi64(vb, t);
            __m256i w = avx2_mm256_srai_epi64(v, e, ve);
            d = _mm256_add_epi64(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
      }
      else if (a == -1 && b == 1 && e == 1)
      { // 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi64(s1, s2);
            __m256i w = avx2_mm256_srai_epi64(t, e, ve);
            d = _mm256_add_epi64(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
        else
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi64(s1, s2);
            __m256i w = avx2_mm256_srai_epi64(t, e, ve);
            d = _mm256_sub_epi64(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
      }
      else if (a == -1)
      { // any case with a == -1, which is not 5/3 predict
        int i = (int)repeat;
        if (synthesis)
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi64(s1, s2);
            __m256i v = _mm256_sub_epi64(vb, t);
            __m256i w = avx2_mm256_srai_epi64(v, e, ve);
            d = _mm256_sub_epi64(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
        else
          for (; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
          {
            __m256i s1 = _mm256_load_si256((__m256i*)src1);
            __m256i s2 = _mm256_load_si256((__m256i*)src2);
            __m256i d = _mm256_load_si256((__m256i*)dst);
            __m256i t = _mm256_add_epi64(s1, s2);
            __m256i v = _mm256_sub_epi64(vb, t);
            __m256i w = avx2_mm256_srai_epi64(v, e, ve);
            d = _mm256_add_epi64(d, w);
            _mm256_store_si256((__m256i*)dst, d);
          }
      }
      else { // general case
        // 64bit multiplication is not supported in avx2;
        // in particular, _mm256_mullo_epi64.
        if (synthesis)
          for (ui32 i = repeat; i > 0; --i)
            *dst++ -= (b + a * (*src1++ + *src2++)) >> e;
        else
          for (ui32 i = repeat; i > 0; --i)
            *dst++ += (b + a * (*src1++ + *src2++)) >> e;
      }
    }

    /////////////////////////////////////////////////////////////////////////
    void avx2_rev_vert_step(const lifting_step* s, const line_buf* sig, 
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
        avx2_rev_vert_step32(s, sig, other, aug, repeat, synthesis);
      }
      else 
      {
        assert((sig == NULL || sig->flags & line_buf::LFT_64BIT) &&
               (other == NULL || other->flags & line_buf::LFT_64BIT) && 
               (aug == NULL || aug->flags & line_buf::LFT_64BIT));
        avx2_rev_vert_step64(s, sig, other, aug, repeat, synthesis);
      }
    }

    /////////////////////////////////////////////////////////////////////////
    static
    void avx2_rev_horz_ana32(const param_atk* atk, const line_buf* ldst, 
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
          avx2_deinterleave32(dpl, dph, sp, w);
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
          const ui8 e  = s->rev.Eatk;
          __m256i va = _mm256_set1_epi32(a);
          __m256i vb = _mm256_set1_epi32(b);

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
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_add_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_add_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i w = _mm256_srai_epi32(t, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i w = _mm256_srai_epi32(t, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_sub_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_sub_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }
          else {
            // general case
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i u = _mm256_mullo_epi32(va, t);
                __m256i v = _mm256_add_epi32(vb, u);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i u = _mm256_mullo_epi32(va, t);
                __m256i v = _mm256_add_epi32(vb, u);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
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
    void avx2_rev_horz_ana64(const param_atk* atk, const line_buf* ldst, 
                             const line_buf* hdst, const line_buf* src, 
                             ui32 width, bool even)
    {
      if (width > 1)
      {
        // split src into ldst and hdst
        {
          double* dpl = (double*)(even ? ldst->p : hdst->p);
          double* dph = (double*)(even ? hdst->p : ldst->p);
          double* sp  = (double*)src->p;
          int w = (int)width;
          avx2_deinterleave64(dpl, dph, sp, w);
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
          const ui8 e  = s->rev.Eatk;
          __m256i vb = _mm256_set1_epi64x(b);
          __m256i ve = _mm256_set1_epi64x(1LL << (63 - e));

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
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi64(s1, s2);
                __m256i v = _mm256_add_epi64(vb, t);
                __m256i w = avx2_mm256_srai_epi64(v, e, ve);
                d = _mm256_add_epi64(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi64(s1, s2);
                __m256i v = _mm256_add_epi64(vb, t);
                __m256i w = avx2_mm256_srai_epi64(v, e, ve);
                d = _mm256_add_epi64(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi64(s1, s2);
                __m256i w = avx2_mm256_srai_epi64(t, e, ve);
                d = _mm256_sub_epi64(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi64(s1, s2);
                __m256i w = avx2_mm256_srai_epi64(t, e, ve);
                d = _mm256_sub_epi64(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)h_width;
            if (even)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi64(s1, s2);
                __m256i v = _mm256_sub_epi64(vb, t);
                __m256i w = avx2_mm256_srai_epi64(v, e, ve);
                d = _mm256_add_epi64(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi64(s1, s2);
                __m256i v = _mm256_sub_epi64(vb, t);
                __m256i w = avx2_mm256_srai_epi64(v, e, ve);
                d = _mm256_add_epi64(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }
          else {
            // general case
            // 64bit multiplication is not supported in avx2;
            // in particular, _mm256_mullo_epi64.
            if (even)
              for (ui32 i = h_width; i > 0; --i, sp++, dp++)
                *dp += (b + a * (sp[0] + sp[1])) >> e;
            else
              for (ui32 i = h_width; i > 0; --i, sp++, dp++)
                *dp += (b + a * (sp[-1] + sp[0])) >> e;
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
    void avx2_rev_horz_ana(const param_atk* atk, const line_buf* ldst, 
                           const line_buf* hdst, const line_buf* src, 
                           ui32 width, bool even)
    {
      if (src->flags & line_buf::LFT_32BIT) 
      {
        assert((ldst == NULL || ldst->flags & line_buf::LFT_32BIT) &&
               (hdst == NULL || hdst->flags & line_buf::LFT_32BIT));
        avx2_rev_horz_ana32(atk, ldst, hdst, src, width, even);
      }
      else 
      {
        assert((ldst == NULL || ldst->flags & line_buf::LFT_64BIT) &&
               (hdst == NULL || hdst->flags & line_buf::LFT_64BIT) && 
               (src == NULL || src->flags & line_buf::LFT_64BIT));
        avx2_rev_horz_ana64(atk, ldst, hdst, src, width, even);
      }
    } 
    
    //////////////////////////////////////////////////////////////////////////
    static
    void avx2_rev_horz_syn32(const param_atk* atk, const line_buf* dst, 
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
          const ui8 e  = s->rev.Eatk;
          __m256i va = _mm256_set1_epi32(a);
          __m256i vb = _mm256_set1_epi32(b);

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
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_add_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_add_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i w = _mm256_srai_epi32(t, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i w = _mm256_srai_epi32(t, e);
                d = _mm256_add_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_sub_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i v = _mm256_sub_epi32(vb, t);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }
          else {
            // general case
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i u = _mm256_mullo_epi32(va, t);
                __m256i v = _mm256_add_epi32(vb, u);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 8, sp += 8, dp += 8)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi32(s1, s2);
                __m256i u = _mm256_mullo_epi32(va, t);
                __m256i v = _mm256_add_epi32(vb, u);
                __m256i w = _mm256_srai_epi32(v, e);
                d = _mm256_sub_epi32(d, w);
                _mm256_store_si256((__m256i*)dp, d);
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
          avx2_interleave32(dp, spl, sph, w);
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
    static
    void avx2_rev_horz_syn64(const param_atk* atk, const line_buf* dst, 
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
          const ui8 e  = s->rev.Eatk;
          __m256i vb = _mm256_set1_epi64x(b);
          __m256i ve = _mm256_set1_epi64x(1LL << (63 - e));      

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
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi64(s1, s2);
                __m256i v = _mm256_add_epi64(vb, t);
                __m256i w = avx2_mm256_srai_epi64(v, e, ve);
                d = _mm256_sub_epi64(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            }
            else
            {
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi64(s1, s2);
                __m256i v = _mm256_add_epi64(vb, t);
                __m256i w = avx2_mm256_srai_epi64(v, e, ve);
                d = _mm256_sub_epi64(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            }
          }
          else if (a == -1 && b == 1 && e == 1)
          {  // 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi64(s1, s2);
                __m256i w = avx2_mm256_srai_epi64(t, e, ve);
                d = _mm256_add_epi64(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi64(s1, s2);
                __m256i w = avx2_mm256_srai_epi64(t, e, ve);
                d = _mm256_add_epi64(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }
          else if (a == -1)
          { // any case with a == -1, which is not 5/3 predict
            int i = (int)aug_width;
            if (ev)
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp - 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi64(s1, s2);
                __m256i v = _mm256_sub_epi64(vb, t);
                __m256i w = avx2_mm256_srai_epi64(v, e, ve);
                d = _mm256_sub_epi64(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
            else
              for (; i > 0; i -= 4, sp += 4, dp += 4)
              {
                __m256i s1 = _mm256_load_si256((__m256i*)sp);
                __m256i s2 = _mm256_loadu_si256((__m256i*)(sp + 1));
                __m256i d = _mm256_load_si256((__m256i*)dp);
                __m256i t = _mm256_add_epi64(s1, s2);
                __m256i v = _mm256_sub_epi64(vb, t);
                __m256i w = avx2_mm256_srai_epi64(v, e, ve);
                d = _mm256_sub_epi64(d, w);
                _mm256_store_si256((__m256i*)dp, d);
              }
          }
          else {
            // general case
            // 64bit multiplication is not supported in avx2;
            // in particular, _mm_mullo_epi64.
            if (ev)
              for (ui32 i = aug_width; i > 0; --i, sp++, dp++)
                *dp -= (b + a * (sp[-1] + sp[0])) >> e;
            else
              for (ui32 i = aug_width; i > 0; --i, sp++, dp++)
                *dp -= (b + a * (sp[0] + sp[1])) >> e;
          }

          // swap buffers
          si64* t = aug; aug = oth; oth = t;
          ev = !ev;
          ui32 w = aug_width; aug_width = oth_width; oth_width = w;
        }

        // combine both lsrc and hsrc into dst
        {
          double* dp  = (double*)dst->p;
          double* spl = (double*)(even ? lsrc->p : hsrc->p);
          double* sph = (double*)(even ? hsrc->p : lsrc->p);
          int w = (int)width;
          avx2_interleave64(dp, spl, sph, w);
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
    void avx2_rev_horz_syn(const param_atk* atk, const line_buf* dst, 
                           const line_buf* lsrc, const line_buf* hsrc, 
                           ui32 width, bool even)
    {
      if (dst->flags & line_buf::LFT_32BIT) 
      {
        assert((lsrc == NULL || lsrc->flags & line_buf::LFT_32BIT) && 
               (hsrc == NULL || hsrc->flags & line_buf::LFT_32BIT));
        avx2_rev_horz_syn32(atk, dst, lsrc, hsrc, width, even);
      }
      else 
      {
        assert((dst == NULL || dst->flags & line_buf::LFT_64BIT) &&
               (lsrc == NULL || lsrc->flags & line_buf::LFT_64BIT) && 
               (hsrc == NULL || hsrc->flags & line_buf::LFT_64BIT));
        avx2_rev_horz_syn64(atk, dst, lsrc, hsrc, width, even);
      }
    }

  } // !local
} // !ojph

#endif
