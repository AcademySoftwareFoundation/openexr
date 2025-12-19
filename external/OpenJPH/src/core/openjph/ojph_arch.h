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
// File: ojph_arch.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_ARCH_H
#define OJPH_ARCH_H

#include <cstdio>
#include <cstdint>
#include <cmath>

#include "ojph_defs.h"


///////////////////////////////////////////////////////////////////////////////
// preprocessor directives for compiler
///////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#define OJPH_COMPILER_MSVC
#elif (defined __GNUC__)
#define OJPH_COMPILER_GNUC
#endif

#ifdef __EMSCRIPTEN__
#define OJPH_EMSCRIPTEN
#endif

#ifdef OJPH_COMPILER_MSVC
#include <intrin.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// preprocessor directives for architecture
///////////////////////////////////////////////////////////////////////////////
#if defined(__arm__) || defined(__TARGET_ARCH_ARM)  \
  || defined(__aarch64__) || defined(_M_ARM64)
  #define OJPH_ARCH_ARM
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
  #define OJPH_ARCH_I386
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64) \
  || defined(_M_X64)
  #define OJPH_ARCH_X86_64
#elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)
  #define OJPH_ARCH_IA64
#elif defined(__ppc__) || defined(__ppc) || defined(__powerpc__)  \
  || defined(_ARCH_COM) || defined(_ARCH_PWR) || defined(_ARCH_PPC)  \
  || defined(_M_MPPC) || defined(_M_PPC)
  #if defined(__ppc64__) || defined(__powerpc64__) || defined(__64BIT__)
    #define OJPH_ARCH_PPC64
  #else
    #define OJPH_ARCH_PPC
  #endif
#else
  #define OJPH_ARCH_UNKNOWN
#endif

namespace ojph {
  ////////////////////////////////////////////////////////////////////////////
  //                  disable SIMD for unknown architecture
  ////////////////////////////////////////////////////////////////////////////
#if !defined(OJPH_ARCH_X86_64) && !defined(OJPH_ARCH_I386) &&  \
    !defined(OJPH_ARCH_ARM) && !defined(OJPH_DISABLE_SIMD)
#define OJPH_DISABLE_SIMD
#endif // !OJPH_ARCH_UNKNOWN

  ////////////////////////////////////////////////////////////////////////////
  //                         OS detection definitions
  ////////////////////////////////////////////////////////////////////////////
#if (defined WIN32) || (defined _WIN32) || (defined _WIN64)
#define OJPH_OS_WINDOWS
#elif (defined __APPLE__)
#define OJPH_OS_APPLE
#elif (defined __ANDROID__)
#define OJPH_OS_ANDROID
#elif (defined __linux)
#define OJPH_OS_LINUX
#endif

  /////////////////////////////////////////////////////////////////////////////
  // defines for dll
  /////////////////////////////////////////////////////////////////////////////
#if defined(OJPH_OS_WINDOWS) && defined(OJPH_BUILD_SHARED_LIBRARY)
#define OJPH_EXPORT __declspec(dllexport)
#else
#define OJPH_EXPORT
#endif

  /////////////////////////////////////////////////////////////////////////////
  //                             cpu features
  /////////////////////////////////////////////////////////////////////////////
  OJPH_EXPORT
  int get_cpu_ext_level();

  enum : int {
    X86_CPU_EXT_LEVEL_GENERIC = 0,
    X86_CPU_EXT_LEVEL_MMX = 1,
    X86_CPU_EXT_LEVEL_SSE = 2,
    X86_CPU_EXT_LEVEL_SSE2 = 3,
    X86_CPU_EXT_LEVEL_SSE3 = 4,
    X86_CPU_EXT_LEVEL_SSSE3 = 5,
    X86_CPU_EXT_LEVEL_SSE41 = 6,
    X86_CPU_EXT_LEVEL_SSE42 = 7,
    X86_CPU_EXT_LEVEL_AVX = 8,
    X86_CPU_EXT_LEVEL_AVX2 = 9,
    X86_CPU_EXT_LEVEL_AVX2FMA = 10,
    X86_CPU_EXT_LEVEL_AVX512 = 11,
  };

  enum : int {
    ARM_CPU_EXT_LEVEL_GENERIC = 0,
    ARM_CPU_EXT_LEVEL_NEON = 1,
    ARM_CPU_EXT_LEVEL_ASIMD = 1,
    ARM_CPU_EXT_LEVEL_SVE = 2,
    ARM_CPU_EXT_LEVEL_SVE2 = 3,
  };

  /////////////////////////////////////////////////////////////////////////////
  static inline ui32 population_count(ui32 val)
  {
  #if defined(OJPH_COMPILER_MSVC)  \
    && (defined(OJPH_ARCH_X86_64) || defined(OJPH_ARCH_I386))
    return (ui32)__popcnt(val);
  #elif (defined OJPH_COMPILER_GNUC)
    return (ui32)__builtin_popcount(val);
  #else
    val -= ((val >> 1) & 0x55555555);
    val = (((val >> 2) & 0x33333333) + (val & 0x33333333));
    val = (((val >> 4) + val) & 0x0f0f0f0f);
    val += (val >> 8);
    val += (val >> 16);
    return (int)(val & 0x0000003f);
  #endif
  }

  /////////////////////////////////////////////////////////////////////////////
#ifdef OJPH_COMPILER_MSVC
  #pragma intrinsic(_BitScanReverse)
#endif
  static inline ui32 count_leading_zeros(ui32 val)
  {
  #ifdef OJPH_COMPILER_MSVC
    unsigned long result = 0;
    _BitScanReverse(&result, val);
    return 31 ^ (ui32)result;
  #elif (defined OJPH_COMPILER_GNUC)
    return (ui32)__builtin_clz(val);
  #else
    val |= (val >> 1);
    val |= (val >> 2);
    val |= (val >> 4);
    val |= (val >> 8);
    val |= (val >> 16);
    return 32 - population_count(val);
  #endif
  }

  /////////////////////////////////////////////////////////////////////////////
#ifdef OJPH_COMPILER_MSVC
  #if (defined OJPH_ARCH_X86_64 || defined OJPH_ARCH_ARM)
    #pragma intrinsic(_BitScanReverse64)
  #elif (defined OJPH_ARCH_I386)
    #pragma intrinsic(_BitScanReverse)
  #else
    #error Error unsupport MSVC version
  #endif
#endif
  static inline ui32 count_leading_zeros(ui64 val)
  {
  #ifdef OJPH_COMPILER_MSVC
    unsigned long result = 0;
    #if (defined OJPH_ARCH_X86_64) || (defined OJPH_ARCH_ARM)
      _BitScanReverse64(&result, val);
    #elif (defined OJPH_ARCH_I386)
      ui32 msb = (ui32)(val >> 32), lsb = (ui32)val;
      if (msb == 0)
        _BitScanReverse(&result, lsb);
      else {
        _BitScanReverse(&result, msb);
        result += 32;
      }
    #else
      #error Error unsupport MSVC version
    #endif
    return 63 ^ (ui32)result;
  #elif (defined OJPH_COMPILER_GNUC)
    return (ui32)__builtin_clzll(val);
  #else
    val |= (val >> 1);
    val |= (val >> 2);
    val |= (val >> 4);
    val |= (val >> 8);
    val |= (val >> 16);
    val |= (val >> 32);
    return 64 - population_count64(val);
  #endif
  }

  /////////////////////////////////////////////////////////////////////////////
#ifdef OJPH_COMPILER_MSVC
  #pragma intrinsic(_BitScanForward)
#endif
  static inline ui32 count_trailing_zeros(ui32 val)
  {
  #ifdef OJPH_COMPILER_MSVC
    unsigned long result = 0;
    _BitScanForward(&result, val);
    return (ui32)result;
  #elif (defined OJPH_COMPILER_GNUC)
    return (ui32)__builtin_ctz(val);
  #else
    val |= (val << 1);
    val |= (val << 2);
    val |= (val << 4);
    val |= (val << 8);
    val |= (val << 16);
    return 32 - population_count(val);
  #endif
  }

  ////////////////////////////////////////////////////////////////////////////
  static inline si32 ojph_round(float val)
  {
  #ifdef OJPH_COMPILER_MSVC
    return (si32)(val + (val >= 0.0f ? 0.5f : -0.5f));
  #elif (defined OJPH_COMPILER_GNUC)
    return (si32)(val + (val >= 0.0f ? 0.5f : -0.5f));
  #else
    return (si32)round(val);
  #endif
  }

  ////////////////////////////////////////////////////////////////////////////
  static inline si32 ojph_trunc(float val)
  {
  #ifdef OJPH_COMPILER_MSVC
    return (si32)(val);
  #elif (defined OJPH_COMPILER_GNUC)
    return (si32)(val);
  #else
    return (si32)trunc(val);
  #endif
  }

  ////////////////////////////////////////////////////////////////////////////
  // constants
  ////////////////////////////////////////////////////////////////////////////
  #ifndef OJPH_EMSCRIPTEN
    const ui32 byte_alignment = 64; // 64 bytes == 512 bits
    const ui32 log_byte_alignment = 31 - count_leading_zeros(byte_alignment);
    const ui32 object_alignment = 8;
  #else
    const ui32 byte_alignment = 16; // 16 bytes == 128 bits
    const ui32 log_byte_alignment = 31 - count_leading_zeros(byte_alignment);
    const ui32 object_alignment = 8;
    #endif

  ////////////////////////////////////////////////////////////////////////////
  // templates for alignment
  ////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////
  // finds the size such that it is a multiple of byte_alignment
  template <typename T, ui32 N>
  size_t calc_aligned_size(size_t size) {
    size = size * sizeof(T) + N - 1;
    size &= ~((1ULL << (31 - count_leading_zeros(N))) - 1);
    size >>= (63 - count_leading_zeros((ui64)sizeof(T)));
    return size;
  }

  ////////////////////////////////////////////////////////////////////////////
  // moves the pointer to first address that is a multiple of byte_alignment
  template <typename T, ui32 N>
  inline T *align_ptr(T *ptr) {
    intptr_t p = reinterpret_cast<intptr_t>(ptr);
    p += N - 1;
    p &= ~((1ULL << (31 - count_leading_zeros(N))) - 1);
    return reinterpret_cast<T *>(p);
  }

}

#endif // !OJPH_ARCH_H
