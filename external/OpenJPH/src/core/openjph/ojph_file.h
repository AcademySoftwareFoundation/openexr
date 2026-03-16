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
// File: ojph_file.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_FILE_H
#define OJPH_FILE_H

#include <cstdlib>
#include <cstdio>

#include "ojph_arch.h"

namespace ojph {

  ////////////////////////////////////////////////////////////////////////////
#ifdef OJPH_OS_WINDOWS
  int inline ojph_fseek(FILE* stream, si64 offset, int origin)
  {
    return _fseeki64(stream, offset, origin);
  }

  si64 inline ojph_ftell(FILE* stream)
  {
    return _ftelli64(stream);
  }
#else
  int inline ojph_fseek(FILE* stream, si64 offset, int origin)
  {
    return fseeko(stream, offset, origin);
  }

  si64 inline ojph_ftell(FILE* stream)
  {
    return ftello(stream);
  }
#endif


  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT outfile_base
  {
  public:
  public:
    enum seek : int {
      OJPH_SEEK_SET = SEEK_SET,
      OJPH_SEEK_CUR = SEEK_CUR,
      OJPH_SEEK_END = SEEK_END
    };
    virtual ~outfile_base() {}

    virtual size_t write(const void *ptr, size_t size) = 0;
    virtual si64 tell() { return 0; }
    virtual int seek(si64 offset, enum outfile_base::seek origin)
    {
      ojph_unused(offset); ojph_unused(origin);
      return -1; /* always fail, to remind you to write an implementation */
    }
    virtual void flush() {}
    virtual void close() {}
  };

  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT j2c_outfile : public outfile_base
  {
  public:
    j2c_outfile() { fh = 0; }
    ~j2c_outfile() override { if (fh) fclose(fh); }

    void open(const char *filename);
    size_t write(const void *ptr, size_t size) override;
    si64 tell() override;
    void flush() override;
    void close() override;

  private:
    FILE *fh;
  };

  //*************************************************************************/
  /**  @brief mem_outfile stores encoded j2k codestreams in memory
   *
   *  This code was first developed by Chris Hafey https://github.com/chafey
   *  I took the code and integrated with OpenJPH, with some modifications.
   *
   *  This class serves as a memory-based file storage.
   *  For example, generated j2k codestream is stored in memory
   *  instead of a conventional file. The memory buffer associated with
   *  this class grows with the addition of new data.
   *
   *  memory data can be accessed using get_data()
   */
  class OJPH_EXPORT mem_outfile : public outfile_base
  {
  public:
    /**  A constructor */
    mem_outfile();
    /**  A destructor */
    ~mem_outfile() override;

    /**
     *  @brief Call this function to open a memory file.
	   *
     *  This function creates a memory buffer to be used for storing
     *  the generated j2k codestream.
     *
     *  @param initial_size is the initial memory buffer size.
     *         The default value is 2^16.
     *  @param clear_mem if set to true, all allocated memory is reset to 0
     */
    void open(size_t initial_size = 65536, bool clear_mem = false);

    /**
     *  @brief Call this function to write data to the memory file.
	   *
     *  This function adds new data to the memory file.  The memory buffer
     *  of the file grows as needed.
     *
     *  @param ptr is a pointer to new data.
     *  @param size the number of bytes in the new data.
     */
    size_t write(const void *ptr, size_t size) override;

    /**
     *  @brief Call this function to know the file size (i.e., number of
     *         bytes used to store the file).
     *
     *  @return the file size.
     */
    si64 tell() override { return cur_ptr - buf; }

    /**
     *  @brief Call this function to change write pointer location; the
     *         function can expand file storage.
     *
     *  @return 0 on success, non-zero otherwise (not used).
     */
    int seek(si64 offset, enum outfile_base::seek origin) override;

    /** Call this function to close the file and deallocate memory
	   *
     *  The object can be used again after calling close
     */
    void close() override;

    /**
     *  @brief Call this function to access memory file data.
	   *
     *  It is not recommended to store the returned value because buffer
     *  storage address can change between write calls.
     *
     *  @return a constant pointer to the data.
     */
    const ui8* get_data() { return buf; }

    /**
     *  @brief Call this function to access memory file data (for const
     *         objects)
	   *
     *  This is similar to the above function, except that it can be used
     *  with constant objects.
     *
     *  @return a constant pointer to the data.
     */
    const ui8* get_data() const { return buf; }

    /**
     *  @brief Call this function to write the memory file data to a file
	   *
     */
    void write_to_file(const char *file_name) const;

    /**
     *  @brief Call this function to get the used size of the memory file.
     *
     *  @return the used size of the memory file in bytes.
     */
    size_t get_used_size() const { return used_size; }

    /**
     *  @brief Call this function to get the total buffer size of the memory
     *         file including unused space (this is the allocated memory).
     *
     *  @return the full size of the memory file in bytes.
     */
     size_t get_buf_size() const { return buf_size; }

  private:
    /**
     *  @brief This function expands storage by x1.5 needed space.
     *
     *  It sets cur_ptr correctly, and clears the extended area of the
     *  buffer.  It optionally clear the whole buffer
     *
     * @param new_size   New size of the buffer
     * @param clear_all  Set to true to clear whole buffer, not just expansion
     */
    void expand_storage(size_t new_size, bool clear_all);

  private:
    bool is_open;
    bool clear_mem;
    size_t buf_size;
    size_t used_size;
    ui8 *buf;
    ui8 *cur_ptr;

  private:
    static const size_t ALIGNED_ALLOC_MASK = 4096 - 1;
  };

  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT infile_base
  {
  public:
    enum seek : int {
      OJPH_SEEK_SET = SEEK_SET,
      OJPH_SEEK_CUR = SEEK_CUR,
      OJPH_SEEK_END = SEEK_END
    };

    virtual ~infile_base() {}

    //read reads size bytes, returns the number of bytes read
    virtual size_t read(void *ptr, size_t size) = 0;
    //seek returns 0 on success
    virtual int seek(si64 offset, enum infile_base::seek origin) = 0;
    virtual si64 tell() = 0;
    virtual bool eof() = 0;
    virtual void close() {}
  };

  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT j2c_infile : public infile_base
  {
  public:
    j2c_infile() { fh = 0; }
    ~j2c_infile() override { if (fh) fclose(fh); }

    void open(const char *filename);

    //read reads size bytes, returns the number of bytes read
    size_t read(void *ptr, size_t size) override;
    //seek returns 0 on success
    int seek(si64 offset, enum infile_base::seek origin) override;
    si64 tell() override;
    bool eof() override { return feof(fh) != 0; }
    void close() override;

  private:
    FILE *fh;
  };

  ////////////////////////////////////////////////////////////////////////////
  class OJPH_EXPORT mem_infile : public infile_base
  {
  public:
    mem_infile() { close(); }
    ~mem_infile() override { }

    void open(const ui8* data, size_t size);

    //read reads size bytes, returns the number of bytes read
    size_t read(void *ptr, size_t size) override;
    //seek returns 0 on success
    int seek(si64 offset, enum infile_base::seek origin) override;
    si64 tell() override { return cur_ptr - data; }
    bool eof() override { return cur_ptr >= data + size; }
    void close() override { data = cur_ptr = NULL; size = 0; }

  private:
    const ui8 *data, *cur_ptr;
    size_t size;
  };


}

#endif // !OJPH_FILE_H
