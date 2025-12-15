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
// File: ojph_colour_sse2.cpp
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

#include <emmintrin.h>

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void sse2_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                       ui32 width)
    {
      uint32_t rounding_mode = _MM_GET_ROUNDING_MODE();
      _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
      __m128 shift = _mm_set1_ps(0.5f);
      __m128 m = _mm_set1_ps(mul);
      for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        __m128 t = _mm_loadu_ps(sp);
        __m128 s = _mm_add_ps(t, shift);
        s = _mm_mul_ps(s, m);
        _mm_storeu_si128((__m128i*)dp, _mm_cvtps_epi32(s));
      }
      _MM_SET_ROUNDING_MODE(rounding_mode);
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                  ui32 width)
    {
      uint32_t rounding_mode = _MM_GET_ROUNDING_MODE();
      _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
      __m128 m = _mm_set1_ps(mul);
      for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        __m128 t = _mm_loadu_ps(sp);
        __m128 s = _mm_mul_ps(t, m);
        _mm_storeu_si128((__m128i*)dp, _mm_cvtps_epi32(s));
      }
      _MM_SET_ROUNDING_MODE(rounding_mode);
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    __m128i ojph_mm_max_ge_epi32(__m128i a, __m128i b, __m128 x, __m128 y)
    {
      __m128 ct = _mm_cmpge_ps(x, y);     // 0xFFFFFFFF for x >= y
      __m128i c = _mm_castps_si128(ct);   // does not generate any code
      __m128i d = _mm_and_si128(c, a);    // keep only a, where x >= y
      __m128i e = _mm_andnot_si128(c, b); // keep only b, where x <  y
      return _mm_or_si128(d, e);          // combine
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    __m128i ojph_mm_min_lt_epi32(__m128i a, __m128i b, __m128 x, __m128 y)
    {
      __m128 ct = _mm_cmplt_ps(x, y);     // 0xFFFFFFFF for x < y
      __m128i c = _mm_castps_si128(ct);   // does not generate any code
      __m128i d = _mm_and_si128(c, a);    // keep only a, where x <  y
      __m128i e = _mm_andnot_si128(c, b); // keep only b, where x >= y
      return _mm_or_si128(d, e);          // combine
    }

    //////////////////////////////////////////////////////////////////////////
    template <bool NLT_TYPE3>
    static inline
    void local_sse2_irv_convert_to_integer(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      assert((src_line->flags & line_buf::LFT_32BIT) &&
             (src_line->flags & line_buf::LFT_INTEGER) == 0 &&
             (dst_line->flags & line_buf::LFT_32BIT) &&
             (dst_line->flags & line_buf::LFT_INTEGER));

      assert(bit_depth <= 32);
      uint32_t rounding_mode = _MM_GET_ROUNDING_MODE();
      _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);

      const float* sp = src_line->f32;
      si32* dp = dst_line->i32 + dst_line_offset;
      // There is the possibility that converting to integer will
      // exceed the dynamic range of 32bit integer; therefore, care must be
      // exercised.
      // We look if the floating point number is outside the half-closed
      // interval [-0.5f, 0.5f). If so, we limit the resulting integer
      // to the maximum/minimum that number supports.
      si32 neg_limit = (si32)INT_MIN >> (32 - bit_depth);
      __m128 mul = _mm_set1_ps((float)(1ull << bit_depth));
      __m128 fl_up_lim = _mm_set1_ps(-(float)neg_limit); // val < upper
      __m128 fl_low_lim = _mm_set1_ps((float)neg_limit); // val >= lower
      __m128i s32_up_lim = _mm_set1_epi32(INT_MAX >> (32 - bit_depth));
      __m128i s32_low_lim = _mm_set1_epi32(INT_MIN >> (32 - bit_depth));

      if (is_signed)
      {
        __m128i zero = _mm_setzero_si128();
        __m128i bias = _mm_set1_epi32(-(si32)((1ULL << (bit_depth - 1)) + 1));
        for (int i = (int)width; i > 0; i -= 4, sp += 4, dp += 4) {
          __m128 t = _mm_loadu_ps(sp);
          t = _mm_mul_ps(t, mul);
          __m128i u = _mm_cvtps_epi32(t);
          u = ojph_mm_max_ge_epi32(u, s32_low_lim, t, fl_low_lim);
          u = ojph_mm_min_lt_epi32(u, s32_up_lim, t, fl_up_lim);
          if (NLT_TYPE3)
          {
            __m128i c = _mm_cmpgt_epi32(zero, u); //0xFFFFFFFF for -ve value
            __m128i neg = _mm_sub_epi32(bias, u); //-bias -value
            neg = _mm_and_si128(c, neg);          //keep only - bias - value
            u = _mm_andnot_si128(c, u);           //keep only +ve or 0
            u = _mm_or_si128(neg, u);             //combine
          }
          _mm_storeu_si128((__m128i*)dp, u);
        }
      }
      else
      {
        __m128i half = _mm_set1_epi32((si32)(1ULL << (bit_depth - 1)));
        for (int i = (int)width; i > 0; i -= 4, sp += 4, dp += 4) {
          __m128 t = _mm_loadu_ps(sp);
          t = _mm_mul_ps(t, mul);
          __m128i u = _mm_cvtps_epi32(t);
          u = ojph_mm_max_ge_epi32(u, s32_low_lim, t, fl_low_lim);
          u = ojph_mm_min_lt_epi32(u, s32_up_lim, t, fl_up_lim);
          u = _mm_add_epi32(u, half);
          _mm_storeu_si128((__m128i*)dp, u);
        }
      }

      _MM_SET_ROUNDING_MODE(rounding_mode);
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_irv_convert_to_integer(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_sse2_irv_convert_to_integer<false>(src_line, dst_line, 
        dst_line_offset, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_irv_convert_to_integer_nlt_type3(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_sse2_irv_convert_to_integer<true>(src_line, dst_line, 
        dst_line_offset, bit_depth, is_signed, width);
    }

    /////////////////////////////////////////////////////////////////////////
    // https://github.com/seung-lab/dijkstra3d/blob/master/libdivide.h
    static inline __m128i sse2_mm_srai_epi64(__m128i a, int amt, __m128i m)
    {
      // note than m must be obtained using
      // __m128i m = _mm_set1_epi64x(1ULL << (63 - amt));
      __m128i x = _mm_srli_epi64(a, amt);
      x = _mm_xor_si128(x, m);
      __m128i result = _mm_sub_epi64(x, m);
      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline __m128i sse2_cvtlo_epi32_epi64(__m128i a, __m128i zero)
    {
      __m128i t;
      t = _mm_cmplt_epi32(a, zero);      // get -ve
      t = _mm_unpacklo_epi32(a, t);
      return t;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline __m128i sse2_cvthi_epi32_epi64(__m128i a, __m128i zero)
    {
      __m128i t;
      t = _mm_cmplt_epi32(a, zero);      // get -ve
      t = _mm_unpackhi_epi32(a, t);
      return t;
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_convert(const line_buf *src_line,
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
          __m128i sh = _mm_set1_epi32((si32)shift);
          for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
          {
            __m128i s = _mm_loadu_si128((__m128i*)sp);
            s = _mm_add_epi32(s, sh);
            _mm_storeu_si128((__m128i*)dp, s);
          }
        }
        else
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si64 *dp = dst_line->i64 + dst_line_offset;
          __m128i zero = _mm_setzero_si128();
          __m128i sh = _mm_set1_epi64x(shift);
          for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
          {
            __m128i s, t;
            s = _mm_loadu_si128((__m128i*)sp);

            t = sse2_cvtlo_epi32_epi64(s, zero);
            t = _mm_add_epi64(t, sh);
            _mm_storeu_si128((__m128i*)dp, t);

            t = sse2_cvthi_epi32_epi64(s, zero);
            t = _mm_add_epi64(t, sh);
            _mm_storeu_si128((__m128i*)dp + 1, t);
          }
        }
      }
      else
      {
        assert(src_line->flags | line_buf::LFT_64BIT);
        assert(dst_line->flags | line_buf::LFT_32BIT);
        const si64 *sp = src_line->i64 + src_line_offset;
        si32 *dp = dst_line->i32 + dst_line_offset;
        __m128i low_bits = _mm_set_epi64x(0, (si64)ULLONG_MAX);
        __m128i sh = _mm_set1_epi64x(shift);
        for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
        {
          __m128i s, t;
          s = _mm_loadu_si128((__m128i*)sp);
          s = _mm_add_epi64(s, sh);

          t = _mm_shuffle_epi32(s, _MM_SHUFFLE(0, 0, 2, 0));
          t = _mm_and_si128(low_bits, t);

          s = _mm_loadu_si128((__m128i*)sp + 1);
          s = _mm_add_epi64(s, sh);

          s = _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 0, 0, 0));
          s = _mm_andnot_si128(low_bits, s);

          t = _mm_or_si128(s, t);
          _mm_storeu_si128((__m128i*)dp, t);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_convert_nlt_type3(const line_buf *src_line,
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
          __m128i sh = _mm_set1_epi32((si32)(-shift));
          __m128i zero = _mm_setzero_si128();
          for (int i = (width + 3) >> 2; i > 0; --i, sp += 4, dp += 4)
          {
            __m128i s = _mm_loadu_si128((__m128i*)sp);
            __m128i c = _mm_cmplt_epi32(s, zero);  // 0xFFFFFFFF for -ve value
            __m128i v_m_sh = _mm_sub_epi32(sh, s); // - shift - value
            v_m_sh = _mm_and_si128(c, v_m_sh);     // keep only - shift - value
            s = _mm_andnot_si128(c, s);            // keep only +ve or 0
            s = _mm_or_si128(s, v_m_sh);           // combine
            _mm_storeu_si128((__m128i*)dp, s);
          }
        }
        else
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si64 *dp = dst_line->i64 + dst_line_offset;
          __m128i sh = _mm_set1_epi64x(-shift);
          __m128i zero = _mm_setzero_si128();
          for (int i = (width + 3) >> 2; i > 0; --i, sp += 4, dp += 4)
          {
            __m128i s, t, u, c, v_m_sh;
            s = _mm_loadu_si128((__m128i*)sp);

            t = _mm_cmplt_epi32(s, zero);      // find -ve 32bit -1
            u = _mm_unpacklo_epi32(s, t);      // correct 64bit data
            c = _mm_unpacklo_epi32(t, t);      // 64bit -1 for -ve value

            v_m_sh = _mm_sub_epi64(sh, u);     // - shift - value
            v_m_sh = _mm_and_si128(c, v_m_sh); // keep only - shift - value
            u = _mm_andnot_si128(c, u);        // keep only +ve or 0
            u = _mm_or_si128(u, v_m_sh);       // combine

            _mm_storeu_si128((__m128i*)dp, u);
            u = _mm_unpackhi_epi32(s, t);      // correct 64bit data
            c = _mm_unpackhi_epi32(t, t);      // 64bit -1 for -ve value

            v_m_sh = _mm_sub_epi64(sh, u);     // - shift - value
            v_m_sh = _mm_and_si128(c, v_m_sh); // keep only - shift - value
            u = _mm_andnot_si128(c, u);        // keep only +ve or 0
            u = _mm_or_si128(u, v_m_sh);       // combine

            _mm_storeu_si128((__m128i*)dp + 1, u);
          }
        }
      }
      else
      {
        assert(src_line->flags | line_buf::LFT_64BIT);
        assert(dst_line->flags | line_buf::LFT_32BIT);
        const si64 *sp = src_line->i64 + src_line_offset;
        si32 *dp = dst_line->i32 + dst_line_offset;
        __m128i sh = _mm_set1_epi64x(-shift);
        __m128i zero = _mm_setzero_si128();
        __m128i half_mask = _mm_set_epi64x(0, (si64)ULLONG_MAX);
        for (int i = (width + 3) >> 2; i > 0; --i, sp += 4, dp += 4)
        {
          // s for source, t for target, p for positive, n for negative,
          // m for mask, and tm for temp
          __m128i s, t, p, n, m, tm;
          s = _mm_loadu_si128((__m128i*)sp);

          tm = _mm_cmplt_epi32(s, zero);   // 32b -1 for -ve value
          m = _mm_shuffle_epi32(tm, _MM_SHUFFLE(3, 3, 1, 1)); // expand to 64b
          tm = _mm_sub_epi64(sh, s);       // - shift - value
          n = _mm_and_si128(m, tm);        // -ve
          p = _mm_andnot_si128(m, s);      // +ve
          tm = _mm_or_si128(n, p);
          tm = _mm_shuffle_epi32(tm, _MM_SHUFFLE(0, 0, 2, 0));
          t = _mm_and_si128(half_mask, tm);

          s = _mm_loadu_si128((__m128i*)sp + 1);
          tm = _mm_cmplt_epi32(s, zero);   // 32b -1 for -ve value
          m = _mm_shuffle_epi32(tm, _MM_SHUFFLE(3, 3, 1, 1)); // expand to 64b
          tm = _mm_sub_epi64(sh, s);       // - shift - value
          n = _mm_and_si128(m, tm);        // -ve
          p = _mm_andnot_si128(m, s);      // +ve
          tm = _mm_or_si128(n, p);
          tm = _mm_shuffle_epi32(tm, _MM_SHUFFLE(2, 0, 0, 0));
          tm = _mm_andnot_si128(half_mask, tm);

          t = _mm_or_si128(t, tm);
           _mm_storeu_si128((__m128i*)dp, t);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    template<bool NLT_TYPE3>
    static inline
    void local_sse2_irv_convert_to_float(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      assert((src_line->flags & line_buf::LFT_32BIT) &&
             (src_line->flags & line_buf::LFT_INTEGER) &&
             (dst_line->flags & line_buf::LFT_32BIT) &&
             (dst_line->flags & line_buf::LFT_INTEGER) == 0);

      assert(bit_depth <= 32);
      __m128 mul = _mm_set1_ps((float)(1.0 / (double)(1ULL << bit_depth)));

      const si32* sp = src_line->i32 + src_line_offset;
      float* dp = dst_line->f32;
      if (is_signed)
      {
        __m128i zero = _mm_setzero_si128();
        __m128i bias = _mm_set1_epi32(-(si32)((1ULL << (bit_depth - 1)) + 1));
        for (int i = (int)width; i > 0; i -= 4, sp += 4, dp += 4) {
          __m128i t = _mm_loadu_si128((__m128i*)sp);
          if (NLT_TYPE3)
          {
            __m128i c = _mm_cmplt_epi32(t, zero); // 0xFFFFFFFF for -ve value
            __m128i neg = _mm_sub_epi32(bias, t); // - bias - value
            neg = _mm_and_si128(c, neg);          // keep only - bias - value
            c = _mm_andnot_si128(c, t);           // keep only +ve or 0
            t = _mm_or_si128(neg, c);             // combine
          }
          __m128 v = _mm_cvtepi32_ps(t);
          v = _mm_mul_ps(v, mul);
          _mm_storeu_ps(dp, v);
        }
      }
      else
      {
        __m128i half = _mm_set1_epi32((si32)(1ULL << (bit_depth - 1)));
        for (int i = (int)width; i > 0; i -= 4, sp += 4, dp += 4) {
          __m128i t = _mm_loadu_si128((__m128i*)sp);
          t = _mm_sub_epi32(t, half);
          __m128 v = _mm_cvtepi32_ps(t);
          v = _mm_mul_ps(v, mul);
          _mm_storeu_ps(dp, v);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_irv_convert_to_float(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_sse2_irv_convert_to_float<false>(src_line, src_line_offset,
        dst_line, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_irv_convert_to_float_nlt_type3(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_sse2_irv_convert_to_float<true>(src_line, src_line_offset,
        dst_line, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rct_forward(const line_buf *r,
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
        for (int i = (repeat + 3) >> 2; i > 0; --i)
        {
          __m128i mr = _mm_load_si128((__m128i*)rp);
          __m128i mg = _mm_load_si128((__m128i*)gp);
          __m128i mb = _mm_load_si128((__m128i*)bp);
          __m128i t = _mm_add_epi32(mr, mb);
          t = _mm_add_epi32(t, _mm_slli_epi32(mg, 1));
          _mm_store_si128((__m128i*)yp, _mm_srai_epi32(t, 2));
          t = _mm_sub_epi32(mb, mg);
          _mm_store_si128((__m128i*)cbp, t);
          t = _mm_sub_epi32(mr, mg);
          _mm_store_si128((__m128i*)crp, t);

          rp += 4; gp += 4; bp += 4;
          yp += 4; cbp += 4; crp += 4;
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
        __m128i zero = _mm_setzero_si128();
        __m128i v2 = _mm_set1_epi64x(1ULL << (63 - 2));
        const si32 *rp = r->i32, *gp = g->i32, *bp = b->i32;
        si64 *yp = y->i64, *cbp = cb->i64, *crp = cr->i64;
        for (int i = (repeat + 3) >> 2; i > 0; --i)
        {
          __m128i mr32 = _mm_load_si128((__m128i*)rp);
          __m128i mg32 = _mm_load_si128((__m128i*)gp);
          __m128i mb32 = _mm_load_si128((__m128i*)bp);
          __m128i mr, mg, mb, t;
          mr = sse2_cvtlo_epi32_epi64(mr32, zero);
          mg = sse2_cvtlo_epi32_epi64(mg32, zero);
          mb = sse2_cvtlo_epi32_epi64(mb32, zero);

          t = _mm_add_epi64(mr, mb);
          t = _mm_add_epi64(t, _mm_slli_epi64(mg, 1));
          _mm_store_si128((__m128i*)yp, sse2_mm_srai_epi64(t, 2, v2));
          t = _mm_sub_epi64(mb, mg);
          _mm_store_si128((__m128i*)cbp, t);
          t = _mm_sub_epi64(mr, mg);
          _mm_store_si128((__m128i*)crp, t);

          yp += 2; cbp += 2; crp += 2;

          mr = sse2_cvthi_epi32_epi64(mr32, zero);
          mg = sse2_cvthi_epi32_epi64(mg32, zero);
          mb = sse2_cvthi_epi32_epi64(mb32, zero);

          t = _mm_add_epi64(mr, mb);
          t = _mm_add_epi64(t, _mm_slli_epi64(mg, 1));
          _mm_store_si128((__m128i*)yp, sse2_mm_srai_epi64(t, 2, v2));
          t = _mm_sub_epi64(mb, mg);
          _mm_store_si128((__m128i*)cbp, t);
          t = _mm_sub_epi64(mr, mg);
          _mm_store_si128((__m128i*)crp, t);

          rp += 4; gp += 4; bp += 4;
          yp += 2; cbp += 2; crp += 2;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rct_backward(const line_buf *y,
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
        for (int i = (repeat + 3) >> 2; i > 0; --i)
        {
          __m128i my  = _mm_load_si128((__m128i*)yp);
          __m128i mcb = _mm_load_si128((__m128i*)cbp);
          __m128i mcr = _mm_load_si128((__m128i*)crp);

          __m128i t = _mm_add_epi32(mcb, mcr);
          t = _mm_sub_epi32(my, _mm_srai_epi32(t, 2));
          _mm_store_si128((__m128i*)gp, t);
          __m128i u = _mm_add_epi32(mcb, t);
          _mm_store_si128((__m128i*)bp, u);
          u = _mm_add_epi32(mcr, t);
          _mm_store_si128((__m128i*)rp, u);

          yp += 4; cbp += 4; crp += 4;
          rp += 4; gp += 4; bp += 4;
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
        __m128i v2 = _mm_set1_epi64x(1ULL << (63 - 2));
        __m128i low_bits = _mm_set_epi64x(0, (si64)ULLONG_MAX);
        const si64 *yp = y->i64, *cbp = cb->i64, *crp = cr->i64;
        si32 *rp = r->i32, *gp = g->i32, *bp = b->i32;
        for (int i = (repeat + 3) >> 2; i > 0; --i)
        {
          __m128i my, mcb, mcr, tr, tg, tb;
          my  = _mm_load_si128((__m128i*)yp);
          mcb = _mm_load_si128((__m128i*)cbp);
          mcr = _mm_load_si128((__m128i*)crp);

          tg = _mm_add_epi64(mcb, mcr);
          tg = _mm_sub_epi64(my, sse2_mm_srai_epi64(tg, 2, v2));
          tb = _mm_add_epi64(mcb, tg);
          tr = _mm_add_epi64(mcr, tg);

          __m128i mr, mg, mb;
          mr = _mm_shuffle_epi32(tr, _MM_SHUFFLE(0, 0, 2, 0));
          mr = _mm_and_si128(low_bits, mr);
          mg = _mm_shuffle_epi32(tg, _MM_SHUFFLE(0, 0, 2, 0));
          mg = _mm_and_si128(low_bits, mg);
          mb = _mm_shuffle_epi32(tb, _MM_SHUFFLE(0, 0, 2, 0));
          mb = _mm_and_si128(low_bits, mb);

          yp += 2; cbp += 2; crp += 2;

          my  = _mm_load_si128((__m128i*)yp);
          mcb = _mm_load_si128((__m128i*)cbp);
          mcr = _mm_load_si128((__m128i*)crp);

          tg = _mm_add_epi64(mcb, mcr);
          tg = _mm_sub_epi64(my, sse2_mm_srai_epi64(tg, 2, v2));
          tb = _mm_add_epi64(mcb, tg);
          tr = _mm_add_epi64(mcr, tg);

          tr = _mm_shuffle_epi32(tr, _MM_SHUFFLE(2, 0, 0, 0));
          tr = _mm_andnot_si128(low_bits, tr);
          mr = _mm_or_si128(mr, tr);
          tg = _mm_shuffle_epi32(tg, _MM_SHUFFLE(2, 0, 0, 0));
          tg = _mm_andnot_si128(low_bits, tg);
          mg = _mm_or_si128(mg, tg);
          tb = _mm_shuffle_epi32(tb, _MM_SHUFFLE(2, 0, 0, 0));
          tb = _mm_andnot_si128(low_bits, tb);
          mb = _mm_or_si128(mb, tb);

          _mm_store_si128((__m128i*)rp, mr);
          _mm_store_si128((__m128i*)gp, mg);
          _mm_store_si128((__m128i*)bp, mb);

          yp += 2; cbp += 2; crp += 2;
          rp += 4; gp += 4; bp += 4;
        }
      }
    }
  }
}

#endif
