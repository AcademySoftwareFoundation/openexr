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
// File: ojph_colour_avx2.cpp
// Author: Aous Naman
// Date: 11 October 2019
//***************************************************************************/

#include "ojph_arch.h"
#if defined(OJPH_ARCH_I386) || defined(OJPH_ARCH_X86_64)

#include <climits>
#include <cmath>

#include "ojph_defs.h"
#include "ojph_mem.h"
#include "ojph_colour.h"

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
    void avx2_rev_convert(const line_buf *src_line,
                          const ui32 src_line_offset,
                          line_buf *dst_line,
                          const ui32 dst_line_offset,
                          si64 shift, ui32 width)
    {
      if (src_line->flags & line_buf::LFT_32BIT)
      {
        if (dst_line->flags & line_buf::LFT_32BIT)
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si32 *dp = dst_line->i32 + dst_line_offset;
          __m256i sh = _mm256_set1_epi32((si32)shift);
          for (int i = (width + 7) >> 3; i > 0; --i, sp+=8, dp+=8)
          {
            __m256i s = _mm256_loadu_si256((__m256i*)sp);
            s = _mm256_add_epi32(s, sh);
            _mm256_storeu_si256((__m256i*)dp, s);
          }
        }
        else
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si64 *dp = dst_line->i64 + dst_line_offset;
          __m256i sh = _mm256_set1_epi64x(shift);
          for (int i = (width + 7) >> 3; i > 0; --i, sp+=8, dp+=8)
          {
            __m256i s, t;
            s = _mm256_loadu_si256((__m256i*)sp);

            t = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(s, 0));
            t = _mm256_add_epi64(t, sh);
            _mm256_storeu_si256((__m256i*)dp, t);

            t = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(s, 1));
            t = _mm256_add_epi64(t, sh);
            _mm256_storeu_si256((__m256i*)dp + 1, t);
          }
        }
      }
      else
      {
        assert(src_line->flags | line_buf::LFT_64BIT);
        assert(dst_line->flags | line_buf::LFT_32BIT);
        const si64 *sp = src_line->i64 + src_line_offset;
        si32 *dp = dst_line->i32 + dst_line_offset;
        __m256i low_bits = _mm256_set_epi64x(0, (si64)ULLONG_MAX,
                                             0, (si64)ULLONG_MAX);
        __m256i sh = _mm256_set1_epi64x(shift);
        for (int i = (width + 7) >> 3; i > 0; --i, sp+=8, dp+=8)
        {
          __m256i s, t;
          s = _mm256_loadu_si256((__m256i*)sp);
          s = _mm256_add_epi64(s, sh);

          t = _mm256_shuffle_epi32(s, _MM_SHUFFLE(0, 0, 2, 0));
          t = _mm256_and_si256(low_bits, t);

          s = _mm256_loadu_si256((__m256i*)sp + 1);
          s = _mm256_add_epi64(s, sh);

          s = _mm256_shuffle_epi32(s, _MM_SHUFFLE(2, 0, 0, 0));
          s = _mm256_andnot_si256(low_bits, s);

          t = _mm256_or_si256(s, t);
          t = _mm256_permute4x64_epi64(t, _MM_SHUFFLE(3, 1, 2, 0));
          _mm256_storeu_si256((__m256i*)dp, t);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_rev_convert_nlt_type3(const line_buf *src_line,
                                    const ui32 src_line_offset,
                                    line_buf *dst_line,
                                    const ui32 dst_line_offset,
                                    si64 shift, ui32 width)
    {
      if (src_line->flags & line_buf::LFT_32BIT)
      {
        if (dst_line->flags & line_buf::LFT_32BIT)
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si32 *dp = dst_line->i32 + dst_line_offset;
          __m256i sh = _mm256_set1_epi32((si32)(-shift));
          __m256i zero = _mm256_setzero_si256();
          for (int i = (width + 7) >> 3; i > 0; --i, sp += 8, dp += 8)
          {
            __m256i s = _mm256_loadu_si256((__m256i*)sp);
            __m256i c = _mm256_cmpgt_epi32(zero, s);  // 0xFFFFFFFF for -ve val
            __m256i v_m_sh = _mm256_sub_epi32(sh, s); // - shift - value
            v_m_sh = _mm256_and_si256(c, v_m_sh);     // keep only -shift-val
            s = _mm256_andnot_si256(c, s);            // keep only +ve or 0
            s = _mm256_or_si256(s, v_m_sh);           // combine
            _mm256_storeu_si256((__m256i*)dp, s);
          }
        }
        else
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si64 *dp = dst_line->i64 + dst_line_offset;
          __m256i sh = _mm256_set1_epi64x(-shift);
          __m256i zero = _mm256_setzero_si256();
          for (int i = (width + 7) >> 3; i > 0; --i, sp += 8, dp += 8)
          {
            __m256i s, t, u0, u1, c, v_m_sh;
            s = _mm256_loadu_si256((__m256i*)sp);

            t = _mm256_cmpgt_epi32(zero, s);      // find -ve 32bit -1
            u0 = _mm256_unpacklo_epi32(s, t);     // correct 64bit data
            c = _mm256_unpacklo_epi32(t, t);      // 64bit -1 for -ve value

            v_m_sh = _mm256_sub_epi64(sh, u0);    // - shift - value
            v_m_sh = _mm256_and_si256(c, v_m_sh); // keep only - shift - value
            u0 = _mm256_andnot_si256(c, u0);      // keep only +ve or 0
            u0 = _mm256_or_si256(u0, v_m_sh);     // combine

            u1 = _mm256_unpackhi_epi32(s, t);     // correct 64bit data
            c = _mm256_unpackhi_epi32(t, t);      // 64bit -1 for -ve value

            v_m_sh = _mm256_sub_epi64(sh, u1);    // - shift - value
            v_m_sh = _mm256_and_si256(c, v_m_sh); // keep only - shift - value
            u1 = _mm256_andnot_si256(c, u1);      // keep only +ve or 0
            u1 = _mm256_or_si256(u1, v_m_sh);     // combine

            t = _mm256_permute2x128_si256(u0, u1, (2 << 4) | 0);
            _mm256_storeu_si256((__m256i*)dp, t);

            t = _mm256_permute2x128_si256(u0, u1, (3 << 4) | 1);
            _mm256_storeu_si256((__m256i*)dp + 1, t);
          }
        }
      }
      else
      {
        assert(src_line->flags | line_buf::LFT_64BIT);
        assert(dst_line->flags | line_buf::LFT_32BIT);
        const si64 *sp = src_line->i64 + src_line_offset;
        si32 *dp = dst_line->i32 + dst_line_offset;
        __m256i sh = _mm256_set1_epi64x(-shift);
        __m256i zero = _mm256_setzero_si256();
        __m256i half_mask = _mm256_set_epi64x(0, (si64)ULLONG_MAX,
                                              0, (si64)ULLONG_MAX);
        for (int i = (width + 7) >> 3; i > 0; --i, sp += 8, dp += 8)
        {
          // s for source, t for target, p for positive, n for negative,
          // m for mask, and tm for temp
          __m256i s, t, p, n, m, tm;
          s = _mm256_loadu_si256((__m256i*)sp);

          m = _mm256_cmpgt_epi64(zero, s);    // 64b -1 for -ve value
          tm = _mm256_sub_epi64(sh, s);       // - shift - value
          n = _mm256_and_si256(m, tm);        // -ve
          p = _mm256_andnot_si256(m, s);      // +ve
          tm = _mm256_or_si256(n, p);
          tm = _mm256_shuffle_epi32(tm, _MM_SHUFFLE(0, 0, 2, 0));
          t = _mm256_and_si256(half_mask, tm);

          s = _mm256_loadu_si256((__m256i*)sp + 1);
          m = _mm256_cmpgt_epi64(zero, s);    // 64b -1 for -ve value
          tm = _mm256_sub_epi64(sh, s);       // - shift - value
          n = _mm256_and_si256(m, tm);        // -ve
          p = _mm256_andnot_si256(m, s);      // +ve
          tm = _mm256_or_si256(n, p);
          tm = _mm256_shuffle_epi32(tm, _MM_SHUFFLE(2, 0, 0, 0));
          tm = _mm256_andnot_si256(half_mask, tm);

          t = _mm256_or_si256(t, tm);
          t = _mm256_permute4x64_epi64(t, _MM_SHUFFLE(3, 1, 2, 0));
           _mm256_storeu_si256((__m256i*)dp, t);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    __m256i ojph_mm256_max_ge_epi32(__m256i a, __m256i b, __m256 x, __m256 y)
    {
      // We must use _CMP_NLT_UQ or _CMP_GE_OQ, _CMP_GE_OS, or _CMP_NLT_US
      // It is not clear to me which to use
      __m256 ct = _mm256_cmp_ps(x, y, _CMP_NLT_UQ); // 0xFFFFFFFF for x >= y
      __m256i c = _mm256_castps_si256(ct);   // does not generate any code
      __m256i d = _mm256_and_si256(c, a);    // keep only a, where x >= y
      __m256i e = _mm256_andnot_si256(c, b); // keep only b, where x <  y
      return _mm256_or_si256(d, e);          // combine
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    __m256i ojph_mm256_min_lt_epi32(__m256i a, __m256i b, __m256 x, __m256 y)
    {
      // We must use _CMP_LT_OQ or _CMP_NGE_UQ, _CMP_LT_OS, or _CMP_NGE_US
      // It is not clear to me which to use
      __m256 ct = _mm256_cmp_ps(x, y, _CMP_NGE_UQ); // 0xFFFFFFFF for x < y
      __m256i c = _mm256_castps_si256(ct);   // does not generate any code
      __m256i d = _mm256_and_si256(c, a);    // keep only a, where x <  y
      __m256i e = _mm256_andnot_si256(c, b); // keep only b, where x >= y
      return _mm256_or_si256(d, e);          // combine
    }

    //////////////////////////////////////////////////////////////////////////
    template<bool NLT_TYPE3>
    static inline
    void local_avx2_irv_convert_to_integer(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      assert((src_line->flags & line_buf::LFT_32BIT) &&
             (src_line->flags & line_buf::LFT_INTEGER) == 0 &&
             (dst_line->flags & line_buf::LFT_32BIT) &&
             (dst_line->flags & line_buf::LFT_INTEGER));

      assert(bit_depth <= 32);
      const float* sp = src_line->f32;
      si32* dp = dst_line->i32 + dst_line_offset;
      // There is the possibility that converting to integer will
      // exceed the dynamic range of 32bit integer; therefore, care must be
      // exercised.
      // We look if the floating point number is outside the half-closed
      // interval [-0.5f, 0.5f). If so, we limit the resulting integer
      // to the maximum/minimum that number supports.
      si32 neg_limit = (si32)INT_MIN >> (32 - bit_depth);
      __m256 mul = _mm256_set1_ps((float)(1ull << bit_depth));
      __m256 fl_up_lim = _mm256_set1_ps(-(float)neg_limit);  // val < upper
      __m256 fl_low_lim = _mm256_set1_ps((float)neg_limit);  // val >= lower
      __m256i s32_up_lim = _mm256_set1_epi32(INT_MAX >> (32 - bit_depth));
      __m256i s32_low_lim = _mm256_set1_epi32(INT_MIN >> (32 - bit_depth));

      if (is_signed)
      {
        __m256i zero = _mm256_setzero_si256();
        __m256i bias = 
          _mm256_set1_epi32(-(si32)((1ULL << (bit_depth - 1)) + 1));
        for (int i = (int)width; i > 0; i -= 8, sp += 8, dp += 8) {
          __m256 t = _mm256_loadu_ps(sp);
          t = _mm256_mul_ps(t, mul);
          __m256i u = _mm256_cvtps_epi32(t);
          u = ojph_mm256_max_ge_epi32(u, s32_low_lim, t, fl_low_lim);
          u = ojph_mm256_min_lt_epi32(u,  s32_up_lim, t,  fl_up_lim);
          if (NLT_TYPE3)
          {
            __m256i c = _mm256_cmpgt_epi32(zero, u); // 0xFFFFFFFF for -ve val
            __m256i neg = _mm256_sub_epi32(bias, u); // -bias -value
            neg = _mm256_and_si256(c, neg);          // keep only - bias - val
            u = _mm256_andnot_si256(c, u);           // keep only +ve or 0
            u = _mm256_or_si256(neg, u);             // combine
          }
          _mm256_storeu_si256((__m256i*)dp, u);
        }
      }
      else
      {
        __m256i half = _mm256_set1_epi32((si32)(1ULL << (bit_depth - 1)));
        for (int i = (int)width; i > 0; i -= 8, sp += 8, dp += 8) {
          __m256 t = _mm256_loadu_ps(sp);
          t = _mm256_mul_ps(t, mul);
          __m256i u = _mm256_cvtps_epi32(t);
          u = ojph_mm256_max_ge_epi32(u, s32_low_lim, t, fl_low_lim);
          u = ojph_mm256_min_lt_epi32(u,  s32_up_lim, t,  fl_up_lim);
          u = _mm256_add_epi32(u, half);
          _mm256_storeu_si256((__m256i*)dp, u);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_irv_convert_to_integer(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_avx2_irv_convert_to_integer<false>(src_line, dst_line, 
        dst_line_offset, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_irv_convert_to_integer_nlt_type3(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_avx2_irv_convert_to_integer<true>(src_line, dst_line, 
        dst_line_offset, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    template<bool NLT_TYPE3>
    static inline    
    void local_avx2_irv_convert_to_float(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      assert((src_line->flags & line_buf::LFT_32BIT) &&
             (src_line->flags & line_buf::LFT_INTEGER) &&
             (dst_line->flags & line_buf::LFT_32BIT) &&
             (dst_line->flags & line_buf::LFT_INTEGER) == 0);

      assert(bit_depth <= 32);
      __m256 mul = _mm256_set1_ps((float)(1.0 / (double)(1ULL << bit_depth)));

      const si32* sp = src_line->i32 + src_line_offset;
      float* dp = dst_line->f32;
      if (is_signed)
      {
        __m256i zero = _mm256_setzero_si256();
        __m256i bias = 
          _mm256_set1_epi32(-(si32)((1ULL << (bit_depth - 1)) + 1));
        for (int i = (int)width; i > 0; i -= 8, sp += 8, dp += 8) {
          __m256i t = _mm256_loadu_si256((__m256i*)sp);
          if (NLT_TYPE3)
          {          
            __m256i c = _mm256_cmpgt_epi32(zero, t); // 0xFFFFFFFF for -ve val
            __m256i neg = _mm256_sub_epi32(bias, t); // - bias - value
            neg = _mm256_and_si256(c, neg);          // keep only - bias - val
            c = _mm256_andnot_si256(c, t);           // keep only +ve or 0
            t = _mm256_or_si256(neg, c);             // combine
          }
          __m256 v = _mm256_cvtepi32_ps(t);
          v = _mm256_mul_ps(v, mul);
          _mm256_storeu_ps(dp, v);
        }
      }
      else
      {
        __m256i half = _mm256_set1_epi32((si32)(1ULL << (bit_depth - 1)));
        for (int i = (int)width; i > 0; i -= 8, sp += 8, dp += 8) {
          __m256i t = _mm256_loadu_si256((__m256i*)sp);
          t = _mm256_sub_epi32(t, half);
          __m256 v = _mm256_cvtepi32_ps(t);
          v = _mm256_mul_ps(v, mul);
          _mm256_storeu_ps(dp, v);
        }
      }
    }

        //////////////////////////////////////////////////////////////////////////
    void avx2_irv_convert_to_float(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_avx2_irv_convert_to_float<false>(src_line, src_line_offset,
        dst_line, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_irv_convert_to_float_nlt_type3(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_avx2_irv_convert_to_float<true>(src_line, src_line_offset,
        dst_line, bit_depth, is_signed, width);
    }


    //////////////////////////////////////////////////////////////////////////
    void avx2_rct_forward(const line_buf *r,
                          const line_buf *g,
                          const line_buf *b,
                          line_buf *y, line_buf *cb, line_buf *cr,
                          ui32 repeat)
    {
      assert((y->flags  & line_buf::LFT_INTEGER) &&
             (cb->flags & line_buf::LFT_INTEGER) &&
             (cr->flags & line_buf::LFT_INTEGER) &&
             (r->flags  & line_buf::LFT_INTEGER) &&
             (g->flags  & line_buf::LFT_INTEGER) &&
             (b->flags  & line_buf::LFT_INTEGER));

      if  (y->flags & line_buf::LFT_32BIT)
      {
        assert((y->flags  & line_buf::LFT_32BIT) &&
               (cb->flags & line_buf::LFT_32BIT) &&
               (cr->flags & line_buf::LFT_32BIT) &&
               (r->flags  & line_buf::LFT_32BIT) &&
               (g->flags  & line_buf::LFT_32BIT) &&
               (b->flags  & line_buf::LFT_32BIT));
        const si32 *rp = r->i32, * gp = g->i32, * bp = b->i32;
        si32 *yp = y->i32, * cbp = cb->i32, * crp = cr->i32;
        for (int i = (repeat + 7) >> 3; i > 0; --i)
        {
          __m256i mr = _mm256_load_si256((__m256i*)rp);
          __m256i mg = _mm256_load_si256((__m256i*)gp);
          __m256i mb = _mm256_load_si256((__m256i*)bp);
          __m256i t = _mm256_add_epi32(mr, mb);
          t = _mm256_add_epi32(t, _mm256_slli_epi32(mg, 1));
          _mm256_store_si256((__m256i*)yp, _mm256_srai_epi32(t, 2));
          t = _mm256_sub_epi32(mb, mg);
          _mm256_store_si256((__m256i*)cbp, t);
          t = _mm256_sub_epi32(mr, mg);
          _mm256_store_si256((__m256i*)crp, t);

          rp += 8; gp += 8; bp += 8;
          yp += 8; cbp += 8; crp += 8;
        }
      }
      else
      {
        assert((y->flags  & line_buf::LFT_64BIT) &&
               (cb->flags & line_buf::LFT_64BIT) &&
               (cr->flags & line_buf::LFT_64BIT) &&
               (r->flags  & line_buf::LFT_32BIT) &&
               (g->flags  & line_buf::LFT_32BIT) &&
               (b->flags  & line_buf::LFT_32BIT));
        __m256i v2 = _mm256_set1_epi64x(1ULL << (63 - 2));
        const si32 *rp = r->i32, *gp = g->i32, *bp = b->i32;
        si64 *yp = y->i64, *cbp = cb->i64, *crp = cr->i64;
        for (int i = (repeat + 7) >> 3; i > 0; --i)
        {
          __m256i mr32 = _mm256_load_si256((__m256i*)rp);
          __m256i mg32 = _mm256_load_si256((__m256i*)gp);
          __m256i mb32 = _mm256_load_si256((__m256i*)bp);
          __m256i mr, mg, mb, t;
          mr = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(mr32, 0));
          mg = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(mg32, 0));
          mb = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(mb32, 0));

          t = _mm256_add_epi64(mr, mb);
          t = _mm256_add_epi64(t, _mm256_slli_epi64(mg, 1));
          _mm256_store_si256((__m256i*)yp, avx2_mm256_srai_epi64(t, 2, v2));
          t = _mm256_sub_epi64(mb, mg);
          _mm256_store_si256((__m256i*)cbp, t);
          t = _mm256_sub_epi64(mr, mg);
          _mm256_store_si256((__m256i*)crp, t);

          yp += 4; cbp += 4; crp += 4;

          mr = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(mr32, 1));
          mg = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(mg32, 1));
          mb = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(mb32, 1));

          t = _mm256_add_epi64(mr, mb);
          t = _mm256_add_epi64(t, _mm256_slli_epi64(mg, 1));
          _mm256_store_si256((__m256i*)yp, avx2_mm256_srai_epi64(t, 2, v2));
          t = _mm256_sub_epi64(mb, mg);
          _mm256_store_si256((__m256i*)cbp, t);
          t = _mm256_sub_epi64(mr, mg);
          _mm256_store_si256((__m256i*)crp, t);

          rp += 8; gp += 8; bp += 8;
          yp += 4; cbp += 4; crp += 4;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void avx2_rct_backward(const line_buf *y,
                           const line_buf *cb,
                           const line_buf *cr,
                           line_buf *r, line_buf *g, line_buf *b,
                           ui32 repeat)
    {
      assert((y->flags  & line_buf::LFT_INTEGER) &&
             (cb->flags & line_buf::LFT_INTEGER) &&
             (cr->flags & line_buf::LFT_INTEGER) &&
             (r->flags  & line_buf::LFT_INTEGER) &&
             (g->flags  & line_buf::LFT_INTEGER) &&
             (b->flags  & line_buf::LFT_INTEGER));

      if (y->flags & line_buf::LFT_32BIT)
      {
        assert((y->flags  & line_buf::LFT_32BIT) &&
               (cb->flags & line_buf::LFT_32BIT) &&
               (cr->flags & line_buf::LFT_32BIT) &&
               (r->flags  & line_buf::LFT_32BIT) &&
               (g->flags  & line_buf::LFT_32BIT) &&
               (b->flags  & line_buf::LFT_32BIT));
        const si32 *yp = y->i32, *cbp = cb->i32, *crp = cr->i32;
        si32 *rp = r->i32, *gp = g->i32, *bp = b->i32;
        for (int i = (repeat + 7) >> 3; i > 0; --i)
        {
          __m256i my  = _mm256_load_si256((__m256i*)yp);
          __m256i mcb = _mm256_load_si256((__m256i*)cbp);
          __m256i mcr = _mm256_load_si256((__m256i*)crp);

          __m256i t = _mm256_add_epi32(mcb, mcr);
          t = _mm256_sub_epi32(my, _mm256_srai_epi32(t, 2));
          _mm256_store_si256((__m256i*)gp, t);
          __m256i u = _mm256_add_epi32(mcb, t);
          _mm256_store_si256((__m256i*)bp, u);
          u = _mm256_add_epi32(mcr, t);
          _mm256_store_si256((__m256i*)rp, u);

          yp += 8; cbp += 8; crp += 8;
          rp += 8; gp += 8; bp += 8;
        }
      }
      else
      {
        assert((y->flags  & line_buf::LFT_64BIT) &&
               (cb->flags & line_buf::LFT_64BIT) &&
               (cr->flags & line_buf::LFT_64BIT) &&
               (r->flags  & line_buf::LFT_32BIT) &&
               (g->flags  & line_buf::LFT_32BIT) &&
               (b->flags  & line_buf::LFT_32BIT));
        __m256i v2 = _mm256_set1_epi64x(1ULL << (63 - 2));
        __m256i low_bits = _mm256_set_epi64x(0, (si64)ULLONG_MAX,
                                             0, (si64)ULLONG_MAX);
        const si64 *yp = y->i64, *cbp = cb->i64, *crp = cr->i64;
        si32 *rp = r->i32, *gp = g->i32, *bp = b->i32;
        for (int i = (repeat + 7) >> 3; i > 0; --i)
        {
          __m256i my, mcb, mcr, tr, tg, tb;
          my  = _mm256_load_si256((__m256i*)yp);
          mcb = _mm256_load_si256((__m256i*)cbp);
          mcr = _mm256_load_si256((__m256i*)crp);

          tg = _mm256_add_epi64(mcb, mcr);
          tg = _mm256_sub_epi64(my, avx2_mm256_srai_epi64(tg, 2, v2));
          tb = _mm256_add_epi64(mcb, tg);
          tr = _mm256_add_epi64(mcr, tg);

          __m256i mr, mg, mb;
          mr = _mm256_shuffle_epi32(tr, _MM_SHUFFLE(0, 0, 2, 0));
          mr = _mm256_and_si256(low_bits, mr);
          mg = _mm256_shuffle_epi32(tg, _MM_SHUFFLE(0, 0, 2, 0));
          mg = _mm256_and_si256(low_bits, mg);
          mb = _mm256_shuffle_epi32(tb, _MM_SHUFFLE(0, 0, 2, 0));
          mb = _mm256_and_si256(low_bits, mb);

          yp += 4; cbp += 4; crp += 4;

          my  = _mm256_load_si256((__m256i*)yp);
          mcb = _mm256_load_si256((__m256i*)cbp);
          mcr = _mm256_load_si256((__m256i*)crp);

          tg = _mm256_add_epi64(mcb, mcr);
          tg = _mm256_sub_epi64(my, avx2_mm256_srai_epi64(tg, 2, v2));
          tb = _mm256_add_epi64(mcb, tg);
          tr = _mm256_add_epi64(mcr, tg);

          tr = _mm256_shuffle_epi32(tr, _MM_SHUFFLE(2, 0, 0, 0));
          tr = _mm256_andnot_si256(low_bits, tr);
          mr = _mm256_or_si256(mr, tr);
          mr = _mm256_permute4x64_epi64(mr, _MM_SHUFFLE(3, 1, 2, 0));

          tg = _mm256_shuffle_epi32(tg, _MM_SHUFFLE(2, 0, 0, 0));
          tg = _mm256_andnot_si256(low_bits, tg);
          mg = _mm256_or_si256(mg, tg);
          mg = _mm256_permute4x64_epi64(mg, _MM_SHUFFLE(3, 1, 2, 0));

          tb = _mm256_shuffle_epi32(tb, _MM_SHUFFLE(2, 0, 0, 0));
          tb = _mm256_andnot_si256(low_bits, tb);
          mb = _mm256_or_si256(mb, tb);
          mb = _mm256_permute4x64_epi64(mb, _MM_SHUFFLE(3, 1, 2, 0));

          _mm256_store_si256((__m256i*)rp, mr);
          _mm256_store_si256((__m256i*)gp, mg);
          _mm256_store_si256((__m256i*)bp, mb);

          yp += 4; cbp += 4; crp += 4;
          rp += 8; gp += 8; bp += 8;
        }
      }
    }

  }
}

#endif
