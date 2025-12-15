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
// File: ojph_codestream_sse2.cpp
// Author: Aous Naman
// Date: 15 May 2022
//***************************************************************************/

#include "ojph_arch.h"
#if defined(OJPH_ARCH_I386) || defined(OJPH_ARCH_X86_64)

#include <climits>
#include <immintrin.h>
#include "ojph_defs.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    ui32 sse2_find_max_val32(ui32* address)
    {
      __m128i x1, x0 = _mm_loadu_si128((__m128i*)address);
      x1 = _mm_shuffle_epi32(x0, 0xEE);   // x1 = x0[2,3,2,3]
      x0 = _mm_or_si128(x0, x1);
      x1 = _mm_shuffle_epi32(x0, 0x55);   // x1 = x0[1,1,1,1]
      x0 = _mm_or_si128(x0, x1);
      _mm_storeu_si128((__m128i*)address, x0);
      return *address;
      // A single movd t, xmm0 can do the trick, but it is not available
      // in SSE2 intrinsics. extract_epi32 is available in sse4.1
      // ui32 t = (ui32)_mm_extract_epi16(x0, 0);
      // t |= (ui32)_mm_extract_epi16(x0, 1) << 16;
      // return t;
    }

    //////////////////////////////////////////////////////////////////////////
    ui64 sse2_find_max_val64(ui64* address)
    {
      __m128i x1, x0 = _mm_loadu_si128((__m128i*)address);
      x1 = _mm_shuffle_epi32(x0, 0xEE);   // x1 = x0[2,3,2,3]
      x0 = _mm_or_si128(x0, x1);
      _mm_storeu_si128((__m128i*)address, x0);
      return *address;
      // A single movd t, xmm0 can do the trick, but it is not available
      // in SSE2 intrinsics. extract_epi32 is available in sse4.1
      // ui32 t = (ui32)_mm_extract_epi16(x0, 0);
      // t |= (ui32)_mm_extract_epi16(x0, 1) << 16;
      // return t;
    }    

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max, 
                             float delta_inv, ui32 count, ui32* max_val)
    {
      ojph_unused(delta_inv);

      // convert to sign and magnitude and keep max_val      
      ui32 shift = 31 - K_max;
      __m128i m0 = _mm_set1_epi32(INT_MIN);
      __m128i zero = _mm_setzero_si128();
      __m128i one = _mm_set1_epi32(1);
      __m128i tmax = _mm_loadu_si128((__m128i*)max_val);
      __m128i *p = (__m128i*)sp;
      for ( ; count >= 4; count -= 4, p += 1, dp += 4)
      {
        __m128i v = _mm_loadu_si128(p);
        __m128i sign = _mm_cmplt_epi32(v, zero);
        __m128i val = _mm_xor_si128(v, sign); // negate 1's complement
        __m128i ones = _mm_and_si128(sign, one);
        val = _mm_add_epi32(val, ones);        // 2's complement
        sign = _mm_and_si128(sign, m0);
        val = _mm_slli_epi32(val, (int)shift);
        tmax = _mm_or_si128(tmax, val);
        val = _mm_or_si128(val, sign);
        _mm_storeu_si128((__m128i*)dp, val);
      }
      if (count)
      {
        __m128i v = _mm_loadu_si128(p);
        __m128i sign = _mm_cmplt_epi32(v, zero);
        __m128i val = _mm_xor_si128(v, sign); // negate 1's complement
        __m128i ones = _mm_and_si128(sign, one);
        val = _mm_add_epi32(val, ones);        // 2's complement
        sign = _mm_and_si128(sign, m0);
        val = _mm_slli_epi32(val, (int)shift);

        __m128i c = _mm_set1_epi32((si32)count);
        __m128i idx = _mm_set_epi32(3, 2, 1, 0);
        __m128i mask = _mm_cmpgt_epi32(c, idx);
        c = _mm_and_si128(val, mask);
        tmax = _mm_or_si128(tmax, c);

        val = _mm_or_si128(val, sign);
        _mm_storeu_si128((__m128i*)dp, val);
      }
      _mm_storeu_si128((__m128i*)max_val, tmax);
    }
                           
    //////////////////////////////////////////////////////////////////////////
    void sse2_irv_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui32* max_val)
    {
      ojph_unused(K_max);

      //quantize and convert to sign and magnitude and keep max_val

      __m128 d = _mm_set1_ps(delta_inv);
      __m128i zero = _mm_setzero_si128();
      __m128i one = _mm_set1_epi32(1);
      __m128i tmax = _mm_loadu_si128((__m128i*)max_val);
      float *p = (float*)sp;
      for ( ; count >= 4; count -= 4, p += 4, dp += 4)
      {
        __m128 vf = _mm_loadu_ps(p);
        vf = _mm_mul_ps(vf, d);                    // multiply
        __m128i val = _mm_cvtps_epi32(vf);         // convert to int
        __m128i sign = _mm_cmplt_epi32(val, zero); // get sign
        val = _mm_xor_si128(val, sign);            // negate 1's complement
        __m128i ones = _mm_and_si128(sign, one);
        val = _mm_add_epi32(val, ones);            // 2's complement
        tmax = _mm_or_si128(tmax, val);
        sign = _mm_slli_epi32(sign, 31);
        val = _mm_or_si128(val, sign);
        _mm_storeu_si128((__m128i*)dp, val);
      }
      if (count)
      {
        __m128 vf = _mm_loadu_ps(p);
        vf = _mm_mul_ps(vf, d);                    // multiply
        __m128i val = _mm_cvtps_epi32(vf);         // convert to int
        __m128i sign = _mm_cmplt_epi32(val, zero); // get sign
        val = _mm_xor_si128(val, sign);            // negate 1's complement
        __m128i ones = _mm_and_si128(sign, one);
        val = _mm_add_epi32(val, ones);            // 2's complement

        __m128i c = _mm_set1_epi32((si32)count);
        __m128i idx = _mm_set_epi32(3, 2, 1, 0);
        __m128i mask = _mm_cmpgt_epi32(c, idx);
        c = _mm_and_si128(val, mask);
        tmax = _mm_or_si128(tmax, c);

        sign = _mm_slli_epi32(sign, 31);
        val = _mm_or_si128(val, sign);
        _mm_storeu_si128((__m128i*)dp, val);
      }
      _mm_storeu_si128((__m128i*)max_val, tmax);
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max, 
                               float delta, ui32 count)
    {
      ojph_unused(delta);
      ui32 shift = 31 - K_max;
      __m128i m1 = _mm_set1_epi32(INT_MAX);
      __m128i zero = _mm_setzero_si128();
      __m128i one = _mm_set1_epi32(1);
      si32 *p = (si32*)dp;
      for (ui32 i = 0; i < count; i += 4, sp += 4, p += 4)
      {
        __m128i v = _mm_load_si128((__m128i*)sp);
        __m128i val = _mm_and_si128(v, m1);
        val = _mm_srli_epi32(val, (int)shift);
        __m128i sign = _mm_cmplt_epi32(v, zero);
        val = _mm_xor_si128(val, sign); // negate 1's complement
        __m128i ones = _mm_and_si128(sign, one);
        val = _mm_add_epi32(val, ones); // 2's complement
        _mm_storeu_si128((__m128i*)p, val);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_irv_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max, 
                               float delta, ui32 count)
    {
      ojph_unused(K_max);
      __m128i m1 = _mm_set1_epi32(INT_MAX);
      __m128 d = _mm_set1_ps(delta);
      float *p = (float*)dp;
      for (ui32 i = 0; i < count; i += 4, sp += 4, p += 4)
      {
        __m128i v = _mm_load_si128((__m128i*)sp);
        __m128i vali = _mm_and_si128(v, m1);
        __m128  valf = _mm_cvtepi32_ps(vali);
        valf = _mm_mul_ps(valf, d);
        __m128i sign = _mm_andnot_si128(m1, v);
        valf = _mm_or_ps(valf, _mm_castsi128_ps(sign));
        _mm_storeu_ps(p, valf);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_tx_to_cb64(const void *sp, ui64 *dp, ui32 K_max, 
                             float delta_inv, ui32 count, ui64* max_val)
    {
      ojph_unused(delta_inv);

      // convert to sign and magnitude and keep max_val      
      ui32 shift = 63 - K_max;
      __m128i m0 = _mm_set1_epi64x(LLONG_MIN);
      __m128i zero = _mm_setzero_si128();
      __m128i one = _mm_set1_epi64x(1);
      __m128i tmax = _mm_loadu_si128((__m128i*)max_val);
      __m128i *p = (__m128i*)sp;
      for ( ; count >= 2; count -= 2, p += 1, dp += 2)
      {
        __m128i v = _mm_loadu_si128(p);
        __m128i sign = _mm_cmplt_epi32(v, zero);
        sign = _mm_shuffle_epi32(sign, 0xF5);  // sign = sign[1,1,3,3];
        __m128i val = _mm_xor_si128(v, sign);  // negate 1's complement
        __m128i ones = _mm_and_si128(sign, one);
        val = _mm_add_epi64(val, ones);        // 2's complement
        sign = _mm_and_si128(sign, m0);
        val = _mm_slli_epi64(val, (int)shift);
        tmax = _mm_or_si128(tmax, val);
        val = _mm_or_si128(val, sign);
        _mm_storeu_si128((__m128i*)dp, val);
      }
      if (count)
      {
        __m128i v = _mm_loadu_si128(p);
        __m128i sign = _mm_cmplt_epi32(v, zero);
        sign = _mm_shuffle_epi32(sign, 0xF5);  // sign = sign[1,1,3,3];
        __m128i val = _mm_xor_si128(v, sign);  // negate 1's complement
        __m128i ones = _mm_and_si128(sign, one);
        val = _mm_add_epi64(val, ones);        // 2's complement
        sign = _mm_and_si128(sign, m0);
        val = _mm_slli_epi64(val, (int)shift);

        __m128i c = _mm_set_epi32(0, 0, (si32)0xFFFFFFFF, (si32)0xFFFFFFFF);
        c = _mm_and_si128(val, c);
        tmax = _mm_or_si128(tmax, c);

        val = _mm_or_si128(val, sign);
        _mm_storeu_si128((__m128i*)dp, val);
      }
      _mm_storeu_si128((__m128i*)max_val, tmax);
    }

    //////////////////////////////////////////////////////////////////////////
    void sse2_rev_tx_from_cb64(const ui64 *sp, void *dp, ui32 K_max, 
                               float delta, ui32 count)
    {
      ojph_unused(delta);
      ui32 shift = 63 - K_max;
      __m128i m1 = _mm_set1_epi64x(LLONG_MAX);
      __m128i zero = _mm_setzero_si128();
      __m128i one = _mm_set1_epi64x(1);
      si64 *p = (si64*)dp;
      for (ui32 i = 0; i < count; i += 2, sp += 2, p += 2)
      {
        __m128i v = _mm_load_si128((__m128i*)sp);
        __m128i val = _mm_and_si128(v, m1);
        val = _mm_srli_epi64(val, (int)shift);
        __m128i sign = _mm_cmplt_epi32(v, zero);
        sign = _mm_shuffle_epi32(sign, 0xF5);      // sign = sign[1,1,3,3];
        val = _mm_xor_si128(val, sign);            // negate 1's complement
        __m128i ones = _mm_and_si128(sign, one);
        val = _mm_add_epi64(val, ones);            // 2's complement
        _mm_storeu_si128((__m128i*)p, val);
      }
    }
  }
}

#endif
