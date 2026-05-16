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
// File: ojph_block_decoder_wasm.cpp
// Author: Aous Naman
// Date: 13 May 2022
//***************************************************************************/

//***************************************************************************/
/** @file ojph_block_decoder_wasm.cpp
 *  @brief implements a faster HTJ2K block decoder using wasm simd
 */

#include <string>
#include <iostream>

#include <cassert>
#include <cstring>
#include "ojph_block_common.h"
#include "ojph_block_decoder.h"
#include "ojph_arch.h"
#include "ojph_message.h"

#include <wasm_simd128.h>

namespace ojph {
  namespace local {

    //************************************************************************/
    /** @brief Macros that help with typing and space
     */
    #define OJPH_REPEAT2(a) a,a
    #define OJPH_REPEAT4(a) a,a,a,a
    #define OJPH_REPEAT8(a) a,a,a,a,a,a,a,a
    #define OJPH_REPEAT16(a) a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a

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
        val = *(ui32*)melp->data;  // read 32 bits from MEL data
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
    /** @brief A structure for reading and unstuffing a segment that grows
     *         backward, such as VLC and MRP
     */ 
    struct rev_struct {
      rev_struct() : data(NULL), tmp(0), bits(0), size(0), unstuff(false)
      {}
      //storage
      ui8* data;     //!<pointer to where to read data
      ui64 tmp;	     //!<temporary buffer of read data
      ui32 bits;     //!<number of bits stored in tmp
      int size;      //!<number of bytes left
      bool unstuff;  //!<true if the last byte is more than 0x8F
                     //!<then the current byte is unstuffed if it is 0x7F
    };

    //************************************************************************/
    /** @brief Read and unstuff data from a backwardly-growing segment
     *
     *  This reader can read up to 8 bytes from before the VLC segment.
     *  Care must be taken not read from unreadable memory, causing a 
     *  segmentation fault.
     * 
     *  Note that there is another subroutine rev_read_mrp that is slightly
     *  different.  The other one fills zeros when the buffer is exhausted.
     *  This one basically does not care if the bytes are consumed, because
     *  any extra data should not be used in the actual decoding.
     *
     *  Unstuffing is needed to prevent sequences more than 0xFF8F from 
     *  appearing in the bits stream; since we are reading backward, we keep
     *  watch when a value larger than 0x8F appears in the bitstream. 
     *  If the byte following this is 0x7F, we unstuff this byte (ignore the 
     *  MSB of that byte, which should be 0).
     *
     *  @param [in]  vlcp is a pointer to rev_struct structure
     */
    static inline 
    void rev_read(rev_struct *vlcp)
    {
      //process 4 bytes at a time
      if (vlcp->bits > 32)  // if there are more than 32 bits in tmp, then 
        return;             // reading 32 bits can overflow vlcp->tmp
      ui32 val = 0;
      //the next line (the if statement) needs to be tested first
      if (vlcp->size > 3)  // if there are more than 3 bytes left in VLC
      {
        // (vlcp->data - 3) move pointer back to read 32 bits at once
        val = *(ui32*)(vlcp->data - 3); // then read 32 bits
        vlcp->data -= 4;          // move data pointer back by 4
        vlcp->size -= 4;          // reduce available byte by 4
      }
      else if (vlcp->size > 0)
      { // 4 or less
        int i = 24;
        while (vlcp->size > 0) {   
          ui32 v = *vlcp->data--; // read one byte at a time
          val |= (v << i);        // put byte in its correct location
          --vlcp->size;
          i -= 8;
        }
      }

      //accumulate in tmp, number of bits in tmp are stored in bits
      ui32 tmp = val >> 24;  //start with the MSB byte
      ui32 bits;

      // test unstuff (previous byte is >0x8F), and this byte is 0x7F
      bits = 8 - ((vlcp->unstuff && (((val >> 24) & 0x7F) == 0x7F)) ? 1 : 0);
      bool unstuff = (val >> 24) > 0x8F; //this is for the next byte

      tmp |= ((val >> 16) & 0xFF) << bits; //process the next byte
      bits += 8 - ((unstuff && (((val >> 16) & 0x7F) == 0x7F)) ? 1 : 0);
      unstuff = ((val >> 16) & 0xFF) > 0x8F;

      tmp |= ((val >> 8) & 0xFF) << bits;
      bits += 8 - ((unstuff && (((val >> 8) & 0x7F) == 0x7F)) ? 1 : 0);
      unstuff = ((val >> 8) & 0xFF) > 0x8F;

      tmp |= (val & 0xFF) << bits;
      bits += 8 - ((unstuff && ((val & 0x7F) == 0x7F)) ? 1 : 0);
      unstuff = (val & 0xFF) > 0x8F;

      // now move the read and unstuffed bits into vlcp->tmp
      vlcp->tmp |= (ui64)tmp << vlcp->bits;
      vlcp->bits += bits;
      vlcp->unstuff = unstuff; // this for the next read
    }

    //************************************************************************/
    /** @brief Initiates the rev_struct structure and reads a few bytes to 
     *         move the read address to multiple of 4
     *
     *  There is another similar rev_init_mrp subroutine.  The difference is
     *  that this one, rev_init, discards the first 12 bits (they have the
     *  sum of the lengths of VLC and MEL segments), and first unstuff depends
     *  on first 4 bits.
     *
     *  @param [in]  vlcp is a pointer to rev_struct structure
     *  @param [in]  data is a pointer to byte at the start of the cleanup pass
     *  @param [in]  lcup is the length of MagSgn+MEL+VLC segments
     *  @param [in]  scup is the length of MEL+VLC segments
     */
    static inline 
    void rev_init(rev_struct *vlcp, ui8* data, int lcup, int scup)
    {
      //first byte has only the upper 4 bits
      vlcp->data = data + lcup - 2;

      //size can not be larger than this, in fact it should be smaller
      vlcp->size = scup - 2;

      ui32 d = *vlcp->data--; // read one byte (this is a half byte)
      vlcp->tmp = d >> 4;    // both initialize and set
      vlcp->bits = 4 - ((vlcp->tmp & 7) == 7); //check standard
      vlcp->unstuff = (d | 0xF) > 0x8F; //this is useful for the next byte

      //This code is designed for an architecture that read address should
      // align to the read size (address multiple of 4 if read size is 4)
      //These few lines take care of the case where data is not at a multiple
      // of 4 boundary. It reads 1,2,3 up to 4 bytes from the VLC bitstream.
      // To read 32 bits, read from (vlcp->data - 3)
      int num = 1 + (int)(intptr_t(vlcp->data) & 0x3);
      int tnum = num < vlcp->size ? num : vlcp->size;
      for (int i = 0; i < tnum; ++i) {
        ui64 d;
        d = *vlcp->data--;  // read one byte and move read pointer
        //check if the last byte was >0x8F (unstuff == true) and this is 0x7F
        ui32 d_bits = 8 - ((vlcp->unstuff && ((d & 0x7F) == 0x7F)) ? 1 : 0);
        vlcp->tmp |= d << vlcp->bits; // move data to vlcp->tmp
        vlcp->bits += d_bits;
        vlcp->unstuff = d > 0x8F; // for next byte
      }
      vlcp->size -= tnum;
      rev_read(vlcp);  // read another 32 buts
    }

    //************************************************************************/
    /** @brief Retrieves 32 bits from the head of a rev_struct structure 
     *
     *  By the end of this call, vlcp->tmp must have no less than 33 bits
     *
     *  @param [in]  vlcp is a pointer to rev_struct structure
     */
    static inline 
    ui32 rev_fetch(rev_struct *vlcp)
    {
      if (vlcp->bits < 32)  // if there are less then 32 bits, read more
      {
        rev_read(vlcp);     // read 32 bits, but unstuffing might reduce this
        if (vlcp->bits < 32)// if there is still space in vlcp->tmp for 32 bits
          rev_read(vlcp);   // read another 32
      }
      return (ui32)vlcp->tmp; // return the head (bottom-most) of vlcp->tmp
    }

    //************************************************************************/
    /** @brief Consumes num_bits from a rev_struct structure
     *
     *  @param [in]  vlcp is a pointer to rev_struct structure
     *  @param [in]  num_bits is the number of bits to be removed
     */
    static inline 
    ui32 rev_advance(rev_struct *vlcp, ui32 num_bits)
    {
      assert(num_bits <= vlcp->bits); // vlcp->tmp must have more than num_bits
      vlcp->tmp >>= num_bits;         // remove bits
      vlcp->bits -= num_bits;         // decrement the number of bits
      return (ui32)vlcp->tmp;
    }

    //************************************************************************/
    /** @brief Reads and unstuffs from rev_struct
     *
     *  This is different than rev_read in that this fills in zeros when the
     *  the available data is consumed.  The other does not care about the
     *  values when all data is consumed.
     *
     *  See rev_read for more information about unstuffing
     *
     *  @param [in]  mrp is a pointer to rev_struct structure
     */
    static inline 
    void rev_read_mrp(rev_struct *mrp)
    {
      //process 4 bytes at a time
      if (mrp->bits > 32)
        return;
      ui32 val = 0;
      if (mrp->size > 3) // If there are 3 byte or more
      { // (mrp->data - 3) move pointer back to read 32 bits at once
        val = *(ui32*)(mrp->data - 3); // read 32 bits
        mrp->data -= 4;                // move back pointer
        mrp->size -= 4;                // reduce count
      }
      else if (mrp->size > 0)
      {
        int i = 24;
        while (mrp->size > 0) {   
          ui32 v = *mrp->data--; // read one byte at a time
          val |= (v << i);       // put byte in its correct location
          --mrp->size;
          i -= 8;
        }
      }

      //accumulate in tmp, and keep count in bits
      ui32 bits, tmp = val >> 24;

      //test if the last byte > 0x8F (unstuff must be true) and this is 0x7F
      bits = 8 - ((mrp->unstuff && (((val >> 24) & 0x7F) == 0x7F)) ? 1 : 0);
      bool unstuff = (val >> 24) > 0x8F;

      //process the next byte
      tmp |= ((val >> 16) & 0xFF) << bits;
      bits += 8 - ((unstuff && (((val >> 16) & 0x7F) == 0x7F)) ? 1 : 0);
      unstuff = ((val >> 16) & 0xFF) > 0x8F;

      tmp |= ((val >> 8) & 0xFF) << bits;
      bits += 8 - ((unstuff && (((val >> 8) & 0x7F) == 0x7F)) ? 1 : 0);
      unstuff = ((val >> 8) & 0xFF) > 0x8F;

      tmp |= (val & 0xFF) << bits;
      bits += 8 - ((unstuff && ((val & 0x7F) == 0x7F)) ? 1 : 0);
      unstuff = (val & 0xFF) > 0x8F;

      mrp->tmp |= (ui64)tmp << mrp->bits; // move data to mrp pointer
      mrp->bits += bits;
      mrp->unstuff = unstuff;             // next byte
    }

    //************************************************************************/
    /** @brief Initialized rev_struct structure for MRP segment, and reads
     *         a number of bytes such that the next 32 bits read are from
     *         an address that is a multiple of 4. Note this is designed for
     *         an architecture that read size must be compatible with the
     *         alignment of the read address
     *
     *  There is another similar subroutine rev_init.  This subroutine does 
     *  NOT skip the first 12 bits, and starts with unstuff set to true.
     *
     *  @param [in]  mrp is a pointer to rev_struct structure
     *  @param [in]  data is a pointer to byte at the start of the cleanup pass
     *  @param [in]  lcup is the length of MagSgn+MEL+VLC segments
     *  @param [in]  len2 is the length of SPP+MRP segments
     */
    static inline 
    void rev_init_mrp(rev_struct *mrp, ui8* data, int lcup, int len2)
    {
      mrp->data = data + lcup + len2 - 1;
      mrp->size = len2;
      mrp->unstuff = true;
      mrp->bits = 0;
      mrp->tmp = 0;

      //This code is designed for an architecture that read address should
      // align to the read size (address multiple of 4 if read size is 4)
      //These few lines take care of the case where data is not at a multiple
      // of 4 boundary.  It reads 1,2,3 up to 4 bytes from the MRP stream
      int num = 1 + (int)(intptr_t(mrp->data) & 0x3);
      for (int i = 0; i < num; ++i) {
        ui64 d;
        //read a byte, 0 if no more data
        d = (mrp->size-- > 0) ? *mrp->data-- : 0; 
        //check if unstuffing is needed
        ui32 d_bits = 8 - ((mrp->unstuff && ((d & 0x7F) == 0x7F)) ? 1 : 0);
        mrp->tmp |= d << mrp->bits; // move data to vlcp->tmp
        mrp->bits += d_bits;
        mrp->unstuff = d > 0x8F; // for next byte
      }
      rev_read_mrp(mrp);
    }

    //************************************************************************/
    /** @brief Retrieves 32 bits from the head of a rev_struct structure 
     *
     *  By the end of this call, mrp->tmp must have no less than 33 bits
     *
     *  @param [in]  mrp is a pointer to rev_struct structure
     */
    static inline 
    ui32 rev_fetch_mrp(rev_struct *mrp)
    {
      if (mrp->bits < 32) // if there are less than 32 bits in mrp->tmp
      {
        rev_read_mrp(mrp);    // read 30-32 bits from mrp
        if (mrp->bits < 32)   // if there is a space of 32 bits
          rev_read_mrp(mrp);  // read more
      }
      return (ui32)mrp->tmp;  // return the head of mrp->tmp
    }

    //************************************************************************/
    /** @brief Consumes num_bits from a rev_struct structure
     *
     *  @param [in]  mrp is a pointer to rev_struct structure
     *  @param [in]  num_bits is the number of bits to be removed
     */
    inline ui32 rev_advance_mrp(rev_struct *mrp, ui32 num_bits)
    {
      assert(num_bits <= mrp->bits); // we must not consume more than mrp->bits
      mrp->tmp >>= num_bits;  // discard the lowest num_bits bits
      mrp->bits -= num_bits;
      return (ui32)mrp->tmp;  // return data after consumption
    }

    //************************************************************************/
    /** @brief State structure for reading and unstuffing of forward-growing 
     *         bitstreams; these are: MagSgn and SPP bitstreams
     */
    struct frwd_struct {
      const ui8* data;  //!<pointer to bitstream
      ui8 tmp[48];      //!<temporary buffer of read data + 16 extra
      ui32 bits;        //!<number of bits stored in tmp
      ui32 unstuff;     //!<1 if a bit needs to be unstuffed from next byte
      int size;         //!<size of data
    };

    //************************************************************************/
    /** @brief Read and unstuffs 16 bytes from forward-growing bitstream
     *  
     *  A template is used to accommodate a different requirement for
     *  MagSgn and SPP bitstreams; in particular, when MagSgn bitstream is
     *  consumed, 0xFF's are fed, while when SPP is exhausted 0's are fed in.
     *  X controls this value.
     *
     *  Unstuffing prevent sequences that are more than 0xFF7F from appearing
     *  in the conpressed sequence.  So whenever a value of 0xFF is coded, the
     *  MSB of the next byte is set 0 and must be ignored during decoding.
     *
     *  Reading can go beyond the end of buffer by up to 16 bytes.
     *
     *  @tparam       X is the value fed in when the bitstream is exhausted
     *  @param  [in]  msp is a pointer to frwd_struct structure
     *
     */
    template<int X>
    static inline 
    void frwd_read(frwd_struct *msp)
    {
      assert(msp->bits <= 128);

      v128_t offset, val, validity, all_xff;
      val = wasm_v128_load(msp->data);
      int bytes = msp->size >= 16 ? 16 : msp->size;
      validity = wasm_i8x16_splat((char)bytes);
      msp->data += bytes;
      msp->size -= bytes;
      ui32 bits = 128;
      offset = wasm_i64x2_const(0x0706050403020100,0x0F0E0D0C0B0A0908);
      validity = wasm_i8x16_gt(validity, offset);
      all_xff = wasm_i8x16_const(OJPH_REPEAT16(-1));
      if (X == 0xFF) // the compiler should remove this if statement
      {
        v128_t t = wasm_v128_xor(validity, all_xff); // complement
        val = wasm_v128_or(t, val); // fill with 0xFF
      }
      else if (X == 0)
        val = wasm_v128_and(validity, val); // fill with zeros 
      else
        assert(0);

      v128_t ff_bytes;
      ff_bytes = wasm_i8x16_eq(val, all_xff);
      ff_bytes = wasm_v128_and(ff_bytes, validity);
      ui32 flags = wasm_i8x16_bitmask(ff_bytes); 
      flags <<= 1; // unstuff following byte
      ui32 next_unstuff = flags >> 16;
      flags |= msp->unstuff;
      flags &= 0xFFFF;
      while (flags) 
      { // bit unstuffing occurs on average once every 256 bytes
        // therefore it is not an issue if it is a bit slow
        // here we process 16 bytes
        --bits; // consuming one stuffing bit

        ui32 loc = 31 - count_leading_zeros(flags);
        flags ^= 1 << loc;

        v128_t m, t, c;
        t = wasm_i8x16_splat((char)loc);
        m = wasm_i8x16_gt(offset, t);

        t = wasm_v128_and(m, val);    // keep bits at locations larger than loc
        c = wasm_u64x2_shr(t, 1);     // 1 bits left
        t = wasm_i64x2_shuffle(t, wasm_i64x2_const(0, 0), 1, 2);
        t = wasm_i64x2_shl(t, 63);    // keep the MSB only
        t = wasm_v128_or(t, c);       // combine the above 3 steps
                                    
        val = wasm_v128_or(t, wasm_v128_andnot(val, m));
      }

      // combine with earlier data
      assert(msp->bits >= 0 && msp->bits <= 128);
      int cur_bytes = msp->bits >> 3;
      ui32 cur_bits = msp->bits & 7;
      v128_t b1, b2;
      b1 = wasm_i64x2_shl(val, cur_bits);
      //next shift 8 bytes right
      b2 = wasm_i64x2_shuffle(wasm_i64x2_const(0, 0), val, 1, 2);
      b2 = wasm_u64x2_shr(b2, 64u - cur_bits);
      b2 = (cur_bits > 0) ? b2 : wasm_i64x2_const(0, 0);
      b1 = wasm_v128_or(b1, b2);
      b2 = wasm_v128_load(msp->tmp + cur_bytes);
      b2 = wasm_v128_or(b1, b2);
      wasm_v128_store(msp->tmp + cur_bytes, b2);

      ui32 consumed_bits = bits < 128u - cur_bits ? bits : 128u - cur_bits;
      cur_bytes = (msp->bits + consumed_bits + 7) >> 3; // round up
      int upper = wasm_u16x8_extract_lane(val, 7);
      upper >>= consumed_bits + 16 - 128;
      msp->tmp[cur_bytes] = (ui8)upper; // copy byte

      msp->bits += bits;
      msp->unstuff = next_unstuff;   // next unstuff
      assert(msp->unstuff == 0 || msp->unstuff == 1);
    }

    //************************************************************************/
    /** @brief Initialize frwd_struct struct and reads some bytes
     *  
     *  @tparam      X is the value fed in when the bitstream is exhausted.
     *               See frwd_read regarding the template
     *  @param [in]  msp is a pointer to frwd_struct
     *  @param [in]  data is a pointer to the start of data
     *  @param [in]  size is the number of byte in the bitstream
     */
    template<int X>
    static inline 
    void frwd_init(frwd_struct *msp, const ui8* data, int size)
    {
      msp->data = data;
      wasm_v128_store(msp->tmp, wasm_i64x2_const(0, 0));
      wasm_v128_store(msp->tmp + 16, wasm_i64x2_const(0, 0));
      wasm_v128_store(msp->tmp + 32, wasm_i64x2_const(0, 0));

      msp->bits = 0;
      msp->unstuff = 0;
      msp->size = size;

      frwd_read<X>(msp); // read 128 bits more
    }

    //************************************************************************/
    /** @brief Consume num_bits bits from the bitstream of frwd_struct
     *
     *  @param [in]  msp is a pointer to frwd_struct
     *  @param [in]  num_bits is the number of bit to consume
     */
    static inline 
    void frwd_advance(frwd_struct *msp, ui32 num_bits)
    {
      assert(num_bits > 0 && num_bits <= msp->bits && num_bits < 128);
      msp->bits -= num_bits;

      v128_t *p = (v128_t*)(msp->tmp + ((num_bits >> 3) & 0x18));
      num_bits &= 63;

      v128_t v0, v1, c0, c1, t;
      v0 = wasm_v128_load(p);
      v1 = wasm_v128_load(p + 1);

      // shift right by num_bits
      c0 = wasm_u64x2_shr(v0, num_bits);
      t = wasm_i64x2_shuffle(v0, wasm_i64x2_const(0, 0), 1, 2);
      t = wasm_i64x2_shl(t, 64 - num_bits);
      t = (num_bits > 0) ? t : wasm_i64x2_const(0, 0);
      c0 = wasm_v128_or(c0, t);
      t = wasm_i64x2_shuffle(wasm_i64x2_const(0, 0), v1, 1, 2);
      t = wasm_i64x2_shl(t, 64 - num_bits);
      t = (num_bits > 0) ? t : wasm_i64x2_const(0, 0);
      c0 = wasm_v128_or(c0, t);

      wasm_v128_store(msp->tmp, c0);

      c1 = wasm_u64x2_shr(v1, num_bits);
      t = wasm_i64x2_shuffle(v1, wasm_i64x2_const(0, 0), 1, 2);
      t = wasm_i64x2_shl(t, 64 - num_bits);
      t = (num_bits > 0) ? t : wasm_i64x2_const(0, 0);
      c1 = wasm_v128_or(c1, t);

      wasm_v128_store(msp->tmp + 16, c1);
    }

    //************************************************************************/
    /** @brief Fetches 32 bits from the frwd_struct bitstream
     *
     *  @tparam      X is the value fed in when the bitstream is exhausted.
     *               See frwd_read regarding the template
     *  @param [in]  msp is a pointer to frwd_struct
     */
    template<int X>
    static inline
    v128_t frwd_fetch(frwd_struct *msp)
    {
      if (msp->bits <= 128)
      {
        frwd_read<X>(msp);
        if (msp->bits <= 128) //need to test
          frwd_read<X>(msp);
      }
      v128_t t = wasm_v128_load(msp->tmp);
      return t;
    }

    //************************************************************************/
    /** @brief decodes one quad, using 32 bit data
     *
     *  @tparam N       0 for the first quad and 1 for the second quad in an
     *                  octet
     *  @param inf_u_q  decoded VLC code, with interleaved u values
     *  @param U_q      U values
     *  @param magsgn   structure for forward data buffer
     *  @param p        bitplane at which we are decoding
     *  @param vn       used for handling E values (stores v_n values)
     *  @return v128_t decoded quad
     */
    template <int N>
    static inline 
    v128_t decode_one_quad32(const v128_t inf_u_q, v128_t U_q,
                              frwd_struct* magsgn, ui32 p, v128_t& vn)
    {
      v128_t w0;    // workers
      v128_t insig; // lanes hold FF's if samples are insignificant
      v128_t flags; // lanes hold e_k, e_1, and rho
      v128_t row;   // decoded row

      row = wasm_i64x2_const(0, 0);
      w0 = wasm_i32x4_shuffle(inf_u_q, inf_u_q, N, N, N, N);
      // we keeps e_k, e_1, and rho in w2
      flags = wasm_v128_and(w0, wasm_i32x4_const(0x1110,0x2220,0x4440,0x8880));
      insig = wasm_i32x4_eq(flags, wasm_i64x2_const(0, 0));
      if (wasm_i8x16_bitmask(insig) != 0xFFFF) //are all insignificant?
      {
        U_q = wasm_i32x4_shuffle(U_q, U_q, N, N, N, N);
        flags = wasm_i16x8_mul(flags, wasm_i16x8_const(8,8,4,4,2,2,1,1));
        v128_t ms_vec = frwd_fetch<0xFF>(magsgn); 

        // U_q holds U_q for this quad
        // flags has e_k, e_1, and rho such that e_k is sitting in the
        // 0x8000, e_1 in 0x800, and rho in 0x80

        // next e_k and m_n
        v128_t m_n;
        w0 = wasm_u32x4_shr(flags, 15); // e_k
        m_n = wasm_i32x4_sub(U_q, w0);
        m_n = wasm_v128_andnot(m_n, insig);

        // find cumulative sums
        // to find at which bit in ms_vec the sample starts
        v128_t ex_sum, shfl, inc_sum = m_n; // inclusive scan
        shfl = wasm_i32x4_shuffle(wasm_i64x2_const(0,0), inc_sum, 3, 4, 5, 6);
        inc_sum = wasm_i32x4_add(inc_sum, shfl);
        shfl = wasm_i64x2_shuffle(wasm_i64x2_const(0,0), inc_sum, 1, 2);
        inc_sum = wasm_i32x4_add(inc_sum, shfl);
        int total_mn = wasm_u16x8_extract_lane(inc_sum, 6);
        ex_sum = wasm_i32x4_shuffle(wasm_i64x2_const(0,0), inc_sum, 3, 4, 5, 6);

        // find the starting byte and starting bit
        v128_t byte_idx = wasm_u32x4_shr(ex_sum, 3);
        v128_t bit_idx = 
          wasm_v128_and(ex_sum, wasm_i32x4_const(OJPH_REPEAT4(7)));
        byte_idx = wasm_i8x16_swizzle(byte_idx, 
          wasm_i32x4_const(0x00000000, 0x04040404, 0x08080808, 0x0C0C0C0C));
        byte_idx = 
          wasm_i32x4_add(byte_idx, wasm_i32x4_const(OJPH_REPEAT4(0x03020100)));
        v128_t d0 = wasm_i8x16_swizzle(ms_vec, byte_idx);
        byte_idx = 
          wasm_i32x4_add(byte_idx, wasm_i32x4_const(OJPH_REPEAT4(0x01010101)));
        v128_t d1 = wasm_i8x16_swizzle(ms_vec, byte_idx);

        // shift samples values to correct location
        bit_idx = wasm_v128_or(bit_idx, wasm_i32x4_shl(bit_idx, 16));
        v128_t bit_shift = wasm_i8x16_swizzle(
          wasm_i8x16_const(-1, 127, 63, 31, 15, 7, 3, 1,
                           -1, 127, 63, 31, 15, 7, 3, 1), bit_idx);
        bit_shift = 
          wasm_i16x8_add(bit_shift, wasm_i16x8_const(OJPH_REPEAT8(0x0101)));
        d0 = wasm_i16x8_mul(d0, bit_shift);
        d0 = wasm_u16x8_shr(d0, 8); // we should have 8 bits in the LSB
        d1 = wasm_i16x8_mul(d1, bit_shift);
        d1 =  // 8 in MSB
          wasm_v128_and(d1, wasm_u32x4_const(OJPH_REPEAT4(0xFF00FF00)));
        d0 = wasm_v128_or(d0, d1);

        // find location of e_k and mask
        v128_t shift;
        v128_t ones = wasm_i32x4_const(OJPH_REPEAT4(1));
        v128_t twos = wasm_i32x4_const(OJPH_REPEAT4(2));
        ui32 U_q_m1 = wasm_u32x4_extract_lane(U_q, 0) - 1u;
        w0 = wasm_i32x4_sub(twos, w0);
        shift = wasm_i32x4_shl(w0, U_q_m1);
        ms_vec = wasm_v128_and(d0, wasm_i32x4_sub(shift, ones));

        // next e_1
        w0 = wasm_v128_and(flags, wasm_i32x4_const(OJPH_REPEAT4(0x800)));
        w0 = wasm_i32x4_eq(w0, wasm_i64x2_const(0, 0));
        w0 = wasm_v128_andnot(shift, w0);  // e_1 in correct position
        ms_vec = wasm_v128_or(ms_vec, w0); // e_1
        w0 = wasm_i32x4_shl(ms_vec, 31);   // sign
        ms_vec = wasm_v128_or(ms_vec, ones); // bin center
        v128_t tvn = ms_vec;
        ms_vec = wasm_i32x4_add(ms_vec, twos);// + 2
        ms_vec = wasm_i32x4_shl(ms_vec, p - 1);
        ms_vec = wasm_v128_or(ms_vec, w0); // sign
        row = wasm_v128_andnot(ms_vec, insig); // significant only

        ms_vec = wasm_v128_andnot(tvn, insig); // significant only
        if (N == 0) // the compiler should remove one
          tvn = wasm_i8x16_swizzle(ms_vec, 
            wasm_i32x4_const(0x07060504, 0x0F0E0D0C, -1, -1));
        else if (N == 1)
          tvn = wasm_i8x16_swizzle(ms_vec, 
            wasm_i32x4_const(-1, 0x07060504, 0x0F0E0D0C, -1));
        else
          assert(0);
        vn = wasm_v128_or(vn, tvn);

        if (total_mn)
          frwd_advance(magsgn, (ui32)total_mn);
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
     *  @return v128_t decoded quad
     */
    static inline 
    v128_t decode_two_quad16(const v128_t inf_u_q, v128_t U_q, 
                              frwd_struct* magsgn, ui32 p, v128_t& vn)
    {
      v128_t w0;     // workers
      v128_t insig;  // lanes hold FF's if samples are insignificant
      v128_t flags;  // lanes hold e_k, e_1, and rho
      v128_t row;    // decoded row

      row = wasm_i64x2_const(0, 0);
      w0 = wasm_i8x16_swizzle(inf_u_q, 
        wasm_i16x8_const(0x0100, 0x0100, 0x0100, 0x0100,
                         0x0504, 0x0504, 0x0504, 0x0504));
      // we keeps e_k, e_1, and rho in w2
      flags = wasm_v128_and(w0, 
        wasm_u16x8_const(0x1110, 0x2220, 0x4440, 0x8880, 
                         0x1110, 0x2220, 0x4440, 0x8880));
      insig = wasm_i16x8_eq(flags, wasm_i64x2_const(0, 0));
      if (wasm_i8x16_bitmask(insig) != 0xFFFF) //are all insignificant?
      {
        U_q = wasm_i8x16_swizzle(U_q, 
          wasm_i16x8_const(0x0100, 0x0100, 0x0100, 0x0100, 
                           0x0504, 0x0504, 0x0504, 0x0504));
        flags = wasm_i16x8_mul(flags, wasm_i16x8_const(8,4,2,1,8,4,2,1));
        v128_t ms_vec = frwd_fetch<0xFF>(magsgn); 

        // U_q holds U_q for this quad
        // flags has e_k, e_1, and rho such that e_k is sitting in the
        // 0x8000, e_1 in 0x800, and rho in 0x80

        // next e_k and m_n
        v128_t m_n;
        w0 = wasm_u16x8_shr(flags, 15); // e_k
        m_n = wasm_i16x8_sub(U_q, w0);
        m_n = wasm_v128_andnot(m_n, insig);

        // find cumulative sums
        // to find at which bit in ms_vec the sample starts
        v128_t ex_sum, shfl, inc_sum = m_n; // inclusive scan
        shfl = wasm_i16x8_shuffle(wasm_i64x2_const(0,0), 
          inc_sum, 7, 8, 9, 10, 11, 12, 13, 14);
        inc_sum = wasm_i16x8_add(inc_sum, shfl);
        shfl = wasm_i32x4_shuffle(wasm_i64x2_const(0,0), inc_sum, 3, 4, 5, 6);
        inc_sum = wasm_i16x8_add(inc_sum, shfl);
        shfl = wasm_i64x2_shuffle(wasm_i64x2_const(0,0), inc_sum, 1, 2);
        inc_sum = wasm_i16x8_add(inc_sum, shfl);
        int total_mn = wasm_u16x8_extract_lane(inc_sum, 7);
        ex_sum = wasm_i16x8_shuffle(wasm_i64x2_const(0,0), 
          inc_sum, 7, 8, 9, 10, 11, 12, 13, 14);

        // find the starting byte and starting bit
        v128_t byte_idx = wasm_u16x8_shr(ex_sum, 3);
        v128_t bit_idx = 
          wasm_v128_and(ex_sum, wasm_i16x8_const(OJPH_REPEAT8(7)));
        byte_idx = wasm_i8x16_swizzle(byte_idx, 
          wasm_i16x8_const(0x0000, 0x0202, 0x0404, 0x0606, 
                           0x0808, 0x0A0A, 0x0C0C, 0x0E0E));
        byte_idx = 
          wasm_i16x8_add(byte_idx, wasm_i16x8_const(OJPH_REPEAT8(0x0100)));
        v128_t d0 = wasm_i8x16_swizzle(ms_vec, byte_idx);
        byte_idx = 
          wasm_i16x8_add(byte_idx, wasm_i16x8_const(OJPH_REPEAT8(0x0101)));
        v128_t d1 = wasm_i8x16_swizzle(ms_vec, byte_idx);

        // shift samples values to correct location
        v128_t bit_shift = wasm_i8x16_swizzle(
          wasm_i8x16_const(-1, 127, 63, 31, 15, 7, 3, 1,
                           -1, 127, 63, 31, 15, 7, 3, 1), bit_idx);
        bit_shift = 
          wasm_i16x8_add(bit_shift, wasm_i16x8_const(OJPH_REPEAT8(0x0101)));
        d0 = wasm_i16x8_mul(d0, bit_shift);
        d0 = wasm_u16x8_shr(d0, 8); // we should have 8 bits in the LSB
        d1 = wasm_i16x8_mul(d1, bit_shift);
        d1 = // 8 in MSB
          wasm_v128_and(d1, wasm_i16x8_const(OJPH_REPEAT8((si16)0xFF00))); 
        d0 = wasm_v128_or(d0, d1);

        // find location of e_k and mask
        v128_t shift, t0, t1;
        v128_t ones = wasm_i16x8_const(OJPH_REPEAT8(1));
        v128_t twos = wasm_i16x8_const(OJPH_REPEAT8(2));
        v128_t U_q_m1 = wasm_i32x4_sub(U_q, ones);
        ui32 Uq0 = wasm_u16x8_extract_lane(U_q_m1, 0);
        ui32 Uq1 = wasm_u16x8_extract_lane(U_q_m1, 4);
        w0 = wasm_i16x8_sub(twos, w0);
        t0 = wasm_v128_and(w0, wasm_i64x2_const(-1, 0));
        t1 = wasm_v128_and(w0, wasm_i64x2_const(0, -1));
        t0 = wasm_i32x4_shl(t0, Uq0);
        t1 = wasm_i32x4_shl(t1, Uq1);
        shift = wasm_v128_or(t0, t1);
        ms_vec = wasm_v128_and(d0, wasm_i16x8_sub(shift, ones));

        // next e_1
        w0 = wasm_v128_and(flags, wasm_i16x8_const(OJPH_REPEAT8(0x800)));
        w0 = wasm_i16x8_eq(w0, wasm_i64x2_const(0, 0));
        w0 = wasm_v128_andnot(shift, w0);  // e_1 in correct position
        ms_vec = wasm_v128_or(ms_vec, w0); // e_1
        w0 = wasm_i16x8_shl(ms_vec, 15);   // sign
        ms_vec = wasm_v128_or(ms_vec, ones); // bin center
        v128_t tvn = ms_vec;
        ms_vec = wasm_i16x8_add(ms_vec, twos);// + 2
        ms_vec = wasm_i16x8_shl(ms_vec, p - 1);
        ms_vec = wasm_v128_or(ms_vec, w0); // sign
        row = wasm_v128_andnot(ms_vec, insig); // significant only

        ms_vec = wasm_v128_andnot(tvn, insig); // significant only
        w0 = wasm_i8x16_swizzle(ms_vec, 
          wasm_i16x8_const(0x0302, 0x0706, -1, -1, -1, -1, -1, -1));
        vn = wasm_v128_or(vn, w0);
        w0 = wasm_i8x16_swizzle(ms_vec, 
          wasm_i16x8_const(-1, 0x0B0A, 0x0F0E, -1, -1, -1, -1, -1));
        vn = wasm_v128_or(vn, w0);

        if (total_mn)
          frwd_advance(magsgn, (ui32)total_mn);
      }
      return row;
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
    bool ojph_decode_codeblock_wasm(ui8* coded_data, ui32* decoded_data,
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
                              "2nd and potential 3rd pass.\n");
        num_passes = 1;
      }

      if (num_passes > 3)
      {
        OJPH_WARN(0x00010002, "We do not support more than 3 coding passes; "
                              "This codeblocks has %d passes.\n",
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
                                "displayed again.\n");
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
                                "displayed again.\n");
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
                                  "again.\n");
          }
        }
      }
      ui32 p = 30 - missing_msbs; // The least significant bitplane for CUP
      // There is a way to handle the case of p == 0, but a different path
      // is required

      if (lengths1 < 2)
      {
        OJPH_WARN(0x00010006, "Wrong codeblock length.\n");
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
      ui16 scratch[8 * 513] = {0};          // 8+ kB

      // We need an extra two entries (one inf and one u_q) beyond
      // the last column. 
      // If the block width is 4 (2 quads), then we use sstr of 8 
      // (enough for 4 quads). If width is 8 (4 quads) we use 
      // sstr is 16 (enough for 8 quads). For a width of 16 (8 
      // quads), we use 24 (enough for 12 quads).
      ui32 sstr = ((width + 2u) + 7u) & ~7u; // multiples of 8

      assert((stride & 0x3) == 0);

      ui32 mmsbp2 = missing_msbs + 2;

      // The cleanup pass is decoded in two steps; in step one,
      // the VLC and MEL segments are decoded, generating a record that 
      // has 2 bytes per quad. The 2 bytes contain, u, rho, e^1 & e^k.
      // This information should be sufficient for the next step.
      // In step 2, we decode the MagSgn segment.
      
      // step 1 decoding VLC and MEL segments
      {
        // init structures
        dec_mel_st mel;
        mel_init(&mel, coded_data, lcup, scup);
        rev_struct vlc;
        rev_init(&vlc, coded_data, lcup, scup);

        int run = mel_get_run(&mel); // decode runs of events from MEL bitstrm
                                     // data represented as runs of 0 events
                                     // See mel_decode description

        ui32 vlc_val;
        ui32 c_q = 0;
        ui16 *sp = scratch;
        //initial quad row
        for (ui32 x = 0; x < width; sp += 4)
        {
          // decode VLC
          /////////////

          // first quad
          vlc_val = rev_fetch(&vlc);

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
          vlc_val = rev_advance(&vlc, t0 & 0x7);

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
          vlc_val = rev_advance(&vlc, t1 & 0x7);
          
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
          vlc_val = rev_advance(&vlc, uvlc_entry & 0x7); 
          uvlc_entry >>= 3; 
          //extract suffixes for quad 0 and 1
          ui32 len = uvlc_entry & 0xF;           //suffix length for 2 quads
          ui32 tmp = vlc_val & ((1 << len) - 1); //suffix value for 2 quads
          vlc_val = rev_advance(&vlc, len);
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
            vlc_val = rev_fetch(&vlc);

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
            vlc_val = rev_advance(&vlc, t0 & 0x7);

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
            vlc_val = rev_advance(&vlc, t1 & 0x7);
          
            // decode u
            /////////////
            // uvlc_mode is made up of u_offset bits from the quad pair
            ui32 uvlc_mode = ((t0 & 0x8U) << 3) | ((t1 & 0x8U) << 4);
            ui32 uvlc_entry = uvlc_tbl1[uvlc_mode + (vlc_val & 0x3F)];
            //remove total prefix length
            vlc_val = rev_advance(&vlc, uvlc_entry & 0x7);
            uvlc_entry >>= 3;
            //extract suffixes for quad 0 and 1
            ui32 len = uvlc_entry & 0xF;           //suffix length for 2 quads
            ui32 tmp = vlc_val & ((1 << len) - 1); //suffix value for 2 quads
            vlc_val = rev_advance(&vlc, len);
            uvlc_entry >>= 4;
            // quad 0 length
            len = uvlc_entry & 0x7; // quad 0 suffix length
            uvlc_entry >>= 3;
            ui16 u_q = (ui16)((uvlc_entry & 7) + (tmp & ~(0xFU << len))); //u_q
            sp[1] = u_q;
            u_q = (ui16)((uvlc_entry >> 3) + (tmp >> len)); // u_q
            sp[3] = u_q;
          }
          sp[0] = sp[1] = 0;
        }
      }

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
        // We allocate a scratch row for storing v_n values.
        // We have 512 quads horizontally.
        // We may go beyond the last entry by up to 4 entries.
        // Here we allocate additional 8 entries.
        // There are two rows in this structure, the bottom
        // row is used to store processed entries.
        const int v_n_size = 512 + 8;
        ui32 v_n_scratch[2 * v_n_size] = {0}; // 4+ kB

        frwd_struct magsgn;
        frwd_init<0xFF>(&magsgn, coded_data, lcup - scup);

        {
          ui16 *sp = scratch;
          ui32 *vp = v_n_scratch;
          ui32 *dp = decoded_data;
          vp[0] = 2; // for easy calculation of emax

          for (ui32 x = 0; x < width; x += 4, sp += 4, vp += 2, dp += 4)
          {
            //here we process two quads
            v128_t w0, w1; // workers
            v128_t inf_u_q, U_q;
            // determine U_q
            {
              inf_u_q = wasm_v128_load(sp);
              U_q = wasm_u32x4_shr(inf_u_q, 16);

              w0 = wasm_i32x4_gt(U_q, wasm_u32x4_splat(mmsbp2));
              ui32 i = wasm_i8x16_bitmask(w0);
              if (i & 0xFF) // only the lower two U_q
                return false;
            }

            v128_t vn = wasm_i32x4_const(OJPH_REPEAT4(2));
            v128_t row0 = decode_one_quad32<0>(inf_u_q, U_q, &magsgn, p, vn);
            v128_t row1 = decode_one_quad32<1>(inf_u_q, U_q, &magsgn, p, vn);
            w0 = wasm_v128_load(vp);
            w0 = wasm_v128_and(w0, wasm_i32x4_const(-1,0,0,0));
            w0 = wasm_v128_or(w0, vn);
            wasm_v128_store(vp, w0);            

            //interleave in ssse3 style 
                 
            w0 = wasm_i32x4_shuffle(row0, row1, 0, 4, 1, 5);
            w1 = wasm_i32x4_shuffle(row0, row1, 2, 6, 3, 7);
            row0 = wasm_i32x4_shuffle(w0, w1, 0, 4, 1, 5);
            row1 = wasm_i32x4_shuffle(w0, w1, 2, 6, 3, 7);
            wasm_v128_store(dp, row0);
            wasm_v128_store(dp + stride, row1);
          }
        }

        for (ui32 y = 2; y < height; y += 2)
        {
          {
            // perform 31 - count_leading_zeros(*vp) here
            ui32 *vp = v_n_scratch;
            const v128_t lut_lo = wasm_i8x16_const(
              31, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4
            );
            const v128_t lut_hi = wasm_i8x16_const(
              31, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0
            );
            const v128_t nibble_mask = wasm_i8x16_const(OJPH_REPEAT16(0x0F));
            const v128_t byte_offset8 = wasm_i16x8_const(OJPH_REPEAT8(8));
            const v128_t byte_offset16 = wasm_i16x8_const(OJPH_REPEAT8(16));
            const v128_t cc = wasm_i32x4_const(OJPH_REPEAT4(31));
            for (ui32 x = 0; x <= width; x += 8, vp += 4)
            {
              v128_t v, t; // workers
              v = wasm_v128_load(vp);

              t = wasm_v128_and(nibble_mask, v);
              v = wasm_v128_and(wasm_u16x8_shr(v, 4), nibble_mask);
              t = wasm_i8x16_swizzle(lut_lo, t);
              v = wasm_i8x16_swizzle(lut_hi, v);
              v = wasm_u8x16_min(v, t);

              t = wasm_u16x8_shr(v, 8);
              v = wasm_v128_or(v, byte_offset8);
              v = wasm_u8x16_min(v, t);

              t = wasm_u32x4_shr(v, 16);
              v = wasm_v128_or(v, byte_offset16);
              v = wasm_u8x16_min(v, t);

              v = wasm_i16x8_sub(cc, v);
              wasm_v128_store(vp + v_n_size, v);
            }
          }

          ui32 *vp = v_n_scratch;
          ui16 *sp = scratch + (y >> 1) * sstr;
          ui32 *dp = decoded_data + y * stride;
          vp[0] = 2; // for easy calculation of emax

          for (ui32 x = 0; x < width; x += 4, sp += 4, vp += 2, dp += 4)
          {
            //process two quads
            v128_t w0, w1; // workers
            v128_t inf_u_q, U_q;
            // determine U_q
            {
              v128_t gamma, emax, kappa, u_q; // needed locally

              inf_u_q = wasm_v128_load(sp);
              gamma = 
                wasm_v128_and(inf_u_q, wasm_i32x4_const(OJPH_REPEAT4(0xF0)));
              w0 = wasm_i32x4_sub(gamma, wasm_i32x4_const(OJPH_REPEAT4(1)));
              gamma = wasm_v128_and(gamma, w0);
              gamma = wasm_i32x4_eq(gamma, wasm_i64x2_const(0, 0));

              emax = wasm_v128_load(vp + v_n_size);
              w0 = wasm_i32x4_shuffle(emax, wasm_i64x2_const(0,0), 1, 2, 3, 4);
              emax = wasm_i16x8_max(w0, emax); // no max_epi32 in ssse3              
              emax = wasm_v128_andnot(emax, gamma);

              kappa = wasm_i32x4_const(OJPH_REPEAT4(1));
              kappa = wasm_i16x8_max(emax, kappa); // no max_epi32 in ssse3

              u_q = wasm_u32x4_shr(inf_u_q, 16);
              U_q = wasm_i32x4_add(u_q, kappa);

              w0 = wasm_i32x4_gt(U_q, wasm_u32x4_splat(mmsbp2));
              ui32 i = wasm_i8x16_bitmask(w0);
              if (i & 0xFF) // only the lower two U_q
                return false;
            }

            v128_t vn = wasm_i32x4_const(OJPH_REPEAT4(2));
            v128_t row0 = decode_one_quad32<0>(inf_u_q, U_q, &magsgn, p, vn);
            v128_t row1 = decode_one_quad32<1>(inf_u_q, U_q, &magsgn, p, vn);
            w0 = wasm_v128_load(vp);
            w0 = wasm_v128_and(w0, wasm_i32x4_const(-1,0,0,0));
            w0 = wasm_v128_or(w0, vn);
            wasm_v128_store(vp, w0);  

            //interleave in ssse3 style
            w0 = wasm_i32x4_shuffle(row0, row1, 0, 4, 1, 5); 
            w1 = wasm_i32x4_shuffle(row0, row1, 2, 6, 3, 7); 
            row0 = wasm_i32x4_shuffle(w0, w1, 0, 4, 1, 5); 
            row1 = wasm_i32x4_shuffle(w0, w1, 2, 6, 3, 7); 
            wasm_v128_store(dp, row0);
            wasm_v128_store(dp + stride, row1);
          }
        }
      }
      else 
      {
        // reduce bitplane by 16 because we now have 16 bits instead of 32
        p -= 16;

        // We allocate a scratch row for storing v_n values.
        // We have 512 quads horizontally.
        // We may go beyond the last entry by up to 8 entries.
        // Therefore we allocate additional 8 entries.
        // There are two rows in this structure, the bottom
        // row is used to store processed entries.
        const int v_n_size = 512 + 8;
        ui16 v_n_scratch[2 * v_n_size] = {0}; // 2+ kB

        frwd_struct magsgn;
        frwd_init<0xFF>(&magsgn, coded_data, lcup - scup);

        {
          ui16 *sp = scratch;
          ui16 *vp = v_n_scratch;
          ui32 *dp = decoded_data;
          vp[0] = 2; // for easy calculation of emax

          for (ui32 x = 0; x < width; x += 4, sp += 4, vp += 2, dp += 4)
          {
            //here we process two quads
            v128_t w0, w1; // workers
            v128_t inf_u_q, U_q;
            // determine U_q
            {
              inf_u_q = wasm_v128_load(sp);
              U_q = wasm_u32x4_shr(inf_u_q, 16);

              w0 = wasm_i32x4_gt(U_q, wasm_u32x4_splat(mmsbp2));
              ui32 i = wasm_i8x16_bitmask(w0);
              if (i & 0xFF) // only the lower two U_q
                return false;
            }

            v128_t vn = wasm_i16x8_const(OJPH_REPEAT8(2));
            v128_t row = decode_two_quad16(inf_u_q, U_q, &magsgn, p, vn);
            w0 = wasm_v128_load(vp);
            w0 = wasm_v128_and(w0, wasm_i16x8_const(-1,0,0,0,0,0,0,0));
            w0 = wasm_v128_or(w0, vn);
            wasm_v128_store(vp, w0);  

            //interleave in ssse3 style 
            w0 = wasm_i8x16_swizzle(row, 
              wasm_i16x8_const(-1, 0x0100, -1, 0x0504, 
                               -1, 0x0908, -1, 0x0D0C));
            wasm_v128_store(dp, w0);
            w1 = wasm_i8x16_swizzle(row, 
              wasm_i16x8_const(-1, 0x0302, -1, 0x0706, 
                               -1, 0x0B0A, -1, 0x0F0E));
            wasm_v128_store(dp + stride, w1);
          }
        }

        for (ui32 y = 2; y < height; y += 2)
        {
          {
            // perform 15 - count_leading_zeros(*vp) here
            ui16 *vp = v_n_scratch;
            const v128_t lut_lo = wasm_i8x16_const(
              15, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4
            );
            const v128_t lut_hi = wasm_i8x16_const(
              15, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0
            );
            const v128_t nibble_mask = wasm_i8x16_const(OJPH_REPEAT16(0x0F));
            const v128_t byte_offset8 = wasm_i16x8_const(OJPH_REPEAT8(8));
            const v128_t cc = wasm_i16x8_const(OJPH_REPEAT8(15));
            for (ui32 x = 0; x <= width; x += 16, vp += 8)
            {
              v128_t v, t; // workers
              v = wasm_v128_load(vp);

              t = wasm_v128_and(nibble_mask, v);
              v = wasm_v128_and(wasm_u16x8_shr(v, 4), nibble_mask);
              t = wasm_i8x16_swizzle(lut_lo, t);
              v = wasm_i8x16_swizzle(lut_hi, v);
              v = wasm_u8x16_min(v, t);

              t = wasm_u16x8_shr(v, 8);
              v = wasm_v128_or(v, byte_offset8);
              v = wasm_u8x16_min(v, t);

              v = wasm_i16x8_sub(cc, v);
              wasm_v128_store(vp + v_n_size, v);
            }
          }

          ui16 *vp = v_n_scratch;
          ui16 *sp = scratch + (y >> 1) * sstr;
          ui32 *dp = decoded_data + y * stride;
          vp[0] = 2; // for easy calculation of emax

          for (ui32 x = 0; x < width; x += 4, sp += 4, vp += 2, dp += 4)
          {
            //process two quads
            v128_t w0, w1; // workers
            v128_t inf_u_q, U_q;
            // determine U_q
            {
              v128_t gamma, emax, kappa, u_q; // needed locally

              inf_u_q = wasm_v128_load(sp);
              gamma = 
                wasm_v128_and(inf_u_q, wasm_i32x4_const(OJPH_REPEAT4(0xF0)));
              w0 = wasm_i32x4_sub(gamma, wasm_i32x4_const(OJPH_REPEAT4(1)));
              gamma = wasm_v128_and(gamma, w0);
              gamma = wasm_i32x4_eq(gamma, wasm_i64x2_const(0, 0));

              emax = wasm_v128_load(vp + v_n_size);
              w0 = wasm_i16x8_shuffle(emax, 
                wasm_i64x2_const(0, 0), 1, 2, 3, 4, 5, 6, 7, 8);
              emax = wasm_i16x8_max(w0, emax); // no max_epi32 in ssse3
              emax = wasm_i8x16_swizzle(emax, 
                wasm_i16x8_const(0x0100, -1, 0x0302, -1, 
                                 0x0504, -1, 0x0706, -1));
              emax = wasm_v128_andnot(emax, gamma);

              kappa = wasm_i32x4_const(OJPH_REPEAT4(1));
              kappa = wasm_i16x8_max(emax, kappa); // no max_epi32 in ssse3

              u_q = wasm_u32x4_shr(inf_u_q, 16);
              U_q = wasm_i32x4_add(u_q, kappa);

              w0 = wasm_i32x4_gt(U_q, wasm_u32x4_splat(mmsbp2));
              ui32 i = wasm_i8x16_bitmask(w0);
              if (i & 0xFF) // only the lower two U_q
                return false;
            }

            v128_t vn = wasm_i16x8_const(OJPH_REPEAT8(2));
            v128_t row = decode_two_quad16(inf_u_q, U_q, &magsgn, p, vn);
            w0 = wasm_v128_load(vp);
            w0 = wasm_v128_and(w0, wasm_i16x8_const(-1,0,0,0,0,0,0,0));
            w0 = wasm_v128_or(w0, vn);
            wasm_v128_store(vp, w0);  

            w0 = wasm_i8x16_swizzle(row, 
              wasm_i16x8_const(-1, 0x0100, -1, 0x0504, 
                               -1, 0x0908, -1, 0x0D0C));
            wasm_v128_store(dp, w0);
            w1 = wasm_i8x16_swizzle(row, 
              wasm_i16x8_const(-1, 0x0302, -1, 0x0706, 
                               -1, 0x0B0A, -1, 0x0F0E));
            wasm_v128_store(dp + stride, w1);
          }
        }

        // increase bitplane back by 16 because we need to process 32 bits
        p += 16;
      }

      if (num_passes > 1)
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

          const v128_t mask_3 = wasm_i32x4_const(OJPH_REPEAT4(0x30));
          const v128_t mask_C = wasm_i32x4_const(OJPH_REPEAT4(0xC0));
          const v128_t shuffle_mask = wasm_i32x4_const(0x0C080400,-1,-1,-1);
          for (y = 0; y < height; y += 4) 
          {
            ui16* sp = scratch + (y >> 1) * sstr;
            ui16* dp = sigma + (y >> 2) * mstr;
            for (ui32 x = 0; x < width; x += 8, sp += 8, dp += 2) 
            {
              v128_t s0, s1, u3, uC, t0, t1;

              s0 = wasm_v128_load(sp);
              u3 = wasm_v128_and(s0, mask_3);
              u3 = wasm_u32x4_shr(u3, 4);
              uC = wasm_v128_and(s0, mask_C);
              uC = wasm_u32x4_shr(uC, 2);
              t0 = wasm_v128_or(u3, uC);

              s1 = wasm_v128_load(sp + sstr);
              u3 = wasm_v128_and(s1, mask_3);
              u3 = wasm_u32x4_shr(u3, 2);
              uC = wasm_v128_and(s1, mask_C);
              t1 = wasm_v128_or(u3, uC);

              v128_t r = wasm_v128_or(t0, t1);
              r = wasm_i8x16_swizzle(r, shuffle_mask);

              wasm_v128_store32_lane(dp, r, 0);
            }
            dp[0] = 0; // set an extra entry on the right with 0
          }
          {
            // reset one row after the codeblock
            ui16* dp = sigma + (y >> 2) * mstr;
            v128_t zero = wasm_i64x2_const(0, 0);
            for (ui32 x = 0; x < width; x += 32, dp += 8)
              wasm_v128_store(dp, zero);
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

          frwd_struct sigprop;
          frwd_init<0>(&sigprop, coded_data + lengths1, (int)lengths2);

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
              ui32 ps = *(ui32*)prev_sig;
              ui32 ns = *(ui32*)(cur_sig + mstr);
              ui32 u = (ps & 0x88888888) >> 3; // the row on top
              if (!stripe_causal)
                u |= (ns & 0x11111111) << 3;   // the row below

              ui32 cs = *(ui32*)cur_sig;
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
                v128_t cwd_vec = frwd_fetch<0>(&sigprop);
                ui32 cwd = wasm_u32x4_extract_lane(cwd_vec, 0);

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
                  // Spread new_sig, such that each bit is in one byte with a
                  // value of 0 if new_sig bit is 0, and 0xFF if new_sig is 1
                  v128_t new_sig_vec = wasm_i16x8_splat((si16)new_sig);
                  new_sig_vec = wasm_i8x16_swizzle(new_sig_vec,
                    wasm_i8x16_const(0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1));
                  new_sig_vec = wasm_v128_and(new_sig_vec,
                    wasm_u64x2_const(OJPH_REPEAT2(0x8040201008040201)));
                  new_sig_vec = wasm_i8x16_eq(new_sig_vec,
                    wasm_u64x2_const(OJPH_REPEAT2(0x8040201008040201)));

                  // find cumulative sums
                  // to find which bit in cwd we should extract
                  v128_t ex_sum, shfl, inc_sum = new_sig_vec; // inclusive scan
                  inc_sum = wasm_i8x16_abs(inc_sum); // cvrt to 0 or 1                  
                  shfl = wasm_i8x16_shuffle(wasm_i64x2_const(0,0), inc_sum,
                    15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30);
                  inc_sum = wasm_i8x16_add(inc_sum, shfl);
                  shfl = wasm_i16x8_shuffle(wasm_i64x2_const(0,0), inc_sum, 
                    7, 8, 9, 10, 11, 12, 13, 14);
                  inc_sum = wasm_i8x16_add(inc_sum, shfl);
                  shfl = wasm_i32x4_shuffle(wasm_i64x2_const(0,0), inc_sum, 
                    3, 4, 5, 6);
                  inc_sum = wasm_i8x16_add(inc_sum, shfl);
                  shfl = wasm_i64x2_shuffle(wasm_i64x2_const(0,0), inc_sum, 
                    1, 2);
                  inc_sum = wasm_i8x16_add(inc_sum, shfl);
                  cnt += wasm_u8x16_extract_lane(inc_sum, 15);
                  // exclusive scan
                  ex_sum = wasm_i8x16_shuffle(wasm_i64x2_const(0,0), inc_sum,
                    15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30);

                  // Spread cwd, such that each bit is in one byte
                  // with a value of 0 or 1.
                  cwd_vec = wasm_i16x8_splat((si16)cwd);
                  cwd_vec = wasm_i8x16_swizzle(cwd_vec,
                    wasm_i8x16_const(0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1));
                  cwd_vec = wasm_v128_and(cwd_vec,
                    wasm_u64x2_const(OJPH_REPEAT2(0x8040201008040201)));
                  cwd_vec = wasm_i8x16_eq(cwd_vec,
                    wasm_u64x2_const(OJPH_REPEAT2(0x8040201008040201)));
                  cwd_vec = wasm_i8x16_abs(cwd_vec);

                  // Obtain bit from cwd_vec correspondig to ex_sum
                  // Basically, collect needed bits from cwd_vec
                  v128_t v = wasm_i8x16_swizzle(cwd_vec, ex_sum);

                  // load data and set spp coefficients
                  v128_t m = wasm_i8x16_const(
                    0,-1,-1,-1,4,-1,-1,-1,8,-1,-1,-1,12,-1,-1,-1);
                  v128_t val = wasm_i32x4_splat(3 << (p - 2));
                  ui32 *dp = dpp;
                  for (int c = 0; c < 4; ++ c) {
                    v128_t s0, s0_ns, s0_val;
                    // load coefficients
                    s0 = wasm_v128_load(dp);

                    // epi32 is -1 only for coefficient that
                    // are changed during the SPP
                    s0_ns = wasm_i8x16_swizzle(new_sig_vec, m);
                    s0_ns = wasm_i32x4_eq(s0_ns, 
                      wasm_i32x4_const(OJPH_REPEAT4(0xFF)));

                    // obtain sign for coefficients in SPP
                    s0_val = wasm_i8x16_swizzle(v, m);
                    s0_val = wasm_i32x4_shl(s0_val, 31);
                    s0_val = wasm_v128_or(s0_val, val);
                    s0_val = wasm_v128_and(s0_val, s0_ns);

                    // update vector
                    s0 = wasm_v128_or(s0, s0_val);
                    // store coefficients
                    wasm_v128_store(dp, s0);
                    // prepare for next row
                    dp += stride;
                    m = wasm_i32x4_add(m, wasm_i32x4_const(OJPH_REPEAT4(1)));
                  }
                }
                frwd_advance(&sigprop, cnt);
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
          rev_struct magref;
          rev_init_mrp(&magref, coded_data, (int)lengths1, (int)lengths2);

          for (ui32 y = 0; y < height; y += 4)
          {
            ui16 *cur_sig = sigma + (y >> 2) * mstr;
            ui32 *dpp = decoded_data + y * stride;
            for (ui32 i = 0; i < width; i += 4, dpp += 4)
            {
              //Process one entry from sigma array at a time
              // Each nibble (4 bits) in the sigma array represents 4 rows,
              ui32 cwd = rev_fetch_mrp(&magref); // get 32 bit data
              ui16 sig = *cur_sig++; // 16 bit that will be processed now
              int total_bits = 0;
              if (sig) // if any of the 32 bits are set
              {
                // We work on 4 rows, with 4 samples each, since
                // data is 32 bit (4 bytes)

                // spread the 16 bits in sig to 0 or 1 bytes in sig_vec
                v128_t sig_vec = wasm_i16x8_splat((si16)sig);
                sig_vec = wasm_i8x16_swizzle(sig_vec,
                  wasm_i8x16_const(0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1));
                sig_vec = wasm_v128_and(sig_vec,
                  wasm_u64x2_const(OJPH_REPEAT2(0x8040201008040201)));
                sig_vec = wasm_i8x16_eq(sig_vec,
                  wasm_u64x2_const(OJPH_REPEAT2(0x8040201008040201)));
                sig_vec = wasm_i8x16_abs(sig_vec);

                // find cumulative sums
                // to find which bit in cwd we should extract
                v128_t ex_sum, shfl, inc_sum = sig_vec; // inclusive scan
                shfl = wasm_i8x16_shuffle(wasm_i64x2_const(0,0), inc_sum,
                  15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30);
                inc_sum = wasm_i8x16_add(inc_sum, shfl);
                shfl = wasm_i16x8_shuffle(wasm_i64x2_const(0,0), inc_sum, 
                  7, 8, 9, 10, 11, 12, 13, 14);
                inc_sum = wasm_i8x16_add(inc_sum, shfl);
                shfl = wasm_i32x4_shuffle(wasm_i64x2_const(0,0), inc_sum, 
                  3, 4, 5, 6);
                inc_sum = wasm_i8x16_add(inc_sum, shfl);
                shfl = wasm_i64x2_shuffle(wasm_i64x2_const(0,0), inc_sum, 
                  1, 2);
                inc_sum = wasm_i8x16_add(inc_sum, shfl);
                total_bits = wasm_u8x16_extract_lane(inc_sum, 15);
                // exclusive scan
                ex_sum = wasm_i8x16_shuffle(wasm_i64x2_const(0,0), inc_sum,
                  15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30);

                // Spread the 16 bits in cwd to inverted 0 or 1 bytes in
                // cwd_vec. Then, convert these to a form suitable
                // for coefficient modifications; in particular, a value
                // of 0 is presented as binary 11, and a value of 1 is
                // represented as binary 01
                v128_t cwd_vec = wasm_i16x8_splat((si16)cwd);
                cwd_vec = wasm_i8x16_swizzle(cwd_vec,
                  wasm_i8x16_const(0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1));
                cwd_vec = wasm_v128_and(cwd_vec, 
                  wasm_u64x2_const(OJPH_REPEAT2(0x8040201008040201)));
                cwd_vec = wasm_i8x16_eq(cwd_vec, 
                  wasm_u64x2_const(OJPH_REPEAT2(0x8040201008040201)));
                cwd_vec = 
                  wasm_i8x16_add(cwd_vec, wasm_i8x16_const(OJPH_REPEAT16(1)));
                cwd_vec = wasm_i8x16_add(cwd_vec, cwd_vec);
                cwd_vec = 
                  wasm_v128_or(cwd_vec, wasm_i8x16_const(OJPH_REPEAT16(1)));

                // load data and insert the mrp bit
                v128_t m = wasm_i8x16_const(0,-1,-1,-1,4,-1,-1,-1,
                                            8,-1,-1,-1,12,-1,-1,-1);
                ui32 *dp = dpp;
                for (int c = 0; c < 4; ++c) {
                  v128_t s0, s0_sig, s0_idx, s0_val;
                  // load coefficients                  
                  s0 = wasm_v128_load(dp);
                  // find significant samples in this row
                  s0_sig = wasm_i8x16_swizzle(sig_vec, m);
                  s0_sig = wasm_i8x16_eq(s0_sig, wasm_i64x2_const(0, 0));
                  // get MRP bit index, and MRP pattern
                  s0_idx = wasm_i8x16_swizzle(ex_sum, m);
                  s0_val = wasm_i8x16_swizzle(cwd_vec, s0_idx);
                  // keep data from significant samples only
                  s0_val = wasm_v128_andnot(s0_val, s0_sig);
                  // move mrp bits to correct position, and employ
                  s0_val = wasm_i32x4_shl(s0_val, p - 2);
                  s0 = wasm_v128_xor(s0, s0_val);
                  // store coefficients
                  wasm_v128_store(dp, s0);
                  // prepare for next row
                  dp += stride;
                  m = wasm_i32x4_add(m, wasm_i32x4_const(OJPH_REPEAT4(1)));
                }
              }
              // consume data according to the number of bits set
              rev_advance_mrp(&magref, (ui32)total_bits);
            }
          }
        }
      }

      return true;
    }
  }
}
