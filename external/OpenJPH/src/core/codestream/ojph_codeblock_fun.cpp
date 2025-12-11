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
// File: ojph_codeblock_fun.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#include <climits>
#include <cmath>

#include "ojph_file.h"
#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream.h"
#include "ojph_codestream_local.h"
#include "ojph_codeblock_fun.h"

#include "../transform/ojph_colour.h"
#include "../transform/ojph_transform.h"
#include "../coding/ojph_block_decoder.h"
#include "../coding/ojph_block_encoder.h"

namespace ojph {

  namespace local
  {

    //////////////////////////////////////////////////////////////////////////
    void gen_mem_clear(void* addr, size_t count);
    void sse_mem_clear(void* addr, size_t count);
    void avx_mem_clear(void* addr, size_t count);
    void wasm_mem_clear(void* addr, size_t count);

    //////////////////////////////////////////////////////////////////////////
    ui32  gen_find_max_val32(ui32* address);
    ui32 sse2_find_max_val32(ui32* address);
    ui32 avx2_find_max_val32(ui32* address);
    ui32 wasm_find_max_val32(ui32* address);
    ui64  gen_find_max_val64(ui64* address);
    ui64 sse2_find_max_val64(ui64* address);
    ui64 avx2_find_max_val64(ui64* address);
    ui64 wasm_find_max_val64(ui64* address);


    //////////////////////////////////////////////////////////////////////////
    void  gen_rev_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui32* max_val);
    void sse2_rev_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui32* max_val);
    void avx2_rev_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui32* max_val);
    void  gen_irv_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui32* max_val);
    void sse2_irv_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui32* max_val);
    void avx2_irv_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui32* max_val);
    void wasm_rev_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui32* max_val);
    void wasm_irv_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui32* max_val);

    void  gen_rev_tx_to_cb64(const void *sp, ui64 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui64* max_val);
    void sse2_rev_tx_to_cb64(const void *sp, ui64 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui64* max_val);
    void avx2_rev_tx_to_cb64(const void *sp, ui64 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui64* max_val);
    void wasm_rev_tx_to_cb64(const void *sp, ui64 *dp, ui32 K_max,
                             float delta_inv, ui32 count, ui64* max_val);

    //////////////////////////////////////////////////////////////////////////
    void  gen_rev_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max,
                               float delta, ui32 count);
    void sse2_rev_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max,
                               float delta, ui32 count);
    void avx2_rev_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max,
                               float delta, ui32 count);
    void  gen_irv_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max,
                               float delta, ui32 count);
    void sse2_irv_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max,
                               float delta, ui32 count);
    void avx2_irv_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max,
                               float delta, ui32 count);
    void wasm_rev_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max,
                               float delta, ui32 count);
    void wasm_irv_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max,
                               float delta, ui32 count);

    void  gen_rev_tx_from_cb64(const ui64 *sp, void *dp, ui32 K_max,
                               float delta, ui32 count);
    void sse2_rev_tx_from_cb64(const ui64 *sp, void *dp, ui32 K_max,
                               float delta, ui32 count);
    void avx2_rev_tx_from_cb64(const ui64 *sp, void *dp, ui32 K_max,
                               float delta, ui32 count);
    void wasm_rev_tx_from_cb64(const ui64 *sp, void *dp, ui32 K_max,
                               float delta, ui32 count);                               

    void codeblock_fun::init(bool reversible) {

#if !defined(OJPH_ENABLE_WASM_SIMD) || !defined(OJPH_EMSCRIPTEN)

      // Default path, no acceleration.  We may change this later
      decode_cb32 = ojph_decode_codeblock32;
      find_max_val32 = gen_find_max_val32;
      mem_clear = gen_mem_clear;
      if (reversible) {
        tx_to_cb32 = gen_rev_tx_to_cb32;
        tx_from_cb32 = gen_rev_tx_from_cb32;
      }
      else
      {
        tx_to_cb32 = gen_irv_tx_to_cb32;
        tx_from_cb32 = gen_irv_tx_from_cb32;
      }
      encode_cb32 = ojph_encode_codeblock32;

      decode_cb64 = ojph_decode_codeblock64;
      find_max_val64 = gen_find_max_val64;
      if (reversible) {
        tx_to_cb64 = gen_rev_tx_to_cb64;
        tx_from_cb64 = gen_rev_tx_from_cb64;
      }
      else
      {
        tx_to_cb64 = NULL;
        tx_from_cb64 = NULL;
      }
      encode_cb64 = ojph_encode_codeblock64;
      bool result = initialize_block_encoder_tables();
      assert(result); ojph_unused(result);      

  #ifndef OJPH_DISABLE_SIMD

    #if (defined(OJPH_ARCH_X86_64) || defined(OJPH_ARCH_I386))

      // Accelerated functions for INTEL/AMD CPUs
      #ifndef OJPH_DISABLE_SSE
        if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_SSE)
          mem_clear = sse_mem_clear;
      #endif // !OJPH_DISABLE_SSE

      #ifndef OJPH_DISABLE_SSE2
        if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_SSE2) {
          find_max_val32 = sse2_find_max_val32;
          if (reversible) {
            tx_to_cb32 = sse2_rev_tx_to_cb32;
            tx_from_cb32 = sse2_rev_tx_from_cb32;
          }
          else {
            tx_to_cb32 = sse2_irv_tx_to_cb32;
            tx_from_cb32 = sse2_irv_tx_from_cb32;
          }
          find_max_val64 = sse2_find_max_val64;
          if (reversible) {
            tx_to_cb64 = sse2_rev_tx_to_cb64;
            tx_from_cb64 = sse2_rev_tx_from_cb64;
          }
          else
          {
            tx_to_cb64 = NULL;
            tx_from_cb64 = NULL;
          }
        }
      #endif // !OJPH_DISABLE_SSE2

      #ifndef OJPH_DISABLE_SSSE3
        if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_SSSE3)
          decode_cb32 = ojph_decode_codeblock_ssse3;
      #endif // !OJPH_DISABLE_SSSE3

      #ifndef OJPH_DISABLE_AVX
        if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_AVX)
          mem_clear = avx_mem_clear;
      #endif // !OJPH_DISABLE_AVX

      #ifndef OJPH_DISABLE_AVX2
        if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_AVX2) {
          decode_cb32 = ojph_decode_codeblock_avx2;
          find_max_val32 = avx2_find_max_val32;
          if (reversible) {
            tx_to_cb32 = avx2_rev_tx_to_cb32;
            tx_from_cb32 = avx2_rev_tx_from_cb32;
          }
          else {
            tx_to_cb32 = avx2_irv_tx_to_cb32;
            tx_from_cb32 = avx2_irv_tx_from_cb32;
          }
          encode_cb32 = ojph_encode_codeblock_avx2;
          bool result = initialize_block_encoder_tables_avx2();
          assert(result); ojph_unused(result);

          find_max_val64 = avx2_find_max_val64;
          if (reversible) {
            tx_to_cb64 = avx2_rev_tx_to_cb64;
            tx_from_cb64 = avx2_rev_tx_from_cb64;
          }
          else
          {
            tx_to_cb64 = NULL;
            tx_from_cb64 = NULL;
          }
        }
      #endif // !OJPH_DISABLE_AVX2

      #if (defined(OJPH_ARCH_X86_64) && !defined(OJPH_DISABLE_AVX512))
        if (get_cpu_ext_level() >= X86_CPU_EXT_LEVEL_AVX512) {
          encode_cb32 = ojph_encode_codeblock_avx512;
          bool result = initialize_block_encoder_tables_avx512();
          assert(result); ojph_unused(result);
        }
      #endif // !OJPH_DISABLE_AVX512

    #elif defined(OJPH_ARCH_ARM)
    
    #endif // !(defined(OJPH_ARCH_X86_64) || defined(OJPH_ARCH_I386))

  #endif // !OJPH_DISABLE_SIMD

#else // OJPH_ENABLE_WASM_SIMD

      // Accelerated functions for WASM SIMD.
      decode_cb32 = ojph_decode_codeblock_wasm;
      find_max_val32 = wasm_find_max_val32;
      mem_clear = wasm_mem_clear;
      if (reversible) {
        tx_to_cb32 = wasm_rev_tx_to_cb32;
        tx_from_cb32 = wasm_rev_tx_from_cb32;
      }
      else {
        tx_to_cb32 = wasm_irv_tx_to_cb32;
        tx_from_cb32 = wasm_irv_tx_from_cb32;
      }
      encode_cb32 = ojph_encode_codeblock32;

      decode_cb64 = ojph_decode_codeblock64;
      find_max_val64 = wasm_find_max_val64;
      if (reversible) {
        tx_to_cb64 = wasm_rev_tx_to_cb64;
        tx_from_cb64 = wasm_rev_tx_from_cb64;
      }
      else
      {
        tx_to_cb64 = NULL;
        tx_from_cb64 = NULL;
      }
      encode_cb64 = ojph_encode_codeblock64;
      bool result = initialize_block_encoder_tables();
      assert(result); ojph_unused(result);      

#endif // !OJPH_ENABLE_WASM_SIMD

    }
  }  // local
}  // ojph
