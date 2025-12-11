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
// File: ojph_bitbuffer_write.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_BITBUFFER_WRITE_H
#define OJPH_BITBUFFER_WRITE_H

#include "ojph_defs.h"
#include "ojph_file.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //defined elsewhere
  class mem_elastic_allocator;
  struct coded_lists;

  namespace local {

//////////////////////////////////////////////////////////////////////////
    struct bit_write_buf
    {
      static const int needed;

      bit_write_buf() { ccl = NULL; avail_bits = 0; tmp = 0; }
      coded_lists* ccl;
      int avail_bits;
      ui64 tmp;
    };

    //////////////////////////////////////////////////////////////////////////
    const int bit_write_buf::needed = 512;

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_expand_buf(bit_write_buf *bbp, mem_elastic_allocator *elastic,
                       coded_lists*& cur_coded_list)
    {
      assert(cur_coded_list == NULL);
      elastic->get_buffer(bit_write_buf::needed, cur_coded_list);
      bbp->ccl = cur_coded_list;
      bbp->tmp = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_init(bit_write_buf *bbp, mem_elastic_allocator *elastic,
                 coded_lists*& cur_coded_list)
    {
      bb_expand_buf(bbp, elastic, cur_coded_list);
      bbp->avail_bits = 8;
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_put_bit(bit_write_buf *bbp, ui32 bit,
                    mem_elastic_allocator *elastic,
                    coded_lists*& cur_coded_list, ui32& ph_bytes)
    {
      --bbp->avail_bits;
      bbp->tmp |= (bit & 1) << bbp->avail_bits;
      if (bbp->avail_bits <= 0)
      {
        bbp->avail_bits = 8 - (bbp->tmp != 0xFF ? 0 : 1);
        bbp->ccl->buf[bbp->ccl->buf_size - bbp->ccl->avail_size] =
          (ui8)(bbp->tmp & 0xFF);
        bbp->tmp = 0;
        --bbp->ccl->avail_size;
        if (bbp->ccl->avail_size == 0)
        {
          bb_expand_buf(bbp, elastic, cur_coded_list->next_list);
          cur_coded_list = cur_coded_list->next_list;
          ph_bytes += bit_write_buf::needed;
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_put_zeros(bit_write_buf *bbp, int num_zeros,
                      mem_elastic_allocator *elastic,
                      coded_lists*& cur_coded_list, ui32& ph_bytes)
    {
      for (int i = num_zeros; i > 0; --i)
        bb_put_bit(bbp, 0, elastic, cur_coded_list, ph_bytes);
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_put_bits(bit_write_buf *bbp, ui32 data, int num_bits,
                     mem_elastic_allocator *elastic,
                     coded_lists*& cur_coded_list, ui32& ph_bytes)
    {
      assert(num_bits <= 32);
      for (int i = num_bits - 1; i >= 0; --i) 
        bb_put_bit(bbp, data >> i, elastic, cur_coded_list, ph_bytes);
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void bb_terminate(bit_write_buf *bbp)
    {
      if (bbp->avail_bits < 8) //bits have been written
      {
        ui8 val = (ui8)(bbp->tmp & 0xFF);
        bbp->ccl->buf[bbp->ccl->buf_size - bbp->ccl->avail_size] = val;
        --bbp->ccl->avail_size;
      }
    }

  }
}
#endif // !OJPH_BITBUFFER_WRITE_H