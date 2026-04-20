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
// File: ojph_codestream_gen.cpp
// Author: Aous Naman
// Date: 15 May 2022
//***************************************************************************/

#include "ojph_defs.h"
#include "ojph_arch.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    void gen_mem_clear(void* addr, size_t count)
    {
      si64* p = (si64*)addr;
      for (size_t i = 0; i < count; i += 8)
        *p++ = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    ui32 gen_find_max_val32(ui32* addr) { return addr[0]; }

    //////////////////////////////////////////////////////////////////////////
    ui64 gen_find_max_val64(ui64* addr) { return addr[0]; }

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max, 
                            float delta_inv, ui32 count, 
                            ui32* max_val)
    {
      ojph_unused(delta_inv);
      ui32 shift = 31 - K_max;
      // convert to sign and magnitude and keep max_val
      ui32 tmax = *max_val;
      si32 *p = (si32*)sp;
      for (ui32 i = count; i > 0; --i)
      {
        si32 v = *p++;
        ui32 sign = v >= 0 ? 0U : 0x80000000U;
        ui32 val = (ui32)(v >= 0 ? v : -v);
        val <<= shift;
        *dp++ = sign | val;
        tmax |= val; // it is more efficient to use or than max
      }
      *max_val = tmax;
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_tx_to_cb64(const void *sp, ui64 *dp, ui32 K_max, 
                            float delta_inv, ui32 count, 
                            ui64* max_val)
    {
      ojph_unused(delta_inv);
      ui32 shift = 63 - K_max;
      // convert to sign and magnitude and keep max_val
      ui64 tmax = *max_val;
      si64 *p = (si64*)sp;
      for (ui32 i = count; i > 0; --i)
      {
        si64 v = *p++;
        ui64 sign = v >= 0 ? 0ULL : 0x8000000000000000ULL;
        ui64 val = (ui64)(v >= 0 ? v : -v);
        val <<= shift;
        *dp++ = sign | val;
        tmax |= val; // it is more efficient to use or than max
      }
      *max_val = tmax;
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_irv_tx_to_cb32(const void *sp, ui32 *dp, ui32 K_max,
                            float delta_inv, ui32 count, 
                            ui32* max_val)
    {
      ojph_unused(K_max);
      //quantize and convert to sign and magnitude and keep max_val
      ui32 tmax = *max_val;
      float *p = (float*)sp;
      for (ui32 i = count; i > 0; --i)
      {
        float v = *p++;
        si32 t = ojph_trunc(v * delta_inv);
        ui32 sign = t >= 0 ? 0U : 0x80000000U;
        ui32 val = (ui32)(t >= 0 ? t : -t);
        *dp++ = sign | val;
        tmax |= val; // it is more efficient to use or than max
      }
      *max_val = tmax;
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max,
                              float delta, ui32 count)
    {
      ojph_unused(delta);
      ui32 shift = 31 - K_max;
      //convert to sign and magnitude
      si32 *p = (si32*)dp;
      for (ui32 i = count; i > 0; --i)
      {
        ui32 v = *sp++;
        si32 val = (v & 0x7FFFFFFFU) >> shift;
        *p++ = (v & 0x80000000U) ? -val : val;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_rev_tx_from_cb64(const ui64 *sp, void *dp, ui32 K_max,
                              float delta, ui32 count)
    {
      ojph_unused(delta);
      ui32 shift = 63 - K_max;
      //convert to sign and magnitude
      si64 *p = (si64*)dp;
      for (ui32 i = count; i > 0; --i)
      {
        ui64 v = *sp++;
        si64 val = (v & 0x7FFFFFFFFFFFFFFFULL) >> shift;
        *p++ = (v & 0x8000000000000000ULL) ? -val : val;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void gen_irv_tx_from_cb32(const ui32 *sp, void *dp, ui32 K_max,
                              float delta, ui32 count)
    {
      ojph_unused(K_max);
      //convert to sign and magnitude
      float *p = (float*)dp;
      for (ui32 i = count; i > 0; --i)
      {
        ui32 v = *sp++;
        float val = (float)(v & 0x7FFFFFFFU) * delta;
        *p++ = (v & 0x80000000U) ? -val : val;
      }
    }
    
 }
}