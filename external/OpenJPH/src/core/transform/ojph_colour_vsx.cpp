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
// File: ojph_colour_vsx.cpp
// Author: Aous Naman
// Date: 9 February 2021
//***************************************************************************/

#include <climits>
#include <cmath>
#include "ojph_simd_vsx.h"

#include "ojph_defs.h"
#include "ojph_mem.h"
#include "ojph_colour.h"
#include "ojph_colour_local.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    static inline
    v128_t ojph_convert_float_to_i32(v128_t a)
    { // We implement ojph_round, which is
      // val + (val >= 0.0f ? 0.5f : -0.5f), where val is float; this is
      // round to nearest with ties away from zero, which is exactly what
      // xvrspi does.  The instruction is used via inline asm because
      // GCC's vec_round rounds ties to even.
      vsx_v_f32 w;
      __asm__("xvrspi %x0,%x1" : "=wa"(w) : "wa"((vsx_v_f32)a));
      return (v128_t)vec_cts(w, 0);     // saturating convert to int32
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_rev_convert(const line_buf *src_line,
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
          v128_t sh = vsx_i32x4_splat((si32)shift);
          for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
          {
            v128_t s = vsx_v128_load(sp);
            s = vsx_i32x4_add(s, sh);
            vsx_v128_store(dp, s);
          }
        }
        else
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si64 *dp = dst_line->i64 + dst_line_offset;
          v128_t sh = vsx_i64x2_splat(shift);
          for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
          {
            v128_t s, t;
            s = vsx_v128_load(sp);

            t = vsx_i64x2_extend_low_i32x4(s);
            t = vsx_i64x2_add(t, sh);
            vsx_v128_store(dp, t);

            t = vsx_i64x2_extend_high_i32x4(s);
            t = vsx_i64x2_add(t, sh);
            vsx_v128_store(dp + 2, t);
          }
        }
      }
      else
      {
        assert(src_line->flags | line_buf::LFT_64BIT);
        assert(dst_line->flags | line_buf::LFT_32BIT);
        const si64 *sp = src_line->i64 + src_line_offset;
        si32 *dp = dst_line->i32 + dst_line_offset;
        v128_t sh = vsx_i64x2_splat(shift);
        for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
        {
          v128_t s0, s1;
          s0 = vsx_v128_load(sp);
          s0 = vsx_i64x2_add(s0, sh);
          s1 = vsx_v128_load(sp + 2);
          s1 = vsx_i64x2_add(s1, sh);
          s0 = vsx_i32x4_shuffle(s0, s1, 0, 2, 4 + 0, 4 + 2);
          vsx_v128_store(dp, s0);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_rev_convert_nlt_type3(const line_buf *src_line,
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
          v128_t sh = vsx_i32x4_splat((si32)(-shift));
          v128_t zero = vsx_i32x4_splat(0);
          for (int i = (width + 3) >> 2; i > 0; --i, sp += 4, dp += 4)
          {
            v128_t s = vsx_v128_load(sp);
            v128_t c = vsx_i32x4_lt(s, zero);     // 0xFFFFFFFF for -ve value
            v128_t v_m_sh = vsx_i32x4_sub(sh, s); // - shift - value
            v_m_sh = vsx_v128_and(c, v_m_sh);     // keep only - shift - value
            s = vsx_v128_andnot(s, c);            // keep only +ve or 0
            s = vsx_v128_or(s, v_m_sh);           // combine
            vsx_v128_store(dp, s);
          }
        }
        else
        {
          const si32 *sp = src_line->i32 + src_line_offset;
          si64 *dp = dst_line->i64 + dst_line_offset;
          v128_t sh = vsx_i64x2_splat(-shift);
          v128_t zero = vsx_i32x4_splat(0);
          for (int i = (width + 3) >> 2; i > 0; --i, sp += 4, dp += 4)
          {
            v128_t s, u, c, v_m_sh;
            s = vsx_v128_load(sp);

            u = vsx_i64x2_extend_low_i32x4(s);
            c = vsx_i64x2_lt(u, zero);        // 64b -1 for -ve value
            v_m_sh = vsx_i64x2_sub(sh, u);    // - shift - value
            v_m_sh = vsx_v128_and(c, v_m_sh); // keep only - shift - value
            u = vsx_v128_andnot(u, c);        // keep only +ve or 0
            u = vsx_v128_or(u, v_m_sh);       // combine

            vsx_v128_store(dp, u);

            u = vsx_i64x2_extend_high_i32x4(s);
            c = vsx_i64x2_lt(u, zero);        // 64b -1 for -ve value
            v_m_sh = vsx_i64x2_sub(sh, u);    // - shift - value
            v_m_sh = vsx_v128_and(c, v_m_sh); // keep only - shift - value
            u = vsx_v128_andnot(u, c);        // keep only +ve or 0
            u = vsx_v128_or(u, v_m_sh);       // combine

            vsx_v128_store(dp + 2, u);
          }
        }
      }
      else
      {
        assert(src_line->flags | line_buf::LFT_64BIT);
        assert(dst_line->flags | line_buf::LFT_32BIT);
        const si64 *sp = src_line->i64 + src_line_offset;
        si32 *dp = dst_line->i32 + dst_line_offset;
        v128_t sh = vsx_i64x2_splat(-shift);
        v128_t zero = vsx_i32x4_splat(0);
        for (int i = (width + 3) >> 2; i > 0; --i, sp += 4, dp += 4)
        {
          // s for source, t for target, p for positive, n for negative,
          // m for mask, and tm for temp
          v128_t s, t0, t1, p, n, m, tm;
          s = vsx_v128_load(sp);
          m = vsx_i64x2_lt(s, zero);   // 64b -1 for -ve value
          tm = vsx_i64x2_sub(sh, s);   // - shift - value
          n = vsx_v128_and(m, tm);     // -ve
          p = vsx_v128_andnot(s, m);   // +ve
          t0 = vsx_v128_or(n, p);

          s = vsx_v128_load(sp + 2);
          m = vsx_i64x2_lt(s, zero);   // 64b -1 for -ve value
          tm = vsx_i64x2_sub(sh, s);   // - shift - value
          n = vsx_v128_and(m, tm);     // -ve
          p = vsx_v128_andnot(s, m);   // +ve
          t1 = vsx_v128_or(n, p);

          t0 = vsx_i32x4_shuffle(t0, t1, 0, 2, 4 + 0, 4 + 2);
          vsx_v128_store(dp, t0);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_cnvrt_si32_to_float_shftd(const si32 *sp, float *dp, float mul,
                                        ui32 width)
    {
      v128_t shift = vsx_f32x4_splat(0.5f);
      v128_t m = vsx_f32x4_splat(mul);
      for (ui32 i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t t = vsx_v128_load(sp);
        v128_t s = vsx_f32x4_convert_i32x4(t);
        s = vsx_f32x4_mul(s, m);
        s = vsx_f32x4_sub(s, shift);
        vsx_v128_store(dp, s);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_cnvrt_si32_to_float(const si32 *sp, float *dp, float mul,
                                  ui32 width)
    {
      v128_t m = vsx_f32x4_splat(mul);
      for (ui32 i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t t = vsx_v128_load(sp);
        v128_t s = vsx_f32x4_convert_i32x4(t);
        s = vsx_f32x4_mul(s, m);
        vsx_v128_store(dp, s);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_cnvrt_float_to_si32_shftd(const float *sp, si32 *dp, float mul,
                                        ui32 width)
    {
      const v128_t half = vsx_f32x4_splat(0.5f);
      v128_t m = vsx_f32x4_splat(mul);
      for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t t = vsx_v128_load(sp);
        v128_t s = vsx_f32x4_add(t, half);
        s = vsx_f32x4_mul(s, m);
        s = vsx_f32x4_add(s, half); // + 0.5 and followed by floor next
        vsx_v128_store(dp, ojph_convert_float_to_i32(s));
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_cnvrt_float_to_si32(const float *sp, si32 *dp, float mul,
                                  ui32 width)
    {
      const v128_t half = vsx_f32x4_splat(0.5f);
      v128_t m = vsx_f32x4_splat(mul);
      for (int i = (width + 3) >> 2; i > 0; --i, sp+=4, dp+=4)
      {
        v128_t t = vsx_v128_load(sp);
        v128_t s = vsx_f32x4_mul(t, m);
        s = vsx_f32x4_add(s, half); // + 0.5 and followed by floor next
        vsx_v128_store(dp, ojph_convert_float_to_i32(s));
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    v128_t ojph_vsx_i32x4_max_ge(v128_t a, v128_t b, v128_t x, v128_t y)
    {
      v128_t c = vsx_f32x4_ge(x, y);    // 0xFFFFFFFF for x >= y
      return (v128_t)vec_sel((vsx_v_u32)b, (vsx_v_u32)a, (vsx_v_u32)c);
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    v128_t ojph_vsx_i32x4_min_lt(v128_t a, v128_t b, v128_t x, v128_t y)
    {
      v128_t c = vsx_f32x4_lt(x, y);    // 0xFFFFFFFF for x < y
      return (v128_t)vec_sel((vsx_v_u32)b, (vsx_v_u32)a, (vsx_v_u32)c);
    }

    //////////////////////////////////////////////////////////////////////////
    template <bool NLT_TYPE3>
    static inline
    void local_vsx_irv_convert_to_integer(const line_buf *src_line,
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
      v128_t mul = vsx_f32x4_splat((float)(1ull << bit_depth));
      v128_t fl_up_lim = vsx_f32x4_splat(-(float)neg_limit); // val < upper
      v128_t fl_low_lim = vsx_f32x4_splat((float)neg_limit); // val >= lower
      v128_t s32_up_lim = vsx_i32x4_splat(INT_MAX >> (32 - bit_depth));
      v128_t s32_low_lim = vsx_i32x4_splat(INT_MIN >> (32 - bit_depth));

      if (is_signed)
      {
        const v128_t zero = vsx_f32x4_splat(0.0f);
        v128_t bias = vsx_i32x4_splat(-(si32)((1ULL << (bit_depth - 1)) + 1));
        for (int i = (int)width; i > 0; i -= 4, sp += 4, dp += 4) {
          v128_t t = vsx_v128_load(sp);
          t = vsx_f32x4_mul(t, mul);
          v128_t u = ojph_convert_float_to_i32(t);
          u = ojph_vsx_i32x4_max_ge(u, s32_low_lim, t, fl_low_lim);
          u = ojph_vsx_i32x4_min_lt(u, s32_up_lim, t, fl_up_lim);
          if (NLT_TYPE3)
          {
            v128_t c = vsx_i32x4_gt(zero, u);    // 0xFFFFFFFF for -ve value
            v128_t neg = vsx_i32x4_sub(bias, u); // -bias -value
            neg = vsx_v128_and(c, neg);          // keep only - bias - value
            u = vsx_v128_andnot(u, c);           // keep only +ve or 0
            u = vsx_v128_or(neg, u);             // combine
          }
          vsx_v128_store(dp, u);
        }
      }
      else
      {
        v128_t ihalf = vsx_i32x4_splat((si32)(1ULL << (bit_depth - 1)));
        for (int i = (int)width; i > 0; i -= 4, sp += 4, dp += 4) {
          v128_t t = vsx_v128_load(sp);
          t = vsx_f32x4_mul(t, mul);
          v128_t u = ojph_convert_float_to_i32(t);
          u = ojph_vsx_i32x4_max_ge(u, s32_low_lim, t, fl_low_lim);
          u = ojph_vsx_i32x4_min_lt(u, s32_up_lim, t, fl_up_lim);
          u = vsx_i32x4_add(u, ihalf);
          vsx_v128_store(dp, u);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_irv_convert_to_integer(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_vsx_irv_convert_to_integer<false>(src_line, dst_line, 
        dst_line_offset, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_irv_convert_to_integer_nlt_type3(const line_buf *src_line,
      line_buf *dst_line, ui32 dst_line_offset,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_vsx_irv_convert_to_integer<true>(src_line, dst_line, 
        dst_line_offset, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    template <bool NLT_TYPE3>
    static inline
    void local_vsx_irv_convert_to_float(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      assert((src_line->flags & line_buf::LFT_32BIT) &&
             (src_line->flags & line_buf::LFT_INTEGER) &&
             (dst_line->flags & line_buf::LFT_32BIT) &&
             (dst_line->flags & line_buf::LFT_INTEGER) == 0);

      assert(bit_depth <= 32);
      v128_t mul = vsx_f32x4_splat((float)(1.0 / (double)(1ULL << bit_depth)));

      const si32* sp = src_line->i32 + src_line_offset;
      float* dp = dst_line->f32;
      if (is_signed)
      {
        v128_t zero = vsx_i32x4_splat(0);
        v128_t bias = vsx_i32x4_splat(-(si32)((1ULL << (bit_depth - 1)) + 1));
        for (int i = (int)width; i > 0; i -= 4, sp += 4, dp += 4) {
          v128_t t = vsx_v128_load(sp);
          if (NLT_TYPE3)
          {
            v128_t c = vsx_i32x4_lt(t, zero);    // 0xFFFFFFFF for -ve value
            v128_t neg = vsx_i32x4_sub(bias, t); // - bias - value
            neg = vsx_v128_and(c, neg);          // keep only - bias - value
            c = vsx_v128_andnot(t, c);           // keep only +ve or 0
            t = vsx_v128_or(neg, c);             // combine
          }
          v128_t v = vsx_f32x4_convert_i32x4(t);
          v = vsx_f32x4_mul(v, mul);
          vsx_v128_store(dp, v);
        }
      }
      else
      {
        v128_t half = vsx_i32x4_splat((si32)(1ULL << (bit_depth - 1)));
        for (int i = (int)width; i > 0; i -= 4, sp += 4, dp += 4) {
          v128_t t = vsx_v128_load(sp);
          t = vsx_i32x4_sub(t, half);
          v128_t v = vsx_f32x4_convert_i32x4(t);
          v = vsx_f32x4_mul(v, mul);
          vsx_v128_store(dp, v);
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_irv_convert_to_float(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_vsx_irv_convert_to_float<false>(src_line, src_line_offset,
        dst_line, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_irv_convert_to_float_nlt_type3(const line_buf *src_line,
      ui32 src_line_offset, line_buf *dst_line,
      ui32 bit_depth, bool is_signed, ui32 width)
    {
      local_vsx_irv_convert_to_float<true>(src_line, src_line_offset,
        dst_line, bit_depth, is_signed, width);
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_rct_forward(const line_buf *r,
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
          v128_t mr = vsx_v128_load(rp);
          v128_t mg = vsx_v128_load(gp);
          v128_t mb = vsx_v128_load(bp);
          v128_t t = vsx_i32x4_add(mr, mb);
          t = vsx_i32x4_add(t, vsx_i32x4_shl(mg, 1));
          vsx_v128_store(yp, vsx_i32x4_shr(t, 2));
          t = vsx_i32x4_sub(mb, mg);
          vsx_v128_store(cbp, t);
          t = vsx_i32x4_sub(mr, mg);
          vsx_v128_store(crp, t);

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
          v128_t mr32 = vsx_v128_load(rp);
          v128_t mg32 = vsx_v128_load(gp);
          v128_t mb32 = vsx_v128_load(bp);
          v128_t mr, mg, mb, t;
          mr = vsx_i64x2_extend_low_i32x4(mr32);
          mg = vsx_i64x2_extend_low_i32x4(mg32);
          mb = vsx_i64x2_extend_low_i32x4(mb32);

          t = vsx_i64x2_add(mr, mb);
          t = vsx_i64x2_add(t, vsx_i64x2_shl(mg, 1));
          vsx_v128_store(yp, vsx_i64x2_shr(t, 2));
          t = vsx_i64x2_sub(mb, mg);
          vsx_v128_store(cbp, t);
          t = vsx_i64x2_sub(mr, mg);
          vsx_v128_store(crp, t);

          yp += 2; cbp += 2; crp += 2;

          mr = vsx_i64x2_extend_high_i32x4(mr32);
          mg = vsx_i64x2_extend_high_i32x4(mg32);
          mb = vsx_i64x2_extend_high_i32x4(mb32);

          t = vsx_i64x2_add(mr, mb);
          t = vsx_i64x2_add(t, vsx_i64x2_shl(mg, 1));
          vsx_v128_store(yp, vsx_i64x2_shr(t, 2));
          t = vsx_i64x2_sub(mb, mg);
          vsx_v128_store(cbp, t);
          t = vsx_i64x2_sub(mr, mg);
          vsx_v128_store(crp, t);

          rp += 4; gp += 4; bp += 4;
          yp += 2; cbp += 2; crp += 2;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_rct_backward(const line_buf *y,
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
          v128_t my  = vsx_v128_load(yp);
          v128_t mcb = vsx_v128_load(cbp);
          v128_t mcr = vsx_v128_load(crp);

          v128_t t = vsx_i32x4_add(mcb, mcr);
          t = vsx_i32x4_sub(my, vsx_i32x4_shr(t, 2));
          vsx_v128_store(gp, t);
          v128_t u = vsx_i32x4_add(mcb, t);
          vsx_v128_store(bp, u);
          u = vsx_i32x4_add(mcr, t);
          vsx_v128_store(rp, u);

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
          my  = vsx_v128_load(yp);
          mcb = vsx_v128_load(cbp);
          mcr = vsx_v128_load(crp);

          tg0 = vsx_i64x2_add(mcb, mcr);
          tg0 = vsx_i64x2_sub(my, vsx_i64x2_shr(tg0, 2));
          tb0 = vsx_i64x2_add(mcb, tg0);
          tr0 = vsx_i64x2_add(mcr, tg0);

          yp += 2; cbp += 2; crp += 2;

          my  = vsx_v128_load(yp);
          mcb = vsx_v128_load(cbp);
          mcr = vsx_v128_load(crp);

          tg1 = vsx_i64x2_add(mcb, mcr);
          tg1 = vsx_i64x2_sub(my, vsx_i64x2_shr(tg1, 2));
          tb1 = vsx_i64x2_add(mcb, tg1);
          tr1 = vsx_i64x2_add(mcr, tg1);

          tr0 = vsx_i32x4_shuffle(tr0, tr1, 0, 2, 4 + 0, 4 + 2);
          tg0 = vsx_i32x4_shuffle(tg0, tg1, 0, 2, 4 + 0, 4 + 2);
          tb0 = vsx_i32x4_shuffle(tb0, tb1, 0, 2, 4 + 0, 4 + 2);

          vsx_v128_store(rp, tr0);
          vsx_v128_store(gp, tg0);
          vsx_v128_store(bp, tb0);

          yp += 2; cbp += 2; crp += 2;
          rp += 4; gp += 4; bp += 4;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_ict_forward(const float *r, const float *g, const float *b,
                          float *y, float *cb, float *cr, ui32 repeat)
    {
      v128_t alpha_rf = vsx_f32x4_splat(CT_CNST::ALPHA_RF);
      v128_t alpha_gf = vsx_f32x4_splat(CT_CNST::ALPHA_GF);
      v128_t alpha_bf = vsx_f32x4_splat(CT_CNST::ALPHA_BF);
      v128_t beta_cbf = vsx_f32x4_splat(CT_CNST::BETA_CbF);
      v128_t beta_crf = vsx_f32x4_splat(CT_CNST::BETA_CrF);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i)
      {
        v128_t mr = vsx_v128_load(r);
        v128_t mb = vsx_v128_load(b);
        v128_t my = vsx_f32x4_mul(alpha_rf, mr);
        my = vsx_f32x4_add(my, vsx_f32x4_mul(alpha_gf, vsx_v128_load(g)));
        my = vsx_f32x4_add(my, vsx_f32x4_mul(alpha_bf, mb));
        vsx_v128_store(y, my);
        vsx_v128_store(cb, vsx_f32x4_mul(beta_cbf, vsx_f32x4_sub(mb, my)));
        vsx_v128_store(cr, vsx_f32x4_mul(beta_crf, vsx_f32x4_sub(mr, my)));

        r += 4; g += 4; b += 4;
        y += 4; cb += 4; cr += 4;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void vsx_ict_backward(const float *y, const float *cb, const float *cr,
                           float *r, float *g, float *b, ui32 repeat)
    {
      v128_t gamma_cr2g = vsx_f32x4_splat(CT_CNST::GAMMA_CR2G);
      v128_t gamma_cb2g = vsx_f32x4_splat(CT_CNST::GAMMA_CB2G);
      v128_t gamma_cr2r = vsx_f32x4_splat(CT_CNST::GAMMA_CR2R);
      v128_t gamma_cb2b = vsx_f32x4_splat(CT_CNST::GAMMA_CB2B);
      for (ui32 i = (repeat + 3) >> 2; i > 0; --i)
      {
        v128_t my = vsx_v128_load(y);
        v128_t mcr = vsx_v128_load(cr);
        v128_t mcb = vsx_v128_load(cb);
        v128_t mg = vsx_f32x4_sub(my, vsx_f32x4_mul(gamma_cr2g, mcr));
        vsx_v128_store(g, vsx_f32x4_sub(mg, vsx_f32x4_mul(gamma_cb2g, mcb)));
        vsx_v128_store(r, vsx_f32x4_add(my, vsx_f32x4_mul(gamma_cr2r, mcr)));
        vsx_v128_store(b, vsx_f32x4_add(my, vsx_f32x4_mul(gamma_cb2b, mcb)));

        y += 4; cb += 4; cr += 4;
        r += 4; g += 4; b += 4;
      }
    }

  }
}
