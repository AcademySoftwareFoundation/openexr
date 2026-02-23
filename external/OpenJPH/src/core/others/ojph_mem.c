//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2025, Aous Naman
// Copyright (c) 2025, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2025, The University of New South Wales, Australia
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
// File: ojph_mem.c
// Author: Aous Naman
// Date: 17 October 2025
//***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////
// OS detection definitions for C only
////////////////////////////////////////////////////////////////////////////////
#if (defined WIN32) || (defined _WIN32) || (defined _WIN64)
#define OJPH_OS_WINDOWS
#elif (defined __APPLE__)
#define OJPH_OS_APPLE
#elif (defined __ANDROID__)
#define OJPH_OS_ANDROID
#elif (defined __linux)
#define OJPH_OS_LINUX
#endif

////////////////////////////////////////////////////////////////////////////////
// Defines for dll in C only
////////////////////////////////////////////////////////////////////////////////
#if defined(OJPH_OS_WINDOWS) && defined(OJPH_BUILD_SHARED_LIBRARY)
#define OJPH_EXPORT __declspec(dllexport)
#else
#define OJPH_EXPORT
#endif

////////////////////////////////////////////////////////////////////////////////
#ifdef OJPH_OS_WINDOWS
  OJPH_EXPORT void* ojph_aligned_malloc(size_t alignment, size_t size)
  {
    assert(alignment != 0 && (alignment & (alignment - 1)) == 0);
    return _aligned_malloc(size, alignment);
  }

  OJPH_EXPORT void ojph_aligned_free(void* pointer)
  {
    _aligned_free(pointer);
  }
#elif (defined OJPH_ALIGNED_ALLOC_EXISTS)
  void* ojph_aligned_malloc(size_t alignment, size_t size)
  {
    assert(alignment != 0 && (alignment & (alignment - 1)) == 0);
    return aligned_alloc(alignment, size);
  }

  void ojph_aligned_free(void* pointer)
  {
    free(pointer);
  }
#elif (defined OJPH_POSIX_MEMALIGN_EXISTS)
  void* ojph_aligned_malloc(size_t alignment, size_t size)
  {
    assert(alignment != 0 && (alignment & (alignment - 1)) == 0);
    void *p = NULL;
    int e = posix_memalign(&p, alignment, size);
    return (e ? NULL : p);
  }

  void ojph_aligned_free(void* pointer)
  {
    free(pointer);
  }
#else
  void* ojph_aligned_malloc(size_t alignment, size_t size)
  {
    assert(alignment != 0 && (alignment & (alignment - 1)) == 0);

    // emulate aligned_alloc
    void* orig_ptr = malloc(size + alignment + sizeof(void*));
    if (orig_ptr == NULL)
      return NULL; // Allocation failed

    uintptr_t start_of_mem = (uintptr_t)orig_ptr + sizeof(void*);
    uintptr_t aligned_addr = (start_of_mem + alignment - 1) & ~(alignment - 1);

    void** ptr_to_orig_ptr = (void**)aligned_addr;
    ptr_to_orig_ptr[-1] = orig_ptr;

    return (void*)aligned_addr;
  }

  void ojph_aligned_free(void* pointer)
  {
    if (pointer) {
      // Retrieve the original pointer stored just before aligned pointer
      void** ptr_to_orig_ptr = (void**)pointer;
      void* orig_ptr = ptr_to_orig_ptr[-1];

      free(orig_ptr);
    }
  }
#endif
