//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman
// Copyright (c) 2019, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2019, The University of New South Wales, Australia
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
// File: ojph_arch.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_ARCH_H
#define OJPH_ARCH_H

#include <cstring>
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

  /////////////////////////////////////////////////////////////////////////////
  // portable force-inline / no-inline function qualifiers
  /////////////////////////////////////////////////////////////////////////////
#ifdef OJPH_COMPILER_MSVC
  #define OJPH_FORCE_INLINE static __forceinline
  #define OJPH_NO_INLINE    static __declspec(noinline)
#else
  #define OJPH_FORCE_INLINE static inline __attribute__((always_inline))
  #define OJPH_NO_INLINE    static __attribute__((noinline))
#endif

///////////////////////////////////////////////////////////////////////////////
// preprocessor directives for architecture
///////////////////////////////////////////////////////////////////////////////
#if defined(__arm__) || defined(__TARGET_ARCH_ARM)  \
  || defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
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

// Only little-endian POWER (ppc64le) is supported for SIMD
#if defined(OJPH_ARCH_PPC64) &&  \
  (defined(__LITTLE_ENDIAN__) ||  \
   (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
  #define OJPH_ARCH_PPC64LE
#endif

namespace ojph {
  ////////////////////////////////////////////////////////////////////////////
  //                  disable SIMD for unknown architecture
  ////////////////////////////////////////////////////////////////////////////
#if !defined(OJPH_ARCH_X86_64) && !defined(OJPH_ARCH_I386) &&  \
    !defined(OJPH_ARCH_ARM) && !defined(OJPH_ARCH_PPC64LE) &&  \
    !defined(OJPH_DISABLE_SIMD)
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
#elif (defined __FreeBSD__)
#define OJPH_OS_FREEBSD
#elif (defined __OpenBSD__)
#define OJPH_OS_OPENBSD
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

  // POWER9 (ISA 3.0) is the minimum supported SIMD level; older CPUs
  // (POWER8 and earlier) use the generic code paths
  enum : int {
    PPC_CPU_EXT_LEVEL_GENERIC = 0,
    PPC_CPU_EXT_LEVEL_ARCH_3_00 = 1, // ISA 3.0  (POWER9)
    PPC_CPU_EXT_LEVEL_ARCH_3_1 = 2,  // ISA 3.1  (POWER10)
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

  /////////////////////////////////////////////////////////////////////////////
#ifdef OJPH_COMPILER_MSVC
  #pragma intrinsic(_BitScanForward64)
#endif
  static inline ui32 count_trailing_zeros(ui64 val)
  {
  #ifdef OJPH_COMPILER_MSVC
    unsigned long result = 0;
    #if (defined OJPH_ARCH_X86_64) || (defined OJPH_ARCH_ARM)
      _BitScanForward64(&result, val);
    #elif (defined OJPH_ARCH_I386)
      ui32 lsb = (ui32)val, msb = (ui32)(val >> 32);
      if (lsb != 0)
        _BitScanForward(&result, lsb);
      else {
        _BitScanForward(&result, msb);
        result += 32;
      }
    #endif
    return (ui32)result;
  #elif (defined OJPH_COMPILER_GNUC)
    return (ui32)__builtin_ctzll(val);
  #else
    if ((ui32)val != 0)
      return count_trailing_zeros((ui32)val);
    return 32 + count_trailing_zeros((ui32)(val >> 32));
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

  ////////////////////////////////////////////////////////////////////////////
  // Determine the byte order of the target at compile time when possible,
  // so that the compiler can remove the branches for the other byte order.
  // __BYTE_ORDER__ is a predefined macro that describes the target
  // architecture, not the machine running the compiler, so it is also
  // correct when cross-compiling.
  // All MSVC targets (x86, x64, ARM64 Windows) are little endian.
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  constexpr bool is_machine_little_endian = false;
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  constexpr bool is_machine_little_endian = true;
#elif defined(OJPH_COMPILER_MSVC)
  constexpr bool is_machine_little_endian = true;
#else
  // fallback in case macro __BYTE_ORDER__ is not defined
  // If the first byte in memory is 0x01, the machine is Little Endian.
  // If the first byte in memory is 0x00, the machine is Big Endian.
  static bool check_if_machine_is_little_endian()
  {
    const uint16_t n = 0x0001;
    bool is_machine_little_endian = (*((uint8_t *)&n) == 0x01);
    return is_machine_little_endian;
  }
  const bool is_machine_little_endian = check_if_machine_is_little_endian();
#endif

  ////////////////////////////////////////////////////////////////////////////
  // swap bytes 1 2 --> 2 1 on big-endian machines
  static inline ui16 swap_bytes_if_be(ui16 t)
  {
    if (is_machine_little_endian)
      return t;
    else
      return (ui16)((t << 8) | (t >> 8));
  }
  ////////////////////////////////////////////////////////////////////////////
  // swap bytes 1 2 --> 2 1 on little-endian machines
  static inline ui16 swap_bytes_if_le(ui16 t)
  {
    if (is_machine_little_endian)
      return (ui16)((t << 8) | (t >> 8));
    else
      return t;
  }
  ////////////////////////////////////////////////////////////////////////////
  // swap bytes 1 2 3 4 --> 4 3 2 1 on big-endian machines
  static inline ui32 swap_bytes_if_be(ui32 t)
  {
    if (is_machine_little_endian)
      return t;
    else
    {
      ui32 u = swap_bytes_if_be((ui16)(t & 0xFFFFu));
      u <<= 16;
      u |= swap_bytes_if_be((ui16)(t >> 16));
      return u;
    }
  }
  ////////////////////////////////////////////////////////////////////////////
  // swap bytes 1 2 3 4 --> 4 3 2 1 on little-endian machines
  static inline ui32 swap_bytes_if_le(ui32 t)
  {
    if (is_machine_little_endian)
    {
      ui32 u = swap_bytes_if_le((ui16)(t & 0xFFFFu));
      u <<= 16;
      u |= swap_bytes_if_le((ui16)(t >> 16));
      return u;
    }
    else
      return t;
  }
  ////////////////////////////////////////////////////////////////////////////
  // swap bytes 1 2 3 4 5 6 7 8 --> 8 7 6 5 4 3 2 1 on little-endian machines
  static inline ui64 swap_bytes_if_le(ui64 t)
  {
    if (is_machine_little_endian)
    {
      ui64 u =
        swap_bytes_if_le((ui32)(t & 0xFFFFFFFFu));
      u <<= 32;
      u |= swap_bytes_if_le((ui32)(t >> 32));
      return u;
    }
    else
      return t;
  }

  ////////////////////////////////////////////////////////////////////////////
  // loads 4 bytes from p as a little-endian 32-bit integer; that is, the
  // byte at the lowest address goes into the least-significant byte of the
  // result, irrespective of the machine's endianness
  static inline ui32 load_le_ui32(const ui8 *p)
  {
    if (is_machine_little_endian) {
      ui32 val;
      std::memcpy(&val, p, sizeof(val));
      return val;
    }
    else
      return (ui32)p[0] | ((ui32)p[1] << 8)
        | ((ui32)p[2] << 16) | ((ui32)p[3] << 24);
  }

  ////////////////////////////////////////////////////////////////////////////
  // loads two consecutive ui16 values from p, placing the one at the lower
  // address in the least-significant 16 bits of the result, irrespective
  // of the machine's endianness
  static inline ui32 load_le_ui16x2(const ui16 *p)
  {
    if (is_machine_little_endian) {
      ui32 val;
      std::memcpy(&val, p, sizeof(val));
      return val;
    }
    else
      return (ui32)p[0] | ((ui32)p[1] << 16);
  }
}

#endif // !OJPH_ARCH_H
