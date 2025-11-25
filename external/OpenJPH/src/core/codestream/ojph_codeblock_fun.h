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
// File: ojph_codeblock_fun.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_CODEBLOCK_FUN_H
#define OJPH_CODEBLOCK_FUN_H

#include "ojph_defs.h"
#include "ojph_file.h"
#include "ojph_params_local.h"

namespace ojph {

  namespace local {

    // define function signature simple memory clearing
    typedef void (*mem_clear_fun)(void* addr, size_t count);

    // define function signature for max value finding
    typedef ui32 (*find_max_val_fun32)(ui32* addr);

    typedef ui64 (*find_max_val_fun64)(ui64* addr);

    // define line transfer function signature from subbands to codeblocks
    typedef void (*tx_to_cb_fun32)(const void *sp, ui32 *dp, ui32 K_max,
                                   float delta_inv, ui32 count, ui32* max_val);

    typedef void (*tx_to_cb_fun64)(const void *sp, ui64 *dp, ui32 K_max,
                                   float delta_inv, ui32 count, ui64* max_val);

    // define line transfer function signature from codeblock to subband
    typedef void (*tx_from_cb_fun32)(const ui32 *sp, void *dp, ui32 K_max,
                                     float delta, ui32 count);

    typedef void (*tx_from_cb_fun64)(const ui64 *sp, void *dp, ui32 K_max,
                                     float delta, ui32 count);

    // define the block decoder function signature
    typedef bool (*cb_decoder_fun32)(ui8* coded_data, ui32* decoded_data,
      ui32 missing_msbs, ui32 num_passes, ui32 lengths1, ui32 lengths2,
      ui32 width, ui32 height, ui32 stride, bool stripe_causal);

    typedef bool (*cb_decoder_fun64)(ui8* coded_data, ui64* decoded_data,
      ui32 missing_msbs, ui32 num_passes, ui32 lengths1, ui32 lengths2,
      ui32 width, ui32 height, ui32 stride, bool stripe_causal);

    // define the block encoder function signature
    typedef void (*cb_encoder_fun32)(ui32* buf, ui32 missing_msbs, 
      ui32 num_passes, ui32 width, ui32 height, ui32 stride,
      ui32* lengths, ojph::mem_elastic_allocator* elastic,
      ojph::coded_lists*& coded);

    typedef void (*cb_encoder_fun64)(ui64* buf, ui32 missing_msbs, 
      ui32 num_passes, ui32 width, ui32 height, ui32 stride,
      ui32* lengths, ojph::mem_elastic_allocator* elastic,
      ojph::coded_lists*& coded);

    //////////////////////////////////////////////////////////////////////////
    struct codeblock_fun {

      void init(bool reversible);

      // a pointer to the max value finding function
      mem_clear_fun mem_clear;
     
      // a pointer to the max value finding function
      find_max_val_fun32 find_max_val32;
      find_max_val_fun64 find_max_val64;
     
      // a pointer to function transferring samples from subbands to codeblocks
      tx_to_cb_fun32 tx_to_cb32;
      tx_to_cb_fun64 tx_to_cb64;
     
      // a pointer to function transferring samples from codeblocks to subbands
      tx_from_cb_fun32 tx_from_cb32;
      tx_from_cb_fun64 tx_from_cb64;
     
      // a pointer to the decoder function
      cb_decoder_fun32 decode_cb32;
      cb_decoder_fun64 decode_cb64;

      // a pointer to the encoder function
      cb_encoder_fun32 encode_cb32;
      cb_encoder_fun64 encode_cb64;
    };
    
  }
}
#endif // !OJPH_CODEBLOCK_FUN_H