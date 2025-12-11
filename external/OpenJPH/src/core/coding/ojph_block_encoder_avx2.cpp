//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman
// Copyright (c) 2019, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2019, The University of New South Wales, Australia
// Copyright (c) 2024, Intel Corporation
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
// File: ojph_block_encoder_avx2.cpp
//***************************************************************************/

#include "ojph_arch.h"
#if defined(OJPH_ARCH_I386) || defined(OJPH_ARCH_X86_64)

#include <cassert>
#include <cstring>
#include <cstdint>
#include <climits>
#include <immintrin.h>

#include "ojph_mem.h"
#include "ojph_arch.h"
#include "ojph_block_encoder.h"
#include "ojph_message.h"

#ifdef OJPH_COMPILER_MSVC
  #define likely(x)       (x)
  #define unlikely(x)     (x)
#else
  #define likely(x)       __builtin_expect((x), 1)
  #define unlikely(x)     __builtin_expect((x), 0)
#endif

namespace ojph {
  namespace local {

    /////////////////////////////////////////////////////////////////////////
    // tables
    /////////////////////////////////////////////////////////////////////////

    //VLC encoding
    // index is (c_q << 8) + (rho << 4) + eps
    // data is  (cwd << 8) + (cwd_len << 4) + eps
    // table 0 is for the initial line of quads
    static ui32 vlc_tbl0[2048];
    static ui32 vlc_tbl1[2048];

    //UVLC encoding
    static ui32 ulvc_cwd_pre[33];
    static int ulvc_cwd_pre_len[33];
    static ui32 ulvc_cwd_suf[33];
    static int ulvc_cwd_suf_len[33];

    /////////////////////////////////////////////////////////////////////////
    static bool vlc_init_tables()
    {
      struct vlc_src_table { int c_q, rho, u_off, e_k, e_1, cwd, cwd_len; };
      vlc_src_table tbl0[] = {
    #include "table0.h"
      };
      size_t tbl0_size = sizeof(tbl0) / sizeof(vlc_src_table);

      si32 pattern_popcnt[16];
      for (ui32 i = 0; i < 16; ++i)
        pattern_popcnt[i] = (si32)population_count(i);

      vlc_src_table* src_tbl = tbl0;
      ui32 *tgt_tbl = vlc_tbl0;
      size_t tbl_size = tbl0_size;
      for (int i = 0; i < 2048; ++i)
      {
        int c_q = i >> 8, rho = (i >> 4) & 0xF, emb = i & 0xF;
        if (((emb & rho) != emb) || (rho == 0 && c_q == 0))
          tgt_tbl[i] = 0;
        else
        {
          vlc_src_table *best_entry = NULL;
          if (emb) // u_off = 1
          {
            int best_e_k = -1;
            for (size_t j = 0; j < tbl_size; ++j)
            {
              if (src_tbl[j].c_q == c_q && src_tbl[j].rho == rho)
                if (src_tbl[j].u_off == 1)
                  if ((emb & src_tbl[j].e_k) == src_tbl[j].e_1)
                  {
                    //now we need to find the smallest cwd with the highest
                    // number of bits set in e_k
                    int ones_count = pattern_popcnt[src_tbl[j].e_k];
                    if (ones_count >= best_e_k)
                    {
                      best_entry = src_tbl + j;
                      best_e_k = ones_count;
                    }
                  }
            }
          }
          else // u_off = 0
          {
            for (size_t j = 0; j < tbl_size; ++j)
            {
              if (src_tbl[j].c_q == c_q && src_tbl[j].rho == rho)
                if (src_tbl[j].u_off == 0)
                {
                  best_entry = src_tbl + j;
                  break;
                }
            }
          }
          assert(best_entry);
          tgt_tbl[i] = (ui16)((best_entry->cwd<<8) + (best_entry->cwd_len<<4)
                             + best_entry->e_k);
        }
      }

      vlc_src_table tbl1[] = {
    #include "table1.h"
      };
      size_t tbl1_size = sizeof(tbl1) / sizeof(vlc_src_table);

      src_tbl = tbl1;
      tgt_tbl = vlc_tbl1;
      tbl_size = tbl1_size;
      for (int i = 0; i < 2048; ++i)
      {
        int c_q = i >> 8, rho = (i >> 4) & 0xF, emb = i & 0xF;
        if (((emb & rho) != emb) || (rho == 0 && c_q == 0))
          tgt_tbl[i] = 0;
        else
        {
          vlc_src_table *best_entry = NULL;
          if (emb) // u_off = 1
          {
            int best_e_k = -1;
            for (size_t j = 0; j < tbl_size; ++j)
            {
              if (src_tbl[j].c_q == c_q && src_tbl[j].rho == rho)
                if (src_tbl[j].u_off == 1)
                  if ((emb & src_tbl[j].e_k) == src_tbl[j].e_1)
                  {
                    //now we need to find the smallest cwd with the highest
                    // number of bits set in e_k
                    int ones_count = pattern_popcnt[src_tbl[j].e_k];
                    if (ones_count >= best_e_k)
                    {
                      best_entry = src_tbl + j;
                      best_e_k = ones_count;
                    }
                  }
            }
          }
          else // u_off = 0
          {
            for (size_t j = 0; j < tbl_size; ++j)
            {
              if (src_tbl[j].c_q == c_q && src_tbl[j].rho == rho)
                if (src_tbl[j].u_off == 0)
                {
                  best_entry = src_tbl + j;
                  break;
                }
            }
          }
          assert(best_entry);
          tgt_tbl[i] = (ui16)((best_entry->cwd<<8) + (best_entry->cwd_len<<4)
                             + best_entry->e_k);
        }
      }


      return true;
    }

    /////////////////////////////////////////////////////////////////////////
    static bool uvlc_init_tables()
    {
      //code goes from 0 to 31, extension and 32 are not supported here
      ulvc_cwd_pre[0] = 0; ulvc_cwd_pre[1] = 1; ulvc_cwd_pre[2] = 2;
      ulvc_cwd_pre[3] = 4; ulvc_cwd_pre[4] = 4;
      ulvc_cwd_pre_len[0] = 0; ulvc_cwd_pre_len[1] = 1;
      ulvc_cwd_pre_len[2] = 2;
      ulvc_cwd_pre_len[3] = 3; ulvc_cwd_pre_len[4] = 3;
      ulvc_cwd_suf[0] = 0; ulvc_cwd_suf[1] = 0; ulvc_cwd_suf[2] = 0;
      ulvc_cwd_suf[3] = 0; ulvc_cwd_suf[4] = 1;
      ulvc_cwd_suf_len[0] = 0; ulvc_cwd_suf_len[1] = 0;
      ulvc_cwd_suf_len[2] = 0;
      ulvc_cwd_suf_len[3] = 1; ulvc_cwd_suf_len[4] = 1;
      for (int i = 5; i < 33; ++i)
      {
        ulvc_cwd_pre[i] = 0;
        ulvc_cwd_pre_len[i] = 3;
        ulvc_cwd_suf[i] = (ui32)(i-5);
        ulvc_cwd_suf_len[i] = 5;
      }
      return true;
    }

    /////////////////////////////////////////////////////////////////////////
    static bool tables_initialized = false;

    /////////////////////////////////////////////////////////////////////////
    bool initialize_block_encoder_tables_avx2() {
      if (!tables_initialized) {
        memset(vlc_tbl0, 0, 2048 * sizeof(ui32));
        memset(vlc_tbl1, 0, 2048 * sizeof(ui32));
        tables_initialized = vlc_init_tables();
        tables_initialized = tables_initialized && uvlc_init_tables();
      }
      return tables_initialized;
    }

    /////////////////////////////////////////////////////////////////////////
    //
    /////////////////////////////////////////////////////////////////////////
    struct mel_struct {
      //storage
      ui8* buf;      //pointer to data buffer
      ui32 pos;      //position of next writing within buf
      ui32 buf_size; //size of buffer, which we must not exceed

      // all these can be replaced by bytes
      int remaining_bits; //number of empty bits in tmp
      int tmp;            //temporary storage of coded bits
      int run;            //number of 0 run
      int k;              //state
      int threshold;      //threshold where one bit must be coded
    };

    //////////////////////////////////////////////////////////////////////////
    static inline void
    mel_init(mel_struct* melp, ui32 buffer_size, ui8* data)
    {
      melp->buf = data;
      melp->pos = 0;
      melp->buf_size = buffer_size;
      melp->remaining_bits = 8;
      melp->tmp = 0;
      melp->run = 0;
      melp->k = 0;
      melp->threshold = 1; // this is 1 << mel_exp[melp->k];
    }

    //////////////////////////////////////////////////////////////////////////
    static inline void
    mel_emit_bit(mel_struct* melp, int v)
    {
      melp->tmp = (melp->tmp << 1) + v;
      melp->remaining_bits--;
      if (melp->remaining_bits == 0) {
        melp->buf[melp->pos++] = (ui8)melp->tmp;
        melp->remaining_bits = (melp->tmp == 0xFF ? 7 : 8);
        melp->tmp = 0;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline void
    mel_encode(mel_struct* melp, bool bit)
    {
      //MEL exponent
      static const int mel_exp[13] = {0,0,0,1,1,1,2,2,2,3,3,4,5};

      if (bit == false) {
        ++melp->run;
        if (melp->run >= melp->threshold) {
          mel_emit_bit(melp, 1);
          melp->run = 0;
          melp->k = ojph_min(12, melp->k + 1);
          melp->threshold = 1 << mel_exp[melp->k];
        }
      } else {
        mel_emit_bit(melp, 0);
        int t = mel_exp[melp->k];
        while (t > 0) {
          mel_emit_bit(melp, (melp->run >> --t) & 1);
        }
        melp->run = 0;
        melp->k = ojph_max(0, melp->k - 1);
        melp->threshold = 1 << mel_exp[melp->k];
      }
    }

    /////////////////////////////////////////////////////////////////////////
    //
    /////////////////////////////////////////////////////////////////////////
    struct vlc_struct_avx2 {
      //storage
      ui8* buf;      //pointer to data buffer
      ui32 pos;      //position of next writing within buf
      ui32 buf_size; //size of buffer, which we must not exceed

      int used_bits; //number of occupied bits in tmp
      ui64 tmp;       //temporary storage of coded bits
      bool last_greater_than_8F; //true if last byte us greater than 0x8F
    };

    //////////////////////////////////////////////////////////////////////////
    static inline void
    vlc_init(vlc_struct_avx2* vlcp, ui32 buffer_size, ui8* data)
    {
      vlcp->buf = data + buffer_size - 1; //points to last byte
      vlcp->pos = 1;                      //locations will be all -pos
      vlcp->buf_size = buffer_size;

      vlcp->buf[0] = 0xFF;
      vlcp->used_bits = 4;
      vlcp->tmp = 0xF;
      vlcp->last_greater_than_8F = true;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline void
    vlc_encode(vlc_struct_avx2* vlcp, ui32 cwd, int cwd_len)
    {
      vlcp->tmp |= (ui64)cwd << vlcp->used_bits;
      vlcp->used_bits += cwd_len;

      while (vlcp->used_bits >= 8) {
          ui8 tmp;

          if (unlikely(vlcp->last_greater_than_8F)) {
              tmp = vlcp->tmp & 0x7F;

              if (likely(tmp != 0x7F)) {
                  tmp = vlcp->tmp & 0xFF;
                  *(vlcp->buf - vlcp->pos) = tmp;
                  vlcp->last_greater_than_8F = tmp > 0x8F;
                  vlcp->tmp >>= 8;
                  vlcp->used_bits -= 8;
              } else {
                  *(vlcp->buf - vlcp->pos) = tmp;
                  vlcp->last_greater_than_8F = false;
                  vlcp->tmp >>= 7;
                  vlcp->used_bits -= 7;
              }

          } else {
              tmp = vlcp->tmp & 0xFF;
              *(vlcp->buf - vlcp->pos) = tmp;
              vlcp->last_greater_than_8F = tmp > 0x8F;
              vlcp->tmp >>= 8;
              vlcp->used_bits -= 8;
          }

          vlcp->pos++;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////////////
    static inline void
    terminate_mel_vlc(mel_struct* melp, vlc_struct_avx2* vlcp)
    {
      if (melp->run > 0)
        mel_emit_bit(melp, 1);

      if (vlcp->last_greater_than_8F && (vlcp->tmp & 0x7f) == 0x7f) {
        *(vlcp->buf - vlcp->pos) = 0x7f;
        vlcp->pos++;
        vlcp->tmp >>= 7;
        vlcp->used_bits -= 7;
      }

      melp->tmp = melp->tmp << melp->remaining_bits;
      int mel_mask = (0xFF << melp->remaining_bits) & 0xFF;
      int vlc_mask = 0xFF >> (8 - vlcp->used_bits);
      if ((mel_mask | vlc_mask) == 0)
        return;  //last mel byte cannot be 0xFF, since then
                 //melp->remaining_bits would be < 8
      if (melp->pos >= melp->buf_size)
        OJPH_ERROR(0x00020003, "mel encoder's buffer is full");
      ui8 vlcp_tmp = (ui8)vlcp->tmp;
      int fuse = melp->tmp | vlcp_tmp;
      if ( ( ((fuse ^ melp->tmp) & mel_mask)
           | ((fuse ^ vlcp_tmp) & vlc_mask) ) == 0
          && (fuse != 0xFF) && vlcp->pos > 1)
      {
        melp->buf[melp->pos++] = (ui8)fuse;
      }
      else
      {
        if (vlcp->pos >= vlcp->buf_size)
          OJPH_ERROR(0x00020004, "vlc encoder's buffer is full");
        melp->buf[melp->pos++] = (ui8)melp->tmp; //melp->tmp cannot be 0xFF
        *(vlcp->buf - vlcp->pos) = (ui8)vlcp_tmp;
        vlcp->pos++;
      }
    }

/////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////
    struct ms_struct {
      //storage
      ui8* buf;      //pointer to data buffer
      ui32 pos;      //position of next writing within buf
      ui32 buf_size; //size of buffer, which we must not exceed

      int max_bits;  //maximum number of bits that can be store in tmp
      int used_bits; //number of occupied bits in tmp
      ui32 tmp;      //temporary storage of coded bits
    };

    //////////////////////////////////////////////////////////////////////////
    static inline void
    ms_init(ms_struct* msp, ui32 buffer_size, ui8* data)
    {
      msp->buf = data;
      msp->pos = 0;
      msp->buf_size = buffer_size;
      msp->max_bits = 8;
      msp->used_bits = 0;
      msp->tmp = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline void
    ms_encode(ms_struct* msp, ui64 cwd, int cwd_len)
    {
      while (cwd_len > 0)
      {
        if (msp->pos >= msp->buf_size)
          OJPH_ERROR(0x00020005, "magnitude sign encoder's buffer is full");
        int t = ojph_min(msp->max_bits - msp->used_bits, cwd_len);
        msp->tmp |= ((ui32)(cwd & ((1U << t) - 1))) << msp->used_bits;
        msp->used_bits += t;
        cwd >>= t;
        cwd_len -= t;
        if (msp->used_bits >= msp->max_bits)
        {
          msp->buf[msp->pos++] = (ui8)msp->tmp;
          msp->max_bits = (msp->tmp == 0xFF) ? 7 : 8;
          msp->tmp = 0;
          msp->used_bits = 0;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline void
    ms_terminate(ms_struct* msp)
    {
      if (msp->used_bits)
      {
        int t = msp->max_bits - msp->used_bits; //unused bits
        msp->tmp |= (0xFF & ((1U << t) - 1)) << msp->used_bits;
        msp->used_bits += t;
        if (msp->tmp != 0xFF)
        {
          if (msp->pos >= msp->buf_size)
            OJPH_ERROR(0x00020006, "magnitude sign encoder's buffer is full");
          msp->buf[msp->pos++] = (ui8)msp->tmp;
        }
      }
      else if (msp->max_bits == 7)
        msp->pos--;
    }

#define ZERO _mm256_setzero_si256()
#define ONE  _mm256_set1_epi32(1)

// https://stackoverflow.com/a/58827596
inline __m256i avx2_lzcnt_epi32(__m256i v) {
    // prevent value from being rounded up to the next power of two
    v = _mm256_andnot_si256(_mm256_srli_epi32(v, 8), v);  // keep 8 MSB

    v = _mm256_castps_si256(_mm256_cvtepi32_ps(v));    // convert an integer to float
    v = _mm256_srli_epi32(v, 23);                   // shift down the exponent
    v = _mm256_subs_epu16(_mm256_set1_epi32(158), v);  // undo bias
    v = _mm256_min_epi16(v, _mm256_set1_epi32(32));    // clamp at 32

    return v;
}

inline __m256i avx2_cmpneq_epi32(__m256i v, __m256i v2) {
    return _mm256_xor_si256(_mm256_cmpeq_epi32(v, v2), _mm256_set1_epi32((int32_t)0xffffffff));
}

static void proc_pixel(__m256i *src_vec, ui32 p,
                       __m256i *eq_vec, __m256i *s_vec,
                       __m256i &rho_vec, __m256i &e_qmax_vec)
{
    __m256i val_vec[4];
    __m256i _eq_vec[4];
    __m256i _s_vec[4];
    __m256i _rho_vec[4];

    for (ui32 i = 0; i < 4; ++i) {
        /* val = t + t; //multiply by 2 and get rid of sign */
        val_vec[i] = _mm256_add_epi32(src_vec[i], src_vec[i]);

        /* val >>= p;  // 2 \mu_p + x */
        val_vec[i] = _mm256_srli_epi32(val_vec[i], (int)p);

        /* val &= ~1u; // 2 \mu_p */
        val_vec[i] = _mm256_and_si256(val_vec[i], _mm256_set1_epi32((int)~1u));

        /* if (val) { */
        const __m256i val_notmask = avx2_cmpneq_epi32(val_vec[i], ZERO);

        /*   rho[i] = 1 << i;
         *   rho is processed below.
         */

        /*   e_q[i] = 32 - (int)count_leading_ZEROs(--val); //2\mu_p - 1 */
        val_vec[i] = _mm256_sub_epi32(val_vec[i], ONE);
        _eq_vec[i] = avx2_lzcnt_epi32(val_vec[i]);
        _eq_vec[i] = _mm256_sub_epi32(_mm256_set1_epi32(32), _eq_vec[i]);

        /*   e_qmax[i] = ojph_max(e_qmax[i], e_q[j]);
         *   e_qmax is processed below
         */

        /*   s[0] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n */
        val_vec[i] = _mm256_sub_epi32(val_vec[i], ONE);
        _s_vec[i] = _mm256_srli_epi32(src_vec[i], 31);
        _s_vec[i] = _mm256_add_epi32(_s_vec[i], val_vec[i]);

        _eq_vec[i] = _mm256_and_si256(_eq_vec[i], val_notmask);
        _s_vec[i] = _mm256_and_si256(_s_vec[i], val_notmask);
        val_vec[i] = _mm256_srli_epi32(val_notmask, 31);
        /* } */
    }

    const __m256i idx = _mm256_set_epi32(7, 5, 3, 1, 6, 4, 2, 0);

    /* Reorder from
     * *_vec[0]:[0, 0], [0, 1], [0, 2], [0, 3], [0, 4], [0, 5], [0, 6], [0, 7]
     * *_vec[1]:[1, 0], [1, 1], [1, 2], [1, 3], [1, 4], [1, 5],.[1, 6], [1, 7]
     * *_vec[2]:[0, 8], [0, 9], [0,10], [0,11], [0,12], [0,13], [0,14], [0,15]
     * *_vec[3]:[1, 8], [1, 9], [1,10], [1,11], [1,12], [1,13], [1,14], [1,15]
     * to
     * *_vec[0]:[0, 0], [0, 2], [0, 4], [0, 6], [0, 8], [0,10], [0,12], [0,14]
     * *_vec[1]:[1, 0], [1, 2], [1, 4], [1, 6], [1, 8], [1,10], [1,12], [1,14]
     * *_vec[2]:[0, 1], [0, 3], [0, 5], [0, 7], [0, 9], [0,11], [0,13], [0,15]
     * *_vec[3]:[1, 1], [1, 3], [1, 5], [1, 7], [1, 9], [1,11], [1,13], [1,15]
     */
    __m256i tmp1, tmp2;
    for (ui32 i = 0; i < 2; ++i) {
        tmp1 = _mm256_permutevar8x32_epi32(_eq_vec[0 + i], idx);
        tmp2 = _mm256_permutevar8x32_epi32(_eq_vec[2 + i], idx);
        eq_vec[0 + i] = _mm256_permute2x128_si256(tmp1, tmp2, (0 << 0) + (2 << 4));
        eq_vec[2 + i] = _mm256_permute2x128_si256(tmp1, tmp2, (1 << 0) + (3 << 4));

        tmp1 = _mm256_permutevar8x32_epi32(_s_vec[0 + i], idx);
        tmp2 = _mm256_permutevar8x32_epi32(_s_vec[2 + i], idx);
        s_vec[0 + i] = _mm256_permute2x128_si256(tmp1, tmp2, (0 << 0) + (2 << 4));
        s_vec[2 + i] = _mm256_permute2x128_si256(tmp1, tmp2, (1 << 0) + (3 << 4));

        tmp1 = _mm256_permutevar8x32_epi32(val_vec[0 + i], idx);
        tmp2 = _mm256_permutevar8x32_epi32(val_vec[2 + i], idx);
        _rho_vec[0 + i] = _mm256_permute2x128_si256(tmp1, tmp2, (0 << 0) + (2 << 4));
        _rho_vec[2 + i] = _mm256_permute2x128_si256(tmp1, tmp2, (1 << 0) + (3 << 4));
    }

    e_qmax_vec = _mm256_max_epi32(eq_vec[0], eq_vec[1]);
    e_qmax_vec = _mm256_max_epi32(e_qmax_vec, eq_vec[2]);
    e_qmax_vec = _mm256_max_epi32(e_qmax_vec, eq_vec[3]);
    _rho_vec[1] = _mm256_slli_epi32(_rho_vec[1], 1);
    _rho_vec[2] = _mm256_slli_epi32(_rho_vec[2], 2);
    _rho_vec[3] = _mm256_slli_epi32(_rho_vec[3], 3);
    rho_vec = _mm256_or_si256(_rho_vec[0], _rho_vec[1]);
    rho_vec = _mm256_or_si256(rho_vec, _rho_vec[2]);
    rho_vec = _mm256_or_si256(rho_vec, _rho_vec[3]);
}

/* from [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, ...]
 *      [0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, ...]
 *      [0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, ...]
 *      [0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, ...]
 *
 * to   [0x00, 0x10, 0x20, 0x30, 0x01, 0x11, 0x21, 0x31,
 *       0x02, 0x12, 0x22, 0x32, 0x03, 0x13, 0x23, 0x33]
 *
 *      [0x04, 0x14, 0x24, 0x34, 0x05, 0x15, 0x25, 0x35,
 *       0x06, 0x16, 0x26, 0x36, 0x07, 0x17, 0x27, 0x37]
 *
 *      [..]
 */
static void rotate_matrix(__m256i *matrix)
{
    __m256i tmp1 = _mm256_unpacklo_epi32(matrix[0], matrix[1]);
    __m256i tmp2 = _mm256_unpacklo_epi32(matrix[2], matrix[3]);
    __m256i tmp3 = _mm256_unpackhi_epi32(matrix[0], matrix[1]);
    __m256i tmp4 = _mm256_unpackhi_epi32(matrix[2], matrix[3]);

    matrix[0] = _mm256_unpacklo_epi64(tmp1, tmp2);
    matrix[1] = _mm256_unpacklo_epi64(tmp3, tmp4);
    matrix[2] = _mm256_unpackhi_epi64(tmp1, tmp2);
    matrix[3] = _mm256_unpackhi_epi64(tmp3, tmp4);

    tmp1 = _mm256_permute2x128_si256(matrix[0], matrix[2], 0x20);
    matrix[2] = _mm256_permute2x128_si256(matrix[0], matrix[2], 0x31);
    matrix[0] = tmp1;

    tmp1 = _mm256_permute2x128_si256(matrix[1], matrix[3], 0x20);
    matrix[3] = _mm256_permute2x128_si256(matrix[1], matrix[3], 0x31);
    matrix[1] = tmp1;
}

static void proc_ms_encode(ms_struct *msp,
                           __m256i &tuple_vec,
                           __m256i &uq_vec,
                           __m256i &rho_vec,
                           __m256i *s_vec)
{
    __m256i m_vec[4];

    /* Prepare parameters for ms_encode */
    /* m = (rho[i] & 1) ? Uq[i] - ((tuple[i] & 1) >> 0) : 0; */
    auto tmp = _mm256_and_si256(tuple_vec, ONE);
    tmp = _mm256_sub_epi32(uq_vec, tmp);
    auto tmp1 = _mm256_and_si256(rho_vec, ONE);
    auto mask = avx2_cmpneq_epi32(tmp1, ZERO);
    m_vec[0] = _mm256_and_si256(mask, tmp);

    /* m = (rho[i] & 2) ? Uq[i] - ((tuple[i] & 2) >> 1) : 0; */
    tmp = _mm256_and_si256(tuple_vec, _mm256_set1_epi32(2));
    tmp = _mm256_srli_epi32(tmp, 1);
    tmp = _mm256_sub_epi32(uq_vec, tmp);
    tmp1 = _mm256_and_si256(rho_vec, _mm256_set1_epi32(2));
    mask = avx2_cmpneq_epi32(tmp1, ZERO);
    m_vec[1] = _mm256_and_si256(mask, tmp);

    /* m = (rho[i] & 4) ? Uq[i] - ((tuple[i] & 4) >> 2) : 0; */
    tmp = _mm256_and_si256(tuple_vec, _mm256_set1_epi32(4));
    tmp = _mm256_srli_epi32(tmp, 2);
    tmp = _mm256_sub_epi32(uq_vec, tmp);
    tmp1 = _mm256_and_si256(rho_vec, _mm256_set1_epi32(4));
    mask = avx2_cmpneq_epi32(tmp1, ZERO);
    m_vec[2] = _mm256_and_si256(mask, tmp);

    /* m = (rho[i] & 8) ? Uq[i] - ((tuple[i] & 8) >> 3) : 0; */
    tmp = _mm256_and_si256(tuple_vec, _mm256_set1_epi32(8));
    tmp = _mm256_srli_epi32(tmp, 3);
    tmp = _mm256_sub_epi32(uq_vec, tmp);
    tmp1 = _mm256_and_si256(rho_vec, _mm256_set1_epi32(8));
    mask = avx2_cmpneq_epi32(tmp1, ZERO);
    m_vec[3] = _mm256_and_si256(mask, tmp);

    rotate_matrix(m_vec);
    /* s_vec from
     * s_vec[0]:[0, 0], [0, 2] ... [0,14], [0, 16], [0, 18] ... [0,30]
     * s_vec[1]:[1, 0], [1, 2] ... [1,14], [1, 16], [1, 18] ... [1,30]
     * s_vec[2]:[0, 1], [0, 3] ... [0,15], [0, 17], [0, 19] ... [0,31]
     * s_vec[3]:[1, 1], [1, 3] ... [1,15], [1, 17], [1, 19] ... [1,31]
     * to
     * s_vec[0]:[0, 0], [1, 0], [0, 1], [1, 1], [0, 2], [1, 2]...[0, 7], [1, 7]
     * s_vec[1]:[0, 8], [1, 8], [0, 9], [1, 9], [0,10], [1,10]...[0,15], [1,15]
     * s_vec[2]:[0,16], [1,16], [0,17], [1,17], [0,18], [1,18]...[0,23], [1,23]
     * s_vec[3]:[0,24], [1,24], [0,25], [1,25], [0,26], [1,26]...[0,31], [1,31]
     */
    rotate_matrix(s_vec);

    ui32 cwd[8];
    int cwd_len[8];
    ui64 _cwd = 0;
    int _cwd_len = 0;

    /* Each iteration process 8 bytes * 2 lines */
    for (ui32 i = 0; i < 4; ++i) {
        /* cwd = s[i * 4 + 0] & ((1U << m) - 1)
         * cwd_len = m
         */
        _mm256_storeu_si256((__m256i *)cwd_len, m_vec[i]);
        tmp = _mm256_sllv_epi32(ONE, m_vec[i]);
        tmp = _mm256_sub_epi32(tmp, ONE);
        tmp = _mm256_and_si256(tmp, s_vec[i]);
        _mm256_storeu_si256((__m256i*)cwd, tmp);

        for (ui32 j = 0; j < 4; ++j) {
            ui32 idx = j * 2;
            _cwd     = cwd[idx];
            _cwd_len = cwd_len[idx];
            _cwd     |= ((ui64)cwd[idx + 1]) << _cwd_len;
            _cwd_len += cwd_len[idx + 1];
            ms_encode(msp, _cwd, _cwd_len);
        }
    }
}

static __m256i cal_eps_vec(__m256i *eq_vec, __m256i &u_q_vec,
                           __m256i &e_qmax_vec)
{
    /* if (u_q[i] > 0) {
     *     eps[i] |= (e_q[i * 4 + 0] == e_qmax[i]);
     *     eps[i] |= (e_q[i * 4 + 1] == e_qmax[i]) << 1;
     *     eps[i] |= (e_q[i * 4 + 2] == e_qmax[i]) << 2;
     *     eps[i] |= (e_q[i * 4 + 3] == e_qmax[i]) << 3;
     * }
     */
    auto u_q_mask = _mm256_cmpgt_epi32(u_q_vec, ZERO);

    auto mask = _mm256_cmpeq_epi32(eq_vec[0], e_qmax_vec);
    auto eps_vec = _mm256_srli_epi32(mask, 31);

    mask = _mm256_cmpeq_epi32(eq_vec[1], e_qmax_vec);
    auto tmp = _mm256_srli_epi32(mask, 31);
    tmp = _mm256_slli_epi32(tmp, 1);
    eps_vec = _mm256_or_si256(eps_vec, tmp);

    mask = _mm256_cmpeq_epi32(eq_vec[2], e_qmax_vec);
    tmp = _mm256_srli_epi32(mask, 31);
    tmp = _mm256_slli_epi32(tmp, 2);
    eps_vec = _mm256_or_si256(eps_vec, tmp);

    mask = _mm256_cmpeq_epi32(eq_vec[3], e_qmax_vec);
    tmp = _mm256_srli_epi32(mask, 31);
    tmp = _mm256_slli_epi32(tmp, 3);
    eps_vec = _mm256_or_si256(eps_vec, tmp);

    return  _mm256_and_si256(u_q_mask, eps_vec);
}

static void update_lep(ui32 x, __m256i &prev_e_val_vec,
                       __m256i *eq_vec, __m256i *e_val_vec,
                       const __m256i left_shift)
{
    /* lep[0] = ojph_max(lep[0], (ui8)e_q[1]); lep++;
     * lep[0] = (ui8)e_q[3];
     * Compare e_q[1] with e_q[3] of the prevous round.
     */
    auto tmp = _mm256_permutevar8x32_epi32(eq_vec[3], left_shift);
    tmp = _mm256_insert_epi32(tmp, _mm_cvtsi128_si32(_mm256_castsi256_si128(prev_e_val_vec)), 0);
    prev_e_val_vec = _mm256_insert_epi32(ZERO, _mm256_extract_epi32(eq_vec[3], 7), 0);
    e_val_vec[x] = _mm256_max_epi32(eq_vec[1], tmp);
}


static void update_lcxp(ui32 x, __m256i &prev_cx_val_vec,
                        __m256i &rho_vec, __m256i *cx_val_vec,
                        const __m256i left_shift)
{
    /* lcxp[0] = (ui8)(lcxp[0] | (ui8)((rho[0] & 2) >> 1)); lcxp++;
     * lcxp[0] = (ui8)((rho[0] & 8) >> 3);
     * Or (rho[0] & 2) and (rho[0] of the previous round & 8).
     */
    auto tmp = _mm256_permutevar8x32_epi32(rho_vec, left_shift);
    tmp = _mm256_insert_epi32(tmp, _mm_cvtsi128_si32(_mm256_castsi256_si128(prev_cx_val_vec)), 0);
    prev_cx_val_vec = _mm256_insert_epi32(ZERO, _mm256_extract_epi32(rho_vec, 7), 0);

    tmp = _mm256_and_si256(tmp, _mm256_set1_epi32(8));
    tmp = _mm256_srli_epi32(tmp, 3);

    auto tmp1 = _mm256_and_si256(rho_vec, _mm256_set1_epi32(2));
    tmp1 = _mm256_srli_epi32(tmp1, 1);
    cx_val_vec[x] = _mm256_or_si256(tmp, tmp1);
}

static __m256i cal_tuple(__m256i &cq_vec, __m256i &rho_vec,
                         __m256i &eps_vec, ui32 *vlc_tbl)
{
    /* tuple[i] = vlc_tbl1[(c_q[i] << 8) + (rho[i] << 4) + eps[i]]; */
    auto tmp = _mm256_slli_epi32(cq_vec, 8);
    auto tmp1 = _mm256_slli_epi32(rho_vec, 4);
    tmp = _mm256_add_epi32(tmp, tmp1);
    tmp = _mm256_add_epi32(tmp, eps_vec);
    return _mm256_i32gather_epi32((const int *)vlc_tbl, tmp, 4);
}

static __m256i proc_cq1(ui32 x, __m256i *cx_val_vec, __m256i &rho_vec,
                        const __m256i right_shift)
{
    ojph_unused(x);
    ojph_unused(cx_val_vec);
    ojph_unused(right_shift);

    /* c_q[i + 1] = (rho[i] >> 1) | (rho[i] & 1); */
    auto tmp = _mm256_srli_epi32(rho_vec, 1);
    auto tmp1 = _mm256_and_si256(rho_vec, ONE);
    return _mm256_or_si256(tmp, tmp1);
}

static __m256i proc_cq2(ui32 x, __m256i *cx_val_vec, __m256i &rho_vec,
                        const __m256i right_shift)
{
    // c_q[i + 1] = (lcxp[i + 1] + (lcxp[i + 2] << 2))
    //            | (((rho[i] & 4) >> 1) | ((rho[i] & 8) >> 2));
    auto lcxp1_vec = _mm256_permutevar8x32_epi32(cx_val_vec[x], right_shift);
    auto tmp = _mm256_permutevar8x32_epi32(lcxp1_vec, right_shift);

#ifdef OJPH_ARCH_X86_64
    tmp = _mm256_insert_epi64(tmp, 
      _mm_cvtsi128_si64(_mm256_castsi256_si128(cx_val_vec[x + 1])), 3);
#elif (defined OJPH_ARCH_I386)
    int lsb = _mm_cvtsi128_si32(_mm256_castsi256_si128(cx_val_vec[x + 1]));
    tmp = _mm256_insert_epi32(tmp, lsb, 6);
    int msb = _mm_extract_epi32(_mm256_castsi256_si128(cx_val_vec[x + 1]), 1);
    tmp = _mm256_insert_epi32(tmp, msb, 7);
#else
    #error Error unsupport compiler
#endif
    tmp = _mm256_slli_epi32(tmp, 2);
    auto tmp1 = _mm256_insert_epi32(lcxp1_vec, 
      _mm_cvtsi128_si32(_mm256_castsi256_si128(cx_val_vec[x + 1])), 7);
    tmp = _mm256_add_epi32(tmp1, tmp);

    tmp1 = _mm256_and_si256(rho_vec, _mm256_set1_epi32(4));
    tmp1 = _mm256_srli_epi32(tmp1, 1);
    tmp = _mm256_or_si256(tmp, tmp1);

    tmp1 = _mm256_and_si256(rho_vec, _mm256_set1_epi32(8));
    tmp1 = _mm256_srli_epi32(tmp1, 2);

    return _mm256_or_si256(tmp, tmp1);
}

using fn_proc_cq = __m256i (*)(ui32, __m256i *, __m256i &, const __m256i);

static void proc_mel_encode1(mel_struct *melp, __m256i &cq_vec,
                             __m256i &rho_vec, __m256i u_q_vec, ui32 ignore,
                             const __m256i right_shift)
{
    int32_t mel_need_encode[8];
    int32_t mel_need_encode2[8];
    int32_t mel_bit[8];
    int32_t mel_bit2[8];
    /* Prepare mel_encode params */
    /* if (c_q[i] == 0) { */
    _mm256_storeu_si256((__m256i *)mel_need_encode, _mm256_cmpeq_epi32(cq_vec, ZERO));
    /*   mel_encode(&mel, rho[i] != 0); */
    _mm256_storeu_si256((__m256i*)mel_bit, _mm256_srli_epi32(avx2_cmpneq_epi32(rho_vec, ZERO), 31));
    /* } */

    /*   mel_encode(&mel, ojph_min(u_q[i], u_q[i + 1]) > 2); */
    auto tmp = _mm256_permutevar8x32_epi32(u_q_vec, right_shift);
    auto tmp1 = _mm256_min_epi32(u_q_vec, tmp);
    _mm256_storeu_si256((__m256i*)mel_bit2, _mm256_srli_epi32(_mm256_cmpgt_epi32(tmp1, _mm256_set1_epi32(2)), 31));

    /* if (u_q[i] > 0 && u_q[i + 1] > 0) { } */
    auto need_encode2 = _mm256_cmpgt_epi32(u_q_vec, ZERO);
    _mm256_storeu_si256((__m256i*)mel_need_encode2, _mm256_and_si256(need_encode2, _mm256_cmpgt_epi32(tmp, ZERO)));

    ui32 i_max = 8 - (ignore / 2);

    for (ui32 i = 0; i < i_max; i += 2) {
        if (mel_need_encode[i]) {
            mel_encode(melp, mel_bit[i]);
        }

        if (i + 1 < i_max) {
            if (mel_need_encode[i + 1]) {
                mel_encode(melp, mel_bit[i + 1]);
            }
        }

        if (mel_need_encode2[i]) {
            mel_encode(melp, mel_bit2[i]);
        }
    }
}

static void proc_mel_encode2(mel_struct *melp, __m256i &cq_vec,
                             __m256i &rho_vec, __m256i u_q_vec, ui32 ignore,
                             const __m256i right_shift)
{
    ojph_unused(u_q_vec);
    ojph_unused(right_shift);
    int32_t mel_need_encode[8];
    int32_t mel_bit[8];

    /* Prepare mel_encode params */
    /* if (c_q[i] == 0) { */
    _mm256_storeu_si256((__m256i*)mel_need_encode, _mm256_cmpeq_epi32(cq_vec, ZERO));
    /*   mel_encode(&mel, rho[i] != 0); */
    _mm256_storeu_si256((__m256i*)mel_bit, _mm256_srli_epi32(avx2_cmpneq_epi32(rho_vec, ZERO), 31));
    /* } */

    ui32 i_max = 8 - (ignore / 2);

    for (ui32 i = 0; i < i_max; ++i) {
        if (mel_need_encode[i]) {
            mel_encode(melp, mel_bit[i]);
        }
    }
}

using fn_proc_mel_encode = void (*)(mel_struct *, __m256i &, __m256i &,
                                    __m256i, ui32, const __m256i);

static void proc_vlc_encode1(vlc_struct_avx2 *vlcp, ui32 *tuple,
                             ui32 *u_q, ui32 ignore)
{
    ui32 i_max = 8 - (ignore / 2);

    for (ui32 i = 0; i < i_max; i += 2) {
        /* 7 bits */
        ui32 val = tuple[i + 0] >> 4;
        int size = tuple[i + 0] & 7;

        if (i + 1 < i_max) {
            /* 7 bits */
            val |= (tuple[i + 1] >> 4) << size;
            size += tuple[i + 1] & 7;
        }

        if (u_q[i] > 2 && u_q[i + 1] > 2) {
            /* 3 bits */
            val |= (ulvc_cwd_pre[u_q[i] - 2]) << size;
            size += ulvc_cwd_pre_len[u_q[i] - 2];

            /* 3 bits */
            val |= (ulvc_cwd_pre[u_q[i + 1] - 2]) << size;
            size += ulvc_cwd_pre_len[u_q[i + 1] - 2];

            /* 5 bits */
            val |= (ulvc_cwd_suf[u_q[i] - 2]) << size;
            size += ulvc_cwd_suf_len[u_q[i] - 2];

            /* 5 bits */
            val |= (ulvc_cwd_suf[u_q[i + 1] - 2]) << size;
            size += ulvc_cwd_suf_len[u_q[i + 1] - 2];

        } else if (u_q[i] > 2 && u_q[i + 1] > 0) {
            /* 3 bits */
            val |= (ulvc_cwd_pre[u_q[i]]) << size;
            size += ulvc_cwd_pre_len[u_q[i]];

            /* 1 bit */
            val |= (u_q[i + 1] - 1) << size;
            size += 1;

            /* 5 bits */
            val |= (ulvc_cwd_suf[u_q[i]]) << size;
            size += ulvc_cwd_suf_len[u_q[i]];

        } else {
            /* 3 bits */
            val |= (ulvc_cwd_pre[u_q[i]]) << size;
            size += ulvc_cwd_pre_len[u_q[i]];

            /* 3 bits */
            val |= (ulvc_cwd_pre[u_q[i + 1]]) << size;
            size += ulvc_cwd_pre_len[u_q[i + 1]];

            /* 5 bits */
            val |= (ulvc_cwd_suf[u_q[i]]) << size;
            size += ulvc_cwd_suf_len[u_q[i]];

            /* 5 bits */
            val |= (ulvc_cwd_suf[u_q[i + 1]]) << size;
            size += ulvc_cwd_suf_len[u_q[i + 1]];
        }

        vlc_encode(vlcp, val, size);
    }
}

static void proc_vlc_encode2(vlc_struct_avx2 *vlcp, ui32 *tuple,
                             ui32 *u_q, ui32 ignore)
{
    ui32 i_max = 8 - (ignore / 2);

    for (ui32 i = 0; i < i_max; i += 2) {
        /* 7 bits */
        ui32 val = tuple[i + 0] >> 4;
        int size = tuple[i + 0] & 7;

        if (i + 1 < i_max) {
            /* 7 bits */
            val |= (tuple[i + 1] >> 4) << size;
            size += tuple[i + 1] & 7;
        }

        /* 3 bits */
        val |= ulvc_cwd_pre[u_q[i]] << size;
        size += ulvc_cwd_pre_len[u_q[i]];

        /* 3 bits */
        val |= (ulvc_cwd_pre[u_q[i + 1]]) << size;
        size += ulvc_cwd_pre_len[u_q[i + 1]];

        /* 5 bits */
        val |= (ulvc_cwd_suf[u_q[i + 0]]) << size;
        size += ulvc_cwd_suf_len[u_q[i + 0]];

        /* 5 bits */
        val |= (ulvc_cwd_suf[u_q[i + 1]]) << size;
        size += ulvc_cwd_suf_len[u_q[i + 1]];

        vlc_encode(vlcp, val, size);
    }
}

using fn_proc_vlc_encode = void (*)(vlc_struct_avx2 *, ui32 *, ui32 *, ui32);

void ojph_encode_codeblock_avx2(ui32* buf, ui32 missing_msbs,
                                ui32 num_passes, ui32 _width, ui32 height,
                                ui32 stride, ui32* lengths,
                                ojph::mem_elastic_allocator *elastic,
                                ojph::coded_lists *& coded)
{
    ojph_unused(num_passes);                      //currently not used

    ui32 width = (_width + 15) & ~15u;
    ui32 ignore = width - _width;
    const int ms_size = (16384 * 16 + 14) / 15; //more than enough
    const int mel_vlc_size = 3072;              //more than enough
    const int mel_size = 192;
    const int vlc_size = mel_vlc_size - mel_size;

    ui8 ms_buf[ms_size];
    ui8 mel_vlc_buf[mel_vlc_size];
    ui8 *mel_buf = mel_vlc_buf;
    ui8 *vlc_buf = mel_vlc_buf + mel_size;

    mel_struct mel;
    mel_init(&mel, mel_size, mel_buf);
    vlc_struct_avx2 vlc;
    vlc_init(&vlc, vlc_size, vlc_buf);
    ms_struct ms;
    ms_init(&ms, ms_size, ms_buf);

    const ui32 p = 30 - missing_msbs;

    //e_val: E values for a line (these are the highest set bit)
    //cx_val: is the context values
    //Each byte stores the info for the 2 sample. For E, it is maximum
    // of the two samples, while for cx, it is the OR of these two samples.
    //The maximum is between the pixel at the bottom left of one quad
    // and the bottom right of the earlier quad. The same is true for cx.
    //For a 1024 pixels, we need 512 bytes, the 2 extra,
    // one for the non-existing earlier quad, and one for beyond the
    // the end
    const __m256i right_shift = _mm256_set_epi32(
        0, 7, 6, 5, 4, 3, 2, 1
    );

    const __m256i left_shift = _mm256_set_epi32(
        6, 5, 4, 3, 2, 1, 0, 7
    );

    ui32 n_loop = (width + 15) / 16;

    __m256i e_val_vec[65];
    for (ui32 i = 0; i <ojph_min(64, n_loop); ++i) {
        e_val_vec[i] = ZERO;
    }
    __m256i prev_e_val_vec = ZERO;

    __m256i cx_val_vec[65];
    __m256i prev_cx_val_vec = ZERO;

    ui32 prev_cq = 0;

    __m256i eq_vec[4];
    __m256i s_vec[4];
    __m256i src_vec[4];

    ui32 *vlc_tbl = vlc_tbl0;
    fn_proc_cq proc_cq = proc_cq1;
    fn_proc_mel_encode proc_mel_encode = proc_mel_encode1;
    fn_proc_vlc_encode proc_vlc_encode = proc_vlc_encode1;

    /* 2 lines per iteration */
    for (ui32 y = 0; y < height; y += 2)
    {
        e_val_vec[n_loop] = prev_e_val_vec;
        /* lcxp[0] = (ui8)((rho[0] & 8) >> 3); */
        __m256i tmp = _mm256_and_si256(prev_cx_val_vec, _mm256_set1_epi32(8));
        cx_val_vec[n_loop] = _mm256_srli_epi32(tmp, 3);

        prev_e_val_vec = ZERO;
        prev_cx_val_vec = ZERO;

        ui32 *sp = buf + y * stride;

        /* 16 bytes per iteration */
        for (ui32 x = 0; x < n_loop; ++x) {

            /* t = sp[i]; */
            if ((x == (n_loop - 1)) && (_width % 16)) {
                ui32 tmp_buf[16] = { 0 };
                memcpy(tmp_buf, sp, (_width % 16) * sizeof(ui32));
                src_vec[0] = _mm256_loadu_si256((__m256i*)(tmp_buf));
                src_vec[2] = _mm256_loadu_si256((__m256i*)(tmp_buf + 8));
                if (y + 1 < height) {
                    memcpy(tmp_buf, sp + stride, (_width % 16) * sizeof(ui32));
                    src_vec[1] = _mm256_loadu_si256((__m256i*)(tmp_buf));
                    src_vec[3] = _mm256_loadu_si256((__m256i*)(tmp_buf + 8));
                }
                else {
                    src_vec[1] = ZERO;
                    src_vec[3] = ZERO;
                }
            }
            else {
                src_vec[0] = _mm256_loadu_si256((__m256i*)(sp));
                src_vec[2] = _mm256_loadu_si256((__m256i*)(sp + 8));

                if (y + 1 < height) {
                    src_vec[1] = _mm256_loadu_si256((__m256i*)(sp + stride));
                    src_vec[3] = _mm256_loadu_si256((__m256i*)(sp + 8 + stride));
                }
                else {
                    src_vec[1] = ZERO;
                    src_vec[3] = ZERO;
                }
                sp += 16;
            }

            /* src_vec layout:
             * src_vec[0]:[0, 0],[0, 1],[0, 2],[0, 3],[0, 4],[0, 5],.[0, 6],.[0, 7]
             * src_vec[1]:[1, 0],[1, 1],[1, 2],[1, 3],[1, 4],[1, 5],.[1, 6],.[1, 7]
             * src_vec[2]:[0, 8],[0, 9],[0,10],[0,11],[0,12],[0,13],.[0,14], [0,15]
             * src_vec[3]:[1, 8],[1, 9],[1,10],[1,11],[1,12],[1,13],.[1,14], [1,15]
             */
            __m256i rho_vec, e_qmax_vec;
            proc_pixel(src_vec, p, eq_vec, s_vec, rho_vec, e_qmax_vec);

            // max_e[(i + 1) % num] = ojph_max(lep[i + 1], lep[i + 2]) - 1;
            tmp = _mm256_permutevar8x32_epi32(e_val_vec[x], right_shift);
            tmp = _mm256_insert_epi32(tmp, _mm_cvtsi128_si32(_mm256_castsi256_si128(e_val_vec[x + 1])), 7);

            auto max_e_vec = _mm256_max_epi32(tmp, e_val_vec[x]);
            max_e_vec = _mm256_sub_epi32(max_e_vec, ONE);

            // kappa[i] = (rho[i] & (rho[i] - 1)) ? ojph_max(1, max_e[i]) : 1;
            tmp = _mm256_max_epi32(max_e_vec, ONE);
            __m256i tmp1 = _mm256_sub_epi32(rho_vec, ONE);
            tmp1 = _mm256_and_si256(rho_vec, tmp1);

            auto cmp = _mm256_cmpeq_epi32(tmp1, ZERO);
            auto kappa_vec1_ = _mm256_and_si256(cmp, ONE);
            auto kappa_vec2_ = _mm256_and_si256(_mm256_xor_si256(cmp, _mm256_set1_epi32((int32_t)0xffffffff)), tmp);
            const __m256i kappa_vec = _mm256_max_epi32(kappa_vec1_, kappa_vec2_);

            /* cq[1 - 16] = cq_vec
             * cq[0] = prev_cq_vec[0]
             */
            tmp = proc_cq(x, cx_val_vec, rho_vec, right_shift);

            auto cq_vec = _mm256_permutevar8x32_epi32(tmp, left_shift);
            cq_vec = _mm256_insert_epi32(cq_vec, prev_cq, 0);
            prev_cq = (ui32)_mm256_extract_epi32(tmp, 7);

            update_lep(x, prev_e_val_vec, eq_vec, e_val_vec, left_shift);
            update_lcxp(x, prev_cx_val_vec, rho_vec, cx_val_vec, left_shift);

            /* Uq[i] = ojph_max(e_qmax[i], kappa[i]); */
            /* u_q[i] = Uq[i] - kappa[i]; */
            auto uq_vec = _mm256_max_epi32(kappa_vec, e_qmax_vec);
            auto u_q_vec = _mm256_sub_epi32(uq_vec, kappa_vec);

            auto eps_vec = cal_eps_vec(eq_vec, u_q_vec, e_qmax_vec);
            __m256i tuple_vec = cal_tuple(cq_vec, rho_vec, eps_vec, vlc_tbl);
            ui32 _ignore = ((n_loop - 1) == x) ? ignore : 0;

            proc_mel_encode(&mel, cq_vec, rho_vec, u_q_vec, _ignore,
                            right_shift);

            proc_ms_encode(&ms, tuple_vec, uq_vec, rho_vec, s_vec);

            // vlc_encode(&vlc, tuple[i*2+0] >> 8, (tuple[i*2+0] >> 4) & 7);
            // vlc_encode(&vlc, tuple[i*2+1] >> 8, (tuple[i*2+1] >> 4) & 7);
            ui32 u_q[8];
            ui32 tuple[8];
            /* The tuple is scaled by 4 due to:
             * vlc_encode(&vlc, tuple0 >> 8, (tuple0 >> 4) & 7, true);
             * So in the vlc_encode, the tuple will only be scaled by 2.
             */
            tuple_vec = _mm256_srli_epi32(tuple_vec, 4);
            _mm256_storeu_si256((__m256i*)tuple, tuple_vec);
            _mm256_storeu_si256((__m256i*)u_q, u_q_vec);

            proc_vlc_encode(&vlc, tuple, u_q, _ignore);
        }

        tmp = _mm256_permutevar8x32_epi32(cx_val_vec[0], right_shift);
        tmp = _mm256_slli_epi32(tmp, 2);
        tmp = _mm256_add_epi32(tmp, cx_val_vec[0]);
        prev_cq = (ui32)_mm_cvtsi128_si32(_mm256_castsi256_si128(tmp));

        proc_cq = proc_cq2;
        vlc_tbl = vlc_tbl1;
        proc_mel_encode = proc_mel_encode2;
        proc_vlc_encode = proc_vlc_encode2;
    }

    ms_terminate(&ms);
    terminate_mel_vlc(&mel, &vlc);

    //copy to elastic
    lengths[0] = mel.pos + vlc.pos + ms.pos;
    elastic->get_buffer(mel.pos + vlc.pos + ms.pos, coded);
    memcpy(coded->buf, ms.buf, ms.pos);
    memcpy(coded->buf + ms.pos, mel.buf, mel.pos);
    memcpy(coded->buf + ms.pos + mel.pos, vlc.buf - vlc.pos + 1, vlc.pos);

    // put in the interface locator word
    ui32 num_bytes = mel.pos + vlc.pos;
    coded->buf[lengths[0]-1] = (ui8)(num_bytes >> 4);
    coded->buf[lengths[0]-2] = coded->buf[lengths[0]-2] & 0xF0;
    coded->buf[lengths[0]-2] =
        (ui8)(coded->buf[lengths[0]-2] | (num_bytes & 0xF));

    coded->avail_size -= lengths[0];
}

} /* namespace local */
} /* namespace ojph */

#endif
