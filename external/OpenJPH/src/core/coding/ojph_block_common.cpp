/***************************************************************************/
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
// File: ojph_block_common.cpp
// Author: Aous Naman
// Date: 13 May 2022
//***************************************************************************/

#include <cassert>
#include <cstddef>
#include <cstring>
#include "ojph_block_common.h"

//***************************************************************************/
/** @file ojph_block_common.cpp
 *  @brief defines tables used for decoding HTJ2K blocks
 */

namespace ojph {
  namespace local {

    //************************************************************************/
    /** @defgroup vlc_decoding_tables_grp VLC decoding tables
     *  @{
     *  VLC decoding tables used in decoding VLC codewords to these fields:  \n
     *  \li \c cwd_len : 3bits -> the codeword length of the VLC codeword;    
     *                   the VLC cwd is in the LSB of bitstream              \n
     *  \li \c u_off   : 1bit  -> u_offset, which is 1 if u value is not 0   \n
     *  \li \c rho     : 4bits -> significant samples within a quad           \n
     *  \li \c e_1     : 4bits -> EMB e_1                                    \n
     *  \li \c e_k     : 4bits -> EMB e_k                                    \n
     *                                                                       \n
     *  The table index is 10 bits and composed of two parts:                \n
     *  The 7 LSBs contain a codeword which might be shorter than 7 bits;    
     *  this word is the next decodable bits in the bitstream.                \n
     *  The 3 MSB is the context of for the codeword.                        \n
     */

    /// @brief vlc_tbl0 contains decoding information for initial row of quads
    ui16 vlc_tbl0[1024] = { 0 };
    /// @brief vlc_tbl1 contains decoding information for non-initial row of 
    ///        quads
    ui16 vlc_tbl1[1024] = { 0 };
    /// @}

    //************************************************************************/
    /** @defgroup uvlc_decoding_tables_grp VLC decoding tables
     *  @{
     *  UVLC decoding tables used to partially decode u values from UVLC     
     *  codewords.                                                           \n
     *  The table index is 8 (or 9)  bits and composed of two parts:         \n
     *  The 6 LSBs carries the head of the VLC to be decoded. Up to 6 bits to 
     *  be used; these are uvlc prefix code for quad 0 and 1                 \n
     *  The 2 (or 3) MSBs contain u_off of quad 0 + 2 * o_off quad 1
     *  + 4 * mel event for initial row of quads when needed                 \n
     *                                                                       \n
     *  Each entry contains, starting from the LSB                           \n
     *  \li \c total total prefix length for quads 0 and 1 (3 bits)          \n
     *  \li \c total total suffix length for quads 0 and 1 (4 bits)          \n
     *  \li \c suffix length for quad 0 (3 bits)                             \n
     *  \li \c prefix for quad 0 (3 bits)                                    \n
     *  \li \c prefix for quad 1 (3 bits)                                    \n
     *                                                                       \n
     *  Another table is uvlc_bias, which is needed to correctly decode the 
     *  extension u_ext for initial row of quads. Under certain condition,
     *  we deduct 1 or 2 from u_q0 and u_q1 before encoding them; so for us 
     *  to know that decoding u_ext is needed, we recreate the u_q0 and u_q1
     *  that we actually encoded.                                            \n
     *  For simplicity, we use the same index as before                      \n
     *  \li \c u_q0 bias is 2 bits                                           \n
     *  \li \c u_q1 bias is 2 bits                                           \n
     */

    /// @brief uvlc_tbl0 contains decoding information for initial row of quads
    ui16 uvlc_tbl0[256+64] = { 0 };
    /// @brief uvlc_tbl1 contains decoding information for non-initial row of 
    ///        quads
    ui16 uvlc_tbl1[256] = { 0 };
    /// @brief uvlc_bias contains decoding info. for initial row of quads
    ui8 uvlc_bias[256+64] = { 0 };
    /// @}

    //************************************************************************/
    /** @ingroup vlc_decoding_tables_grp
     *  @brief Initializes vlc_tbl0 and vlc_tbl1 tables, from table0.h and
     *         table1.h
     */
    static bool vlc_init_tables()
    {
      const bool debug = false; //useful for checking 

      //Data in the table is arranged in this format (taken from the standard)
      // c_q is the context for a quad
      // rho is the significance pattern for a quad
      // u_off indicate if u value is 0 (u_off is 0), or communicated
      // e_k, e_1 EMB patterns
      // cwd VLC codeword
      // cwd VLC codeword length
      struct vlc_src_table { int c_q, rho, u_off, e_k, e_1, cwd, cwd_len; };
      // initial quad rows
      vlc_src_table tbl0[] = {
    #include "table0.h"
      };
      // number of entries in the table
      size_t tbl0_size = sizeof(tbl0) / sizeof(vlc_src_table); 

      // nono-initial quad rows
      vlc_src_table tbl1[] = {
    #include "table1.h"
      };
      // number of entries in the table
      size_t tbl1_size = sizeof(tbl1) / sizeof(vlc_src_table);

      if (debug) memset(vlc_tbl0, 0, sizeof(vlc_tbl0)); //unnecessary

      // this is to convert table entries into values for decoder look up
      // There can be at most 1024 possibilities, not all of them are valid.
      // 
      for (int i = 0; i < 1024; ++i)
      {
        int cwd = i & 0x7F; // from i extract codeword
        int c_q = i >> 7;   // from i extract context
        // See if this case exist in the table, if so then set the entry in
        // vlc_tbl0
        for (size_t j = 0; j < tbl0_size; ++j) 
          if (tbl0[j].c_q == c_q) // this is an and operation
            if (tbl0[j].cwd == (cwd & ((1 << tbl0[j].cwd_len) - 1)))
            {
              if (debug) assert(vlc_tbl0[i] == 0);
              // Put this entry into the table
              vlc_tbl0[i] = (ui16)((tbl0[j].rho << 4) | (tbl0[j].u_off << 3)
                | (tbl0[j].e_k << 12) | (tbl0[j].e_1 << 8) | tbl0[j].cwd_len);
            }
      }

      if (debug) memset(vlc_tbl1, 0, sizeof(vlc_tbl1)); //unnecessary

      // this the same as above but for non-initial rows
      for (int i = 0; i < 1024; ++i)
      {
        int cwd = i & 0x7F; //7 bits
        int c_q = i >> 7;
        for (size_t j = 0; j < tbl1_size; ++j)
          if (tbl1[j].c_q == c_q) // this is an and operation
            if (tbl1[j].cwd == (cwd & ((1 << tbl1[j].cwd_len) - 1)))
            {
              if (debug) assert(vlc_tbl1[i] == 0);
              vlc_tbl1[i] = (ui16)((tbl1[j].rho << 4) | (tbl1[j].u_off << 3)
                | (tbl1[j].e_k << 12) | (tbl1[j].e_1 << 8) | tbl1[j].cwd_len);
            }
      }

      return true;
    }

    //************************************************************************/
    /** @ingroup uvlc_decoding_tables_grp
     *  @brief Initializes uvlc_tbl0 and uvlc_tbl1 tables
     */
    static bool uvlc_init_tables()
    {
      // table stores possible decoding three bits from vlc
      // there are 8 entries for xx1, x10, 100, 000, where x means do not
      // care table value is made up of
      // 2 bits in the LSB for prefix length 
      // 3 bits for suffix length
      // 3 bits in the MSB for prefix value (u_pfx in Table 3 of ITU T.814)
      static const ui8 dec[8] = { // the index is the prefix codeword
        3 | (5 << 2) | (5 << 5), //000 == 000, prefix codeword "000"
        1 | (0 << 2) | (1 << 5), //001 == xx1, prefix codeword "1"
        2 | (0 << 2) | (2 << 5), //010 == x10, prefix codeword "01"
        1 | (0 << 2) | (1 << 5), //011 == xx1, prefix codeword "1"
        3 | (1 << 2) | (3 << 5), //100 == 100, prefix codeword "001"
        1 | (0 << 2) | (1 << 5), //101 == xx1, prefix codeword "1"
        2 | (0 << 2) | (2 << 5), //110 == x10, prefix codeword "01"
        1 | (0 << 2) | (1 << 5)  //111 == xx1, prefix codeword "1"
      };

      for (ui32 i = 0; i < 256 + 64; ++i)
      { 
        ui32 mode = i >> 6;
        ui32 vlc = i & 0x3F;

        if (mode == 0) {      // both u_off are 0
          uvlc_tbl0[i] = 0;
          uvlc_bias[i] = 0;
        }
        else if (mode <= 2) // u_off are either 01 or 10
        {
          ui32 d = dec[vlc & 0x7];   //look at the least significant 3 bits

          ui32 total_prefix = d & 0x3;
          ui32 total_suffix = (d >> 2) & 0x7;
          ui32 u0_suffix_len = (mode == 1) ? total_suffix : 0;
          ui32 u0 = (mode == 1) ? (d >> 5) : 0;
          ui32 u1 = (mode == 1) ? 0 : (d >> 5);

          uvlc_tbl0[i] = (ui16)(total_prefix | 
                               (total_suffix << 3) |
                               (u0_suffix_len << 7) |
                               (u0 << 10) |
                               (u1 << 13));
          
        }
        else if (mode == 3) // both u_off are 1, and MEL event is 0
        {
          ui32 d0 = dec[vlc & 0x7];  // LSBs of VLC are prefix codeword
          vlc >>= d0 & 0x3;          // Consume bits
          ui32 d1 = dec[vlc & 0x7];  // LSBs of VLC are prefix codeword

          ui32 total_prefix, u0_suffix_len, total_suffix, u0, u1;
          if ((d0 & 0x3) == 3)
          {
            total_prefix = (d0 & 0x3) + 1;
            u0_suffix_len = (d0 >> 2) & 0x7;
            total_suffix = u0_suffix_len;
            u0 = d0 >> 5;
            u1 = (vlc & 1) + 1;
            uvlc_bias[i] = 4; // 0b00 for u0 and 0b01 for u1
          }
          else
          {
            total_prefix = (d0 & 0x3) + (d1 & 0x3);
            u0_suffix_len = (d0 >> 2) & 0x7;
            total_suffix = u0_suffix_len + ((d1 >> 2) & 0x7);
            u0 = d0 >> 5;
            u1 = d1 >> 5;
            uvlc_bias[i] = 0;
          }

          uvlc_tbl0[i] = (ui16)(total_prefix | 
                               (total_suffix << 3) |
                               (u0_suffix_len << 7) |
                               (u0 << 10) |
                               (u1 << 13));
        }
        else if (mode == 4) // both u_off are 1, and MEL event is 1
        {
          ui32 d0 = dec[vlc & 0x7];  // LSBs of VLC are prefix codeword
          vlc >>= d0 & 0x3;          // Consume bits
          ui32 d1 = dec[vlc & 0x7];  // LSBs of VLC are prefix codeword

          ui32 total_prefix = (d0 & 0x3) + (d1 & 0x3);
          ui32 u0_suffix_len = (d0 >> 2) & 0x7;
          ui32 total_suffix = u0_suffix_len + ((d1 >> 2) & 0x7);
          ui32 u0 = (d0 >> 5) + 2;
          ui32 u1 = (d1 >> 5) + 2;

          uvlc_tbl0[i] = (ui16)(total_prefix | 
                               (total_suffix << 3) |
                               (u0_suffix_len << 7) |
                               (u0 << 10) |
                               (u1 << 13));
          uvlc_bias[i] = 10; // 0b10 for u0 and 0b10 for u1
        }
      }

      for (ui32 i = 0; i < 256; ++i)
      {
        ui32 mode = i >> 6;
        ui32 vlc = i & 0x3F;

        if (mode == 0)       // both u_off are 0
          uvlc_tbl1[i] = 0;
        else if (mode <= 2)  // u_off are either 01 or 10
        {
          ui32 d = dec[vlc & 0x7];   // look at the 3 LSB bits

          ui32 total_prefix = d & 0x3;
          ui32 total_suffix = (d >> 2) & 0x7;
          ui32 u0_suffix_len = (mode == 1) ? total_suffix : 0;
          ui32 u0 = (mode == 1) ? (d >> 5) : 0;
          ui32 u1 = (mode == 1) ? 0 : (d >> 5);

          uvlc_tbl1[i] = (ui16)(total_prefix | 
                               (total_suffix << 3) |
                               (u0_suffix_len << 7) |
                               (u0 << 10) |
                               (u1 << 13));
        }
        else if (mode == 3) // both u_off are 1
        {
          ui32 d0 = dec[vlc & 0x7];  // LSBs of VLC are prefix codeword
          vlc >>= d0 & 0x3;          // Consume bits
          ui32 d1 = dec[vlc & 0x7];  // LSBs of VLC are prefix codeword

          ui32 total_prefix = (d0 & 0x3) + (d1 & 0x3);
          ui32 u0_suffix_len = (d0 >> 2) & 0x7;
          ui32 total_suffix = u0_suffix_len + ((d1 >> 2) & 0x7);
          ui32 u0 = d0 >> 5;
          ui32 u1 = d1 >> 5;

          uvlc_tbl1[i] = (ui16)(total_prefix | 
                               (total_suffix << 3) |
                               (u0_suffix_len << 7) |
                               (u0 << 10) |
                               (u1 << 13));
        }
      }
      return true;
    }

    //************************************************************************/
    /** @ingroup vlc_decoding_tables_grp
     *  @brief Initializes VLC tables vlc_tbl0 and vlc_tbl1
     */
    static bool vlc_tables_initialized = vlc_init_tables();

    //************************************************************************/
    /** @ingroup uvlc_decoding_tables_grp
     *  @brief Initializes UVLC tables uvlc_tbl0 and uvlc_tbl1
     */
    static bool uvlc_tables_initialized = uvlc_init_tables();

  } // !namespace local
} // !namespace ojph
