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
// File: ojph_block_decoder.cpp
// Author: Aous Naman
// Date: 13 May 2022
//***************************************************************************/

//***************************************************************************/
/** @file ojph_block_decoder.cpp
 *  @brief implements a HTJ2K block decoder
 */

#include <string>
#include <iostream>

#include <cassert>
#include <cstring>
#include "ojph_block_common.h"
#include "ojph_block_decoder.h"
#include "ojph_arch.h"
#include "ojph_message.h"

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
    static inline 
    ui32 rev_advance_mrp(rev_struct *mrp, ui32 num_bits)
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
    struct frwd_struct32 {
      const ui8* data;  //!<pointer to bitstream
      ui64 tmp;         //!<temporary buffer of read data
      ui32 bits;        //!<number of bits stored in tmp
      ui32 unstuff;     //!<1 if a bit needs to be unstuffed from next byte
      int size;         //!<size of data
    };

    //************************************************************************/
    /** @brief Read and unstuffs 32 bits from forward-growing bitstream
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
     *  Reading can go beyond the end of buffer by up to 3 bytes.
     *
     *  @tparam       X is the value fed in when the bitstream is exhausted
     *  @param  [in]  msp is a pointer to frwd_struct32 structure
     *
     */ 
    template<int X>
    static inline 
    void frwd_read(frwd_struct32 *msp)
    {
      assert(msp->bits <= 32); // assert that there is a space for 32 bits

      ui32 val = 0;
      if (msp->size > 3) {
        val = *(ui32*)msp->data;  // read 32 bits
        msp->data += 4;           // increment pointer
        msp->size -= 4;           // reduce size
      }
      else if (msp->size > 0)
      {
        int i = 0;
        val = X != 0 ? 0xFFFFFFFFu : 0;
        while (msp->size > 0) {   
          ui32 v = *msp->data++;    // read one byte at a time
          ui32 m = ~(0xFFu << i);    // mask of location
          val = (val & m) | (v << i);// put one byte in its correct location
          --msp->size;
          i += 8;          
        }
      }
      else
        val = X != 0 ? 0xFFFFFFFFu : 0;

      // we accumulate in t and keep a count of the number of bits in bits
      ui32 bits = 8 - msp->unstuff;        
      ui32 t = val & 0xFF;
      bool unstuff = ((val & 0xFF) == 0xFF);  // Do we need unstuffing next?

      t |= ((val >> 8) & 0xFF) << bits;
      bits += 8 - unstuff;
      unstuff = (((val >> 8) & 0xFF) == 0xFF);

      t |= ((val >> 16) & 0xFF) << bits;
      bits += 8 - unstuff;
      unstuff = (((val >> 16) & 0xFF) == 0xFF);

      t |= ((val >> 24) & 0xFF) << bits;
      bits += 8 - unstuff;
      msp->unstuff = (((val >> 24) & 0xFF) == 0xFF); // for next byte

      msp->tmp |= ((ui64)t) << msp->bits;  // move data to msp->tmp
      msp->bits += bits;
    }

    //************************************************************************/
    /** @brief Initialize frwd_struct32 struct and reads some bytes
     *  
     *  @tparam      X is the value fed in when the bitstream is exhausted.
     *               See frwd_read regarding the template
     *  @param [in]  msp is a pointer to frwd_struct32
     *  @param [in]  data is a pointer to the start of data
     *  @param [in]  size is the number of byte in the bitstream
     */
    template<int X>
    static inline
    void frwd_init(frwd_struct32 *msp, const ui8* data, int size)
    {
      msp->data = data;
      msp->tmp = 0;
      msp->bits = 0;
      msp->unstuff = 0;
      msp->size = size;

      //This code is designed for an architecture that read address should
      // align to the read size (address multiple of 4 if read size is 4)
      //These few lines take care of the case where data is not at a multiple
      // of 4 boundary.  It reads 1,2,3 up to 4 bytes from the bitstream
      int num = 4 - (int)(intptr_t(msp->data) & 0x3);
      for (int i = 0; i < num; ++i)
      {
        ui64 d;
        //read a byte if the buffer is not exhausted, otherwise set it to X
        d = msp->size-- > 0 ? *msp->data++ : X;
        msp->tmp |= (d << msp->bits);      // store data in msp->tmp
        msp->bits += 8 - msp->unstuff;     // number of bits added to msp->tmp
        msp->unstuff = ((d & 0xFF) == 0xFF); // unstuffing for next byte
      }
      frwd_read<X>(msp); // read 32 bits more
    }

    //************************************************************************/
    /** @brief Consume num_bits bits from the bitstream of frwd_struct32
     *
     *  @param [in]  msp is a pointer to frwd_struct32
     *  @param [in]  num_bits is the number of bit to consume
     */
    static inline 
    void frwd_advance(frwd_struct32 *msp, ui32 num_bits)
    {
      assert(num_bits <= msp->bits);
      msp->tmp >>= num_bits;  // consume num_bits
      msp->bits -= num_bits;
    }

    //************************************************************************/
    /** @brief Fetches 32 bits from the frwd_struct32 bitstream
     *
     *  @tparam      X is the value fed in when the bitstream is exhausted.
     *               See frwd_read regarding the template
     *  @param [in]  msp is a pointer to frwd_struct32
     */
    template<int X>
    static inline 
    ui32 frwd_fetch(frwd_struct32 *msp)
    {
      if (msp->bits < 32)
      {
        frwd_read<X>(msp);
        if (msp->bits < 32) //need to test
          frwd_read<X>(msp);
      }
      return (ui32)msp->tmp;
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
    bool ojph_decode_codeblock32(ui8* coded_data, ui32* decoded_data,
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
      ui16 scratch[8 * 513] = {0};       // 8 kB

      // We need an extra two entries (one inf and one u_q) beyond
      // the last column. 
      // If the block width is 4 (2 quads), then we use sstr of 8 
      // (enough for 4 quads). If width is 8 (4 quads) we use 
      // sstr is 16 (enough for 8 quads). For a width of 16 (8 
      // quads), we use 24 (enough for 12 quads).
      ui32 sstr = ((width + 2u) + 7u) & ~7u; // multiples of 8

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
          ui16 u_q = (ui16)(1 + (uvlc_entry&7) + (tmp&~(0xFFU<<len)));//kap. 1
          sp[1] = u_q;
          u_q = (ui16)(1 + (uvlc_entry >> 3) + (tmp >> len));  //kappa == 1
          sp[3]= u_q;
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
            ui16 u_q = (ui16)((uvlc_entry & 7) + (tmp & ~(0xFFU << len)));
            sp[1] = u_q;
            u_q = (ui16)((uvlc_entry >> 3) + (tmp >> len)); // u_q
            sp[3] = u_q;
          }
          sp[0] = sp[1] = 0;
        }
      }

      // step2 we decode magsgn
      {
        // We allocate a scratch row for storing v_n values.
        // We have 512 quads horizontally.
        // We need an extra entry to handle the case of vp[1]
        // when vp is at the last column.
        // Here, we allocate 4 instead of 1 to make the buffer size
        // a multipled of 16 bytes.
        const int v_n_size = 512 + 4;
        ui32 v_n_scratch[v_n_size] = {0};  // 2+ kB

        frwd_struct32 magsgn;
        frwd_init<0xFF>(&magsgn, coded_data, lcup - scup);

        ui16 *sp = scratch;
        ui32 *vp = v_n_scratch;
        ui32 *dp = decoded_data;

        ui32 prev_v_n = 0;
        for (ui32 x = 0; x < width; sp += 2, ++vp)
        {
          ui32 inf = sp[0];
          ui32 U_q = sp[1];
          if (U_q > mmsbp2)
            return false;

          ui32 v_n;
          ui32 val = 0;
          ui32 bit = 0;
          if (inf & (1 << (4 + bit)))
          {
            //get 32 bits of magsgn data
            ui32 ms_val = frwd_fetch<0xFF>(&magsgn); 
            ui32 m_n = U_q - ((inf >> (12 + bit)) & 1); // remove e_k
            frwd_advance(&magsgn, m_n);                 //consume m_n

            val = ms_val << 31;                     // get sign bit
            v_n = ms_val & ((1 << m_n) - 1);        // keep only m_n bits
            v_n |= ((inf >> (8 + bit)) & 1) << m_n; // add EMB e_1 as MSB
            v_n |= 1;                               // add center of bin    
            //v_n now has 2 * (\mu - 1) + 0.5 with correct sign bit
            //add 2 to make it 2*\mu+0.5, shift it up to missing MSBs
            val |= (v_n + 2) << (p - 1);
          }
          dp[0] = val;

          v_n = 0;
          val = 0;
          bit = 1;
          if (inf & (1 << (4 + bit)))
          {
            //get 32 bits of magsgn data
            ui32 ms_val = frwd_fetch<0xFF>(&magsgn); 
            ui32 m_n = U_q - ((inf >> (12 + bit)) & 1); // remove e_k
            frwd_advance(&magsgn, m_n);                 //consume m_n

            val = ms_val << 31;                      // get sign bit
            v_n = ms_val & ((1 << m_n) - 1);        // keep only m_n bits
            v_n |= ((inf >> (8 + bit)) & 1) << m_n; // add EMB e_1 as MSB
            v_n |= 1;                               // add center of bin    
            //v_n now has 2 * (\mu - 1) + 0.5 with correct sign bit
            //add 2 to make it 2*\mu+0.5, shift it up to missing MSBs
            val |= (v_n + 2) << (p - 1);
          }
          dp[stride] = val;
          vp[0] = prev_v_n | v_n;
          prev_v_n = 0;
          ++dp;
          if (++x >= width)
          { ++vp; break; }

          val = 0;
          bit = 2;
          if (inf & (1 << (4 + bit)))
          {
            //get 32 bits of magsgn data
            ui32 ms_val = frwd_fetch<0xFF>(&magsgn); 
            ui32 m_n = U_q - ((inf >> (12 + bit)) & 1); // remove e_k
            frwd_advance(&magsgn, m_n);                 //consume m_n

            val = ms_val << 31;                     // get sign bit
            v_n = ms_val & ((1 << m_n) - 1);        // keep only m_n bits
            v_n |= ((inf >> (8 + bit)) & 1) << m_n; // add EMB e_1 as MSB
            v_n |= 1;                               // add center of bin    
            //v_n now has 2 * (\mu - 1) + 0.5 with correct sign bit
            //add 2 to make it 2*\mu+0.5, shift it up to missing MSBs
            val |= (v_n + 2) << (p - 1);
          }
          dp[0] = val;

          v_n = 0;
          val = 0;
          bit = 3;
          if (inf & (1 << (4 + bit)))
          {
            //get 32 bits of magsgn data
            ui32 ms_val = frwd_fetch<0xFF>(&magsgn); 
            ui32 m_n = U_q - ((inf >> (12 + bit)) & 1); // remove e_k
            frwd_advance(&magsgn, m_n);                 //consume m_n

            val = ms_val << 31;                     // get sign bit
            v_n = ms_val & ((1 << m_n) - 1);        // keep only m_n bits
            v_n |= ((inf >> (8 + bit)) & 1) << m_n; // add EMB e_1 as MSB
            v_n |= 1;                               // add center of bin    
            //v_n now has 2 * (\mu - 1) + 0.5 with correct sign bit
            //add 2 to make it 2*\mu+0.5, shift it up to missing MSBs
            val |= (v_n + 2) << (p - 1);
          }
          dp[stride] = val;
          prev_v_n = v_n;
          ++dp;
          ++x;
        }
        vp[0] = prev_v_n;

        for (ui32 y = 2; y < height; y += 2)
        {
          ui16 *sp = scratch + (y >> 1) * sstr;
          ui32 *vp = v_n_scratch;
          ui32 *dp = decoded_data + y * stride;

          prev_v_n = 0;
          for (ui32 x = 0; x < width; sp += 2, ++vp)
          {
            ui32 inf = sp[0];
            ui32 u_q = sp[1];

            ui32 gamma = inf & 0xF0; gamma &= gamma - 0x10; //is gamma_q 1?
            ui32 emax = vp[0] | vp[1];
            emax = 31 - count_leading_zeros(emax | 2); // emax - 1
            ui32 kappa = gamma ? emax : 1;

            ui32 U_q = u_q + kappa;
            if (U_q > mmsbp2)
              return false;

            ui32 v_n;
            ui32 val = 0;
            ui32 bit = 0;
            if (inf & (1 << (4 + bit)))
            {
              //get 32 bits of magsgn data
              ui32 ms_val = frwd_fetch<0xFF>(&magsgn); 
              ui32 m_n = U_q - ((inf >> (12 + bit)) & 1); // remove e_k
              frwd_advance(&magsgn, m_n);                 //consume m_n

              val = ms_val << 31;                     // get sign bit
              v_n = ms_val & ((1 << m_n) - 1);        // keep only m_n bits
              v_n |= ((inf >> (8 + bit)) & 1) << m_n; // add EMB e_1 as MSB
              v_n |= 1;                               // add center of bin    
              //v_n now has 2 * (\mu - 1) + 0.5 with correct sign bit
              //add 2 to make it 2*\mu+0.5, shift it up to missing MSBs
              val |= (v_n + 2) << (p - 1);
            }
            dp[0] = val;

            v_n = 0;
            val = 0;
            bit = 1;
            if (inf & (1 << (4 + bit)))
            {
              //get 32 bits of magsgn data
              ui32 ms_val = frwd_fetch<0xFF>(&magsgn); 
              ui32 m_n = U_q - ((inf >> (12 + bit)) & 1); // remove e_k
              frwd_advance(&magsgn, m_n);                 //consume m_n

              val = ms_val << 31;                     // get sign bit
              v_n = ms_val & ((1 << m_n) - 1);        // keep only m_n bits
              v_n |= ((inf >> (8 + bit)) & 1) << m_n; // add EMB e_1 as MSB
              v_n |= 1;                               // add center of bin    
              //v_n now has 2 * (\mu - 1) + 0.5 with correct sign bit
              //add 2 to make it 2*\mu+0.5, shift it up to missing MSBs
              val |= (v_n + 2) << (p - 1);
            }
            dp[stride] = val;
            vp[0] = prev_v_n | v_n;
            prev_v_n = 0;
            ++dp;
            if (++x >= width)
            { ++vp; break; }

            val = 0;
            bit = 2;
            if (inf & (1 << (4 + bit)))
            {
              //get 32 bits of magsgn data
              ui32 ms_val = frwd_fetch<0xFF>(&magsgn); 
              ui32 m_n = U_q - ((inf >> (12 + bit)) & 1); // remove e_k
              frwd_advance(&magsgn, m_n);                 //consume m_n

              val = ms_val << 31;                     // get sign bit
              v_n = ms_val & ((1 << m_n) - 1);        // keep only m_n bits
              v_n |= ((inf >> (8 + bit)) & 1) << m_n; // add EMB e_1 as MSB
              v_n |= 1;                               // add center of bin    
              //v_n now has 2 * (\mu - 1) + 0.5 with correct sign bit
              //add 2 to make it 2*\mu+0.5, shift it up to missing MSBs
              val |= (v_n + 2) << (p - 1);
            }
            dp[0] = val;

            v_n = 0;
            val = 0;
            bit = 3;
            if (inf & (1 << (4 + bit)))
            {
              //get 32 bits of magsgn data
              ui32 ms_val = frwd_fetch<0xFF>(&magsgn); 
              ui32 m_n = U_q - ((inf >> (12 + bit)) & 1); // remove e_k
              frwd_advance(&magsgn, m_n);                 //consume m_n

              val = ms_val << 31;                     // get sign bit
              v_n = ms_val & ((1 << m_n) - 1);        // keep only m_n bits
              v_n |= ((inf >> (8 + bit)) & 1) << m_n; // add EMB e_1 as MSB
              v_n |= 1;                               // add center of bin    
              //v_n now has 2 * (\mu - 1) + 0.5 with correct sign bit
              //add 2 to make it 2*\mu+0.5, shift it up to missing MSBs
              val |= (v_n + 2) << (p - 1);
            }
            dp[stride] = val;
            prev_v_n = v_n;
            ++dp;
            ++x;
          }
          vp[0] = prev_v_n;
        }
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
          for (y = 0; y < height; y += 4)
          {
            ui16* sp = scratch + (y >> 1) * sstr;
            ui16* dp = sigma + (y >> 2) * mstr;
            for (ui32 x = 0; x < width; x += 4, sp += 4, ++dp) {
              ui32 t0 = 0, t1 = 0;
              t0  = ((sp[0     ] & 0x30u) >> 4)  | ((sp[0     ] & 0xC0u) >> 2);
              t0 |= ((sp[2     ] & 0x30u) << 4)  | ((sp[2     ] & 0xC0u) << 6);
              t1  = ((sp[0+sstr] & 0x30u) >> 2)  | ((sp[0+sstr] & 0xC0u)     );
              t1 |= ((sp[2+sstr] & 0x30u) << 6)  | ((sp[2+sstr] & 0xC0u) << 8);
              dp[0] = (ui16)(t0 | t1);
            }
            dp[0] = 0; // set an extra entry on the right with 0
          }
          {
            // reset one row after the codeblock
            ui16* dp = sigma + (y >> 2) * mstr;
            for (ui32 x = 0; x < width; x += 4, ++dp)
              dp[0] = 0;
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

          frwd_struct32 sigprop;
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
            for (ui32 x = 0; x < width; x += 4, ++cur_sig, ++prev_sig)
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
                ui32 cwd = frwd_fetch<0>(&sigprop);

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
                  // new_sig has newly-discovered sig. samples during SPP
                  // find the signs and update decoded_data
                  ui32 *dp = dpp + x;
                  ui32 val = 3u << (p - 2);
                  col_mask = 0xFu;
                  for (int i = 0; i < 4; ++i, ++dp, col_mask <<= 4)
                  {
                    if ((col_mask & new_sig) == 0)
                      continue;

                    //scan 4 signs
                    ui32 sample_mask = 0x1111u & col_mask;
                    if (new_sig & sample_mask)
                    {
                      assert(dp[0] == 0);
                      dp[0] = (cwd << 31) | val;
                      cwd >>= 1; ++cnt;
                    }

                    sample_mask += sample_mask;
                    if (new_sig & sample_mask)
                    {
                      assert(dp[stride] == 0);
                      dp[stride] = (cwd << 31) | val;
                      cwd >>= 1; ++cnt;
                    }

                    sample_mask += sample_mask;
                    if (new_sig & sample_mask)
                    {
                      assert(dp[2 * stride] == 0);
                      dp[2 * stride] = (cwd << 31) | val;
                      cwd >>= 1; ++cnt;
                    }

                    sample_mask += sample_mask;
                    if (new_sig & sample_mask)
                    {
                      assert(dp[3 * stride] == 0);
                      dp[3 * stride] = (cwd << 31) | val;
                      cwd >>= 1; ++cnt;
                    }
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
            ui32 *cur_sig = (ui32*)(sigma + (y >> 2) * mstr);
            ui32 *dpp = decoded_data + y * stride;
            ui32 half = 1 << (p - 2);
            for (ui32 i = 0; i < width; i += 8)
            {
              //Process one entry from sigma array at a time
              // Each nibble (4 bits) in the sigma array represents 4 rows,
              // and the 32 bits contain 8 columns
              ui32 cwd = rev_fetch_mrp(&magref); // get 32 bit data
              ui32 sig = *cur_sig++; // 32 bit that will be processed now
              ui32 col_mask = 0xFu;  // a mask for a column in sig
              if (sig) // if any of the 32 bits are set
              {
                for (int j = 0; j < 8; ++j) //one column at a time
                {
                  if (sig & col_mask) // lowest nibble
                  {
                    ui32 *dp = dpp + i + j; // next column in decoded samples
                    ui32 sample_mask = 0x11111111u & col_mask; //LSB

                    for (int k = 0; k < 4; ++k) {
                      if (sig & sample_mask) //if LSB is set
                      {
                        assert(dp[0] != 0); // decoded value cannot be zero
                        assert((dp[0] & half) == 0); // no half
                        ui32 sym = cwd & 1;          // get it value
                        sym = (1 - sym) << (p - 1); // previous center of bin
                        sym |= half;            // put half the center of bin
                        dp[0] ^= sym;    // remove old bin center and put new
                        cwd >>= 1;       // consume word
                      }
                      sample_mask += sample_mask; //next row
                      dp += stride; // next samples row
                    }
                  }
                  col_mask <<= 4; //next column
                }
              }
              // consume data according to the number of bits set
              rev_advance_mrp(&magref, population_count(sig));
            }
          }
        }
      }
      return true;
    }
  }
}