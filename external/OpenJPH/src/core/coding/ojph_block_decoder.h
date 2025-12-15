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
// File: ojph_block_decoder.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_BLOCK_DECODER_H
#define OJPH_BLOCK_DECODER_H

#include "ojph_defs.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    //decodes the cleanup pass, significance propagation pass,
    // and magnitude refinement pass

    // generic decoder
    bool
      ojph_decode_codeblock32(ui8* coded_data, ui32* decoded_data,
        ui32 missing_msbs, ui32 num_passes, ui32 lengths1, ui32 lengths2,
        ui32 width, ui32 height, ui32 stride, bool stripe_causal);

    bool
      ojph_decode_codeblock64(ui8* coded_data, ui64* decoded_data,
        ui32 missing_msbs, ui32 num_passes, ui32 lengths1, ui32 lengths2,
        ui32 width, ui32 height, ui32 stride, bool stripe_causal);

    // SSSE3-accelerated decoder
    bool
      ojph_decode_codeblock_ssse3(ui8* coded_data, ui32* decoded_data,
        ui32 missing_msbs, ui32 num_passes, ui32 lengths1, ui32 lengths2,
        ui32 width, ui32 height, ui32 stride, bool stripe_causal);

    // AVX2-accelerated decoder
    bool
      ojph_decode_codeblock_avx2(ui8* coded_data, ui32* decoded_data,
        ui32 missing_msbs, ui32 num_passes, ui32 lengths1, ui32 lengths2,
        ui32 width, ui32 height, ui32 stride, bool stripe_causal);

    // WASM SIMD-accelerated decoder
    bool
      ojph_decode_codeblock_wasm(ui8* coded_data, ui32* decoded_data,
        ui32 missing_msbs, ui32 num_passes, ui32 lengths1, ui32 lengths2,
        ui32 width, ui32 height, ui32 stride, bool stripe_causal);

  }
}

#endif // !OJPH_BLOCK_DECODER_H
