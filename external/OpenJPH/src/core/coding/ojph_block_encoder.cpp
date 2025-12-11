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
// File: ojph_block_encoder.cpp
// Author: Aous Naman
// Date: 17 September 2019
//***************************************************************************/

//***************************************************************************/
/** @file ojph_block_encoder.cpp
 *  @brief implements HTJ2K block encoder
 */

#include <cassert>
#include <cstring>
#include <cstdint>
#include <climits>

#include "ojph_mem.h"
#include "ojph_arch.h"
#include "ojph_block_encoder.h"
#include "ojph_message.h"

namespace ojph {
  namespace local {

    /////////////////////////////////////////////////////////////////////////
    // tables
    /////////////////////////////////////////////////////////////////////////

    //VLC encoding
    // index is (c_q << 8) + (rho << 4) + eps
    // data is  (cwd << 8) + (cwd_len << 4) + eps
    // table 0 is for the initial line of quads
    static ui16 vlc_tbl0[2048] = { 0 };
    static ui16 vlc_tbl1[2048] = { 0 };

    //UVLC encoding
    const int num_uvlc_entries = 75;
    struct uvlc_tbl_struct {
      ui8 pre, pre_len, suf, suf_len, ext, ext_len;
    };
    static uvlc_tbl_struct uvlc_tbl[num_uvlc_entries];
    
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
      ui16 *tgt_tbl = vlc_tbl0;
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
      uvlc_tbl[0].pre = 0;
      uvlc_tbl[0].pre_len = 0;
      uvlc_tbl[0].suf = 0;
      uvlc_tbl[0].suf_len = 0;
      uvlc_tbl[0].ext = 0;
      uvlc_tbl[0].ext_len = 0;

      uvlc_tbl[1].pre = 1;
      uvlc_tbl[1].pre_len = 1;
      uvlc_tbl[1].suf = 0;
      uvlc_tbl[1].suf_len = 0;
      uvlc_tbl[1].ext = 0;
      uvlc_tbl[1].ext_len = 0;

      uvlc_tbl[2].pre = 2;
      uvlc_tbl[2].pre_len = 2;
      uvlc_tbl[2].suf = 0;
      uvlc_tbl[2].suf_len = 0;
      uvlc_tbl[2].ext = 0;
      uvlc_tbl[2].ext_len = 0;

      uvlc_tbl[3].pre = 4;
      uvlc_tbl[3].pre_len = 3;
      uvlc_tbl[3].suf = 0;
      uvlc_tbl[3].suf_len = 1;
      uvlc_tbl[3].ext = 0;
      uvlc_tbl[3].ext_len = 0;

      uvlc_tbl[4].pre = 4;
      uvlc_tbl[4].pre_len = 3;
      uvlc_tbl[4].suf = 1;
      uvlc_tbl[4].suf_len = 1;
      uvlc_tbl[4].ext = 0;
      uvlc_tbl[4].ext_len = 0;

      for (int i = 5; i < 33; ++i)
      {
        uvlc_tbl[i].pre = 0;
        uvlc_tbl[i].pre_len = 3;
        uvlc_tbl[i].suf = (ui8)(i - 5);
        uvlc_tbl[i].suf_len = 5;
        uvlc_tbl[i].ext = 0;
        uvlc_tbl[i].ext_len = 0;
      }

      for (int i = 33; i < num_uvlc_entries; ++i)
      {
        uvlc_tbl[i].pre = 0;
        uvlc_tbl[i].pre_len = 3;
        uvlc_tbl[i].suf = (ui8)(28 + (i - 33) % 4);
        uvlc_tbl[i].suf_len = 5;
        uvlc_tbl[i].ext = (ui8)((i - 33) / 4);
        uvlc_tbl[i].ext_len = 4;
      }

      return true;
    }

    /////////////////////////////////////////////////////////////////////////
    static bool tables_initialized = false;

    /////////////////////////////////////////////////////////////////////////
    bool initialize_block_encoder_tables() {
      if (!tables_initialized) {
        memset(vlc_tbl0, 0, 2048 * sizeof(ui16));
        memset(vlc_tbl1, 0, 2048 * sizeof(ui16));
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
      assert(v == 0 || v == 1);
      melp->tmp = (melp->tmp << 1) + v;
      melp->remaining_bits--;
      if (melp->remaining_bits == 0)
      {
        if (melp->pos >= melp->buf_size)
          OJPH_ERROR(0x00020001, "mel encoder's buffer is full");

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

      if (bit == false)
      {
        ++melp->run;
        if (melp->run >= melp->threshold)
        {
          mel_emit_bit(melp, 1);
          melp->run = 0;
          melp->k = ojph_min(12, melp->k + 1);
          melp->threshold = 1 << mel_exp[melp->k];
        }
      }
      else
      {
        mel_emit_bit(melp, 0);
        int t = mel_exp[melp->k];
        while (t > 0)
          mel_emit_bit(melp, (melp->run >> --t) & 1);
        melp->run = 0;
        melp->k = ojph_max(0, melp->k - 1);
        melp->threshold = 1 << mel_exp[melp->k];
      }
    }

    /////////////////////////////////////////////////////////////////////////
    //
    /////////////////////////////////////////////////////////////////////////
    struct vlc_struct {
      //storage
      ui8* buf;      //pointer to data buffer
      ui32 pos;      //position of next writing within buf
      ui32 buf_size; //size of buffer, which we must not exceed

      int used_bits; //number of occupied bits in tmp
      int tmp;       //temporary storage of coded bits
      bool last_greater_than_8F; //true if last byte us greater than 0x8F
    };

    //////////////////////////////////////////////////////////////////////////
    static inline void
    vlc_init(vlc_struct* vlcp, ui32 buffer_size, ui8* data)
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
    vlc_encode(vlc_struct* vlcp, int cwd, int cwd_len)
    {
      while (cwd_len > 0)
      {
        if (vlcp->pos >= vlcp->buf_size)
          OJPH_ERROR(0x00020002, "vlc encoder's buffer is full");

        int avail_bits = 8 - vlcp->last_greater_than_8F - vlcp->used_bits;
        int t = ojph_min(avail_bits, cwd_len);
        vlcp->tmp |= (cwd & ((1 << t) - 1)) << vlcp->used_bits;
        vlcp->used_bits += t;
        avail_bits -= t;
        cwd_len -= t;
        cwd >>= t;
        if (avail_bits == 0)
        {
          if (vlcp->last_greater_than_8F && vlcp->tmp != 0x7F)
          {
            vlcp->last_greater_than_8F = false;
            continue; //one empty bit remaining
          }
          *(vlcp->buf - vlcp->pos) = (ui8)(vlcp->tmp);
          vlcp->pos++;
          vlcp->last_greater_than_8F = vlcp->tmp > 0x8F;
          vlcp->tmp = 0;
          vlcp->used_bits = 0;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////////////
    static inline void
    terminate_mel_vlc(mel_struct* melp, vlc_struct* vlcp)
    {
      if (melp->run > 0)
        mel_emit_bit(melp, 1);

      melp->tmp = melp->tmp << melp->remaining_bits;
      int mel_mask = (0xFF << melp->remaining_bits) & 0xFF;
      int vlc_mask = 0xFF >> (8 - vlcp->used_bits);
      if ((mel_mask | vlc_mask) == 0)
        return;  //last mel byte cannot be 0xFF, since then
                 //melp->remaining_bits would be < 8
      if (melp->pos >= melp->buf_size)
        OJPH_ERROR(0x00020003, "mel encoder's buffer is full");
      int fuse = melp->tmp | vlcp->tmp;
      if ( ( ((fuse ^ melp->tmp) & mel_mask)
           | ((fuse ^ vlcp->tmp) & vlc_mask) ) == 0
          && (fuse != 0xFF) && vlcp->pos > 1)
      {
        melp->buf[melp->pos++] = (ui8)fuse;
      }
      else
      {
        if (vlcp->pos >= vlcp->buf_size)
          OJPH_ERROR(0x00020004, "vlc encoder's buffer is full");
        melp->buf[melp->pos++] = (ui8)melp->tmp; //melp->tmp cannot be 0xFF
        *(vlcp->buf - vlcp->pos) = (ui8)vlcp->tmp;
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
    ms_encode(ms_struct* msp, ui32 cwd, int cwd_len)
    {
      while (cwd_len > 0)
      {
        if (msp->pos >= msp->buf_size)
          OJPH_ERROR(0x00020005, "magnitude sign encoder's buffer is full");
        int t = ojph_min(msp->max_bits - msp->used_bits, cwd_len);
        msp->tmp |= (cwd & ((1U << t) - 1)) << msp->used_bits;
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
    ms_encode64(ms_struct* msp, ui64 cwd, int cwd_len)
    {
      while (cwd_len > 0)
      {
        if (msp->pos >= msp->buf_size)
          OJPH_ERROR(0x00020005, "magnitude sign encoder's buffer is full");
        int t = ojph_min(msp->max_bits - msp->used_bits, cwd_len);
        msp->tmp |= (ui32)((cwd & ((1ULL << t) - 1)) << msp->used_bits);
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

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////
    void ojph_encode_codeblock32(ui32* buf, ui32 missing_msbs, ui32 num_passes,
                                 ui32 width, ui32 height, ui32 stride,
                                 ui32* lengths,
                                 ojph::mem_elastic_allocator *elastic,
                                 ojph::coded_lists *& coded)
    {
      assert(num_passes == 1);
      (void)num_passes;                      //currently not used
      const int ms_size = (16384*16+14)/15;  //more than enough
      ui8 ms_buf[ms_size];
      const int mel_vlc_size = 3072;         //more than enough
      ui8 mel_vlc_buf[mel_vlc_size];
      const int mel_size = 192;
      ui8 *mel_buf = mel_vlc_buf;
      const int vlc_size = mel_vlc_size - mel_size;
      ui8 *vlc_buf = mel_vlc_buf + mel_size;

      mel_struct mel;
      mel_init(&mel, mel_size, mel_buf);
      vlc_struct vlc;
      vlc_init(&vlc, vlc_size, vlc_buf);
      ms_struct ms;
      ms_init(&ms, ms_size, ms_buf);

      ui32 p = 30 - missing_msbs;

      //e_val: E values for a line (these are the highest set bit)
      //cx_val: is the context values
      //Each byte stores the info for the 2 sample. For E, it is maximum
      // of the two samples, while for cx, it is the OR of these two samples.
      //The maximum is between the pixel at the bottom left of one quad
      // and the bottom right of the earlier quad. The same is true for cx.
      //For a 1024 pixels, we need 512 bytes, the 2 extra,
      // one for the non-existing earlier quad, and one for beyond the
      // the end
      ui8 e_val[513];
      ui8 cx_val[513];
      ui8* lep = e_val;     lep[0] = 0;
      ui8* lcxp = cx_val;   lcxp[0] = 0;

      //initial row of quads
      int e_qmax[2] = {0,0}, e_q[8] = {0,0,0,0,0,0,0,0};
      int rho[2] = {0,0};
      int c_q0 = 0;
      ui32 s[8] = {0,0,0,0,0,0,0,0}, val, t;
      ui32 y = 0;
      ui32 *sp = buf;
      for (ui32 x = 0; x < width; x += 4)
      {
        //prepare two quads
        t = sp[0];
        val = t + t; //multiply by 2 and get rid of sign
        val >>= p;  // 2 \mu_p + x
        val &= ~1u; // 2 \mu_p
        if (val)
        {
          rho[0] = 1;
          e_q[0] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
          e_qmax[0] = e_q[0];
          s[0] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
        }

        t = height > 1 ? sp[stride] : 0;
        ++sp;
        val = t + t; //multiply by 2 and get rid of sign
        val >>= p; // 2 \mu_p + x
        val &= ~1u;// 2 \mu_p
        if (val)
        {
          rho[0] += 2;
          e_q[1] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
          e_qmax[0] = ojph_max(e_qmax[0], e_q[1]);
          s[1] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
        }

        if (x+1 < width)
        {
          t = sp[0];
          val = t + t; //multiply by 2 and get rid of sign
          val >>= p; // 2 \mu_p + x
          val &= ~1u;// 2 \mu_p
          if (val)
          {
            rho[0] += 4;
            e_q[2] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
            e_qmax[0] = ojph_max(e_qmax[0], e_q[2]);
            s[2] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
          }

          t = height > 1 ? sp[stride] : 0;
          ++sp;
          val = t + t; //multiply by 2 and get rid of sign
          val >>= p; // 2 \mu_p + x
          val &= ~1u;// 2 \mu_p
          if (val)
          {
            rho[0] += 8;
            e_q[3] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
            e_qmax[0] = ojph_max(e_qmax[0], e_q[3]);
            s[3] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
          }
        }

        int Uq0 = ojph_max(e_qmax[0], 1); //kappa_q = 1
        int u_q0 = Uq0 - 1, u_q1 = 0; //kappa_q = 1

        int eps0 = 0;
        if (u_q0 > 0)
        {
          eps0 |= (e_q[0] == e_qmax[0]);
          eps0 |= (e_q[1] == e_qmax[0]) << 1;
          eps0 |= (e_q[2] == e_qmax[0]) << 2;
          eps0 |= (e_q[3] == e_qmax[0]) << 3;
        }
        lep[0] = ojph_max(lep[0], (ui8)e_q[1]); lep++;
        lep[0] = (ui8)e_q[3];
        lcxp[0] = (ui8)(lcxp[0] | (ui8)((rho[0] & 2) >> 1)); lcxp++;
        lcxp[0] = (ui8)((rho[0] & 8) >> 3);

        ui16 tuple0 = vlc_tbl0[(c_q0 << 8) + (rho[0] << 4) + eps0];
        vlc_encode(&vlc, tuple0 >> 8, (tuple0 >> 4) & 7);

        if (c_q0 == 0)
            mel_encode(&mel, rho[0] != 0);

        int m = (rho[0] & 1) ? Uq0 - (tuple0 & 1) : 0;
        ms_encode(&ms, s[0] & ((1U<<m)-1), m);
        m = (rho[0] & 2) ? Uq0 - ((tuple0 & 2) >> 1) : 0;
        ms_encode(&ms, s[1] & ((1U<<m)-1), m);
        m = (rho[0] & 4) ? Uq0 - ((tuple0 & 4) >> 2) : 0;
        ms_encode(&ms, s[2] & ((1U<<m)-1), m);
        m = (rho[0] & 8) ? Uq0 - ((tuple0 & 8) >> 3) : 0;
        ms_encode(&ms, s[3] & ((1U<<m)-1), m);

        if (x+2 < width)
        {
          t = sp[0];
          val = t + t; //multiply by 2 and get rid of sign
          val >>= p; // 2 \mu_p + x
          val &= ~1u;// 2 \mu_p
          if (val)
          {
            rho[1] = 1;
            e_q[4] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
            e_qmax[1] = e_q[4];
            s[4] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
          }

          t = height > 1 ? sp[stride] : 0;
          ++sp;
          val = t + t; //multiply by 2 and get rid of sign
          val >>= p; // 2 \mu_p + x
          val &= ~1u;// 2 \mu_p
          if (val)
          {
            rho[1] += 2;
            e_q[5] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
            e_qmax[1] = ojph_max(e_qmax[1], e_q[5]);
            s[5] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
          }

          if (x+3 < width)
          {
            t = sp[0];
            val = t + t; //multiply by 2 and get rid of sign
            val >>= p; // 2 \mu_p + x
            val &= ~1u;// 2 \mu_p
            if (val)
            {
              rho[1] += 4;
              e_q[6] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
              e_qmax[1] = ojph_max(e_qmax[1], e_q[6]);
              s[6] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
            }

            t = height > 1 ? sp[stride] : 0;
            ++sp;
            val = t + t; //multiply by 2 and get rid of sign
            val >>= p; // 2 \mu_p + x
            val &= ~1u;// 2 \mu_p
            if (val)
            {
              rho[1] += 8;
              e_q[7] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
              e_qmax[1] = ojph_max(e_qmax[1], e_q[7]);
              s[7] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
            }
          }

          int c_q1 = (rho[0] >> 1) | (rho[0] & 1);
          int Uq1 = ojph_max(e_qmax[1], 1); //kappa_q = 1
          u_q1 = Uq1 - 1; //kappa_q = 1

          int eps1 = 0;
          if (u_q1 > 0)
          {
            eps1 |= (e_q[4] == e_qmax[1]);
            eps1 |= (e_q[5] == e_qmax[1]) << 1;
            eps1 |= (e_q[6] == e_qmax[1]) << 2;
            eps1 |= (e_q[7] == e_qmax[1]) << 3;
          }
          lep[0] = ojph_max(lep[0], (ui8)e_q[5]); lep++;
          lep[0] = (ui8)e_q[7];
          lcxp[0] |= (ui8)(lcxp[0] | (ui8)((rho[1] & 2) >> 1)); lcxp++;
          lcxp[0] = (ui8)((rho[1] & 8) >> 3);
          ui16 tuple1 = vlc_tbl0[(c_q1 << 8) + (rho[1] << 4) + eps1];
          vlc_encode(&vlc, tuple1 >> 8, (tuple1 >> 4) & 7);

          if (c_q1 == 0)
            mel_encode(&mel, rho[1] != 0);

          int m = (rho[1] & 1) ? Uq1 - (tuple1 & 1) : 0;
          ms_encode(&ms, s[4] & ((1U<<m)-1), m);
          m = (rho[1] & 2) ? Uq1 - ((tuple1 & 2) >> 1) : 0;
          ms_encode(&ms, s[5] & ((1U<<m)-1), m);
          m = (rho[1] & 4) ? Uq1 - ((tuple1 & 4) >> 2) : 0;
          ms_encode(&ms, s[6] & ((1U<<m)-1), m);
          m = (rho[1] & 8) ? Uq1 - ((tuple1 & 8) >> 3) : 0;
          ms_encode(&ms, s[7] & ((1U<<m)-1), m);
        }

        if (u_q0 > 0 && u_q1 > 0)
          mel_encode(&mel, ojph_min(u_q0, u_q1) > 2);

        if (u_q0 > 2 && u_q1 > 2)
        {
          vlc_encode(&vlc, uvlc_tbl[u_q0-2].pre, uvlc_tbl[u_q0-2].pre_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1-2].pre, uvlc_tbl[u_q1-2].pre_len);
          vlc_encode(&vlc, uvlc_tbl[u_q0-2].suf, uvlc_tbl[u_q0-2].suf_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1-2].suf, uvlc_tbl[u_q1-2].suf_len);
        }
        else if (u_q0 > 2 && u_q1 > 0)
        {
          vlc_encode(&vlc, uvlc_tbl[u_q0].pre, uvlc_tbl[u_q0].pre_len);
          vlc_encode(&vlc, u_q1 - 1, 1);
          vlc_encode(&vlc, uvlc_tbl[u_q0].suf, uvlc_tbl[u_q0].suf_len);
        }
        else
        {
          vlc_encode(&vlc, uvlc_tbl[u_q0].pre, uvlc_tbl[u_q0].pre_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1].pre, uvlc_tbl[u_q1].pre_len);
          vlc_encode(&vlc, uvlc_tbl[u_q0].suf, uvlc_tbl[u_q0].suf_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1].suf, uvlc_tbl[u_q1].suf_len);
        }

        //prepare for next iteration
        c_q0 = (rho[1] >> 1) | (rho[1] & 1);
        s[0] = s[1] = s[2] = s[3] = s[4] = s[5] = s[6] = s[7] = 0;
        e_q[0]=e_q[1]=e_q[2]=e_q[3]=e_q[4]=e_q[5]=e_q[6]=e_q[7]=0;
        rho[0] = rho[1] = 0; e_qmax[0] = e_qmax[1] = 0;
      }

      lep[1] = 0;

      for (y = 2; y < height; y += 2)
      {
        lep = e_val;
        int max_e = ojph_max(lep[0], lep[1]) - 1;
        lep[0] = 0;
        lcxp = cx_val;
        c_q0 = lcxp[0] + (lcxp[1] << 2);
        lcxp[0] = 0;

        sp = buf + y * stride;
        for (ui32 x = 0; x < width; x += 4)
        {
          //prepare two quads
          t = sp[0];
          val = t + t; //multiply by 2 and get rid of sign
          val >>= p; // 2 \mu_p + x
          val &= ~1u;// 2 \mu_p
          if (val)
          {
            rho[0] = 1;
            e_q[0] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
            e_qmax[0] = e_q[0];
            s[0] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
          }

          t = y + 1 < height ? sp[stride] : 0;
          ++sp;
          val = t + t; //multiply by 2 and get rid of sign
          val >>= p; // 2 \mu_p + x
          val &= ~1u;// 2 \mu_p
          if (val)
          {
            rho[0] += 2;
            e_q[1] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
            e_qmax[0] = ojph_max(e_qmax[0], e_q[1]);
            s[1] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
          }

          if (x+1 < width)
          {
            t = sp[0];
            val = t + t; //multiply by 2 and get rid of sign
            val >>= p; // 2 \mu_p + x
            val &= ~1u;// 2 \mu_p
            if (val)
            {
              rho[0] += 4;
              e_q[2] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
              e_qmax[0] = ojph_max(e_qmax[0], e_q[2]);
              s[2] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
            }

            t = y + 1 < height ? sp[stride] : 0;
            ++sp;
            val = t + t; //multiply by 2 and get rid of sign
            val >>= p; // 2 \mu_p + x
            val &= ~1u;// 2 \mu_p
            if (val)
            {
              rho[0] += 8;
              e_q[3] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
              e_qmax[0] = ojph_max(e_qmax[0], e_q[3]);
              s[3] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
            }
          }

          int kappa = (rho[0] & (rho[0]-1)) ? ojph_max(1,max_e) : 1;
          int Uq0 = ojph_max(e_qmax[0], kappa);
          int u_q0 = Uq0 - kappa, u_q1 = 0;

          int eps0 = 0;
          if (u_q0 > 0)
          {
            eps0 |= (e_q[0] == e_qmax[0]);
            eps0 |= (e_q[1] == e_qmax[0]) << 1;
            eps0 |= (e_q[2] == e_qmax[0]) << 2;
            eps0 |= (e_q[3] == e_qmax[0]) << 3;
          }
          lep[0] = ojph_max(lep[0], (ui8)e_q[1]); lep++;
          max_e = ojph_max(lep[0], lep[1]) - 1;
          lep[0] = (ui8)e_q[3];
          lcxp[0] = (ui8)(lcxp[0] | (ui8)((rho[0] & 2) >> 1)); lcxp++;
          int c_q1 = lcxp[0] + (lcxp[1] << 2);
          lcxp[0] = (ui8)((rho[0] & 8) >> 3);
          ui16 tuple0 = vlc_tbl1[(c_q0 << 8) + (rho[0] << 4) + eps0];
          vlc_encode(&vlc, tuple0 >> 8, (tuple0 >> 4) & 7);

          if (c_q0 == 0)
              mel_encode(&mel, rho[0] != 0);

          int m = (rho[0] & 1) ? Uq0 - (tuple0 & 1) : 0;
          ms_encode(&ms, s[0] & ((1U<<m)-1), m);
          m = (rho[0] & 2) ? Uq0 - ((tuple0 & 2) >> 1) : 0;
          ms_encode(&ms, s[1] & ((1U<<m)-1), m);
          m = (rho[0] & 4) ? Uq0 - ((tuple0 & 4) >> 2) : 0;
          ms_encode(&ms, s[2] & ((1U<<m)-1), m);
          m = (rho[0] & 8) ? Uq0 - ((tuple0 & 8) >> 3) : 0;
          ms_encode(&ms, s[3] & ((1U<<m)-1), m);

          if (x+2 < width)
          {
            t = sp[0];
            val = t + t; //multiply by 2 and get rid of sign
            val >>= p; // 2 \mu_p + x
            val &= ~1u;// 2 \mu_p
            if (val)
            {
              rho[1] = 1;
              e_q[4] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
              e_qmax[1] = e_q[4];
              s[4] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
            }

            t = y + 1 < height ? sp[stride] : 0;
            ++sp;
            val = t + t; //multiply by 2 and get rid of sign
            val >>= p; // 2 \mu_p + x
            val &= ~1u;// 2 \mu_p
            if (val)
            {
              rho[1] += 2;
              e_q[5] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
              e_qmax[1] = ojph_max(e_qmax[1], e_q[5]);
              s[5] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
            }

            if (x+3 < width)
            {
              t = sp[0];
              val = t + t; //multiply by 2 and get rid of sign
              val >>= p; // 2 \mu_p + x
              val &= ~1u;// 2 \mu_p
              if (val)
              {
                rho[1] += 4;
                e_q[6] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
                e_qmax[1] = ojph_max(e_qmax[1], e_q[6]);
                s[6] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
              }

              t = y + 1 < height ? sp[stride] : 0;
              ++sp;
              val = t + t; //multiply by 2 and get rid of sign
              val >>= p; // 2 \mu_p + x
              val &= ~1u;// 2 \mu_p
              if (val)
              {
                rho[1] += 8;
                e_q[7] = 32 - (int)count_leading_zeros(--val); //2\mu_p - 1
                e_qmax[1] = ojph_max(e_qmax[1], e_q[7]);
                s[7] = --val + (t >> 31); //v_n = 2(\mu_p-1) + s_n
              }
            }

            kappa = (rho[1] & (rho[1]-1)) ? ojph_max(1,max_e) : 1;
            c_q1 |= ((rho[0] & 4) >> 1) | ((rho[0] & 8) >> 2);
            int Uq1 = ojph_max(e_qmax[1], kappa);
            u_q1 = Uq1 - kappa;

            int eps1 = 0;
            if (u_q1 > 0)
            {
              eps1 |= (e_q[4] == e_qmax[1]);
              eps1 |= (e_q[5] == e_qmax[1]) << 1;
              eps1 |= (e_q[6] == e_qmax[1]) << 2;
              eps1 |= (e_q[7] == e_qmax[1]) << 3;
            }
            lep[0] = ojph_max(lep[0], (ui8)e_q[5]); lep++;
            max_e = ojph_max(lep[0], lep[1]) - 1;
            lep[0] = (ui8)e_q[7];
            lcxp[0] = (ui8)(lcxp[0] | (ui8)((rho[1] & 2) >> 1)); lcxp++;
            c_q0 = lcxp[0] + (lcxp[1] << 2);
            lcxp[0] = (ui8)((rho[1] & 8) >> 3);
            ui16 tuple1 = vlc_tbl1[(c_q1 << 8) + (rho[1] << 4) + eps1];
            vlc_encode(&vlc, tuple1 >> 8, (tuple1 >> 4) & 7);

            if (c_q1 == 0)
              mel_encode(&mel, rho[1] != 0);

            int m = (rho[1] & 1) ? Uq1 - (tuple1 & 1) : 0;
            ms_encode(&ms, s[4] & ((1U<<m)-1), m);
            m = (rho[1] & 2) ? Uq1 - ((tuple1 & 2) >> 1) : 0;
            ms_encode(&ms, s[5] & ((1U<<m)-1), m);
            m = (rho[1] & 4) ? Uq1 - ((tuple1 & 4) >> 2) : 0;
            ms_encode(&ms, s[6] & ((1U<<m)-1), m);
            m = (rho[1] & 8) ? Uq1 - ((tuple1 & 8) >> 3) : 0;
            ms_encode(&ms, s[7] & ((1U<<m)-1), m);
          }

          vlc_encode(&vlc, uvlc_tbl[u_q0].pre, uvlc_tbl[u_q0].pre_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1].pre, uvlc_tbl[u_q1].pre_len);
          vlc_encode(&vlc, uvlc_tbl[u_q0].suf, uvlc_tbl[u_q0].suf_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1].suf, uvlc_tbl[u_q1].suf_len);

          //prepare for next iteration
          c_q0 |= ((rho[1] & 4) >> 1) | ((rho[1] & 8) >> 2);
          s[0] = s[1] = s[2] = s[3] = s[4] = s[5] = s[6] = s[7] = 0;
          e_q[0]=e_q[1]=e_q[2]=e_q[3]=e_q[4]=e_q[5]=e_q[6]=e_q[7]=0;
          rho[0] = rho[1] = 0; e_qmax[0] = e_qmax[1] = 0;
        }
      }


      terminate_mel_vlc(&mel, &vlc);
      ms_terminate(&ms);

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

    //////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////
    void ojph_encode_codeblock64(ui64* buf, ui32 missing_msbs, ui32 num_passes,
                                 ui32 width, ui32 height, ui32 stride,
                                 ui32* lengths,
                                 ojph::mem_elastic_allocator *elastic,
                                 ojph::coded_lists *& coded)
    {
      assert(num_passes == 1);
      (void)num_passes;                      //currently not used
      // 38 bits/sample + 1 color + 4 wavelet = 43 bits per sample.
      // * 4096 samples / 8 bits per byte = 22016; then rounded up to the 
      // nearest 1 kB, givin 22528.  This expanded further to take into 
      // consideration stuffing at a max rate of 16 bits per 15 bits 
      // (1 bit for every 15 bits of data); in reality, it is much smaller
      // than this.
      const int ms_size = (22528 * 16 + 14) / 15;  //more than enough
      ui8 ms_buf[ms_size];
      // For each quad, we need at most, 7 bits for VLC and 12 bits for UVLC.
      // So we have 1024 quads * 19 / 8, which is 2432.  This must be 
      // multiplied by 16 / 15 to accommodate stuffing.  
      // The mel is at most around 1 bit/quad, giving around 128 byte -- in
      // practice there was on case where it got to 132 bytes.  Even 
      // accounting for stuffing, it is smaller than 192.  Therefore,
      // 3072 is more than enough
      const int mel_vlc_size = 3072;         //more than enough
      ui8 mel_vlc_buf[mel_vlc_size];
      const int mel_size = 192;
      ui8 *mel_buf = mel_vlc_buf;
      const int vlc_size = mel_vlc_size - mel_size;
      ui8 *vlc_buf = mel_vlc_buf + mel_size;

      mel_struct mel;
      mel_init(&mel, mel_size, mel_buf);
      vlc_struct vlc;
      vlc_init(&vlc, vlc_size, vlc_buf);
      ms_struct ms;
      ms_init(&ms, ms_size, ms_buf);

      ui32 p = 62 - missing_msbs;

      //e_val: E values for a line (these are the highest set bit)
      //cx_val: is the context values
      //Each byte stores the info for the 2 sample. For E, it is maximum
      // of the two samples, while for cx, it is the OR of these two samples.
      //The maximum is between the pixel at the bottom left of one quad
      // and the bottom right of the earlier quad. The same is true for cx.
      //For a 1024 pixels, we need 512 bytes, the 2 extra,
      // one for the non-existing earlier quad, and one for beyond the
      // the end
      ui8 e_val[513];
      ui8 cx_val[513];
      ui8* lep = e_val;     lep[0] = 0;
      ui8* lcxp = cx_val;   lcxp[0] = 0;

      //initial row of quads
      int e_qmax[2] = {0,0}, e_q[8] = {0,0,0,0,0,0,0,0};
      int rho[2] = {0,0};
      int c_q0 = 0;
      ui64 s[8] = {0,0,0,0,0,0,0,0}, val, t;
      ui32 y = 0;
      ui64 *sp = buf;
      for (ui32 x = 0; x < width; x += 4)
      {
        //prepare two quads
        t = sp[0];
        val = t + t; //multiply by 2 and get rid of sign
        val >>= p;  // 2 \mu_p + x
        val &= ~1ULL; // 2 \mu_p
        if (val)
        {
          rho[0] = 1;
          e_q[0] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
          e_qmax[0] = e_q[0];
          s[0] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
        }

        t = height > 1 ? sp[stride] : 0;
        ++sp;
        val = t + t; //multiply by 2 and get rid of sign
        val >>= p; // 2 \mu_p + x
        val &= ~1ULL;// 2 \mu_p
        if (val)
        {
          rho[0] += 2;
          e_q[1] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
          e_qmax[0] = ojph_max(e_qmax[0], e_q[1]);
          s[1] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
        }

        if (x + 1 < width)
        {
          t = sp[0];
          val = t + t; //multiply by 2 and get rid of sign
          val >>= p; // 2 \mu_p + x
          val &= ~1ULL;// 2 \mu_p
          if (val)
          {
            rho[0] += 4;
            e_q[2] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
            e_qmax[0] = ojph_max(e_qmax[0], e_q[2]);
            s[2] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
          }

          t = height > 1 ? sp[stride] : 0;
          ++sp;
          val = t + t; //multiply by 2 and get rid of sign
          val >>= p; // 2 \mu_p + x
          val &= ~1ULL;// 2 \mu_p
          if (val)
          {
            rho[0] += 8;
            e_q[3] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
            e_qmax[0] = ojph_max(e_qmax[0], e_q[3]);
            s[3] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
          }
        }

        int Uq0 = ojph_max(e_qmax[0], 1); //kappa_q = 1
        int u_q0 = Uq0 - 1, u_q1 = 0; //kappa_q = 1

        int eps0 = 0;
        if (u_q0 > 0)
        {
          eps0 |= (e_q[0] == e_qmax[0]);
          eps0 |= (e_q[1] == e_qmax[0]) << 1;
          eps0 |= (e_q[2] == e_qmax[0]) << 2;
          eps0 |= (e_q[3] == e_qmax[0]) << 3;
        }
        lep[0] = ojph_max(lep[0], (ui8)e_q[1]); lep++;
        lep[0] = (ui8)e_q[3];
        lcxp[0] = (ui8)(lcxp[0] | (ui8)((rho[0] & 2) >> 1)); lcxp++;
        lcxp[0] = (ui8)((rho[0] & 8) >> 3);

        ui16 tuple0 = vlc_tbl0[(c_q0 << 8) + (rho[0] << 4) + eps0];
        vlc_encode(&vlc, tuple0 >> 8, (tuple0 >> 4) & 7);

        if (c_q0 == 0)
          mel_encode(&mel, rho[0] != 0);

        int m = (rho[0] & 1) ? Uq0 - (tuple0 & 1) : 0;
        ms_encode64(&ms, s[0] & ((1ULL << m) - 1), m);
        m = (rho[0] & 2) ? Uq0 - ((tuple0 & 2) >> 1) : 0;
        ms_encode64(&ms, s[1] & ((1ULL << m) - 1), m);
        m = (rho[0] & 4) ? Uq0 - ((tuple0 & 4) >> 2) : 0;
        ms_encode64(&ms, s[2] & ((1ULL << m) - 1), m);
        m = (rho[0] & 8) ? Uq0 - ((tuple0 & 8) >> 3) : 0;
        ms_encode64(&ms, s[3] & ((1ULL << m) - 1), m);

        if (x + 2 < width)
        {
          t = sp[0];
          val = t + t; //multiply by 2 and get rid of sign
          val >>= p; // 2 \mu_p + x
          val &= ~1ULL;// 2 \mu_p
          if (val)
          {
            rho[1] = 1;
            e_q[4] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
            e_qmax[1] = e_q[4];
            s[4] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
          }

          t = height > 1 ? sp[stride] : 0;
          ++sp;
          val = t + t; //multiply by 2 and get rid of sign
          val >>= p; // 2 \mu_p + x
          val &= ~1ULL;// 2 \mu_p
          if (val)
          {
            rho[1] += 2;
            e_q[5] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
            e_qmax[1] = ojph_max(e_qmax[1], e_q[5]);
            s[5] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
          }

          if (x + 3 < width)
          {
            t = sp[0];
            val = t + t; //multiply by 2 and get rid of sign
            val >>= p; // 2 \mu_p + x
            val &= ~1ULL;// 2 \mu_p
            if (val)
            {
              rho[1] += 4;
              e_q[6] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
              e_qmax[1] = ojph_max(e_qmax[1], e_q[6]);
              s[6] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
            }

            t = height > 1 ? sp[stride] : 0;
            ++sp;
            val = t + t; //multiply by 2 and get rid of sign
            val >>= p; // 2 \mu_p + x
            val &= ~1ULL;// 2 \mu_p
            if (val)
            {
              rho[1] += 8;
              e_q[7] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
              e_qmax[1] = ojph_max(e_qmax[1], e_q[7]);
              s[7] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
            }
          }

          int c_q1 = (rho[0] >> 1) | (rho[0] & 1);
          int Uq1 = ojph_max(e_qmax[1], 1); //kappa_q = 1
          u_q1 = Uq1 - 1; //kappa_q = 1

          int eps1 = 0;
          if (u_q1 > 0)
          {
            eps1 |= (e_q[4] == e_qmax[1]);
            eps1 |= (e_q[5] == e_qmax[1]) << 1;
            eps1 |= (e_q[6] == e_qmax[1]) << 2;
            eps1 |= (e_q[7] == e_qmax[1]) << 3;
          }
          lep[0] = ojph_max(lep[0], (ui8)e_q[5]); lep++;
          lep[0] = (ui8)e_q[7];
          lcxp[0] |= (ui8)(lcxp[0] | (ui8)((rho[1] & 2) >> 1)); lcxp++;
          lcxp[0] = (ui8)((rho[1] & 8) >> 3);
          ui16 tuple1 = vlc_tbl0[(c_q1 << 8) + (rho[1] << 4) + eps1];
          vlc_encode(&vlc, tuple1 >> 8, (tuple1 >> 4) & 7);

          if (c_q1 == 0)
            mel_encode(&mel, rho[1] != 0);

          int m = (rho[1] & 1) ? Uq1 - (tuple1 & 1) : 0;
          ms_encode64(&ms, s[4] & ((1ULL << m) - 1), m);
          m = (rho[1] & 2) ? Uq1 - ((tuple1 & 2) >> 1) : 0;
          ms_encode64(&ms, s[5] & ((1ULL << m) - 1), m);
          m = (rho[1] & 4) ? Uq1 - ((tuple1 & 4) >> 2) : 0;
          ms_encode64(&ms, s[6] & ((1ULL << m) - 1), m);
          m = (rho[1] & 8) ? Uq1 - ((tuple1 & 8) >> 3) : 0;
          ms_encode64(&ms, s[7] & ((1ULL << m) - 1), m);
        }

        if (u_q0 > 0 && u_q1 > 0)
          mel_encode(&mel, ojph_min(u_q0, u_q1) > 2);

        if (u_q0 > 2 && u_q1 > 2)
        {
          vlc_encode(&vlc, uvlc_tbl[u_q0-2].pre, uvlc_tbl[u_q0-2].pre_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1-2].pre, uvlc_tbl[u_q1-2].pre_len);
          vlc_encode(&vlc, uvlc_tbl[u_q0-2].suf, uvlc_tbl[u_q0-2].suf_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1-2].suf, uvlc_tbl[u_q1-2].suf_len);
          vlc_encode(&vlc, uvlc_tbl[u_q0-2].ext, uvlc_tbl[u_q0-2].ext_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1-2].ext, uvlc_tbl[u_q1-2].ext_len);
        }
        else if (u_q0 > 2 && u_q1 > 0)
        {
          vlc_encode(&vlc, uvlc_tbl[u_q0].pre, uvlc_tbl[u_q0].pre_len);
          vlc_encode(&vlc, u_q1 - 1, 1);
          vlc_encode(&vlc, uvlc_tbl[u_q0].suf, uvlc_tbl[u_q0].suf_len);
          vlc_encode(&vlc, uvlc_tbl[u_q0].ext, uvlc_tbl[u_q0].ext_len);
        }
        else
        {
          vlc_encode(&vlc, uvlc_tbl[u_q0].pre, uvlc_tbl[u_q0].pre_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1].pre, uvlc_tbl[u_q1].pre_len);
          vlc_encode(&vlc, uvlc_tbl[u_q0].suf, uvlc_tbl[u_q0].suf_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1].suf, uvlc_tbl[u_q1].suf_len);
          vlc_encode(&vlc, uvlc_tbl[u_q0].ext, uvlc_tbl[u_q0].ext_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1].ext, uvlc_tbl[u_q1].ext_len);
        }

        //prepare for next iteration
        c_q0 = (rho[1] >> 1) | (rho[1] & 1);
        s[0] = s[1] = s[2] = s[3] = s[4] = s[5] = s[6] = s[7] = 0;
        e_q[0]=e_q[1]=e_q[2]=e_q[3]=e_q[4]=e_q[5]=e_q[6]=e_q[7]=0;
        rho[0] = rho[1] = 0; e_qmax[0] = e_qmax[1] = 0;
      }

      lep[1] = 0;

      for (y = 2; y < height; y += 2)
      {
        lep = e_val;
        int max_e = ojph_max(lep[0], lep[1]) - 1;
        lep[0] = 0;
        lcxp = cx_val;
        c_q0 = lcxp[0] + (lcxp[1] << 2);
        lcxp[0] = 0;

        sp = buf + y * stride;
        for (ui32 x = 0; x < width; x += 4)
        {
          //prepare two quads
          t = sp[0];
          val = t + t; //multiply by 2 and get rid of sign
          val >>= p; // 2 \mu_p + x
          val &= ~1ULL;// 2 \mu_p
          if (val)
          {
            rho[0] = 1;
            e_q[0] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
            e_qmax[0] = e_q[0];
            s[0] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
          }

          t = y + 1 < height ? sp[stride] : 0;
          ++sp;
          val = t + t; //multiply by 2 and get rid of sign
          val >>= p; // 2 \mu_p + x
          val &= ~1ULL;// 2 \mu_p
          if (val)
          {
            rho[0] += 2;
            e_q[1] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
            e_qmax[0] = ojph_max(e_qmax[0], e_q[1]);
            s[1] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
          }

          if (x + 1 < width)
          {
            t = sp[0];
            val = t + t; //multiply by 2 and get rid of sign
            val >>= p; // 2 \mu_p + x
            val &= ~1ULL;// 2 \mu_p
            if (val)
            {
              rho[0] += 4;
              e_q[2] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
              e_qmax[0] = ojph_max(e_qmax[0], e_q[2]);
              s[2] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
            }

            t = y + 1 < height ? sp[stride] : 0;
            ++sp;
            val = t + t; //multiply by 2 and get rid of sign
            val >>= p; // 2 \mu_p + x
            val &= ~1ULL;// 2 \mu_p
            if (val)
            {
              rho[0] += 8;
              e_q[3] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
              e_qmax[0] = ojph_max(e_qmax[0], e_q[3]);
              s[3] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
            }
          }

          int kappa = (rho[0] & (rho[0]-1)) ? ojph_max(1,max_e) : 1;
          int Uq0 = ojph_max(e_qmax[0], kappa);
          int u_q0 = Uq0 - kappa, u_q1 = 0;

          int eps0 = 0;
          if (u_q0 > 0)
          {
            eps0 |= (e_q[0] == e_qmax[0]);
            eps0 |= (e_q[1] == e_qmax[0]) << 1;
            eps0 |= (e_q[2] == e_qmax[0]) << 2;
            eps0 |= (e_q[3] == e_qmax[0]) << 3;
          }
          lep[0] = ojph_max(lep[0], (ui8)e_q[1]); lep++;
          max_e = ojph_max(lep[0], lep[1]) - 1;
          lep[0] = (ui8)e_q[3];
          lcxp[0] = (ui8)(lcxp[0] | (ui8)((rho[0] & 2) >> 1)); lcxp++;
          int c_q1 = lcxp[0] + (lcxp[1] << 2);
          lcxp[0] = (ui8)((rho[0] & 8) >> 3);
          ui16 tuple0 = vlc_tbl1[(c_q0 << 8) + (rho[0] << 4) + eps0];
          vlc_encode(&vlc, tuple0 >> 8, (tuple0 >> 4) & 7);

          if (c_q0 == 0)
              mel_encode(&mel, rho[0] != 0);

          int m = (rho[0] & 1) ? Uq0 - (tuple0 & 1) : 0;
          ms_encode64(&ms, s[0] & ((1ULL << m) - 1), m);
          m = (rho[0] & 2) ? Uq0 - ((tuple0 & 2) >> 1) : 0;
          ms_encode64(&ms, s[1] & ((1ULL << m) - 1), m);
          m = (rho[0] & 4) ? Uq0 - ((tuple0 & 4) >> 2) : 0;
          ms_encode64(&ms, s[2] & ((1ULL << m) - 1), m);
          m = (rho[0] & 8) ? Uq0 - ((tuple0 & 8) >> 3) : 0;
          ms_encode64(&ms, s[3] & ((1ULL << m) - 1), m);

          if (x + 2 < width)
          {
            t = sp[0];
            val = t + t; //multiply by 2 and get rid of sign
            val >>= p; // 2 \mu_p + x
            val &= ~1ULL;// 2 \mu_p
            if (val)
            {
              rho[1] = 1;
              e_q[4] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
              e_qmax[1] = e_q[4];
              s[4] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
            }

            t = y + 1 < height ? sp[stride] : 0;
            ++sp;
            val = t + t; //multiply by 2 and get rid of sign
            val >>= p; // 2 \mu_p + x
            val &= ~1ULL;// 2 \mu_p
            if (val)
            {
              rho[1] += 2;
              e_q[5] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
              e_qmax[1] = ojph_max(e_qmax[1], e_q[5]);
              s[5] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
            }

            if (x + 3 < width)
            {
              t = sp[0];
              val = t + t; //multiply by 2 and get rid of sign
              val >>= p; // 2 \mu_p + x
              val &= ~1ULL;// 2 \mu_p
              if (val)
              {
                rho[1] += 4;
                e_q[6] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
                e_qmax[1] = ojph_max(e_qmax[1], e_q[6]);
                s[6] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
              }

              t = y + 1 < height ? sp[stride] : 0;
              ++sp;
              val = t + t; //multiply by 2 and get rid of sign
              val >>= p; // 2 \mu_p + x
              val &= ~1ULL;// 2 \mu_p
              if (val)
              {
                rho[1] += 8;
                e_q[7] = 64 - (int)count_leading_zeros(--val); //2\mu_p - 1
                e_qmax[1] = ojph_max(e_qmax[1], e_q[7]);
                s[7] = --val + (t >> 63); //v_n = 2(\mu_p-1) + s_n
              }
            }

            kappa = (rho[1] & (rho[1]-1)) ? ojph_max(1,max_e) : 1;
            c_q1 |= ((rho[0] & 4) >> 1) | ((rho[0] & 8) >> 2);
            int Uq1 = ojph_max(e_qmax[1], kappa);
            u_q1 = Uq1 - kappa;

            int eps1 = 0;
            if (u_q1 > 0)
            {
              eps1 |= (e_q[4] == e_qmax[1]);
              eps1 |= (e_q[5] == e_qmax[1]) << 1;
              eps1 |= (e_q[6] == e_qmax[1]) << 2;
              eps1 |= (e_q[7] == e_qmax[1]) << 3;
            }
            lep[0] = ojph_max(lep[0], (ui8)e_q[5]); lep++;
            max_e = ojph_max(lep[0], lep[1]) - 1;
            lep[0] = (ui8)e_q[7];
            lcxp[0] = (ui8)(lcxp[0] | (ui8)((rho[1] & 2) >> 1)); lcxp++;
            c_q0 = lcxp[0] + (lcxp[1] << 2);
            lcxp[0] = (ui8)((rho[1] & 8) >> 3);
            ui16 tuple1 = vlc_tbl1[(c_q1 << 8) + (rho[1] << 4) + eps1];
            vlc_encode(&vlc, tuple1 >> 8, (tuple1 >> 4) & 7);

            if (c_q1 == 0)
              mel_encode(&mel, rho[1] != 0);

            int m = (rho[1] & 1) ? Uq1 - (tuple1 & 1) : 0;
            ms_encode64(&ms, s[4] & ((1ULL << m) - 1), m);
            m = (rho[1] & 2) ? Uq1 - ((tuple1 & 2) >> 1) : 0;
            ms_encode64(&ms, s[5] & ((1ULL << m) - 1), m);
            m = (rho[1] & 4) ? Uq1 - ((tuple1 & 4) >> 2) : 0;
            ms_encode64(&ms, s[6] & ((1ULL << m) - 1), m);
            m = (rho[1] & 8) ? Uq1 - ((tuple1 & 8) >> 3) : 0;
            ms_encode64(&ms, s[7] & ((1ULL << m) - 1), m);
          }

          vlc_encode(&vlc, uvlc_tbl[u_q0].pre, uvlc_tbl[u_q0].pre_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1].pre, uvlc_tbl[u_q1].pre_len);
          vlc_encode(&vlc, uvlc_tbl[u_q0].suf, uvlc_tbl[u_q0].suf_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1].suf, uvlc_tbl[u_q1].suf_len);
          vlc_encode(&vlc, uvlc_tbl[u_q0].ext, uvlc_tbl[u_q0].ext_len);
          vlc_encode(&vlc, uvlc_tbl[u_q1].ext, uvlc_tbl[u_q1].ext_len);

          //prepare for next iteration
          c_q0 |= ((rho[1] & 4) >> 1) | ((rho[1] & 8) >> 2);
          s[0] = s[1] = s[2] = s[3] = s[4] = s[5] = s[6] = s[7] = 0;
          e_q[0]=e_q[1]=e_q[2]=e_q[3]=e_q[4]=e_q[5]=e_q[6]=e_q[7]=0;
          rho[0] = rho[1] = 0; e_qmax[0] = e_qmax[1] = 0;
        }
      }


      terminate_mel_vlc(&mel, &vlc);
      ms_terminate(&ms);

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
  }
}
