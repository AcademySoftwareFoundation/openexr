//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2022, Aous Naman 
// Copyright (c) 2022, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2022, The University of New South Wales, Australia
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
// File: ojph_codestream_wasm.cpp
// Author: Aous Naman
// Date: 15 May 2022
//***************************************************************************/

#include <climits>
#include <cstddef> 
#include <wasm_simd128.h>

#include "ojph_defs.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void wasm_mem_clear(void* addr, size_t count)
    {
      float* p = (float*)addr;
      v128_t zero = wasm_i32x4_splat(0);
      for (size_t i = 0; i < count; i += 16, p += 4)
        wasm_v128_store(p, zero);
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 wasm_find_max_val32(ui32* address)
    {
      v128_t x1, x0 = wasm_v128_load(address);
      x1 = wasm_i32x4_shuffle(x0, x0, 2, 3, 2, 3);   // x1 = x0[2,3,2,3]
      x0 = wasm_v128_or(x0, x1);
      x1 = wasm_i32x4_shuffle(x0, x0, 1, 1, 1, 1);   // x1 = x0[1,1,1,1]
      x0 = wasm_v128_or(x0, x1);
      ui32 t = (ui32)wasm_i32x4_extract_lane(x0, 0);
      return t;
    }

    //////////////////////////////////////////////////////////////////////////
    ui64 wasm_find_max_val64(ui64* address)
    {
      v128_t x1, x0 = wasm_v128_load(address);
      x1 = wasm_i64x2_shuffle(x0, x0, 1, 1);   // x1 = x0[2,3,2,3]
      x0 = wasm_v128_or(x0, x1);
      ui64 t = (ui64)wasm_i64x2_extract_lane(x0, 0);
      return t;
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max, 
                             float delta_inv, ui32 count, ui32* max_val)
    {
      ojph_unused(delta_inv);

      // convert to sign and magnitude and keep max_val      
      ui32 shift = 31 - K_max;
      v128_t m0 = wasm_i32x4_splat(INT_MIN);
      v128_t zero = wasm_i32x4_splat(0);
      v128_t one = wasm_i32x4_splat(1);
      v128_t tmax = wasm_v128_load(max_val);
      si32 *p = (si32*)sp;
      for ( ; count >= 4; count -= 4, p += 4, dp += 4)
      {
        v128_t v = wasm_v128_load(p);
        v128_t sign = wasm_i32x4_lt(v, zero);
        v128_t val = wasm_v128_xor(v, sign); // negate 1's complement
        v128_t ones = wasm_v128_and(sign, one);
        val = wasm_i32x4_add(val, ones);     // 2's complement
        sign = wasm_v128_and(sign, m0);
        val = wasm_i32x4_shl(val, shift);
        tmax = wasm_v128_or(tmax, val);
        val = wasm_v128_or(val, sign);
        wasm_v128_store(dp, val);
      }
      if (count)
      {
        v128_t v = wasm_v128_load(p);
        v128_t sign = wasm_i32x4_lt(v, zero);
        v128_t val = wasm_v128_xor(v, sign); // negate 1's complement
        v128_t ones = wasm_v128_and(sign, one);
        val = wasm_i32x4_add(val, ones);     // 2's complement
        sign = wasm_v128_and(sign, m0);
        val = wasm_i32x4_shl(val, shift);

        v128_t c = wasm_i32x4_splat((si32)count);
        v128_t idx = wasm_i32x4_make(0, 1, 2, 3);
        v128_t mask = wasm_i32x4_gt(c, idx);
        c = wasm_v128_and(val, mask);
        tmax = wasm_v128_or(tmax, c);

        val = wasm_v128_or(val, sign);
        wasm_v128_store(dp, val);
      }
      wasm_v128_store(max_val, tmax);
    }
                           
    //////////////////////////////////////////////////////////////////////////
    void wasm_irv_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui32* max_val)
    {
      ojph_unused(K_max);

      //quantize and convert to sign and magnitude and keep max_val

      v128_t d = wasm_f32x4_splat(delta_inv);
      v128_t zero = wasm_i32x4_splat(0);
      v128_t one = wasm_i32x4_splat(1);
      v128_t tmax = wasm_v128_load(max_val);
      float *p = (float*)sp;
      for ( ; count >= 4; count -= 4, p += 4, dp += 4)
      {
        v128_t vf = wasm_v128_load(p);
        vf = wasm_f32x4_mul(vf, d);                   // multiply
        v128_t val = wasm_i32x4_trunc_sat_f32x4(vf);  // convert to signed int
        v128_t sign = wasm_i32x4_lt(val, zero);       // get sign
        val = wasm_v128_xor(val, sign);               // negate 1's complement
        v128_t ones = wasm_v128_and(sign, one);
        val = wasm_i32x4_add(val, ones);              // 2's complement
        tmax = wasm_v128_or(tmax, val);
        sign = wasm_i32x4_shl(sign, 31);
        val = wasm_v128_or(val, sign);
        wasm_v128_store(dp, val);
      }
      if (count)
      {
        v128_t vf = wasm_v128_load(p);
        vf = wasm_f32x4_mul(vf, d);                   // multiply
        v128_t val = wasm_i32x4_trunc_sat_f32x4(vf);  // convert to signed int
        v128_t sign = wasm_i32x4_lt(val, zero);       // get sign
        val = wasm_v128_xor(val, sign);               // negate 1's complement
        v128_t ones = wasm_v128_and(sign, one);
        val = wasm_i32x4_add(val, ones);              // 2's complement

        v128_t c = wasm_i32x4_splat((si32)count);
        v128_t idx = wasm_i32x4_make(0, 1, 2, 3);
        v128_t mask = wasm_i32x4_gt(c, idx);
        c = wasm_v128_and(val, mask);
        tmax = wasm_v128_or(tmax, c);

        sign = wasm_i32x4_shl(sign, 31);
        val = wasm_v128_or(val, sign);
        wasm_v128_store(dp, val);
      }
      wasm_v128_store(max_val, tmax);
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max, 
                               float delta, ui32 count)
    {
      ojph_unused(delta);
      ui32 shift = 31 - K_max;
      v128_t m1 = wasm_i32x4_splat(INT_MAX);
      v128_t zero = wasm_i32x4_splat(0);
      v128_t one = wasm_i32x4_splat(1);
      si32 *p = (si32*)dp;
      for (ui32 i = 0; i < count; i += 4, sp += 4, p += 4)
      {
          v128_t v = wasm_v128_load((v128_t*)sp);
          v128_t val = wasm_v128_and(v, m1);
          val = wasm_i32x4_shr(val, shift);
          v128_t sign = wasm_i32x4_lt(v, zero);
          val = wasm_v128_xor(val, sign); // negate 1's complement
          v128_t ones = wasm_v128_and(sign, one);
          val = wasm_i32x4_add(val, ones); // 2's complement
          wasm_v128_store(p, val);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_irv_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max, 
                               float delta, ui32 count)
    {
      ojph_unused(K_max);
      v128_t m1 = wasm_i32x4_splat(INT_MAX);
      v128_t d = wasm_f32x4_splat(delta);
      float *p = (float*)dp;
      for (ui32 i = 0; i < count; i += 4, sp += 4, p += 4)
      {
        v128_t v = wasm_v128_load((v128_t*)sp);
        v128_t vali = wasm_v128_and(v, m1);
        v128_t  valf = wasm_f32x4_convert_i32x4(vali);
        valf = wasm_f32x4_mul(valf, d);
        v128_t sign = wasm_v128_andnot(v, m1);
        valf = wasm_v128_or(valf, sign);
        wasm_v128_store(p, valf);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_tx_to_cb64(const void *sp, ui64 *dp, ui32 K_max, 
                             float delta_inv, ui32 count, ui64* max_val)
    {
      ojph_unused(delta_inv);

      // convert to sign and magnitude and keep max_val      
      ui32 shift = 63 - K_max;
      v128_t m0 = wasm_i64x2_splat(LLONG_MIN);
      v128_t zero = wasm_i64x2_splat(0);
      v128_t one = wasm_i64x2_splat(1);
      v128_t tmax = wasm_v128_load(max_val);
      si64 *p = (si64*)sp;
      for ( ; count >= 2; count -= 2, p += 2, dp += 2)
      {
        v128_t v = wasm_v128_load(p);
        v128_t sign = wasm_i64x2_lt(v, zero);
        v128_t val = wasm_v128_xor(v, sign); // negate 1's complement
        v128_t ones = wasm_v128_and(sign, one);
        val = wasm_i64x2_add(val, ones);     // 2's complement
        sign = wasm_v128_and(sign, m0);
        val = wasm_i64x2_shl(val, shift);
        tmax = wasm_v128_or(tmax, val);
        val = wasm_v128_or(val, sign);
        wasm_v128_store(dp, val);
      }
      if (count)
      {
        v128_t v = wasm_v128_load(p);
        v128_t sign = wasm_i64x2_lt(v, zero);
        v128_t val = wasm_v128_xor(v, sign); // negate 1's complement
        v128_t ones = wasm_v128_and(sign, one);
        val = wasm_i64x2_add(val, ones);     // 2's complement
        sign = wasm_v128_and(sign, m0);
        val = wasm_i64x2_shl(val, shift);

        v128_t c = wasm_i32x4_make((si32)0xFFFFFFFF, (si32)0xFFFFFFFF, 0, 0);
        c = wasm_v128_and(val, c);
        tmax = wasm_v128_or(tmax, c);

        val = wasm_v128_or(val, sign);
        wasm_v128_store(dp, val);
      }

      wasm_v128_store(max_val, tmax);
    }   

    //////////////////////////////////////////////////////////////////////////
    void wasm_rev_tx_from_cb64(const ui64 *sp, void *dp, ui32 K_max, 
                               float delta, ui32 count)
    {
      ojph_unused(delta);
      ui32 shift = 63 - K_max;
      v128_t m1 = wasm_i64x2_splat(LLONG_MAX);
      v128_t zero = wasm_i64x2_splat(0);
      v128_t one = wasm_i64x2_splat(1);
      si64 *p = (si64*)dp;
      for (ui32 i = 0; i < count; i += 2, sp += 2, p += 2)
      {
          v128_t v = wasm_v128_load((v128_t*)sp);
          v128_t val = wasm_v128_and(v, m1);
          val = wasm_i64x2_shr(val, shift);
          v128_t sign = wasm_i64x2_lt(v, zero);
          val = wasm_v128_xor(val, sign); // negate 1's complement
          v128_t ones = wasm_v128_and(sign, one);
          val = wasm_i64x2_add(val, ones); // 2's complement
          wasm_v128_store(p, val);
      }
    }
  }
}