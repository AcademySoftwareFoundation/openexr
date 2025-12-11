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
// File: ojph_bitbuffer_read.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_BITBUFFER_READ_H
#define OJPH_BITBUFFER_READ_H

#include "ojph_defs.h"
#include "ojph_file.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //defined elsewhere
  class mem_elastic_allocator;
  struct coded_lists;

  namespace local {


    //////////////////////////////////////////////////////////////////////////
    struct bit_read_buf
    {
      infile_base *file;
      ui32 tmp;
      int avail_bits;
      bool unstuff;
      ui32 bytes_left;
    };

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_init(bit_read_buf *bbp, ui32 bytes_left, infile_base* file)
    {
      bbp->avail_bits = 0;
      bbp->file = file;
      bbp->bytes_left = bytes_left;
      bbp->tmp = 0;
      bbp->unstuff = false;
    }

    /////////////////////////////////////////////////////////////////////////////
    static inline
    bool bb_read(bit_read_buf *bbp)
    {
      if (bbp->bytes_left > 0)
      {
        ui32 t = 0;
        if (bbp->file->read(&t, 1) != 1)
          throw "error reading from file";
        bbp->tmp = t;
        bbp->avail_bits = 8 - bbp->unstuff;
        bbp->unstuff = (t == 0xFF);
        --bbp->bytes_left;
        return true;
      }
      else
      {
        bbp->tmp = 0;
        bbp->avail_bits = 8 - bbp->unstuff;
        bbp->unstuff = false;
        return false;
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    bool bb_read_bit(bit_read_buf *bbp, ui32& bit)
    {
      bool result = true;
      if (bbp->avail_bits == 0)
        result = bb_read(bbp);
      bit = (bbp->tmp >> --bbp->avail_bits) & 1;
      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    bool bb_read_bits(bit_read_buf *bbp, int num_bits, ui32& bits)
    {
      assert(num_bits <= 32);

      bits = 0;
      bool result = true;
      while (num_bits) {
        if (bbp->avail_bits == 0)
          result = bb_read(bbp);
        int tx_bits = ojph_min(bbp->avail_bits, num_bits);
        bits <<= tx_bits;
        bbp->avail_bits -= tx_bits;
        num_bits -= tx_bits;
        bits |= (bbp->tmp >> bbp->avail_bits) & ((1 << tx_bits) - 1);
      }
      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    bool bb_read_chunk(bit_read_buf *bbp, ui32 num_bytes,
                       coded_lists*& cur_coded_list,
                       mem_elastic_allocator *elastic)
    {
      assert(bbp->avail_bits == 0 && bbp->unstuff == false);
      elastic->get_buffer(num_bytes + coded_cb_header::prefix_buf_size
        + coded_cb_header::suffix_buf_size, cur_coded_list);
      ui32 bytes = ojph_min(num_bytes, bbp->bytes_left);
      ui32 bytes_read = (ui32)bbp->file->read(
        cur_coded_list->buf + coded_cb_header::prefix_buf_size, bytes);
      if (num_bytes > bytes_read)
        memset(
          cur_coded_list->buf + coded_cb_header::prefix_buf_size + bytes_read,
          0, num_bytes - bytes_read);
      bbp->bytes_left -= bytes_read;
      return bytes_read == bytes;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_skip_eph(bit_read_buf *bbp)
    {
      if (bbp->bytes_left >= 2)
      {
        ui8 marker[2];
        if (bbp->file->read(marker, 2) != 2)
          throw "error reading from file";
        bbp->bytes_left -= 2;
        if ((int)marker[0] != (EPH >> 8) || (int)marker[1] != (EPH & 0xFF))
          throw "should find EPH, but found something else";
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    bool bb_terminate(bit_read_buf *bbp, bool uses_eph)
    {
      bool result = true;
      if (bbp->unstuff)
        result = bb_read(bbp);
      assert(bbp->unstuff == false);
      if (uses_eph)
        bb_skip_eph(bbp);
      bbp->tmp = 0;
      bbp->avail_bits = 0;
      return result;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    bool bb_skip_sop(bit_read_buf *bbp)
    {
      if (bbp->bytes_left >= 2)
      {
        ui8 marker[2];
        if (bbp->file->read(marker, 2) != 2)
          throw "error reading from file";
        if ((int)marker[0] == (SOP >> 8) && (int)marker[1] == (SOP & 0xFF))
        {
          bbp->bytes_left -= 2;
          if (bbp->bytes_left >= 4)
          {
            ui16 com_len;
            if (bbp->file->read(&com_len, 2) != 2)
              throw "error reading from file";
            com_len = swap_byte(com_len);
            if (com_len != 4)
              throw "something is wrong with SOP length";
            int result = 
              bbp->file->seek(com_len - 2, infile_base::OJPH_SEEK_CUR);
            if (result != 0)
              throw "error seeking file";
            bbp->bytes_left -= com_len;
          }
          else
            throw "precinct truncated early";
          return true;
        }
        else
        {
          //put the bytes back
          if (bbp->file->seek(-2, infile_base::OJPH_SEEK_CUR) != 0)
            throw "error seeking file";
          return false;
        }
      }

      return false;
    }

  }
}

#endif // !OJPH_BITBUFFER_READ_H