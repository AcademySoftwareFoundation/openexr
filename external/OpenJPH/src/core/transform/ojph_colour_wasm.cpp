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
// File: ojph_colour_wasm.cpp
// Author: Aous Naman
// Date: 9 February 2021
//***************************************************************************/

#include <climits>
#include <cmath>
#include <wasm_simd128.h>

#include "ojph_defs.h"
#include "ojph_mem.h"
#include "ojph_colour.h"
#include "ojph_colour_local.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    static inline
    v128_t ojph_convert_float_to_i32(v128_t a, v128_t zero, v128_t half)
    { // We implement ojph_round, which is
      // val + (val >= 0.0f ? 0.5f : -0.5f), where val is float
      v128_t c = wasm_f32x4_ge(a, zero);   // greater or equal to zero
      v128_t p = wasm_f32x4_add(a, half);  // for positive, add half
      v128_t n = wasm_f32x4_sub(a, half);  // for negative, subtract half
      v128_t d = wasm_v128_and(c, p);      // keep positive only
      v128_t e = wasm_v128_andnot(n, c);   // keep negative only
      v128_t v = wasm_v128_or(d, e);       // combine
      return wasm_i32x4_trunc_sat_f32x4(v);// truncate (towards 0)
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_convert(const line_buf *src_line,
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
          v128_t sh = wasm_i32x4_splat((si32)shift);
          for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
          {
            v128_t s = wasm_v128_load(sp);
            s = wasm_i32x4_add(s, sh);
            wasm_v128_store(dp, s);
          }
        }
        else
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si64 *dp = dst_line->i64 + dst_line_offset;
          v128_t sh = wasm_i64x2_splat(shift);
          for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
          {
            v128_t s, t;
            s = wasm_v128_load(sp);

            t = wasm_i64x2_extend_low_i32x4(s);
            t = wasm_i64x2_add(t, sh);
            wasm_v128_store(dp, t);

            t = wasm_i64x2_extend_high_i32x4(s);
            t = wasm_i64x2_add(t, sh);
            wasm_v128_store(dp + 2, t);
          }
        }
      }
      else
      {
        assert(src_line->flags | line_buf::LFT_64BIT);
        assert(dst_line->flags | line_buf::LFT_32BIT);
        const si64 *sp = src_line->i64 + src_line_offset;
        si32 *dp = dst_line->i32 + dst_line_offset;
        v128_t sh = wasm_i64x2_splat(shift);
        for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
        {
          v128_t s0, s1;
          s0 = wasm_v128_load(sp);
          s0 = wasm_i64x2_add(s0, sh);
          s1 = wasm_v128_load(sp + 2);
          s1 = wasm_i64x2_add(s1, sh);
          s0 = wasm_i32x4_shuffle(s0, s1, 0, 2, 4 + 0, 4 + 2);
          wasm_v128_store(dp, s0);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_convert_nlt_type3(const line_buf *src_line,
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
          v128_t sh = wasm_i32x4_splat((si32)(-shift));
          v128_t zero = wasm_i32x4_splat(0);
          for (int i = (width + 3) >> 2; i > 0; --i, sp += 4, dp += 4)
          {
            v128_t s = wasm_v128_load(sp);
            v128_t c = wasm_i32x4_lt(s, zero);     // 0xFFFFFFFF for -ve value
            v128_t v_m_sh = wasm_i32x4_sub(sh, s); // - shift - value
            v_m_sh = wasm_v128_and(c, v_m_sh);     // keep only - shift - value
            s = wasm_v128_andnot(s, c);            // keep only +ve or 0
            s = wasm_v128_or(s, v_m_sh);           // combine
            wasm_v128_store(dp, s);
          }
        }
        else
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si64 *dp = dst_line->i64 + dst_line_offset;
          v128_t sh = wasm_i64x2_splat(-shift);
          v128_t zero = wasm_i32x4_splat(0);
          for (int i = (width + 3) >> 2; i > 0; --i, sp += 4, dp += 4)
          {
            v128_t s, u, c, v_m_sh;
            s = wasm_v128_load(sp);

            u = wasm_i64x2_extend_low_i32x4(s);
            c = wasm_i64x2_lt(u, zero);        // 64b -1 for -ve value
            v_m_sh = wasm_i64x2_sub(sh, u);    // - shift - value
            v_m_sh = wasm_v128_and(c, v_m_sh); // keep only - shift - value
            u = wasm_v128_andnot(u, c);        // keep only +ve or 0
            u = wasm_v128_or(u, v_m_sh);       // combine

            wasm_v128_store(dp, u);

            u = wasm_i64x2_extend_high_i32x4(s);
            c = wasm_i64x2_lt(u, zero);        // 64b -1 for -ve value
            v_m_sh = wasm_i64x2_sub(sh, u);    // - shift - value
            v_m_sh = wasm_v128_and(c, v_m_sh); // keep only - shift - value
            u = wasm_v128_andnot(u, c);        // keep only +ve or 0
            u = wasm_v128_or(u, v_m_sh);       // combine

            wasm_v128_store(dp + 2, u);
          }
        }
      }
      else
      {
        assert(src_line->flags | line_buf::LFT_64BIT);
        assert(dst_line->flags | line_buf::LFT_32BIT);
        const si64 *sp = src_line->i64 + src_line_offset;
        si32 *dp = dst_line->i32 + dst_line_offset;
        v128_t sh = wasm_i64x2_splat(-shift);
        v128_t zero = wasm_i32x4_splat(0);
        for (int i = (width + 3) >> 2; i > 0; --i, sp += 4, dp += 4)
        {
          // s for source, t for target, p for positive, n for negative,
          // m for mask, and tm for temp
          v128_t s, t0, t1, p, n, m, tm;
          s = wasm_v128_load(sp);
          m = wasm_i64x2_lt(s, zero);   // 64b -1 for -ve value
          tm = wasm_i64x2_sub(sh, s);   // - shift - value
          n = wasm_v128_and(m, tm);     // -ve
          p = wasm_v128_andnot(s, m);   // +ve
          t0 = wasm_v128_or(n, p);

          s = wasm_v128_load(sp + 2);
          m = wasm_i64x2_lt(s, zero);   // 64b -1 for -ve value
          tm = wasm_i64x2_sub(sh, s);   // - shift - value
          n = wasm_v128_and(m, tm);     // -ve
          p = wasm_v128_andnot(s, m);   // +ve
          t1 = wasm_v128_or(n, p);

          t0 = wasm_i32x4_shuffle(t0, t1, 0, 2, 4 + 0, 4 + 2);
          wasm_v128_store(dp, t0);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_si32_to_float_shftd(const si32 *sp, float *dp, float mul,
                                        ui32 width)
    {
      v128_t shift = wasm_f32x4_splat(0.5f);
      v128_t m = wasm_f32x4_splat(mul);
      for (ui32 i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t t = wasm_v128_load(sp);
        v128_t s = wasm_f32x4_convert_i32x4(t);
        s = wasm_f32x4_mul(s, m);
        s = wasm_f32x4_sub(s, shift);
        wasm_v128_store(dp, s);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_si32_to_float(const si32 *sp, float *dp, float mul,
                                  ui32 width)
    {
      v128_t m = wasm_f32x4_splat(mul);
      for (ui32 i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t t = wasm_v128_load(sp);
        v128_t s = wasm_f32x4_convert_i32x4(t);
        s = wasm_f32x4_mul(s, m);
        wasm_v128_store(dp, s);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                        ui32 width)
    {
      const v128_t zero = wasm_f32x4_splat(0.0f);
      const v128_t half = wasm_f32x4_splat(0.5f);
      v128_t m = wasm_f32x4_splat(mul);
      for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t t = wasm_v128_load(sp);
        v128_t s = wasm_f32x4_add(t, half);
        s = wasm_f32x4_mul(s, m);
        s = wasm_f32x4_add(s, half); // + 0.5 and followed by floor next
        wasm_v128_store(dp, ojph_convert_float_to_i32(s, zero, half));
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                  ui32 width)
    {
      const v128_t zero = wasm_f32x4_splat(0.0f);
      const v128_t half = wasm_f32x4_splat(0.5f);
      v128_t m = wasm_f32x4_splat(mul);
      for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t t = wasm_v128_load(sp);
        v128_t s = wasm_f32x4_mul(t, m);
        s = wasm_f32x4_add(s, half); // + 0.5 and followed by floor next
        wasm_v128_store(dp, ojph_convert_float_to_i32(s, zero, half));
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    v128_t ojph_wasm_i32x4_max_ge(v128_t a, v128_t b, v128_t x, v128_t y)
    {
      v128_t c = wasm_f32x4_ge(x, y);    // 0xFFFFFFFF for x >= y
      v128_t d = wasm_v128_and(c, a);    // keep only a, where x >= y
      v128_t e = wasm_v128_andnot(b, c); // keep only b, where x <  y
      return wasm_v128_or(d, e);         // combine
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    v128_t ojph_wasm_i32x4_min_lt(v128_t a, v128_t b, v128_t x, v128_t y)
    {
      v128_t c = wasm_f32x4_lt(x, y);    // 0xFFFFFFFF for x < y
      v128_t d = wasm_v128_and(c, a);    // keep only a, where x <  y
      v128_t e = wasm_v128_andnot(b, c); // keep only b, where x >= y
      return wasm_v128_or(d, e);         // combine
    }

    //////////////////////////////////////////////////////////////////////////
    template <bool NLT_TYPE3>
    static inline
    void local_wasm_irv_convert_to_integer(const line_buf *src_line,
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
      v128_t mul = wasm_f32x4_splat((float)(1ull << bit_depth));
      v128_t fl_up_lim = wasm_f32x4_splat(-(float)neg_limit); // val < upper
      v128_t fl_low_lim = wasm_f32x4_splat((float)neg_limit); // val >= lower
      v128_t s32_up_lim = wasm_i32x4_splat(INT_MAX >> (32 - bit_depth));
      v128_t s32_low_lim = wasm_i32x4_splat(INT_MIN >> (32 - bit_depth));

      if (is_signed)
      {
        const v128_t zero = wasm_f32x4_splat(0.0f);
        const v128_t half = wasm_f32x4_splat(0.5f);
        v128_t bias = wasm_i32x4_splat(-(si32)((1ULL << (bit_depth - 1)) + 1));
        for (int i = (int)width; i > 0; i -= 4, sp += 4, dp += 4) {
          v128_t t = wasm_v128_load(sp);
          t = wasm_f32x4_mul(t, mul);
          v128_t u = ojph_convert_float_to_i32(t, zero, half);
          u = ojph_wasm_i32x4_max_ge(u, s32_low_lim, t, fl_low_lim);
          u = ojph_wasm_i32x4_min_lt(u, s32_up_lim, t, fl_up_lim);
          if (NLT_TYPE3)
          {
            v128_t c = wasm_i32x4_gt(zero, u);    // 0xFFFFFFFF for -ve value
            v128_t neg = wasm_i32x4_sub(bias, u); // -bias -value
            neg = wasm_v128_and(c, neg);          // keep only - bias - value
            u = wasm_v128_andnot(u, c);           // keep only +ve or 0
            u = wasm_v128_or(neg, u);             // combine
          }
          wasm_v128_store(dp, u);
        }
      }
      else
      {
        const v128_t zero = wasm_f32x4_splat(0.0f);
        const v128_t half = wasm_f32x4_splat(0.5f);
        v128_t ihalf = wasm_i32x4_splat((si32)(1ULL << (bit_depth - 1)));
        for (int i = (int)width; i > 0; i -= 4, sp += 4, dp += 4) {
          v128_t t = wasm_v128_load(sp);
          t = wasm_f32x4_mul(t, mul);
          v128_t u = ojph_convert_float_to_i32(t, zero, half);
          u = ojph_wasm_i32x4_max_ge(u, s32_low_lim, t, fl_low_lim);
          u = ojph_wasm_i32x4_min_lt(u, s32_up_lim, t, fl_up_lim);
          u = wasm_i32x4_add(u, ihalf);
          wasm_v128_store(dp, u);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_irv_convert_to_integer(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_wasm_irv_convert_to_integer<false>(src_line, dst_line, 
        dst_line_offset, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_irv_convert_to_integer_nlt_type3(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_wasm_irv_convert_to_integer<true>(src_line, dst_line, 
        dst_line_offset, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    template <bool NLT_TYPE3>
    static inline
    void local_wasm_irv_convert_to_float(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      assert((src_line->flags & line_buf::LFT_32BIT) &&
             (src_line->flags & line_buf::LFT_INTEGER) &&
             (dst_line->flags & line_buf::LFT_32BIT) &&
             (dst_line->flags & line_buf::LFT_INTEGER) == 0);

      assert(bit_depth <= 32);
      v128_t mul = wasm_f32x4_splat((float)(1.0 / (double)(1ULL << bit_depth)));

      const si32* sp = src_line->i32 + src_line_offset;
      float* dp = dst_line->f32;
      if (is_signed)
      {
        v128_t zero = wasm_i32x4_splat(0);
        v128_t bias = wasm_i32x4_splat(-(si32)((1ULL << (bit_depth - 1)) + 1));
        for (int i = (int)width; i > 0; i -= 4, sp += 4, dp += 4) {
          v128_t t = wasm_v128_load(sp);
          if (NLT_TYPE3)
          {
            v128_t c = wasm_i32x4_lt(t, zero);    // 0xFFFFFFFF for -ve value
            v128_t neg = wasm_i32x4_sub(bias, t); // - bias - value
            neg = wasm_v128_and(c, neg);          // keep only - bias - value
            c = wasm_v128_andnot(t, c);           // keep only +ve or 0
            t = wasm_v128_or(neg, c);             // combine
          }
          v128_t v = wasm_f32x4_convert_i32x4(t);
          v = wasm_f32x4_mul(v, mul);
          wasm_v128_store(dp, v);
        }
      }
      else
      {
        v128_t half = wasm_i32x4_splat((si32)(1ULL << (bit_depth - 1)));
        for (int i = (int)width; i > 0; i -= 4, sp += 4, dp += 4) {
          v128_t t = wasm_v128_load(sp);
          t = wasm_i32x4_sub(t, half);
          v128_t v = wasm_f32x4_convert_i32x4(t);
          v = wasm_f32x4_mul(v, mul);
          wasm_v128_store(dp, v);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_irv_convert_to_float(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_wasm_irv_convert_to_float<false>(src_line, src_line_offset,
        dst_line, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_irv_convert_to_float_nlt_type3(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_wasm_irv_convert_to_float<true>(src_line, src_line_offset,
        dst_line, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rct_forward(const line_buf *r,
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
          v128_t mr = wasm_v128_load(rp);
          v128_t mg = wasm_v128_load(gp);
          v128_t mb = wasm_v128_load(bp);
          v128_t t = wasm_i32x4_add(mr, mb);
          t = wasm_i32x4_add(t, wasm_i32x4_shl(mg, 1));
          wasm_v128_store(yp, wasm_i32x4_shr(t, 2));
          t = wasm_i32x4_sub(mb, mg);
          wasm_v128_store(cbp, t);
          t = wasm_i32x4_sub(mr, mg);
          wasm_v128_store(crp, t);

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
        const si32 *rp = r->i32, *gp = g->i32, *bp = b->i32;
        si64 *yp = y->i64, *cbp = cb->i64, *crp = cr->i64;
        for (int i = (repeat + 3) >> 2; i > 0; --i)
        {
          v128_t mr32 = wasm_v128_load(rp);
          v128_t mg32 = wasm_v128_load(gp);
          v128_t mb32 = wasm_v128_load(bp);
          v128_t mr, mg, mb, t;
          mr = wasm_i64x2_extend_low_i32x4(mr32);
          mg = wasm_i64x2_extend_low_i32x4(mg32);
          mb = wasm_i64x2_extend_low_i32x4(mb32);

          t = wasm_i64x2_add(mr, mb);
          t = wasm_i64x2_add(t, wasm_i64x2_shl(mg, 1));
          wasm_v128_store(yp, wasm_i64x2_shr(t, 2));
          t = wasm_i64x2_sub(mb, mg);
          wasm_v128_store(cbp, t);
          t = wasm_i64x2_sub(mr, mg);
          wasm_v128_store(crp, t);

          yp += 2; cbp += 2; crp += 2;

          mr = wasm_i64x2_extend_high_i32x4(mr32);
          mg = wasm_i64x2_extend_high_i32x4(mg32);
          mb = wasm_i64x2_extend_high_i32x4(mb32);

          t = wasm_i64x2_add(mr, mb);
          t = wasm_i64x2_add(t, wasm_i64x2_shl(mg, 1));
          wasm_v128_store(yp, wasm_i64x2_shr(t, 2));
          t = wasm_i64x2_sub(mb, mg);
          wasm_v128_store(cbp, t);
          t = wasm_i64x2_sub(mr, mg);
          wasm_v128_store(crp, t);

          rp += 4; gp += 4; bp += 4;
          yp += 2; cbp += 2; crp += 2;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rct_backward(const line_buf *y,
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
          v128_t my  = wasm_v128_load(yp);
          v128_t mcb = wasm_v128_load(cbp);
          v128_t mcr = wasm_v128_load(crp);

          v128_t t = wasm_i32x4_add(mcb, mcr);
          t = wasm_i32x4_sub(my, wasm_i32x4_shr(t, 2));
          wasm_v128_store(gp, t);
          v128_t u = wasm_i32x4_add(mcb, t);
          wasm_v128_store(bp, u);
          u = wasm_i32x4_add(mcr, t);
          wasm_v128_store(rp, u);

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
        const si64 *yp = y->i64, *cbp = cb->i64, *crp = cr->i64;
        si32 *rp = r->i32, *gp = g->i32, *bp = b->i32;
        for (int i = (repeat + 3) >> 2; i > 0; --i)
        {
          v128_t my, mcb, mcr, tr0, tg0, tb0, tr1, tg1, tb1;
          my  = wasm_v128_load(yp);
          mcb = wasm_v128_load(cbp);
          mcr = wasm_v128_load(crp);

          tg0 = wasm_i64x2_add(mcb, mcr);
          tg0 = wasm_i64x2_sub(my, wasm_i64x2_shr(tg0, 2));
          tb0 = wasm_i64x2_add(mcb, tg0);
          tr0 = wasm_i64x2_add(mcr, tg0);

          yp += 2; cbp += 2; crp += 2;

          my  = wasm_v128_load(yp);
          mcb = wasm_v128_load(cbp);
          mcr = wasm_v128_load(crp);

          tg1 = wasm_i64x2_add(mcb, mcr);
          tg1 = wasm_i64x2_sub(my, wasm_i64x2_shr(tg1, 2));
          tb1 = wasm_i64x2_add(mcb, tg1);
          tr1 = wasm_i64x2_add(mcr, tg1);

          tr0 = wasm_i32x4_shuffle(tr0, tr1, 0, 2, 4 + 0, 4 + 2);
          tg0 = wasm_i32x4_shuffle(tg0, tg1, 0, 2, 4 + 0, 4 + 2);
          tb0 = wasm_i32x4_shuffle(tb0, tb1, 0, 2, 4 + 0, 4 + 2);

          wasm_v128_store(rp, tr0);
          wasm_v128_store(gp, tg0);
          wasm_v128_store(bp, tb0);

          yp += 2; cbp += 2; crp += 2;
          rp += 4; gp += 4; bp += 4;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_ict_forward(const float *r, const float *g, const float *b,
                          float *y, float *cb, float *cr, ui32 repeat)
    {
      v128_t alpha_rf = wasm_f32x4_splat(CT_CNST::ALPHA_RF);
      v128_t alpha_gf = wasm_f32x4_splat(CT_CNST::ALPHA_GF);
      v128_t alpha_bf = wasm_f32x4_splat(CT_CNST::ALPHA_BF);
      v128_t beta_cbf = wasm_f32x4_splat(CT_CNST::BETA_CbF);
      v128_t beta_crf = wasm_f32x4_splat(CT_CNST::BETA_CrF);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i)
      {
        v128_t mr = wasm_v128_load(r);
        v128_t mb = wasm_v128_load(b);
        v128_t my = wasm_f32x4_mul(alpha_rf, mr);
        my = wasm_f32x4_add(my, wasm_f32x4_mul(alpha_gf, wasm_v128_load(g)));
        my = wasm_f32x4_add(my, wasm_f32x4_mul(alpha_bf, mb));
        wasm_v128_store(y, my);
        wasm_v128_store(cb, wasm_f32x4_mul(beta_cbf, wasm_f32x4_sub(mb, my)));
        wasm_v128_store(cr, wasm_f32x4_mul(beta_crf, wasm_f32x4_sub(mr, my)));

        r += 4; g += 4; b += 4;
        y += 4; cb += 4; cr += 4;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_ict_backward(const float *y, const float *cb, const float *cr,
                           float *r, float *g, float *b, ui32 repeat)
    {
      v128_t gamma_cr2g = wasm_f32x4_splat(CT_CNST::GAMMA_CR2G);
      v128_t gamma_cb2g = wasm_f32x4_splat(CT_CNST::GAMMA_CB2G);
      v128_t gamma_cr2r = wasm_f32x4_splat(CT_CNST::GAMMA_CR2R);
      v128_t gamma_cb2b = wasm_f32x4_splat(CT_CNST::GAMMA_CB2B);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i)
      {
        v128_t my = wasm_v128_load(y);
        v128_t mcr = wasm_v128_load(cr);
        v128_t mcb = wasm_v128_load(cb);
        v128_t mg = wasm_f32x4_sub(my, wasm_f32x4_mul(gamma_cr2g, mcr));
        wasm_v128_store(g, wasm_f32x4_sub(mg, wasm_f32x4_mul(gamma_cb2g, mcb)));
        wasm_v128_store(r, wasm_f32x4_add(my, wasm_f32x4_mul(gamma_cr2r, mcr)));
        wasm_v128_store(b, wasm_f32x4_add(my, wasm_f32x4_mul(gamma_cb2b, mcb)));

        y += 4; cb += 4; cr += 4;
        r += 4; g += 4; b += 4;
      }
    }

  }
}
