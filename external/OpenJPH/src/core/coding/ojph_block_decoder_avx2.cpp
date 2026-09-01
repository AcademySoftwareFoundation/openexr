//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2022, Aous Naman
// Copyright (c) 2022, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2022, The University of New South Wales, Australia
// Copyright (c) 2024, Intel Corporation
// Copyright (c) 2026, Osamu Watanabe
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
// File: ojph_block_decoder_avx2.cpp
//***************************************************************************/

//***************************************************************************/
/** @file ojph_block_decoder_avx2.cpp
 *  @brief implements a faster HTJ2K block decoder using avx2
 */

#include "ojph_arch.h"
#if defined(OJPH_ARCH_I386) || defined(OJPH_ARCH_X86_64)

#include <string>
#include <iostream>

#include <cassert>
#include <cstring>
#include "ojph_block_common.h"
#include "ojph_block_decoder.h"
#include "ojph_message.h"

#include <immintrin.h>

namespace ojph {
  namespace local {

    //************************************************************************/
    /** @brief MEL state structure for reading and decoding the MEL bitstream
     *
     *  A number of events is decoded from the MEL bitstream ahead of time
     *  and stored in run/num_runs.
     *  Each run represents the number of zero events before a one event.
     */
    struct dec_mel_st {
      dec_mel_st() : data(NULL), tmp(0), bits(0), size(0), unstuff(false),
        k(0), num_runs(0), runs(0)
      {}
      // data decoding machinery
      ui8* data;    //!<the address of data (or bitstream)
      ui64 tmp;     //!<temporary buffer for read data
      int bits;     //!<number of bits stored in tmp
      int size;     //!<number of bytes in MEL code
      bool unstuff; //!<true if the next bit needs to be unstuffed
      int k;        //!<state of MEL decoder

      // queue of decoded runs
      int num_runs; //!<number of decoded runs left in runs (maximum 8)
      ui64 runs;    //!<runs of decoded MEL codewords (7 bits/run)
    };

    //************************************************************************/
    /** @brief Reads and unstuffs the MEL bitstream
     *
     *  This design needs more bytes in the codeblock buffer than the length
     *  of the cleanup pass by up to 2 bytes.
     *
     *  Unstuffing removes the MSB of the byte following a byte whose
     *  value is 0xFF; this prevents sequences larger than 0xFF7F in value
     *  from appearing the bitstream.
     *
     *  @param [in]  melp is a pointer to dec_mel_st structure
     */
    static inline
    void mel_read(dec_mel_st *melp)
    {
      if (melp->bits > 32)  //there are enough bits in the tmp variable
        return;             // return without reading new data

      ui32 val = 0xFFFFFFFF;       // feed in 0xFF if buffer is exhausted
      if (melp->size > 4) {        // if there is data in the MEL segment
        memcpy(&val, melp->data, 4);  // read 32 bits from MEL data
        melp->data += 4;           // advance pointer
        melp->size -= 4;           // reduce counter
      }
      else if (melp->size > 0)
      { // 4 or less
        int i = 0;
        while (melp->size > 1) {
          ui32 v = *melp->data++;    // read one byte at a time
          ui32 m = ~(0xFFu << i);    // mask of location
          val = (val & m) | (v << i);// put one byte in its correct location
          --melp->size;
          i += 8;
        }
        // size equal to 1
        ui32 v = *melp->data++;    // the one before the last is different
        v |= 0xF;                  // MEL and VLC segments can overlap
        ui32 m = ~(0xFFu << i);
        val = (val & m) | (v << i);
        --melp->size;
      }

      // next we unstuff them before adding them to the buffer
      int bits = 32 - melp->unstuff; // number of bits in val, subtract 1 if
                                     // the previously read byte requires
                                     // unstuffing

      // data is unstuffed and accumulated in t
      // bits has the number of bits in t
      ui32 t = val & 0xFF;
      bool unstuff = ((val & 0xFF) == 0xFF); // true if we need unstuffing
      bits -= unstuff; // there is one less bit in t if unstuffing is needed
      t = t << (8 - unstuff); // move up to make room for the next byte

      //this is a repeat of the above
      t |= (val>>8) & 0xFF;
      unstuff = (((val >> 8) & 0xFF) == 0xFF);
      bits -= unstuff;
      t = t << (8 - unstuff);

      t |= (val>>16) & 0xFF;
      unstuff = (((val >> 16) & 0xFF) == 0xFF);
      bits -= unstuff;
      t = t << (8 - unstuff);

      t |= (val>>24) & 0xFF;
      melp->unstuff = (((val >> 24) & 0xFF) == 0xFF);

      // move t to tmp, and push the result all the way up, so we read from
      // the MSB
      melp->tmp |= ((ui64)t) << (64 - bits - melp->bits);
      melp->bits += bits; //increment the number of bits in tmp
    }

    //************************************************************************/
    /** @brief Decodes unstuffed MEL segment bits stored in tmp to runs
     *
     *  Runs are stored in "runs" and the number of runs in "num_runs".
     *  Each run represents a number of zero events that may or may not
     *  terminate in a 1 event.
     *  Each run is stored in 7 bits.  The LSB is 1 if the run terminates in
     *  a 1 event, 0 otherwise.  The next 6 bits, for the case terminating
     *  with 1, contain the number of consecutive 0 zero events * 2; for the
     *  case terminating with 0, they store (number of consecutive 0 zero
     *  events - 1) * 2.
     *  A total of 6 bits (made up of 1 + 5) should have been enough.
     *
     *  @param [in]  melp is a pointer to dec_mel_st structure
     */
    static inline
    void mel_decode(dec_mel_st *melp)
    {
      static const int mel_exp[13] = { //MEL exponents
        0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 5
      };

      if (melp->bits < 6) // if there are less than 6 bits in tmp
        mel_read(melp);   // then read from the MEL bitstream
                          // 6 bits is the largest decodable MEL cwd

      //repeat so long that there is enough decodable bits in tmp,
      // and the runs store is not full (num_runs < 8)
      while (melp->bits >= 6 && melp->num_runs < 8)
      {
        int eval = mel_exp[melp->k]; // number of bits associated with state
        int run = 0;
        if (melp->tmp & (1ull<<63)) //The next bit to decode (stored in MSB)
        { //one is found
          run = 1 << eval;
          run--; // consecutive runs of 0 events - 1
          melp->k = melp->k + 1 < 12 ? melp->k + 1 : 12;//increment, max is 12
          melp->tmp <<= 1; // consume one bit from tmp
          melp->bits -= 1;
          run = run << 1; // a stretch of zeros not terminating in one
        }
        else
        { //0 is found
          run = (int)(melp->tmp >> (63 - eval)) & ((1 << eval) - 1);
          melp->k = melp->k - 1 > 0 ? melp->k - 1 : 0; //decrement, min is 0
          melp->tmp <<= eval + 1; //consume eval + 1 bits (max is 6)
          melp->bits -= eval + 1;
          run = (run << 1) + 1; // a stretch of zeros terminating with one
        }
        eval = melp->num_runs * 7;           // 7 bits per run
        melp->runs &= ~((ui64)0x3F << eval); // 6 bits are sufficient
        melp->runs |= ((ui64)run) << eval;   // store the value in runs
        melp->num_runs++;                    // increment count
      }
    }

    //************************************************************************/
    /** @brief Initiates a dec_mel_st structure for MEL decoding and reads
     *         some bytes in order to get the read address to a multiple
     *         of 4
     *
     *  @param [in]  melp is a pointer to dec_mel_st structure
     *  @param [in]  bbuf is a pointer to byte buffer
     *  @param [in]  lcup is the length of MagSgn+MEL+VLC segments
     *  @param [in]  scup is the length of MEL+VLC segments
     */
    static inline
    void mel_init(dec_mel_st *melp, ui8* bbuf, int lcup, int scup)
    {
      melp->data = bbuf + lcup - scup; // move the pointer to the start of MEL
      melp->bits = 0;                  // 0 bits in tmp
      melp->tmp = 0;                   //
      melp->unstuff = false;           // no unstuffing
      melp->size = scup - 1;           // size is the length of MEL+VLC-1
      melp->k = 0;                     // 0 for state
      melp->num_runs = 0;              // num_runs is 0
      melp->runs = 0;                  //

      //This code is borrowed; original is for a different architecture
      //These few lines take care of the case where data is not at a multiple
      // of 4 boundary.  It reads 1,2,3 up to 4 bytes from the MEL segment
      int num = 4 - (int)(intptr_t(melp->data) & 0x3);
      for (int i = 0; i < num; ++i) { // this code is similar to mel_read
        assert(melp->unstuff == false || melp->data[0] <= 0x8F);
        ui64 d = (melp->size > 0) ? *melp->data : 0xFF;//if buffer is consumed
                                                       //set data to 0xFF
        if (melp->size == 1) d |= 0xF; //if this is MEL+VLC-1, set LSBs to 0xF
                                       // see the standard
        melp->data += melp->size-- > 0; //increment if the end is not reached
        int d_bits = 8 - melp->unstuff; //if unstuffing is needed, reduce by 1
        melp->tmp = (melp->tmp << d_bits) | d; //store bits in tmp
        melp->bits += d_bits;  //increment tmp by number of bits
        melp->unstuff = ((d & 0xFF) == 0xFF); //true of next byte needs
                                              //unstuffing
      }
      melp->tmp <<= (64 - melp->bits); //push all the way up so the first bit
                                       // is the MSB
    }

    //************************************************************************/
    /** @brief Retrieves one run from dec_mel_st; if there are no runs stored
     *         MEL segment is decoded
     *
     * @param [in]  melp is a pointer to dec_mel_st structure
     */
    static inline
    int mel_get_run(dec_mel_st *melp)
    {
      if (melp->num_runs == 0)  //if no runs, decode more bit from MEL segment
        mel_decode(melp);

      int t = melp->runs & 0x7F; //retrieve one run
      melp->runs >>= 7;  // remove the retrieved run
      melp->num_runs--;
      return t; // return run
    }


    //************************************************************************/
    /** @brief De-stuffs a forward-growing bitstream into a flat buffer
     *
     *  This replicates, in one pass, the bit production of the frwd_struct
     *  bitstream reader used by the other block decoders: a byte following
     *  a 0xFF byte contributes only 7 bits (its MSB -- guaranteed 0 by the
     *  encoder -- is OR'ed with the following bit), and bits beyond the
     *  end of the data
     *  read as X (1s for MagSgn, 0s for SPP).  With the stream flattened,
     *  bits for any quad can be fetched at an absolute bit position (see
     *  dfetch), removing the serial fetch/advance state from the decode
     *  loops.
     *
     *  Writes at most cap + 1 destuffed bytes followed by 64 bytes of
     *  padding; dst must have room for cap + 65 bytes.  cap must be
     *  no smaller than the maximum number of bits the decoder can consume
     *  divided by 8, so clipping the data at cap loses no decodable bits.
     *
     *  @tparam      X is the value fed in when the bitstream is exhausted
     *  @param [in]  src is a pointer to the start of the bitstream data
     *  @param [in]  size is the number of bytes in the bitstream data
     *  @param [in]  dst is the output buffer, of at least cap + 65 bytes
     *  @param [in]  cap is the maximum number of destuffed bytes to write
     *  @return ui32 clamp offset for dfetch; bytes at or beyond this
     *               offset hold no stream bits (they read as X)
     */
    template<int X>
    static inline
    ui32 destuff_frwd(const ui8* src, int size, ui8* dst, ui32 cap)
    {
      if (size < 0)
        size = 0;
      ui8* o = dst;
      ui8* o_end = dst + cap;
      const ui8* s = src;
      const ui8* s_end = src + size;
      ui64 acc = 0;       // partial output byte, low nb bits are valid
      ui32 nb = 0;        // number of valid bits in acc; always < 8
      bool prev_ff = false;

      // fast path; 16 source bytes at a time when they contain no 0xFF
      while (s + 16 <= s_end && o + 24 <= o_end)
      {
        __m128i v = _mm_loadu_si128((const __m128i*)s);
        int ff = _mm_movemask_epi8(_mm_cmpeq_epi8(v, _mm_set1_epi8(-1)));
        if (ff != 0 || prev_ff)
        { // process these 16 bytes one at a time
          for (int i = 0; i < 16; ++i) {
            ui8 b = *s++;
            acc |= (ui64)b << nb;
            nb += prev_ff ? 7u : 8u;
            prev_ff = (b == 0xFFu);
            if (nb >= 8) { *o++ = (ui8)acc; acc >>= 8; nb -= 8; }
          }
          continue;
        }
        ui64 v0, v1;
        memcpy(&v0, s, 8);
        memcpy(&v1, s + 8, 8);
        ui64 w0 = acc | (v0 << nb);
        ui64 w1 = (v1 << nb) | (nb ? (v0 >> (64 - nb)) : 0);
        memcpy(o, &w0, 8);
        memcpy(o + 8, &w1, 8);
        acc = nb ? (v1 >> (64 - nb)) : 0;
        o += 16;
        s += 16;
      }
      // tail; one byte at a time
      while (s < s_end && o < o_end)
      {
        ui8 b = *s++;
        acc |= (ui64)b << nb;
        nb += prev_ff ? 7u : 8u;
        prev_ff = (b == 0xFFu);
        if (nb >= 8) { *o++ = (ui8)acc; acc >>= 8; nb -= 8; }
      }
      // fill the bits above nb with X, and pad with X bytes
      ui32 fill = (X == 0xFF) ? (0xFFu << nb) : 0;
      *o = (ui8)((ui32)acc | fill);
      __m256i pad = _mm256_set1_epi8((char)X);
      _mm256_storeu_si256((__m256i*)(o + 1), pad);
      _mm256_storeu_si256((__m256i*)(o + 33), pad);
      return (ui32)(o - dst) + 1;
    }

    //************************************************************************/
    /** @brief Fetches 128 bits from a destuffed MagSgn buffer
     *
     *  Returns the 128 bits starting at bit position pos of the buffer
     *  produced by destuff_frwd.  Unlike a stateful bitstream reader,
     *  this carries no
     *  serial state; fetches at independent positions can execute out of
     *  order.  The byte offset is clamped to limit so that positions past
     *  the end of the stream read as 1s without leaving the buffer.
     *
     *  @param [in]  dbuf is the destuffed MagSgn buffer
     *  @param [in]  limit is the clamp offset returned by destuff_frwd
     *  @param [in]  pos is the absolute bit position to fetch from
     */
    OJPH_FORCE_INLINE
    __m128i dfetch(const ui8* dbuf, ui32 limit, ui32 pos)
    {
      ui32 off = pos >> 3;
      off = off < limit ? off : limit;
      const ui8* p = dbuf + off;
      __m128i v = _mm_loadu_si128((const __m128i*)p);
      __m128i w = _mm_loadu_si128((const __m128i*)(p + 8));
      int k = (int)(pos & 7);
      __m128i r = _mm_srl_epi64(v, _mm_cvtsi32_si128(k));
      __m128i c = _mm_sll_epi64(w, _mm_cvtsi32_si128(64 - k));
      return _mm_or_si128(r, c);
    }

    //************************************************************************/
    /** @brief Fetches at least 57 bits from a destuffed bitstream
     *
     *  Scalar counterpart of dfetch; returns the bits starting at bit
     *  position pos of a buffer produced by destuff_frwd, in the lower
     *  bits of the result.  Bits above 64 - (pos & 7) are garbage.
     *
     *  @param [in]  dbuf is the destuffed bitstream buffer
     *  @param [in]  limit is the clamp offset returned by destuff_frwd
     *  @param [in]  pos is the absolute bit position to fetch from
     */
    OJPH_FORCE_INLINE
    ui64 dfetch64(const ui8* dbuf, ui32 limit, ui32 pos)
    {
      ui32 off = pos >> 3;
      off = off < limit ? off : limit;
      ui64 v;
      memcpy(&v, dbuf + off, 8);
      return v >> (pos & 7);
    }

    //************************************************************************/
    /** @brief Branchless refill of a register-resident bit window
     *
     *  Loads 8 bytes from a destuffed buffer and inserts whole bytes
     *  above the bits remaining in the window, leaving 56 to 63 valid
     *  bits.  Because the inserted bits land above the remaining ones,
     *  consumers of the low bits need not wait for the load, keeping it
     *  off the critical dependency chain.  The read offset is clamped to
     *  limit so that, when consumption overruns the stream (truncated
     *  codeblocks), reads come from the zero padding -- the bytes there
     *  are exactly the fill the stream would produce -- instead of from
     *  uninitialized buffer memory.
     *
     *  @param [in,out]  val is the bit window; its low bits are valid
     *  @param [in,out]  bits is the number of valid bits in val
     *  @param [in,out]  off is the read offset in the destuffed buffer
     *  @param [in]      dbuf is the destuffed bitstream buffer
     *  @param [in]      limit is the clamp offset returned by destuff_vlc
     */
    OJPH_FORCE_INLINE
    void drefill(ui64& val, ui32& bits, ui32& off,
                 const ui8* dbuf, ui32 limit)
    {
      ui64 v;
      ui32 o = off < limit ? off : limit;
      memcpy(&v, dbuf + o, 8);
      val |= v << bits;
      off += (63 - bits) >> 3;
      bits |= 56;
    }

    //************************************************************************/
    /** @brief Consumes bits from a window refilled by drefill
     *
     *  @param [in,out]  val is the bit window
     *  @param [in,out]  bits is the number of valid bits in val
     *  @param [in]      num_bits is the number of bits to consume
     */
    OJPH_FORCE_INLINE
    void dconsume(ui64& val, ui32& bits, ui32 num_bits)
    {
      val >>= num_bits;
      bits -= num_bits;
    }

    //************************************************************************/
    /** @brief De-stuffs a backward-growing bitstream into a flat buffer
     *
     *  This replicates the bit production of the rev_struct bitstream
     *  reader used by the other block decoders, for both its VLC and MRP
     *  variants: reading backward from p, a byte contributes only 7 bits
     *  (its MSB -- guaranteed 0 by the encoder -- is OR'ed with the
     *  following bit) when the byte after it in memory is greater than
     *  0x8F and its own low 7 bits are all 1s.  Bits beyond the end of
     *  the data read as 0s, matching both readers.  The first byte is
     *  processed with the caller-provided unstuff state; after that the
     *  state always equals (p[1] > 0x8F), so the fast path can detect
     *  stuffing with a pairwise byte comparison within the segment.
     *
     *  Writes at most cap + 1 destuffed bytes followed by 64 bytes of
     *  zero padding; dst must have room for cap + 65 bytes.  cap must be
     *  no smaller than the maximum number of bits the decoder can consume
     *  divided by 8, so clipping the data at cap loses no decodable bits.
     *
     *  @param [in]  p is a pointer to the last byte of the segment, where
     *               backward reading starts
     *  @param [in]  size is the number of bytes in the segment
     *  @param [in]  unstuff is the initial unstuffing state
     *  @param [in]  acc holds bits produced by initialization, low nb bits
     *  @param [in]  nb is the number of valid bits in acc; less than 8
     *  @param [in]  dst is the output buffer, of at least cap + 65 bytes
     *  @param [in]  cap is the maximum number of destuffed bytes to write
     *  @return ui32 clamp offset for dfetch/dfetch64; bytes at or beyond
     *               this offset hold no stream bits (they read as 0s)
     */
    static inline
    ui32 destuff_rev(const ui8* p, int size, bool unstuff,
                     ui64 acc, ui32 nb, ui8* dst, ui32 cap)
    {
      ui8* o = dst;
      ui8* o_end = dst + cap;

      // process the first byte with the caller-provided unstuff state;
      // afterwards the state is implied by the byte at p[1], which the
      // fast path checks vectorially (and which stays inside the segment)
      if (size > 0 && o < o_end)
      {
        ui32 d = *p--;
        --size;
        acc |= (ui64)d << nb;
        nb += 8 - ((unstuff && ((d & 0x7F) == 0x7F)) ? 1u : 0u);
        unstuff = d > 0x8F;
        if (nb >= 8) { *o++ = (ui8)acc; acc >>= 8; nb -= 8; }
      }

      // fast path; 16 source bytes at a time when none needs unstuffing
      while (size >= 16 && o + 24 <= o_end)
      {
        __m128i v = _mm_loadu_si128((const __m128i*)(p - 15));
        __m128i nx = _mm_loadu_si128((const __m128i*)(p - 14));
        __m128i is7f = _mm_cmpeq_epi8(
            _mm_and_si128(v, _mm_set1_epi8(0x7F)), _mm_set1_epi8(0x7F));
        // le8f is 0xFF where the byte after (in memory) is <= 0x8F
        __m128i le8f = _mm_cmpeq_epi8(
            _mm_subs_epu8(nx, _mm_set1_epi8((char)0x8F)),
            _mm_setzero_si128());
        __m128i stuff = _mm_andnot_si128(le8f, is7f);
        if (!_mm_testz_si128(stuff, stuff))
        { // process these 16 bytes one at a time
          for (int i = 0; i < 16; ++i) {
            ui32 d = *p--;
            acc |= (ui64)d << nb;
            nb += 8 - ((unstuff && ((d & 0x7F) == 0x7F)) ? 1u : 0u);
            unstuff = d > 0x8F;
            if (nb >= 8) { *o++ = (ui8)acc; acc >>= 8; nb -= 8; }
          }
          size -= 16;
          continue;
        }
        __m128i r = _mm_shuffle_epi8(v,
            _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7,
                         8, 9, 10, 11, 12, 13, 14, 15)); // reverse bytes
#ifdef OJPH_ARCH_X86_64
        ui64 v0 = (ui64)_mm_cvtsi128_si64(r);
        ui64 v1 = (ui64)_mm_extract_epi64(r, 1);
#else // 32-bit x86 lacks the 64-bit extract intrinsics
        ui64 v0 = (ui32)_mm_cvtsi128_si32(r)
                | ((ui64)(ui32)_mm_extract_epi32(r, 1) << 32);
        ui64 v1 = (ui32)_mm_extract_epi32(r, 2)
                | ((ui64)(ui32)_mm_extract_epi32(r, 3) << 32);
#endif
        ui64 w0 = acc | (v0 << nb);
        ui64 w1 = (v1 << nb) | (nb ? (v0 >> (64 - nb)) : 0);
        memcpy(o, &w0, 8);
        memcpy(o + 8, &w1, 8);
        acc = nb ? (v1 >> (64 - nb)) : 0;
        o += 16;
        p -= 16;
        size -= 16;
        unstuff = p[1] > 0x8F;
      }
      // tail; one byte at a time
      while (size > 0 && o < o_end)
      {
        ui32 d = *p--;
        --size;
        acc |= (ui64)d << nb;
        nb += 8 - ((unstuff && ((d & 0x7F) == 0x7F)) ? 1u : 0u);
        unstuff = d > 0x8F;
        if (nb >= 8) { *o++ = (ui8)acc; acc >>= 8; nb -= 8; }
      }
      // the bits above nb are already 0 = fill; pad with zero bytes
      *o = (ui8)acc;
      __m256i z = _mm256_setzero_si256();
      _mm256_storeu_si256((__m256i*)(o + 1), z);
      _mm256_storeu_si256((__m256i*)(o + 33), z);
      return (ui32)(o - dst) + 1;
    }

    //************************************************************************/
    /** @brief De-stuffs the VLC segment into a flat buffer
     *
     *  Performs the same initialization rev_init does -- the first byte
     *  contributes only its upper 4 bits (3 bits if its lower 3 bits are
     *  all 1s) -- then de-stuffs the remaining scup - 2 bytes backward;
     *  see destuff_rev.
     *
     *  @param [in]  data is a pointer to byte at the start of the cleanup
     *               pass
     *  @param [in]  lcup is the length of MagSgn+MEL+VLC segments
     *  @param [in]  scup is the length of MEL+VLC segments
     *  @param [in]  dst is the output buffer, of at least cap + 65 bytes
     *  @param [in]  cap is the maximum number of destuffed bytes to write
     *  @return ui32 clamp offset for dfetch64
     */
    static inline
    ui32 destuff_vlc(const ui8* data, int lcup, int scup,
                     ui8* dst, ui32 cap)
    {
      const ui8* p = data + lcup - 2;
      ui32 d = *p; // first byte, only the upper 4 bits are used
      ui64 acc = d >> 4;
      ui32 nb = 4 - ((acc & 7) == 7); // check standard
      bool unstuff = (d | 0xF) > 0x8F;
      return destuff_rev(p - 1, scup - 2, unstuff, acc, nb, dst, cap);
    }

    //************************************************************************/
    /** @brief De-stuffs the MRP segment into a flat buffer
     *
     *  Performs the same initialization rev_init_mrp does -- reading
     *  starts at the last byte of the SPP+MRP segments with the unstuff
     *  state set -- then de-stuffs backward; see destuff_rev.
     *
     *  @param [in]  data is a pointer to byte at the start of the cleanup
     *               pass
     *  @param [in]  lcup is the length of MagSgn+MEL+VLC segments
     *  @param [in]  len2 is the length of SPP+MRP segments
     *  @param [in]  dst is the output buffer, of at least cap + 65 bytes
     *  @param [in]  cap is the maximum number of destuffed bytes to write
     *  @return ui32 clamp offset for dfetch64
     */
    static inline
    ui32 destuff_mrp(const ui8* data, int lcup, int len2,
                     ui8* dst, ui32 cap)
    {
      return destuff_rev(data + lcup + len2 - 1, len2, true, 0, 0,
                         dst, cap);
    }

    //************************************************************************/
    /** @brief decodes twos consecutive quads (one octet), using 32 bit data
     *
     *  @param inf_u_q  decoded VLC code, with interleaved u values
     *  @param U_q      U values
     *  @param magsgn   structure for forward data buffer
     *  @param p        bitplane at which we are decoding
     *  @param vn       used for handling E values (stores v_n values)
     *  @return __m256i decoded two quads
     */
    OJPH_FORCE_INLINE
    __m256i decode_two_quad32_avx2(__m256i inf_u_q, __m256i U_q,
                                   const ui8* dbuf, ui32 limit, ui32& pos,
                                   ui32 p, __m128i& vn) {
        __m256i row = _mm256_setzero_si256();

        // we keeps e_k, e_1, and rho in w2
        __m256i flags = _mm256_and_si256(inf_u_q, _mm256_set_epi32(0x8880, 0x4440, 0x2220, 0x1110, 0x8880, 0x4440, 0x2220, 0x1110));
        __m256i insig = _mm256_cmpeq_epi32(flags, _mm256_setzero_si256());

        if ((uint32_t)_mm256_movemask_epi8(insig) != (uint32_t)0xFFFFFFFF) //are all insignificant?
        {
            flags = _mm256_mullo_epi16(flags, _mm256_set_epi16(1, 1, 2, 2, 4, 4, 8, 8, 1, 1, 2, 2, 4, 4, 8, 8));

            // U_q holds U_q for this quad
            // flags has e_k, e_1, and rho such that e_k is sitting in the
            // 0x8000, e_1 in 0x800, and rho in 0x80

            // next e_k and m_n
            __m256i m_n;
            __m256i w0 = _mm256_srli_epi32(flags, 15); // e_k
            m_n = _mm256_sub_epi32(U_q, w0);
            m_n = _mm256_andnot_si256(insig, m_n);

            // find cumulative sums
            // to find at which bit in ms_vec the sample starts
            __m256i inc_sum = m_n; // inclusive scan
            inc_sum = _mm256_add_epi32(inc_sum, _mm256_bslli_epi128(inc_sum, 4));
            inc_sum = _mm256_add_epi32(inc_sum, _mm256_bslli_epi128(inc_sum, 8));
            ui32 total_mn1 = (ui32)_mm256_extract_epi16(inc_sum, 6);
            ui32 total_mn2 = (ui32)_mm256_extract_epi16(inc_sum, 14);

            __m128i ms_vec0 = dfetch(dbuf, limit, pos);
            __m128i ms_vec1 = dfetch(dbuf, limit, pos + total_mn1);
            pos += total_mn1 + total_mn2;

            __m256i ms_vec = _mm256_inserti128_si256(_mm256_castsi128_si256(ms_vec0), ms_vec1, 0x1);

            __m256i ex_sum = _mm256_bslli_epi128(inc_sum, 4); // exclusive scan

            // find the starting byte and starting bit
            __m256i byte_idx = _mm256_srli_epi32(ex_sum, 3);
            __m256i bit_idx = _mm256_and_si256(ex_sum, _mm256_set1_epi32(7));
            byte_idx = _mm256_shuffle_epi8(byte_idx,
                _mm256_set_epi32(0x0C0C0C0C, 0x08080808, 0x04040404, 0x00000000, 0x0C0C0C0C, 0x08080808, 0x04040404, 0x00000000));
            byte_idx = _mm256_add_epi32(byte_idx, _mm256_set1_epi32(0x03020100));
            __m256i d0 = _mm256_shuffle_epi8(ms_vec, byte_idx);
            byte_idx = _mm256_add_epi32(byte_idx, _mm256_set1_epi32(0x01010101));
            __m256i d1 = _mm256_shuffle_epi8(ms_vec, byte_idx);

            // shift samples values to correct location
            bit_idx = _mm256_or_si256(bit_idx, _mm256_slli_epi32(bit_idx, 16));

            __m128i a = _mm_set_epi8(1, 3, 7, 15, 31, 63, 127, -1, 1, 3, 7, 15, 31, 63, 127, -1);
            __m256i aa = _mm256_inserti128_si256(_mm256_castsi128_si256(a), a, 0x1);

            __m256i bit_shift = _mm256_shuffle_epi8(aa, bit_idx);
            bit_shift = _mm256_add_epi16(bit_shift, _mm256_set1_epi16(0x0101));
            d0 = _mm256_mullo_epi16(d0, bit_shift);
            d0 = _mm256_srli_epi16(d0, 8); // we should have 8 bits in the LSB
            d1 = _mm256_mullo_epi16(d1, bit_shift);
            d1 = _mm256_and_si256(d1, _mm256_set1_epi32((si32)0xFF00FF00)); // 8 in MSB
            d0 = _mm256_or_si256(d0, d1);

            // find location of e_k and mask
            __m256i shift;
            __m256i ones = _mm256_set1_epi32(1);
            __m256i twos = _mm256_set1_epi32(2);
            __m256i U_q_m1 = _mm256_sub_epi32(U_q, ones);
            U_q_m1 = _mm256_and_si256(U_q_m1, _mm256_set_epi32(0, 0, 0, 0x1F, 0, 0, 0, 0x1F));
            U_q_m1 = _mm256_shuffle_epi32(U_q_m1, 0);
            w0 = _mm256_sub_epi32(twos, w0);
            shift = _mm256_sllv_epi32(w0, U_q_m1); // U_q_m1 must be no more than 31
            ms_vec = _mm256_and_si256(d0, _mm256_sub_epi32(shift, ones));

            // next e_1
            w0 = _mm256_and_si256(flags, _mm256_set1_epi32(0x800));
            w0 = _mm256_cmpeq_epi32(w0, _mm256_setzero_si256());
            w0 = _mm256_andnot_si256(w0, shift);  // e_1 in correct position
            ms_vec = _mm256_or_si256(ms_vec, w0); // e_1
            w0 = _mm256_slli_epi32(ms_vec, 31);   // sign
            ms_vec = _mm256_or_si256(ms_vec, ones); // bin center
            __m256i tvn = ms_vec;
            ms_vec = _mm256_add_epi32(ms_vec, twos);// + 2
            ms_vec = _mm256_slli_epi32(ms_vec, (si32)p - 1);
            ms_vec = _mm256_or_si256(ms_vec, w0); // sign
            row = _mm256_andnot_si256(insig, ms_vec); // significant only

            ms_vec = _mm256_andnot_si256(insig, tvn); // significant only

            tvn = _mm256_shuffle_epi8(ms_vec, _mm256_set_epi32(-1, 0x0F0E0D0C, 0x07060504, -1, -1, -1, 0x0F0E0D0C, 0x07060504));

            vn = _mm_or_si128(vn, _mm256_castsi256_si128(tvn));
            vn = _mm_or_si128(vn, _mm256_extracti128_si256(tvn, 0x1));
        }
        return row;
    }


   //************************************************************************/
    /** @brief decodes twos consecutive quads (one octet), using 16 bit data
     *
     *  @param inf_u_q  decoded VLC code, with interleaved u values
     *  @param U_q      U values
     *  @param magsgn   structure for forward data buffer
     *  @param p        bitplane at which we are decoding
     *  @param vn       used for handling E values (stores v_n values)
     *  @return __m128i decoded quad
     */

    OJPH_FORCE_INLINE
    __m256i decode_four_quad16(const __m128i inf_u_q, __m128i U_q,
                               const ui8* dbuf, ui32 limit, ui32& pos,
                               ui32 p, __m128i& vn) {

        __m256i w0;     // workers
        __m256i insig;  // lanes hold FF's if samples are insignificant
        __m256i flags;  // lanes hold e_k, e_1, and rho

        __m256i row = _mm256_setzero_si256();
        __m128i ddd = _mm_shuffle_epi8(inf_u_q,
            _mm_set_epi16(0x0d0c, 0x0d0c, 0x0908, 0x908, 0x0504, 0x0504, 0x0100, 0x0100));
        w0 = _mm256_permutevar8x32_epi32(_mm256_castsi128_si256(ddd),
            _mm256_setr_epi32(0, 0, 1, 1, 2, 2, 3, 3));
        // we keeps e_k, e_1, and rho in w2
        flags = _mm256_and_si256(w0,
            _mm256_set_epi16((si16)0x8880, 0x4440, 0x2220, 0x1110,
                             (si16)0x8880, 0x4440, 0x2220, 0x1110,
                             (si16)0x8880, 0x4440, 0x2220, 0x1110,
                             (si16)0x8880, 0x4440, 0x2220, 0x1110));
        insig = _mm256_cmpeq_epi16(flags, _mm256_setzero_si256());
        if ((uint32_t)_mm256_movemask_epi8(insig) != (uint32_t)0xFFFFFFFF) //are all insignificant?
        {
            ddd = _mm_or_si128(_mm_bslli_si128(U_q, 2), U_q);
            __m256i U_q_avx = _mm256_permutevar8x32_epi32(_mm256_castsi128_si256(ddd),
                _mm256_setr_epi32(0, 0, 1, 1, 2, 2, 3, 3));
            flags = _mm256_mullo_epi16(flags, _mm256_set_epi16(1, 2, 4, 8, 1, 2, 4, 8, 1, 2, 4, 8, 1, 2, 4, 8));

            // U_q holds U_q for this quad
            // flags has e_k, e_1, and rho such that e_k is sitting in the
            // 0x8000, e_1 in 0x800, and rho in 0x80

            // next e_k and m_n
            __m256i m_n;
            w0 = _mm256_srli_epi16(flags, 15); // e_k
            m_n = _mm256_sub_epi16(U_q_avx, w0);
            m_n = _mm256_andnot_si256(insig, m_n);

            // find cumulative sums
            // to find at which bit in ms_vec the sample starts
            __m256i inc_sum = m_n; // inclusive scan
            inc_sum = _mm256_add_epi16(inc_sum, _mm256_bslli_epi128(inc_sum, 2));
            inc_sum = _mm256_add_epi16(inc_sum, _mm256_bslli_epi128(inc_sum, 4));
            inc_sum = _mm256_add_epi16(inc_sum, _mm256_bslli_epi128(inc_sum, 8));
            ui32 total_mn1 = (ui32)_mm256_extract_epi16(inc_sum, 7);
            ui32 total_mn2 = (ui32)_mm256_extract_epi16(inc_sum, 15);
            __m256i ex_sum = _mm256_bslli_epi128(inc_sum, 2); // exclusive scan

            __m128i ms_vec0 = dfetch(dbuf, limit, pos);
            __m128i ms_vec1 = dfetch(dbuf, limit, pos + total_mn1);
            pos += total_mn1 + total_mn2;

            __m256i ms_vec = _mm256_inserti128_si256(_mm256_castsi128_si256(ms_vec0), ms_vec1, 0x1);

            // find the starting byte and starting bit
            __m256i byte_idx = _mm256_srli_epi16(ex_sum, 3);
            __m256i bit_idx = _mm256_and_si256(ex_sum, _mm256_set1_epi16(7));
            byte_idx = _mm256_shuffle_epi8(byte_idx,
                _mm256_set_epi16(0x0E0E, 0x0C0C, 0x0A0A, 0x0808,
                    0x0606, 0x0404, 0x0202, 0x0000, 0x0E0E, 0x0C0C, 0x0A0A, 0x0808,
                    0x0606, 0x0404, 0x0202, 0x0000));
            byte_idx = _mm256_add_epi16(byte_idx, _mm256_set1_epi16(0x0100));
            __m256i d0 = _mm256_shuffle_epi8(ms_vec, byte_idx);
            byte_idx = _mm256_add_epi16(byte_idx, _mm256_set1_epi16(0x0101));
            __m256i d1 = _mm256_shuffle_epi8(ms_vec, byte_idx);

            // shift samples values to correct location
            __m256i bit_shift = _mm256_shuffle_epi8(
                _mm256_set_epi8(1, 3, 7, 15, 31, 63, 127, -1,
                    1, 3, 7, 15, 31, 63, 127, -1, 1, 3, 7, 15, 31, 63, 127, -1,
                    1, 3, 7, 15, 31, 63, 127, -1), bit_idx);
            bit_shift = _mm256_add_epi16(bit_shift, _mm256_set1_epi16(0x0101));
            d0 = _mm256_mullo_epi16(d0, bit_shift);
            d0 = _mm256_srli_epi16(d0, 8); // we should have 8 bits in the LSB
            d1 = _mm256_mullo_epi16(d1, bit_shift);
            d1 = _mm256_and_si256(d1, _mm256_set1_epi16((si16)0xFF00)); // 8 in MSB
            d0 = _mm256_or_si256(d0, d1);

            // find location of e_k and mask
            __m256i shift;
            __m256i ones = _mm256_set1_epi16(1);
            __m256i twos = _mm256_set1_epi16(2);
            // shift = (2 - e_k) << (U_q - 1); AVX2 has no _mm256_sllv_epi16,
            // so the variable shift is emulated with a pshufb power-of-two
            // lookup and a 16-bit multiply.  U_q - 1 <= 14 in this path;
            // for U_q == 0 the lookup indices have their MSB set, so pshufb
            // returns 0, the same shift value the former uniform 16-bit
            // shift emulation produced (it shifted by 31)
            __m256i kq = _mm256_sub_epi16(U_q_avx, ones);
            __m256i idx = _mm256_or_si256(kq,
                _mm256_slli_epi16(_mm256_sub_epi16(kq,
                                            _mm256_set1_epi16(8)), 8));
            const __m256i pow2_tbl = _mm256_setr_epi8(
                1, 2, 4, 8, 16, 32, 64, (char)128, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 2, 4, 8, 16, 32, 64, (char)128, 0, 0, 0, 0, 0, 0, 0, 0);
            __m256i pow2 = _mm256_shuffle_epi8(pow2_tbl, idx);
            w0 = _mm256_sub_epi16(twos, w0);
            shift = _mm256_mullo_epi16(w0, pow2);
            ms_vec = _mm256_and_si256(d0, _mm256_sub_epi16(shift, ones));

            // next e_1
            w0 = _mm256_and_si256(flags, _mm256_set1_epi16(0x800));
            w0 = _mm256_cmpeq_epi16(w0, _mm256_setzero_si256());
            w0 = _mm256_andnot_si256(w0, shift);  // e_1 in correct position
            ms_vec = _mm256_or_si256(ms_vec, w0); // e_1
            w0 = _mm256_slli_epi16(ms_vec, 15);   // sign
            ms_vec = _mm256_or_si256(ms_vec, ones); // bin center
            __m256i tvn = ms_vec;
            ms_vec = _mm256_add_epi16(ms_vec, twos);// + 2
            ms_vec = _mm256_slli_epi16(ms_vec, (si32)p - 1);
            ms_vec = _mm256_or_si256(ms_vec, w0); // sign
            row = _mm256_andnot_si256(insig, ms_vec); // significant only

            ms_vec = _mm256_andnot_si256(insig, tvn); // significant only

            __m256i ms_vec_shuffle1 = _mm256_shuffle_epi8(ms_vec,
                _mm256_set_epi16(-1, -1, -1, -1, 0x0706, 0x0302, -1, -1,
                                 -1, -1, -1, -1, -1, -1, 0x0706, 0x0302));
            __m256i ms_vec_shuffle2 = _mm256_shuffle_epi8(ms_vec,
                _mm256_set_epi16(-1, -1, -1, 0x0F0E, 0x0B0A, -1, -1, -1,
                                 -1, -1, -1, -1, -1, 0x0F0E, 0x0B0A, -1));
            ms_vec = _mm256_or_si256(ms_vec_shuffle1, ms_vec_shuffle2);

            vn = _mm_or_si128(vn, _mm256_castsi256_si128(ms_vec));
            vn = _mm_or_si128(vn, _mm256_extracti128_si256(ms_vec, 0x1));
        }
        return row;
    }

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

    //************************************************************************/
    /** @brief Decodes one codeblock, processing the cleanup, siginificance
     *         propagation, and magnitude refinement pass
     *
     *  @param [in]   coded_data is a pointer to bitstream
     *  @param [in]   decoded_data is a pointer to decoded codeblock data buf.
     *  @param [in]   missing_msbs is the number of missing MSBs
     *  @param [in]   num_passes is the number of passes: 1 if CUP only,
     *                2 for CUP+SPP, and 3 for CUP+SPP+MRP
     *  @param [in]   lengths1 is the length of cleanup pass
     *  @param [in]   lengths2 is the length of refinement passes (either SPP
     *                only or SPP+MRP)
     *  @param [in]   width is the decoded codeblock width
     *  @param [in]   height is the decoded codeblock height
     *  @param [in]   stride is the decoded codeblock buffer stride
     *  @param [in]   stripe_causal is true for stripe causal mode
     */
    //************************************************************************/
    /** @brief Step-2 MagSgn decode for the 16-bit (4-quad) path.
     *
     *  Outlined into its own function so that decode_four_quad16 can be
     *  force-inlined here without inflating register pressure in the
     *  much larger ojph_decode_codeblock_avx2 (which also hosts the
     *  mutually-exclusive 32-bit step-2 path). Returns false on a
     *  precision-overflow error, true otherwise.
     */
    OJPH_NO_INLINE
    bool decode_cb_step2_16bit(ui16* scratch, ui32* decoded_data,
                               ui8* coded_data, ui32 width, ui32 height,
                               ui32 stride, ui32 sstr, ui32 p, ui32 mmsbp2,
                               int lcup, int scup)
    {
        // reduce bitplane by 16 because we now have 16 bits instead of 32
        p -= 16;

        const int v_n_size = 512 + 16;
#ifdef __MINGW64__
        ui16 v_n_scratch[v_n_size] = {0};
        ui32 v_n_scratch_32[v_n_size] = {0};
#else
        ui16 v_n_scratch[v_n_size];
        memset(v_n_scratch + (width >> 1) + 4, 0, 8 * sizeof(ui16));
        ui32 v_n_scratch_32[v_n_size];
#endif

        // maximum consumable MagSgn bits: 4096 samples x (mmsbp2 < 16) bits
        const ui32 dbuf_cap = 4096 * 15 / 8;
        ui8 dbuf[dbuf_cap + 72];
        ui32 limit = destuff_frwd<0xFF>(coded_data, lcup - scup, dbuf, dbuf_cap);
        ui32 pos = 0;

        {
          ui16 *sp = scratch;
          ui16 *vp = v_n_scratch;
          ui32 *dp = decoded_data;
          vp[0] = 2; // for easy calculation of emax

          for (ui32 x = 0; x < width; x += 8, sp += 8, vp += 4, dp += 8) {
              ////process four quads
              __m128i inf_u_q = _mm_loadu_si128((__m128i*)sp);
              __m128i U_q = _mm_srli_epi32(inf_u_q, 16);
              __m128i w = _mm_cmpgt_epi32(U_q, _mm_set1_epi32((int)mmsbp2));
              if (!_mm_testz_si128(w, w)) {
                  return false;
              }

              __m128i vn = _mm_set1_epi16(2);
              __m256i row = decode_four_quad16(inf_u_q, U_q, dbuf, limit, pos, p, vn);

              w = _mm_cvtsi32_si128(*(unsigned short const*)(vp));
              _mm_storeu_si128((__m128i*)vp, _mm_or_si128(w, vn));

              __m256i  w0 = _mm256_shuffle_epi8(row, _mm256_set_epi16(0x0D0C, -1, 0x0908, -1, 0x0504, -1, 0x0100, -1, 0x0D0C, -1, 0x0908, -1, 0x0504, -1, 0x0100, -1));
              __m256i  w1 = _mm256_shuffle_epi8(row, _mm256_set_epi16(0x0F0E, -1, 0x0B0A, -1, 0x0706, -1, 0x0302, -1, 0x0F0E, -1, 0x0B0A, -1, 0x0706, -1, 0x0302, -1));

              _mm256_storeu_si256((__m256i*)dp, w0);
              _mm256_storeu_si256((__m256i*)(dp + stride), w1);
          }
        }

        for (ui32 y = 2; y < height; y += 2) {
          {
            // perform 15 - count_leading_zeros(*vp) here
            ui16 *vp = v_n_scratch;
            ui32 *vp_32 = v_n_scratch_32;

            ui16* sp = scratch + (y >> 1) * sstr;
            const __m256i avx_mmsbp2 = _mm256_set1_epi32((int)mmsbp2);
            const __m256i avx_31 = _mm256_set1_epi32(31);
            const __m256i avx_f0 = _mm256_set1_epi32(0xF0);
            const __m256i avx_1 = _mm256_set1_epi32(1);
            const __m256i avx_0 = _mm256_setzero_si256();

            for (ui32 x = 0; x <= width; x += 16, vp += 8, sp += 16, vp_32 += 8) {
              __m128i v = _mm_loadu_si128((__m128i*)vp);
              __m128i v_p1 = _mm_loadu_si128((__m128i*)(vp + 1));
              v = _mm_or_si128(v, v_p1);

              __m256i v_avx = _mm256_cvtepu16_epi32(v);
              v_avx = avx2_lzcnt_epi32(v_avx);
              v_avx = _mm256_sub_epi32(avx_31, v_avx);

              __m256i inf_u_q = _mm256_loadu_si256((__m256i*)sp);
              __m256i gamma = _mm256_and_si256(inf_u_q, avx_f0);
              __m256i w0 = _mm256_sub_epi32(gamma, avx_1);
              gamma = _mm256_and_si256(gamma, w0);
              gamma = _mm256_cmpeq_epi32(gamma, avx_0);

              v_avx = _mm256_andnot_si256(gamma, v_avx);
              v_avx = _mm256_max_epi32(v_avx, avx_1);

              inf_u_q = _mm256_srli_epi32(inf_u_q, 16);
              v_avx = _mm256_add_epi32(inf_u_q, v_avx);

              w0 = _mm256_cmpgt_epi32(v_avx, avx_mmsbp2);
              if (!_mm256_testz_si256(w0, w0)) {
                  return false;
              }

              _mm256_storeu_si256((__m256i*)vp_32, v_avx);
            }
          }

          ui16 *vp = v_n_scratch;
          ui32* vp_32 = v_n_scratch_32;
          ui16 *sp = scratch + (y >> 1) * sstr;
          ui32 *dp = decoded_data + y * stride;
          vp[0] = 2; // for easy calculation of emax

          for (ui32 x = 0; x < width; x += 8, sp += 8, vp += 4, dp += 8, vp_32 += 4) {
            ////process four quads
              __m128i inf_u_q = _mm_loadu_si128((__m128i*)sp);
              __m128i U_q = _mm_loadu_si128((__m128i*)vp_32);

            __m128i vn = _mm_set1_epi16(2);
            __m256i row = decode_four_quad16(inf_u_q, U_q, dbuf, limit, pos, p, vn);

            __m128i w = _mm_cvtsi32_si128(*(unsigned short const*)(vp));
            _mm_storeu_si128((__m128i*)vp, _mm_or_si128(w, vn));

            __m256i  w0 = _mm256_shuffle_epi8(row, _mm256_set_epi16(0x0D0C, -1, 0x0908, -1, 0x0504, -1, 0x0100, -1, 0x0D0C, -1, 0x0908, -1, 0x0504, -1, 0x0100, -1));
            __m256i  w1 = _mm256_shuffle_epi8(row, _mm256_set_epi16(0x0F0E, -1, 0x0B0A, -1, 0x0706, -1, 0x0302, -1, 0x0F0E, -1, 0x0B0A, -1, 0x0706, -1, 0x0302, -1));

            _mm256_storeu_si256((__m256i*)dp, w0);
            _mm256_storeu_si256((__m256i*)(dp + stride), w1);
          }
        }
        return true;
    }

    //************************************************************************/
    /** @brief Step-2 MagSgn decode for the 32-bit (2-quad) path.
     *
     *  Outlined for the same reason as decode_cb_step2_16bit: it keeps the
     *  always-inlined decode_two_quad32_avx2 kernel in its own register
     *  allocation scope, isolated from step-1 and from the 16-bit path.
     *  Returns false on a precision-overflow error, true otherwise.
     */
    OJPH_NO_INLINE
    bool decode_cb_step2_32bit(ui16* scratch, ui32* decoded_data,
                               ui8* coded_data, ui32 width, ui32 height,
                               ui32 stride, ui32 sstr, ui32 p, ui32 mmsbp2,
                               int lcup, int scup)
    {
        const int v_n_size = 512 + 16;
#ifdef __MINGW64__
        ui32 v_n_scratch[2 * v_n_size] = {0};
#else
        ui32 v_n_scratch[2 * v_n_size];
        memset(v_n_scratch + (width >> 1) + 2, 0, 14 * sizeof(ui32));
#endif

        // maximum consumable MagSgn bits: 4096 samples x (mmsbp2 <= 32) bits
        const ui32 dbuf_cap = 4096 * 32 / 8;
        ui8 dbuf[dbuf_cap + 72];
        ui32 limit = destuff_frwd<0xFF>(coded_data, lcup - scup, dbuf, dbuf_cap);
        ui32 pos = 0;

        const __m256i avx_mmsbp2 = _mm256_set1_epi32((int)mmsbp2);

        {
          ui16 *sp = scratch;
          ui32 *vp = v_n_scratch;
          ui32 *dp = decoded_data;
          vp[0] = 2; // for easy calculation of emax

          for (ui32 x = 0; x < width; x += 4, sp += 4, vp += 2, dp += 4)
          {
            __m128i vn = _mm_set1_epi32(2);

            __m256i inf_u_q = _mm256_castsi128_si256(_mm_loadl_epi64((__m128i*)sp));
            inf_u_q = _mm256_permutevar8x32_epi32(inf_u_q, _mm256_setr_epi32(0, 0, 0, 0, 1, 1, 1, 1));

            __m256i U_q = _mm256_srli_epi32(inf_u_q, 16);
            __m256i w = _mm256_cmpgt_epi32(U_q, avx_mmsbp2);
            if (!_mm256_testz_si256(w, w)) {
                return false;
            }

            __m256i row = decode_two_quad32_avx2(inf_u_q, U_q, dbuf, limit, pos, p, vn);
            row = _mm256_permutevar8x32_epi32(row, _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7));
            _mm_store_si128((__m128i*)dp, _mm256_castsi256_si128(row));
            _mm_store_si128((__m128i*)(dp + stride), _mm256_extracti128_si256(row, 0x1));

            __m128i w0 = _mm_cvtsi32_si128(*(int const*)vp);
            w0 = _mm_or_si128(w0, vn);
            _mm_storeu_si128((__m128i*)vp, w0);
          }
        }

        for (ui32 y = 2; y < height; y += 2)
        {
          {
            // perform 31 - count_leading_zeros(*vp) here
            ui32 *vp = v_n_scratch;
            ui16* sp = scratch + (y >> 1) * sstr;

            const __m256i avx_31 = _mm256_set1_epi32(31);
            const __m256i avx_f0 = _mm256_set1_epi32(0xF0);
            const __m256i avx_1 = _mm256_set1_epi32(1);
            const __m256i avx_0 = _mm256_setzero_si256();

            for (ui32 x = 0; x <= width; x += 16, vp += 8, sp += 16) {
              __m256i v = _mm256_loadu_si256((__m256i*)vp);
              __m256i v_p1 = _mm256_loadu_si256((__m256i*)(vp + 1));
              v = _mm256_or_si256(v, v_p1);
              v = avx2_lzcnt_epi32(v);
              v = _mm256_sub_epi32(avx_31, v);

              __m256i inf_u_q = _mm256_loadu_si256((__m256i*)sp);
              __m256i gamma = _mm256_and_si256(inf_u_q, avx_f0);
              __m256i w0 = _mm256_sub_epi32(gamma, avx_1);
              gamma = _mm256_and_si256(gamma, w0);
              gamma = _mm256_cmpeq_epi32(gamma, avx_0);

              v = _mm256_andnot_si256(gamma, v);
              v = _mm256_max_epi32(v, avx_1);

              inf_u_q = _mm256_srli_epi32(inf_u_q, 16);
              v = _mm256_add_epi32(inf_u_q, v);

              w0 = _mm256_cmpgt_epi32(v, avx_mmsbp2);
              if (!_mm256_testz_si256(w0, w0)) {
                  return false;
              }

              _mm256_storeu_si256((__m256i*)(vp + v_n_size), v);
            }
          }

          ui32 *vp = v_n_scratch;
          ui16 *sp = scratch + (y >> 1) * sstr;
          ui32 *dp = decoded_data + y * stride;
          vp[0] = 2; // for easy calculation of emax

          for (ui32 x = 0; x < width; x += 4, sp += 4, vp += 2, dp += 4) {
            //process two quads
            __m128i vn = _mm_set1_epi32(2);

            __m256i inf_u_q = _mm256_castsi128_si256(_mm_loadl_epi64((__m128i*)sp));
            inf_u_q = _mm256_permutevar8x32_epi32(inf_u_q, _mm256_setr_epi32(0, 0, 0, 0, 1, 1, 1, 1));

            __m256i U_q = _mm256_castsi128_si256(_mm_loadl_epi64((__m128i*)(vp + v_n_size)));
            U_q = _mm256_permutevar8x32_epi32(U_q, _mm256_setr_epi32(0, 0, 0, 0, 1, 1, 1, 1));

            __m256i row = decode_two_quad32_avx2(inf_u_q, U_q, dbuf, limit, pos, p, vn);
            row = _mm256_permutevar8x32_epi32(row, _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7));
            _mm_store_si128((__m128i*)dp, _mm256_castsi256_si128(row));
            _mm_store_si128((__m128i*)(dp + stride), _mm256_extracti128_si256(row, 0x1));

            __m128i w0 = _mm_cvtsi32_si128(*(int const*)vp);
            w0 = _mm_or_si128(w0, vn);
            _mm_storeu_si128((__m128i*)vp, w0);
          }
        }
        return true;
    }

    //************************************************************************/
    /** @brief Significance-Propagation and Magnitude-Refinement passes.
     *
     *  Outlined from ojph_decode_codeblock_avx2 so the (lossless cleanup-only)
     *  common path does not pay the register-allocation cost of this ~375-line
     *  block. Only runs when num_passes > 1.
     */
    OJPH_NO_INLINE
    void decode_cb_spp_mrp(ui16* scratch, ui32* decoded_data, ui8* coded_data,
                           ui32 width, ui32 height, ui32 stride, ui32 sstr,
                           ui32 p, ui32 num_passes, ui32 lengths1,
                           ui32 lengths2, bool stripe_causal)
    {
        // We use scratch again, we can divide it into multiple regions
        // sigma holds all the significant samples, and it cannot
        // be modified after it is set.  it will be used during the
        // Magnitude Refinement Pass
        ui16* const sigma = scratch;

        ui32 mstr = (width + 3u) >> 2;   // divide by 4, since each
                                         // ui16 contains 4 columns
        mstr = ((mstr + 2u) + 7u) & ~7u; // multiples of 8

        // We re-arrange quad significance, where each 4 consecutive
        // bits represent one quad, into column significance, where,
        // each 4 consequtive bits represent one column of 4 rows
        {
          ui32 y;

          const __m128i mask_3 = _mm_set1_epi32(0x30);
          const __m128i mask_C = _mm_set1_epi32(0xC0);
          const __m128i shuffle_mask = _mm_set_epi32(-1, -1, -1, 0x0C080400);
          for (y = 0; y < height; y += 4)
          {
            ui16* sp = scratch + (y >> 1) * sstr;
            ui16* dp = sigma + (y >> 2) * mstr;
            for (ui32 x = 0; x < width; x += 8, sp += 8, dp += 2)
            {
              __m128i s0, s1, u3, uC, t0, t1;

              s0 = _mm_loadu_si128((__m128i*)(sp));
              u3 = _mm_and_si128(s0, mask_3);
              u3 = _mm_srli_epi32(u3, 4);
              uC = _mm_and_si128(s0, mask_C);
              uC = _mm_srli_epi32(uC, 2);
              t0 = _mm_or_si128(u3, uC);

              s1 = _mm_loadu_si128((__m128i*)(sp + sstr));
              u3 = _mm_and_si128(s1, mask_3);
              u3 = _mm_srli_epi32(u3, 2);
              uC = _mm_and_si128(s1, mask_C);
              t1 = _mm_or_si128(u3, uC);

              __m128i r = _mm_or_si128(t0, t1);
              r = _mm_shuffle_epi8(r, shuffle_mask);

              ui32 t = (ui32)_mm_extract_epi32(r, 0);
              memcpy(dp, &t, 4);
            }
            dp[0] = 0; // set an extra entry on the right with 0
          }
          {
            // reset one row after the codeblock
            ui16* dp = sigma + (y >> 2) * mstr;
            __m128i zero = _mm_setzero_si128();
            for (ui32 x = 0; x < width; x += 32, dp += 8)
              _mm_storeu_si128((__m128i*)dp, zero);
            dp[0] = 0; // set an extra entry on the right with 0
          }
        }

        // We perform Significance Propagation Pass here
        {
          // This stores significance information of the previous
          // 4 rows.  Significance information in this array includes
          // all signicant samples in bitplane p - 1; that is,
          // significant samples for bitplane p (discovered during the
          // cleanup pass and stored in sigma) and samples that have recently
          // became significant (during the SPP) in bitplane p-1.
          // We store enough for the widest row, containing 1024 columns,
          // which is equivalent to 256 of ui16, since each stores 4 columns.
          // We add an extra 8 entries, just in case we need more
          ui16 prev_row_sig[256 + 8] = {0}; // 528 Bytes

          // maximum consumable SPP bits: 4096 samples x 2 bits (one
          // significance bit and one sign bit per sample)
          const ui32 spp_cap = 4096 * 2 / 8;
          ui8 spp_buf[spp_cap + 72];
          ui32 spp_limit = destuff_frwd<0>(coded_data + lengths1,
                                           (int)lengths2, spp_buf, spp_cap);
          ui32 spp_pos = 0;

          for (ui32 y = 0; y < height; y += 4)
          {
            ui32 pattern = 0xFFFFu; // a pattern needed samples
            if (height - y < 4) {
              pattern = 0x7777u;
              if (height - y < 3) {
                pattern = 0x3333u;
                if (height - y < 2)
                  pattern = 0x1111u;
              }
            }

            // prev holds sign. info. for the previous quad, together
            // with the rows on top of it and below it.
            ui32 prev = 0;
            ui16 *prev_sig = prev_row_sig;
            ui16 *cur_sig = sigma + (y >> 2) * mstr;
            ui32 *dpp = decoded_data + y * stride;
            for (ui32 x = 0; x < width; x += 4, dpp += 4, ++cur_sig, ++prev_sig)
            {
              // only rows and columns inside the stripe are included
              si32 s = (si32)x + 4 - (si32)width;
              s = ojph_max(s, 0);
              pattern = pattern >> (s * 4);

              // We first find locations that need to be tested (potential
              // SPP members); these location will end up in mbr
              // In each iteration, we produce 16 bits because cwd can have
              // up to 16 bits of significance information, followed by the
              // corresponding 16 bits of sign information; therefore, it is
              // sufficient to fetch 32 bit data per loop.

              // Althougth we are interested in 16 bits only, we load 32 bits.
              // For the 16 bits we are producing, we need the next 4 bits --
              // We need data for at least 5 columns out of 8.
              // Therefore loading 32 bits is easier than loading 16 bits
              // twice.
              ui32 ps, ns, cs;
              memcpy(&ps, prev_sig, 4);
              memcpy(&ns, cur_sig + mstr, 4);
              ui32 u = (ps & 0x88888888) >> 3; // the row on top
              if (!stripe_causal)
                u |= (ns & 0x11111111) << 3;   // the row below

              memcpy(&cs, cur_sig, 4);
              // vertical integration
              ui32 mbr =  cs;                // this sig. info.
              mbr |= (cs & 0x77777777) << 1; //above neighbors
              mbr |= (cs & 0xEEEEEEEE) >> 1; //below neighbors
              mbr |= u;
              // horizontal integration
              ui32 t = mbr;
              mbr |= t << 4;      // neighbors on the left
              mbr |= t >> 4;      // neighbors on the right
              mbr |= prev >> 12;  // significance of previous group

              // remove outside samples, and already significant samples
              mbr &= pattern;
              mbr &= ~cs;

              // find samples that become significant during the SPP
              ui32 new_sig = mbr;
              if (new_sig)
              {
                ui64 cwd = dfetch64(spp_buf, spp_limit, spp_pos);

                ui32 cnt = 0;
                ui32 col_mask = 0xFu;
                ui32 inv_sig = ~cs & pattern;
                for (int i = 0; i < 16; i += 4, col_mask <<= 4)
                {
                  if ((col_mask & new_sig) == 0)
                    continue;

                  //scan one column
                  ui32 sample_mask = 0x1111u & col_mask;
                  if (new_sig & sample_mask)
                  {
                    new_sig &= ~sample_mask;
                    if (cwd & 1)
                    {
                      ui32 t = 0x33u << i;
                      new_sig |= t & inv_sig;
                    }
                    cwd >>= 1; ++cnt;
                  }

                  sample_mask <<= 1;
                  if (new_sig & sample_mask)
                  {
                    new_sig &= ~sample_mask;
                    if (cwd & 1)
                    {
                      ui32 t = 0x76u << i;
                      new_sig |= t & inv_sig;
                    }
                    cwd >>= 1; ++cnt;
                  }

                  sample_mask <<= 1;
                  if (new_sig & sample_mask)
                  {
                    new_sig &= ~sample_mask;
                    if (cwd & 1)
                    {
                      ui32 t = 0xECu << i;
                      new_sig |= t & inv_sig;
                    }
                    cwd >>= 1; ++cnt;
                  }

                  sample_mask <<= 1;
                  if (new_sig & sample_mask)
                  {
                    new_sig &= ~sample_mask;
                    if (cwd & 1)
                    {
                      ui32 t = 0xC8u << i;
                      new_sig |= t & inv_sig;
                    }
                    cwd >>= 1; ++cnt;
                  }
                }

                if (new_sig)
                {
                  // the sign bits sit right after the cnt consumed bits
                  // of cwd; no reassembly is needed

                  // Spread new_sig, such that each bit is in one byte with a
                  // value of 0 if new_sig bit is 0, and 0xFF if new_sig is 1
                  __m128i new_sig_vec = _mm_set1_epi16((si16)new_sig);
                  new_sig_vec = _mm_shuffle_epi8(new_sig_vec,
                    _mm_set_epi8(1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0));
                  new_sig_vec = _mm_and_si128(new_sig_vec,
                    _mm_set1_epi64x((si64)0x8040201008040201));
                  new_sig_vec = _mm_cmpeq_epi8(new_sig_vec,
                    _mm_set1_epi64x((si64)0x8040201008040201));

                  // find cumulative sums
                  // to find which bit in cwd we should extract
                  __m128i inc_sum = new_sig_vec; // inclusive scan
                  inc_sum = _mm_abs_epi8(inc_sum); // cvrt to 0 or 1
                  inc_sum = _mm_add_epi8(inc_sum, _mm_bslli_si128(inc_sum, 1));
                  inc_sum = _mm_add_epi8(inc_sum, _mm_bslli_si128(inc_sum, 2));
                  inc_sum = _mm_add_epi8(inc_sum, _mm_bslli_si128(inc_sum, 4));
                  inc_sum = _mm_add_epi8(inc_sum, _mm_bslli_si128(inc_sum, 8));
                  cnt += (ui32)_mm_extract_epi16(inc_sum, 7) >> 8;
                  // exclusive scan
                  __m128i ex_sum = _mm_bslli_si128(inc_sum, 1);

                  // Spread cwd, such that each bit is in one byte
                  // with a value of 0 or 1.
                  __m128i cwd_vec = _mm_set1_epi16((si16)cwd);
                  cwd_vec = _mm_shuffle_epi8(cwd_vec,
                    _mm_set_epi8(1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0));
                  cwd_vec = _mm_and_si128(cwd_vec,
                    _mm_set1_epi64x((si64)0x8040201008040201));
                  cwd_vec = _mm_cmpeq_epi8(cwd_vec,
                    _mm_set1_epi64x((si64)0x8040201008040201));
                  cwd_vec = _mm_abs_epi8(cwd_vec);

                  // Obtain bit from cwd_vec correspondig to ex_sum
                  // Basically, collect needed bits from cwd_vec
                  __m128i v = _mm_shuffle_epi8(cwd_vec, ex_sum);

                  // load data and set spp coefficients
                  __m128i m =
                    _mm_set_epi8(-1,-1,-1,12,-1,-1,-1,8,-1,-1,-1,4,-1,-1,-1,0);
                  __m128i val = _mm_set1_epi32(3 << (p - 2));
                  ui32 *dp = dpp;
                  for (int c = 0; c < 4; ++ c) {
                    __m128i s0, s0_ns, s0_val;
                    // load coefficients
                    s0 = _mm_load_si128((__m128i*)dp);

                    // epi32 is -1 only for coefficient that
                    // are changed during the SPP
                    s0_ns = _mm_shuffle_epi8(new_sig_vec, m);
                    s0_ns = _mm_cmpeq_epi32(s0_ns, _mm_set1_epi32(0xFF));

                    // obtain sign for coefficients in SPP
                    s0_val = _mm_shuffle_epi8(v, m);
                    s0_val = _mm_slli_epi32(s0_val, 31);
                    s0_val = _mm_or_si128(s0_val, val);
                    s0_val = _mm_and_si128(s0_val, s0_ns);

                    // update vector
                    s0 = _mm_or_si128(s0, s0_val);
                    // store coefficients
                    _mm_store_si128((__m128i*)dp, s0);
                    // prepare for next row
                    dp += stride;
                    m = _mm_add_epi32(m, _mm_set1_epi32(1));
                  }
                }
                spp_pos += cnt;
              }

              new_sig |= cs;
              *prev_sig = (ui16)(new_sig);

              // vertical integration for the new sig. info.
              t = new_sig;
              new_sig |= (t & 0x7777) << 1; //above neighbors
              new_sig |= (t & 0xEEEE) >> 1; //below neighbors
              // add sig. info. from the row on top and below
              prev = new_sig | u;
              // we need only the bits in 0xF000
              prev &= 0xF000;
            }
          }
        }

        // We perform Magnitude Refinement Pass here
        if (num_passes > 2)
        {
          // de-stuff the MRP segment; consumption is at most 1 bit per
          // sample = 512 bytes, so 1024 bytes always suffice
          const ui32 mrp_cap = 1024;
          ui8 mrp_buf[mrp_cap + 72];
          ui32 mrp_limit = destuff_mrp(coded_data, (int)lengths1,
                                       (int)lengths2, mrp_buf, mrp_cap);
          ui32 mrp_pos = 0;

          for (ui32 y = 0; y < height; y += 4)
          {
            ui16 *cur_sig = sigma + (y >> 2) * mstr;
            ui32 *dpp = decoded_data + y * stride;
            for (ui32 i = 0; i < width; i += 4, dpp += 4)
            {
              //Process one entry from sigma array at a time
              // Each nibble (4 bits) in the sigma array represents 4 rows,
              ui64 cwd = dfetch64(mrp_buf, mrp_limit, mrp_pos);
              ui16 sig = *cur_sig++; // 16 bit that will be processed now
              int total_bits = 0;
              if (sig) // if any of the 32 bits are set
              {
                // We work on 4 rows, with 4 samples each, since
                // data is 32 bit (4 bytes)

                // spread the 16 bits in sig to 0 or 1 bytes in sig_vec
                __m128i sig_vec = _mm_set1_epi16((si16)sig);
                sig_vec = _mm_shuffle_epi8(sig_vec,
                  _mm_set_epi8(1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0));
                sig_vec = _mm_and_si128(sig_vec,
                  _mm_set1_epi64x((si64)0x8040201008040201));
                sig_vec = _mm_cmpeq_epi8(sig_vec,
                  _mm_set1_epi64x((si64)0x8040201008040201));
                sig_vec = _mm_abs_epi8(sig_vec);

                // find cumulative sums
                // to find which bit in cwd we should extract
                __m128i inc_sum = sig_vec; // inclusive scan
                inc_sum = _mm_add_epi8(inc_sum, _mm_bslli_si128(inc_sum, 1));
                inc_sum = _mm_add_epi8(inc_sum, _mm_bslli_si128(inc_sum, 2));
                inc_sum = _mm_add_epi8(inc_sum, _mm_bslli_si128(inc_sum, 4));
                inc_sum = _mm_add_epi8(inc_sum, _mm_bslli_si128(inc_sum, 8));
                total_bits = _mm_extract_epi16(inc_sum, 7) >> 8;
                __m128i ex_sum = _mm_bslli_si128(inc_sum, 1); // exclusive scan

                // Spread the 16 bits in cwd to inverted 0 or 1 bytes in
                // cwd_vec. Then, convert these to a form suitable
                // for coefficient modifications; in particular, a value
                // of 0 is presented as binary 11, and a value of 1 is
                // represented as binary 01
                __m128i cwd_vec = _mm_set1_epi16((si16)cwd);
                cwd_vec = _mm_shuffle_epi8(cwd_vec,
                  _mm_set_epi8(1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0));
                cwd_vec = _mm_and_si128(cwd_vec,
                  _mm_set1_epi64x((si64)0x8040201008040201));
                cwd_vec = _mm_cmpeq_epi8(cwd_vec,
                  _mm_set1_epi64x((si64)0x8040201008040201));
                cwd_vec = _mm_add_epi8(cwd_vec, _mm_set1_epi8(1));
                cwd_vec = _mm_add_epi8(cwd_vec, cwd_vec);
                cwd_vec = _mm_or_si128(cwd_vec, _mm_set1_epi8(1));

                // load data and insert the mrp bit
                __m128i m =
                  _mm_set_epi8(-1,-1,-1,12,-1,-1,-1,8,-1,-1,-1,4,-1,-1,-1,0);
                ui32 *dp = dpp;
                for (int c = 0; c < 4; ++c) {
                  __m128i s0, s0_sig, s0_idx, s0_val;
                  // load coefficients
                  s0 = _mm_load_si128((__m128i*)dp);
                  // find significant samples in this row
                  s0_sig = _mm_shuffle_epi8(sig_vec, m);
                  s0_sig = _mm_cmpeq_epi8(s0_sig, _mm_setzero_si128());
                  // get MRP bit index, and MRP pattern
                  s0_idx = _mm_shuffle_epi8(ex_sum, m);
                  s0_val = _mm_shuffle_epi8(cwd_vec, s0_idx);
                  // keep data from significant samples only
                  s0_val = _mm_andnot_si128(s0_sig, s0_val);
                  // move mrp bits to correct position, and employ
                  s0_val = _mm_slli_epi32(s0_val, (si32)p - 2);
                  s0 = _mm_xor_si128(s0, s0_val);
                  // store coefficients
                  _mm_store_si128((__m128i*)dp, s0);
                  // prepare for next row
                  dp += stride;
                  m = _mm_add_epi32(m, _mm_set1_epi32(1));
                }
              }
              // consume data according to the number of bits set
              mrp_pos += (ui32)total_bits;
            }
          }
        }
    }

    //************************************************************************/
    /** @brief Step-1: decode the VLC and MEL segments into the scratch buffer.
     *
     *  Outlined so the serial scalar VLC/MEL chain gets its own register
     *  allocation, isolated from the step-2 / SPP-MRP code paths.
     */
    OJPH_NO_INLINE
    void decode_cb_step1_vlc(ui16* scratch, ui8* coded_data, int lcup,
                             int scup, ui32 width, ui32 height, ui32 sstr)
    {
        // init structures
        dec_mel_st mel;
        mel_init(&mel, coded_data, lcup, scup);

        // de-stuff the VLC segment; its size is at most scup - 1 < 4095
        // bytes (scup is a 12-bit value), and consumption per quad pair is
        // at most 7 + 7 + 30 bits, so 4096 bytes always suffice
        const ui32 vlc_cap = 4096;
        ui8 vlc_buf[vlc_cap + 72];
        ui32 vlc_limit = destuff_vlc(coded_data, lcup, scup,
                                     vlc_buf, vlc_cap);
        ui32 vlc_off = 0;
        ui32 vlc_bits = 0;

        int run = mel_get_run(&mel); // decode runs of events from MEL bitstrm
                                     // data represented as runs of 0 events
                                     // See mel_decode description

        ui64 vlc_val = 0;
        ui32 c_q = 0;
        ui16 *sp = scratch;
        //initial quad row
        for (ui32 x = 0; x < width; sp += 4)
        {
          // decode VLC
          /////////////

          // first quad
          drefill(vlc_val, vlc_bits, vlc_off, vlc_buf, vlc_limit);

          //decode VLC using the context c_q and the head of VLC bitstream
          ui16 t0 = vlc_tbl0[ c_q + (vlc_val & 0x7F) ];

          // if context is zero, use one MEL event
          if (c_q == 0) //zero context
          {
            run -= 2; //subtract 2, since events number if multiplied by 2

            // Is the run terminated in 1? if so, use decoded VLC code,
            // otherwise, discard decoded data, since we will decoded again
            // using a different context
            t0 = (run == -1) ? t0 : 0;

            // is run -1 or -2? this means a run has been consumed
            if (run < 0)
              run = mel_get_run(&mel);  // get another run
          }
          //run -= (c_q == 0) ? 2 : 0;
          //t0 = (c_q != 0 || run == -1) ? t0 : 0;
          //if (run < 0)
          //  run = mel_get_run(&mel);  // get another run
          sp[0] = t0;
          x += 2;

          // prepare context for the next quad; eqn. 1 in ITU T.814
          c_q = ((t0 & 0x10U) << 3) | ((t0 & 0xE0U) << 2);

          //remove data from vlc stream (0 bits are removed if vlc is not used)
          dconsume(vlc_val, vlc_bits, t0 & 0x7u);

          //second quad
          ui16 t1 = 0;

          //decode VLC using the context c_q and the head of VLC bitstream
          t1 = vlc_tbl0[c_q + (vlc_val & 0x7F)];

          // if context is zero, use one MEL event
          if (c_q == 0 && x < width) //zero context
          {
            run -= 2; //subtract 2, since events number if multiplied by 2

            // if event is 0, discard decoded t1
            t1 = (run == -1) ? t1 : 0;

            if (run < 0) // have we consumed all events in a run
              run = mel_get_run(&mel); // if yes, then get another run
          }
          t1 = x < width ? t1 : 0;
          //run -= (c_q == 0 && x < width) ? 2 : 0;
          //t1 = (c_q != 0 || run == -1) ? t1 : 0;
          //if (run < 0)
          //  run = mel_get_run(&mel);  // get another run
          sp[2] = t1;
          x += 2;

          //prepare context for the next quad, eqn. 1 in ITU T.814
          c_q = ((t1 & 0x10U) << 3) | ((t1 & 0xE0U) << 2);

          //remove data from vlc stream, if qinf is not used, cwdlen is 0
          dconsume(vlc_val, vlc_bits, t1 & 0x7u);

          // decode u
          /////////////
          // uvlc_mode is made up of u_offset bits from the quad pair
          ui32 uvlc_mode = ((t0 & 0x8U) << 3) | ((t1 & 0x8U) << 4);
          if (uvlc_mode == 0xc0)// if both u_offset are set, get an event from
          {                     // the MEL run of events
            run -= 2; //subtract 2, since events number if multiplied by 2

            uvlc_mode += (run == -1) ? 0x40 : 0; // increment uvlc_mode by
                                                 // is 0x40

            if (run < 0)//if run is consumed (run is -1 or -2), get another run
              run = mel_get_run(&mel);
          }
          //run -= (uvlc_mode == 0xc0) ? 2 : 0;
          //uvlc_mode += (uvlc_mode == 0xc0 && run == -1) ? 0x40 : 0;
          //if (run < 0)
          //  run = mel_get_run(&mel);  // get another run

          //decode uvlc_mode to get u for both quads
          ui32 uvlc_entry = uvlc_tbl0[uvlc_mode + (vlc_val & 0x3F)];
          //remove total prefix length
          dconsume(vlc_val, vlc_bits, uvlc_entry & 0x7u);
          uvlc_entry >>= 3;
          //extract suffixes for quad 0 and 1
          ui32 len = uvlc_entry & 0xF;           //suffix length for 2 quads
          ui32 tmp = (ui32)vlc_val & ((1u << len) - 1); //suffix value, 2 quads
          dconsume(vlc_val, vlc_bits, len);
          uvlc_entry >>= 4;
          // quad 0 length
          len = uvlc_entry & 0x7; // quad 0 suffix length
          uvlc_entry >>= 3;
          ui16 u_q = (ui16)(1 + (uvlc_entry&7) + (tmp&~(0xFFU<<len))); //kap. 1
          sp[1] = u_q;
          u_q = (ui16)(1 + (uvlc_entry >> 3) + (tmp >> len));  //kappa == 1
          sp[3] = u_q;
        }
        sp[0] = sp[1] = 0;

        //non initial quad rows
        for (ui32 y = 2; y < height; y += 2)
        {
          c_q = 0;                                // context
          ui16 *sp = scratch + (y >> 1) * sstr;   // this row of quads

          for (ui32 x = 0; x < width; sp += 4)
          {
            // decode VLC
            /////////////

            // sigma_q (n, ne, nf)
            c_q |= ((sp[0 - (si32)sstr] & 0xA0U) << 2);
            c_q |= ((sp[2 - (si32)sstr] & 0x20U) << 4);

            // first quad
            drefill(vlc_val, vlc_bits, vlc_off, vlc_buf, vlc_limit);

            //decode VLC using the context c_q and the head of VLC bitstream
            ui16 t0 = vlc_tbl1[ c_q + (vlc_val & 0x7F) ];

            // if context is zero, use one MEL event
            if (c_q == 0) //zero context
            {
              run -= 2; //subtract 2, since events number is multiplied by 2

              // Is the run terminated in 1? if so, use decoded VLC code,
              // otherwise, discard decoded data, since we will decoded again
              // using a different context
              t0 = (run == -1) ? t0 : 0;

              // is run -1 or -2? this means a run has been consumed
              if (run < 0)
                run = mel_get_run(&mel);  // get another run
            }
            //run -= (c_q == 0) ? 2 : 0;
            //t0 = (c_q != 0 || run == -1) ? t0 : 0;
            //if (run < 0)
            //  run = mel_get_run(&mel);  // get another run
            sp[0] = t0;
            x += 2;

            // prepare context for the next quad; eqn. 2 in ITU T.814
            // sigma_q (w, sw)
            c_q = ((t0 & 0x40U) << 2) | ((t0 & 0x80U) << 1);
            // sigma_q (nw)
            c_q |= sp[0 - (si32)sstr] & 0x80;
            // sigma_q (n, ne, nf)
            c_q |= ((sp[2 - (si32)sstr] & 0xA0U) << 2);
            c_q |= ((sp[4 - (si32)sstr] & 0x20U) << 4);

            //remove data from vlc stream (0 bits are removed if vlc is unused)
            dconsume(vlc_val, vlc_bits, t0 & 0x7u);

            //second quad
            ui16 t1 = 0;

            //decode VLC using the context c_q and the head of VLC bitstream
            t1 = vlc_tbl1[ c_q + (vlc_val & 0x7F)];

            // if context is zero, use one MEL event
            if (c_q == 0 && x < width) //zero context
            {
              run -= 2; //subtract 2, since events number if multiplied by 2

              // if event is 0, discard decoded t1
              t1 = (run == -1) ? t1 : 0;

              if (run < 0) // have we consumed all events in a run
                run = mel_get_run(&mel); // if yes, then get another run
            }
            t1 = x < width ? t1 : 0;
            //run -= (c_q == 0 && x < width) ? 2 : 0;
            //t1 = (c_q != 0 || run == -1) ? t1 : 0;
            //if (run < 0)
            //  run = mel_get_run(&mel);  // get another run
            sp[2] = t1;
            x += 2;

            // partial c_q, will be completed when we process the next quad
            // sigma_q (w, sw)
            c_q = ((t1 & 0x40U) << 2) | ((t1 & 0x80U) << 1);
            // sigma_q (nw)
            c_q |= sp[2 - (si32)sstr] & 0x80;

            //remove data from vlc stream, if qinf is not used, cwdlen is 0
            dconsume(vlc_val, vlc_bits, t1 & 0x7u);

            // decode u using wide UVLC table
            /////////////
            ui32 uvlc_mode = (((t0 >> 3) & 1u) | (((t1 >> 3) & 1u) << 1));
            ui32 uvlc_entry =
              uvlc_tbl1_wide[(uvlc_mode << 10) | (vlc_val & 0x3FF)];
            ui32 total_bits = uvlc_entry & 0x1F;
            if (total_bits < 0x1F) {
              sp[1] = (ui16)((uvlc_entry >> 5) & 0xFF);
              sp[3] = (ui16)((uvlc_entry >> 13) & 0xFF);
              dconsume(vlc_val, vlc_bits, total_bits);
            } else {
              uvlc_mode = ((t0 & 0x8U) << 3) | ((t1 & 0x8U) << 4);
              uvlc_entry = uvlc_tbl1[uvlc_mode + (vlc_val & 0x3F)];
              dconsume(vlc_val, vlc_bits, uvlc_entry & 0x7u);
              uvlc_entry >>= 3;
              ui32 len = uvlc_entry & 0xF;
              ui32 tmp = (ui32)vlc_val & ((1u << len) - 1);
              dconsume(vlc_val, vlc_bits, len);
              uvlc_entry >>= 4;
              len = uvlc_entry & 0x7;
              uvlc_entry >>= 3;
              sp[1] = (ui16)((uvlc_entry & 7) + (tmp & ~(0xFFU << len)));
              sp[3] = (ui16)((uvlc_entry >> 3) + (tmp >> len));
            }
          }
          sp[0] = sp[1] = 0;
        }
    }

    bool ojph_decode_codeblock_avx2(ui8* coded_data, ui32* decoded_data,
                                    ui32 missing_msbs, ui32 num_passes,
                                    ui32 lengths1, ui32 lengths2,
                                    ui32 width, ui32 height, ui32 stride,
                                    bool stripe_causal)
    {
      static bool insufficient_precision = false;
      static bool modify_code = false;
      static bool truncate_spp_mrp = false;

      if (num_passes > 1 && lengths2 == 0)
      {
        OJPH_WARN(0x00010001, "A malformed codeblock that has more than "
                              "one coding pass, but zero length for "
                              "2nd and potential 3rd pass.");
        num_passes = 1;
      }

      if (num_passes > 3)
      {
        OJPH_WARN(0x00010002, "We do not support more than 3 coding passes; "
                              "This codeblocks has %d passes.",
                              num_passes);
        return false;
      }

      if (missing_msbs > 30) // p < 0
      {
        if (insufficient_precision == false)
        {
          insufficient_precision = true;
          OJPH_WARN(0x00010003, "32 bits are not enough to decode this "
                                "codeblock. This message will not be "
                                "displayed again.");
        }
        return false;
      }
      else if (missing_msbs == 30) // p == 0
      { // not enough precision to decode and set the bin center to 1
        if (modify_code == false) {
          modify_code = true;
          OJPH_WARN(0x00010004, "Not enough precision to decode the cleanup "
                                "pass. The code can be modified to support "
                                "this case. This message will not be "
                                "displayed again.");
        }
         return false;         // 32 bits are not enough to decode this
       }
      else if (missing_msbs == 29) // if p is 1, then num_passes must be 1
      {
        if (num_passes > 1) {
          num_passes = 1;
          if (truncate_spp_mrp == false) {
            truncate_spp_mrp = true;
            OJPH_WARN(0x00010005, "Not enough precision to decode the SgnProp "
                                  "nor MagRef passes; both will be skipped. "
                                  "This message will not be displayed "
                                  "again.");
          }
        }
      }
      ui32 p = 30 - missing_msbs; // The least significant bitplane for CUP
      // There is a way to handle the case of p == 0, but a different path
      // is required

      if (lengths1 < 2)
      {
        OJPH_WARN(0x00010006, "Wrong codeblock length.");
        return false;
      }

      // read scup and fix the bytes there
      int lcup, scup;
      lcup = (int)lengths1;  // length of CUP
      //scup is the length of MEL + VLC
      scup = (((int)coded_data[lcup-1]) << 4) + (coded_data[lcup-2] & 0xF);
      if (scup < 2 || scup > lcup || scup > 4079) //something is wrong
        return false;

      // The temporary storage scratch holds two types of data in an
      // interleaved fashion. The interleaving allows us to use one
      // memory pointer.
      // We have one entry for a decoded VLC code, and one entry for UVLC.
      // Entries are 16 bits each, corresponding to one quad,
      // but since we want to use XMM registers of the SSE family
      // of SIMD; we allocated 16 bytes or more per quad row; that is,
      // the width is no smaller than 16 bytes (or 8 entries), and the
      // height is 512 quads
      // Each VLC entry contains, in the following order, starting
      // from MSB
      // e_k (4bits), e_1 (4bits), rho (4bits), useless for step 2 (4bits)
      // Each entry in UVLC contains u_q
      // One extra row to handle the case of SPP propagating downwards
      // when codeblock width is 4
      // We need an extra two entries (one inf and one u_q) beyond
      // the last column.
      // If the block width is 4 (2 quads), then we use sstr of 8
      // (enough for 4 quads). If width is 8 (4 quads) we use
      // sstr is 16 (enough for 8 quads). For a width of 16 (8
      // quads), we use 24 (enough for 12 quads).
      ui32 sstr = ((width + 2u) + 7u) & ~7u; // multiples of 8

#ifdef __MINGW64__
      ui16 scratch[8 * 513] = {0};
#else
      ui16 scratch[8 * 513];
      ui32 quad_rows = (height + 1u) >> 1;
      size_t scratch_zero = (size_t)(quad_rows + 1) * sstr;
      if (scratch_zero > 8 * 513) scratch_zero = 8 * 513;
      memset(scratch, 0, scratch_zero * sizeof(ui16));
#endif

      assert((stride & 0x3) == 0);

      ui32 mmsbp2 = missing_msbs + 2;

      // The cleanup pass is decoded in two steps; in step one,
      // the VLC and MEL segments are decoded, generating a record that
      // has 2 bytes per quad. The 2 bytes contain, u, rho, e^1 & e^k.
      // This information should be sufficient for the next step.
      // In step 2, we decode the MagSgn segment.

      // step 1: decode VLC and MEL segments into scratch
      decode_cb_step1_vlc(scratch, coded_data, lcup, scup, width, height, sstr);

      // step2 we decode magsgn
      // mmsbp2 equals K_max + 1 (we decode up to K_max bits + 1 sign bit)
      // The 32 bit path decode 16 bits data, for which one would think
      // 16 bits are enough, because we want to put in the center of the
      // bin.
      // If you have mmsbp2 equals 16 bit, and reversible coding, and
      // no bitplanes are missing, then we can decoding using the 16 bit
      // path, but we are not doing this here.
      if (mmsbp2 >= 16)
      {
        if (!decode_cb_step2_32bit(scratch, decoded_data, coded_data,
                                   width, height, stride, sstr, p, mmsbp2,
                                   lcup, scup))
          return false;
      }
      else {
        if (!decode_cb_step2_16bit(scratch, decoded_data, coded_data,
                                   width, height, stride, sstr, p, mmsbp2,
                                   lcup, scup))
          return false;
      }

      if (num_passes > 1)
        decode_cb_spp_mrp(scratch, decoded_data, coded_data, width, height,
                          stride, sstr, p, num_passes, lengths1, lengths2,
                          stripe_causal);

      return true;
    }
  }
}

#endif
