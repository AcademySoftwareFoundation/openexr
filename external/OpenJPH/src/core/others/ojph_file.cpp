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
// File: ojph_file.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


/** @file ojph_file.cpp
 *  @brief contains implementations of classes related to file operations
 */

#include <cassert>
#include <cstddef>

#include "ojph_mem.h"
#include "ojph_file.h"
#include "ojph_message.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void j2c_outfile::open(const char *filename)
  {
    assert(fh == 0);
    fh = fopen(filename, "wb");
    if (fh == NULL)
      OJPH_ERROR(0x00060001, "failed to open %s for writing", filename);
  }

  ////////////////////////////////////////////////////////////////////////////
  size_t j2c_outfile::write(const void *ptr, size_t size)
  {
    assert(fh);
    return fwrite(ptr, 1, size, fh);
  }

  ////////////////////////////////////////////////////////////////////////////
  si64 j2c_outfile::tell()
  {
    assert(fh);
    return ojph_ftell(fh);
  }

  ////////////////////////////////////////////////////////////////////////////
  void j2c_outfile::flush()
  {
    assert(fh);
    fflush(fh);
  }

  ////////////////////////////////////////////////////////////////////////////
  void j2c_outfile::close()
  {
    assert(fh);
    fclose(fh);
    fh = NULL;
  }

  //*************************************************************************/
  // mem_outfile
  //*************************************************************************/

  /**  */
  mem_outfile::mem_outfile()
  {
    is_open = clear_mem = false;
    buf_size = used_size = 0;
    buf = cur_ptr = NULL;
  }

  /**  */
  mem_outfile::~mem_outfile()
  {
    if (buf)
      ojph_aligned_free(buf);
    is_open = clear_mem = false;
    buf_size = used_size = 0;
    buf = cur_ptr = NULL;
  }

  /**  */
  void mem_outfile::open(size_t initial_size, bool clear_mem)
  {
    assert(this->is_open == false);
    assert(this->cur_ptr == this->buf);

    // do initial buffer allocation or buffer expansion
    this->is_open = true;
    this->clear_mem = clear_mem;
    expand_storage(initial_size, this->clear_mem);
    this->used_size = 0;
    this->cur_ptr = this->buf;
  }

  /**  */
  void mem_outfile::close() {
    is_open = false;
    cur_ptr = buf;
  }

  /** The seek function expands the buffer whenever offset goes beyond
   *  the buffer end
   */
  int mem_outfile::seek(si64 offset, enum outfile_base::seek origin)
  {
    if (origin == OJPH_SEEK_SET)
      ; // do nothing
    else if (origin == OJPH_SEEK_CUR)
      offset += tell();
    else if (origin == OJPH_SEEK_END)
      offset += (si64)used_size;
    else {
      assert(0);
      return -1;
    }

    if (offset < 0)  // offset before the start of file
      return -1;

    expand_storage((size_t)offset, false); // See if expansion is needed

    cur_ptr = buf + offset;
    return 0;
  }

  /** Whenever the need arises, the buffer is expanded by a factor approx 1.5x
   */
  size_t mem_outfile::write(const void *ptr, size_t new_size)
  {
    assert(this->is_open);
    assert(this->buf_size);
    assert(this->buf);
    assert(this->cur_ptr);

    // expand buffer if needed to make sure it has room for this write
    size_t needed_size = (size_t)tell() + new_size; //needed size
    expand_storage(needed_size, false);

    // copy bytes into buffer and adjust cur_ptr
    memcpy(this->cur_ptr, ptr, new_size);
    cur_ptr += new_size;
    used_size = ojph_max(used_size, (size_t)tell());

    return new_size;
  }

  /** */
  void mem_outfile::write_to_file(const char *file_name) const
  {
    assert(is_open == false);
    FILE *f = fopen(file_name, "wb");
    if (f == NULL)
      OJPH_ERROR(0x00060003, "failed to open %s for writing", file_name);
    if (f != NULL)
      if (fwrite(this->buf, 1, used_size, f) != used_size)
        OJPH_ERROR(0x00060004, "failed writing to %s", file_name);
    fclose(f);
  }

  /** */
  void mem_outfile::expand_storage(size_t needed_size, bool clear_all)
  {
    if (needed_size > buf_size)
    {
      needed_size += (needed_size + 1) >> 1; // x1.5
      // expand buffer to multiples of (ALIGNED_ALLOC_MASK + 1)
      needed_size = (needed_size + ALIGNED_ALLOC_MASK) & (~ALIGNED_ALLOC_MASK);

      ui8* new_buf;
      new_buf = (ui8*)ojph_aligned_malloc(ALIGNED_ALLOC_MASK + 1, needed_size);
      if (new_buf == NULL)
        OJPH_ERROR(0x00060005, "failed to allocate memory (%zu bytes)",
          needed_size);

      if (this->buf != NULL)
      {
        if (!clear_all)
          memcpy(new_buf, this->buf, used_size);
        ojph_aligned_free(this->buf);
      }
      this->cur_ptr = new_buf + tell();
      this->buf = new_buf;

      if (clear_mem && !clear_all) // will be cleared later
        memset(this->buf + buf_size, 0, needed_size - this->buf_size);
      this->buf_size = needed_size;
    }
    if (clear_all)
      memset(this->buf, 0, this->buf_size);
  }


  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void j2c_infile::open(const char *filename)
  {
    assert(fh == NULL);
    fh = fopen(filename, "rb");
    if (fh == NULL)
      OJPH_ERROR(0x00060002, "failed to open %s for reading", filename);
  }

  ////////////////////////////////////////////////////////////////////////////
  size_t j2c_infile::read(void *ptr, size_t size)
  {
    assert(fh);
    return fread(ptr, 1, size, fh);
  }

  ////////////////////////////////////////////////////////////////////////////
  int j2c_infile::seek(si64 offset, enum infile_base::seek origin)
  {
    assert(fh);
    return ojph_fseek(fh, offset, origin);
  }

  ////////////////////////////////////////////////////////////////////////////
  si64 j2c_infile::tell()
  {
    assert(fh);
    return ojph_ftell(fh);
  }

  ////////////////////////////////////////////////////////////////////////////
  void j2c_infile::close()
  {
    assert(fh);
    fclose(fh);
    fh = NULL;
  }


  ////////////////////////////////////////////////////////////////////////////
  //
  //
  //
  //
  //
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  void mem_infile::open(const ui8* data, size_t size)
  {
    assert(this->data == NULL);
    cur_ptr = this->data = data;
    this->size = size;
  }

  ////////////////////////////////////////////////////////////////////////////
  size_t mem_infile::read(void *ptr, size_t size)
  {
    std::ptrdiff_t bytes_left = (data + this->size) - cur_ptr;
    if (bytes_left > 0)
    {
      size_t bytes_to_read = ojph_min(size, (size_t)bytes_left);
      memcpy(ptr, cur_ptr, bytes_to_read);
      cur_ptr += bytes_to_read;
      return bytes_to_read;
    }
    else
      return 0;
  }

  ////////////////////////////////////////////////////////////////////////////
  int mem_infile::seek(si64 offset, enum infile_base::seek origin)
  {
    int result = -1;
    if (origin == OJPH_SEEK_SET)
    {
      if (offset >= 0 && (size_t)offset <= size)
      {
        cur_ptr = data + offset;
        result = 0;
      }
    }
    else if (origin == OJPH_SEEK_CUR)
    {
      si64 bytes_off = (si64)(cur_ptr - data) + offset;
      if (bytes_off >= 0 && (size_t)bytes_off <= size)
      {
        cur_ptr = data + bytes_off;
        result = 0;
      }
    }
    else if (origin == OJPH_SEEK_END)
    {
      if (offset <= 0 && (std::ptrdiff_t)size + offset >= 0)
      {
        cur_ptr = data + size + offset;
        result = 0;
      }
    }
    else
      assert(0);

    return result;
  }


}
