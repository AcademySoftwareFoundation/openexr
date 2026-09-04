//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2026, Aous Naman
// Copyright (c) 2026, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2026, The University of New South Wales, Australia
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
// File: ojph_simd_vsx.h
//
// 128-bit SIMD helpers for POWER VSX, used by the ojph_*_vsx.cpp
// kernels.  Lane numbering and operation semantics follow the same
// conventions as the other 128-bit kernels in this codebase (lane 0
// is the lowest memory address).  Supported targets are POWER9
// (ISA 3.0) and newer, little-endian only (ppc64le).
//***************************************************************************/

#ifndef OJPH_SIMD_VSX_H
#define OJPH_SIMD_VSX_H

#if !defined(__powerpc64__) && !defined(__PPC64__)
  #error "this header is for 64-bit POWER targets only"
#endif
#if !defined(__LITTLE_ENDIAN__) && \
    !(defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  #error "this header assumes a little-endian target (ppc64le)"
#endif

#include <altivec.h>
#include <cstring>

#include "ojph_defs.h"

// altivec.h leaks these context-sensitive keywords as macros under GNU C;
// they break standard headers and the codebase (e.g. std::vector)
#undef vector
#undef pixel
#undef bool

typedef __vector unsigned char v128_t;

typedef __vector signed char        vsx_v_i8;
typedef __vector unsigned char      vsx_v_u8;
typedef __vector signed short       vsx_v_i16;
typedef __vector unsigned short     vsx_v_u16;
typedef __vector signed int         vsx_v_i32;
typedef __vector unsigned int       vsx_v_u32;
typedef __vector signed long long   vsx_v_i64;
typedef __vector unsigned long long vsx_v_u64;
typedef __vector float              vsx_v_f32;

//---------------------------------------------------------------------------
// load/store (alignment-agnostic; lxv/stxv handle unaligned addresses)
//---------------------------------------------------------------------------
static inline v128_t vsx_v128_load(const void *p)
{ return vec_xl(0, (const unsigned char *)p); }

static inline void vsx_v128_store(void *p, v128_t a)
{ vec_xst(a, 0, (unsigned char *)p); }

#define vsx_v128_store32_lane(p, a, i) \
  do { vsx_v_i32 t_ = (vsx_v_i32)(a); int v_ = t_[(i)]; \
       std::memcpy((p), &v_, 4); } while (0)

//---------------------------------------------------------------------------
// constants, splats, makes
//---------------------------------------------------------------------------
// functions, not macros, so that an argument that is itself a macro
// expanding to an argument list (e.g. OJPH_REPEAT4) works
static inline v128_t vsx_i8x16_const(
  signed char c0, signed char c1, signed char c2, signed char c3,
  signed char c4, signed char c5, signed char c6, signed char c7,
  signed char c8, signed char c9, signed char c10, signed char c11,
  signed char c12, signed char c13, signed char c14, signed char c15)
{ vsx_v_i8 v = {c0,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14,c15};
  return (v128_t)v; }
static inline v128_t vsx_i16x8_const(short c0, short c1, short c2,
                                      short c3, short c4, short c5,
                                      short c6, short c7)
{ vsx_v_i16 v = {c0,c1,c2,c3,c4,c5,c6,c7}; return (v128_t)v; }
static inline v128_t vsx_u16x8_const(unsigned short c0, unsigned short c1,
                                      unsigned short c2, unsigned short c3,
                                      unsigned short c4, unsigned short c5,
                                      unsigned short c6, unsigned short c7)
{ vsx_v_u16 v = {c0,c1,c2,c3,c4,c5,c6,c7}; return (v128_t)v; }
static inline v128_t vsx_i32x4_const(int c0, int c1, int c2, int c3)
{ vsx_v_i32 v = {c0,c1,c2,c3}; return (v128_t)v; }
static inline v128_t vsx_u32x4_const(unsigned int c0, unsigned int c1,
                                      unsigned int c2, unsigned int c3)
{ vsx_v_u32 v = {c0,c1,c2,c3}; return (v128_t)v; }
static inline v128_t vsx_i64x2_const(long long c0, long long c1)
{ vsx_v_i64 v = {c0,c1}; return (v128_t)v; }
static inline v128_t vsx_u64x2_const(unsigned long long c0,
                                      unsigned long long c1)
{ vsx_v_u64 v = {c0,c1}; return (v128_t)v; }

static inline v128_t vsx_i8x16_splat(signed char x)
{ ojph_unused(x); return (v128_t)vec_splats(x); }
static inline v128_t vsx_i16x8_splat(short x)
{ ojph_unused(x); return (v128_t)vec_splats(x); }
static inline v128_t vsx_i32x4_splat(int x)
{ ojph_unused(x); return (v128_t)vec_splats(x); }
static inline v128_t vsx_u32x4_splat(unsigned int x)
{ ojph_unused(x); return (v128_t)vec_splats(x); }
static inline v128_t vsx_i64x2_splat(long long x)
{ ojph_unused(x); return (v128_t)vec_splats((signed long long)x); }
static inline v128_t vsx_f32x4_splat(float x)
{ ojph_unused(x); return (v128_t)vec_splats(x); }

static inline v128_t vsx_i32x4_make(int a, int b, int c, int d)
{ return (v128_t)(vsx_v_i32){a, b, c, d}; }

//---------------------------------------------------------------------------
// lane extraction (subscript is little-endian lane order)
//---------------------------------------------------------------------------
#define vsx_u8x16_extract_lane(a, i)  (((vsx_v_u8)(a))[(i)])
#define vsx_u16x8_extract_lane(a, i)  (((vsx_v_u16)(a))[(i)])
#define vsx_i32x4_extract_lane(a, i)  (((vsx_v_i32)(a))[(i)])
#define vsx_u32x4_extract_lane(a, i)  (((vsx_v_u32)(a))[(i)])
#define vsx_i64x2_extract_lane(a, i)  (((vsx_v_i64)(a))[(i)])

//---------------------------------------------------------------------------
// bitwise
//---------------------------------------------------------------------------
static inline v128_t vsx_v128_and(v128_t a, v128_t b)
{ return vec_and(a, b); }
static inline v128_t vsx_v128_or(v128_t a, v128_t b)
{ return vec_or(a, b); }
static inline v128_t vsx_v128_xor(v128_t a, v128_t b)
{ return vec_xor(a, b); }
// a & ~b  (same operand order as vec_andc)
static inline v128_t vsx_v128_andnot(v128_t a, v128_t b)
{ return vec_andc(a, b); }

//---------------------------------------------------------------------------
// integer arithmetic
//---------------------------------------------------------------------------
static inline v128_t vsx_i8x16_add(v128_t a, v128_t b)
{ return (v128_t)vec_add((vsx_v_i8)a, (vsx_v_i8)b); }
static inline v128_t vsx_i16x8_add(v128_t a, v128_t b)
{ return (v128_t)vec_add((vsx_v_i16)a, (vsx_v_i16)b); }
static inline v128_t vsx_i32x4_add(v128_t a, v128_t b)
{ return (v128_t)vec_add((vsx_v_i32)a, (vsx_v_i32)b); }
static inline v128_t vsx_i64x2_add(v128_t a, v128_t b)
{ return (v128_t)vec_add((vsx_v_i64)a, (vsx_v_i64)b); }

static inline v128_t vsx_i16x8_sub(v128_t a, v128_t b)
{ return (v128_t)vec_sub((vsx_v_i16)a, (vsx_v_i16)b); }
static inline v128_t vsx_i32x4_sub(v128_t a, v128_t b)
{ return (v128_t)vec_sub((vsx_v_i32)a, (vsx_v_i32)b); }
static inline v128_t vsx_i64x2_sub(v128_t a, v128_t b)
{ return (v128_t)vec_sub((vsx_v_i64)a, (vsx_v_i64)b); }

// low half of products; vmladduhm / vmuluwm; i64x2 is lowered by the
// compiler (mulld on ISA 3.0, vmulld on ISA 3.1)
static inline v128_t vsx_i16x8_mul(v128_t a, v128_t b)
{ return (v128_t)((vsx_v_i16)a * (vsx_v_i16)b); }
static inline v128_t vsx_i32x4_mul(v128_t a, v128_t b)
{ return (v128_t)((vsx_v_i32)a * (vsx_v_i32)b); }
static inline v128_t vsx_i64x2_mul(v128_t a, v128_t b)
{ return (v128_t)((vsx_v_i64)a * (vsx_v_i64)b); }

static inline v128_t vsx_i8x16_abs(v128_t a)
{ return (v128_t)vec_abs((vsx_v_i8)a); }
static inline v128_t vsx_u8x16_min(v128_t a, v128_t b)
{ return (v128_t)vec_min((vsx_v_u8)a, (vsx_v_u8)b); }
static inline v128_t vsx_i16x8_max(v128_t a, v128_t b)
{ return (v128_t)vec_max((vsx_v_i16)a, (vsx_v_i16)b); }

//---------------------------------------------------------------------------
// shifts (scalar count, modulo lane width)
//---------------------------------------------------------------------------
static inline v128_t vsx_i16x8_shl(v128_t a, int n)
{ return (v128_t)vec_sl((vsx_v_i16)a, vec_splats((unsigned short)n)); }
static inline v128_t vsx_i32x4_shl(v128_t a, int n)
{ return (v128_t)vec_sl((vsx_v_i32)a, vec_splats((unsigned int)n)); }
static inline v128_t vsx_i64x2_shl(v128_t a, int n)
{ return (v128_t)vec_sl((vsx_v_i64)a,
                        vec_splats((unsigned long long)n)); }

static inline v128_t vsx_i32x4_shr(v128_t a, int n)   // arithmetic
{ return (v128_t)vec_sra((vsx_v_i32)a, vec_splats((unsigned int)n)); }
static inline v128_t vsx_i64x2_shr(v128_t a, int n)   // arithmetic
{ return (v128_t)vec_sra((vsx_v_i64)a,
                         vec_splats((unsigned long long)n)); }

static inline v128_t vsx_u16x8_shr(v128_t a, int n)   // logical
{ return (v128_t)vec_sr((vsx_v_u16)a, vec_splats((unsigned short)n)); }
static inline v128_t vsx_u32x4_shr(v128_t a, int n)   // logical
{ return (v128_t)vec_sr((vsx_v_u32)a, vec_splats((unsigned int)n)); }
static inline v128_t vsx_u64x2_shr(v128_t a, int n)   // logical
{ return (v128_t)vec_sr((vsx_v_u64)a,
                        vec_splats((unsigned long long)n)); }

//---------------------------------------------------------------------------
// comparisons (true lanes -> all-ones, false lanes -> all-zeros)
//---------------------------------------------------------------------------
static inline v128_t vsx_i8x16_eq(v128_t a, v128_t b)
{ return (v128_t)vec_cmpeq((vsx_v_i8)a, (vsx_v_i8)b); }
static inline v128_t vsx_i16x8_eq(v128_t a, v128_t b)
{ return (v128_t)vec_cmpeq((vsx_v_i16)a, (vsx_v_i16)b); }
static inline v128_t vsx_i32x4_eq(v128_t a, v128_t b)
{ return (v128_t)vec_cmpeq((vsx_v_i32)a, (vsx_v_i32)b); }

static inline v128_t vsx_i8x16_gt(v128_t a, v128_t b)
{ return (v128_t)vec_cmpgt((vsx_v_i8)a, (vsx_v_i8)b); }
static inline v128_t vsx_i32x4_gt(v128_t a, v128_t b)
{ return (v128_t)vec_cmpgt((vsx_v_i32)a, (vsx_v_i32)b); }
static inline v128_t vsx_i32x4_lt(v128_t a, v128_t b)
{ return (v128_t)vec_cmplt((vsx_v_i32)a, (vsx_v_i32)b); }
static inline v128_t vsx_i64x2_lt(v128_t a, v128_t b)
{ return (v128_t)vec_cmplt((vsx_v_i64)a, (vsx_v_i64)b); }

static inline v128_t vsx_f32x4_ge(v128_t a, v128_t b)
{ return (v128_t)vec_cmpge((vsx_v_f32)a, (vsx_v_f32)b); }
static inline v128_t vsx_f32x4_lt(v128_t a, v128_t b)
{ return (v128_t)vec_cmplt((vsx_v_f32)a, (vsx_v_f32)b); }

//---------------------------------------------------------------------------
// float arithmetic and conversions
//---------------------------------------------------------------------------
static inline v128_t vsx_f32x4_add(v128_t a, v128_t b)
{ return (v128_t)vec_add((vsx_v_f32)a, (vsx_v_f32)b); }
static inline v128_t vsx_f32x4_sub(v128_t a, v128_t b)
{ return (v128_t)vec_sub((vsx_v_f32)a, (vsx_v_f32)b); }
static inline v128_t vsx_f32x4_mul(v128_t a, v128_t b)
{ return (v128_t)vec_mul((vsx_v_f32)a, (vsx_v_f32)b); }

// xvcvspsxws: truncating, saturating (NaN gives 0x80000000; the
// callers never pass NaN)
static inline v128_t vsx_i32x4_trunc_sat_f32x4(v128_t a)
{ return (v128_t)vec_cts((vsx_v_f32)a, 0); }
static inline v128_t vsx_f32x4_convert_i32x4(v128_t a)
{ return (v128_t)vec_ctf((vsx_v_i32)a, 0); }

//---------------------------------------------------------------------------
// widening
//---------------------------------------------------------------------------
static inline v128_t vsx_i64x2_extend_low_i32x4(v128_t a)
{
  // vsx_v_i32 v = (vsx_v_i32)a;
  // return (v128_t)__builtin_convertvector(
  //   __builtin_shufflevector(v, v, 0, 1), vsx_v_i64);

  // Unpacks and sign-extends elements 0 and 1 on Little Endian
  return (v128_t)vec_unpackl((vsx_v_i32)a);
}
static inline v128_t vsx_i64x2_extend_high_i32x4(v128_t a)
{
  // vsx_v_i32 v = (vsx_v_i32)a;
  // return (v128_t)__builtin_convertvector(
  //   __builtin_shufflevector(v, v, 2, 3), vsx_v_i64);

  // Unpacks and sign-extends elements 2 and 3 on Little Endian
  return (v128_t)vec_unpackh((vsx_v_i32)a);
}

//---------------------------------------------------------------------------
// shuffles (immediate lane indices; 0..N-1 from a, N..2N-1 from b)
//---------------------------------------------------------------------------
// #define vsx_i8x16_shuffle(a, b, c0,c1,c2,c3,c4,c5,c6,c7,
//                                  c8,c9,c10,c11,c12,c13,c14,c15)
//   ((v128_t)__builtin_shufflevector((vsx_v_u8)(a), (vsx_v_u8)(b),
//     c0,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14,c15))
// #define vsx_i16x8_shuffle(a, b, c0,c1,c2,c3,c4,c5,c6,c7)
//   ((v128_t)__builtin_shufflevector((vsx_v_i16)(a), (vsx_v_i16)(b),
//     c0,c1,c2,c3,c4,c5,c6,c7))
// #define vsx_i32x4_shuffle(a, b, c0,c1,c2,c3)
//   ((v128_t)__builtin_shufflevector((vsx_v_i32)(a), (vsx_v_i32)(b),
//     c0,c1,c2,c3))
// #define vsx_i64x2_shuffle(a, b, c0,c1)
//   ((v128_t)__builtin_shufflevector((vsx_v_i64)(a), (vsx_v_i64)(b), c0,c1))

// 8-bit Shuffle (Maps direct element indices to raw byte indices)
#define vsx_i8x16_shuffle(a, b, c0,c1,c2,c3,c4,c5,c6,c7, \
                                 c8,c9,c10,c11,c12,c13,c14,c15) \
  ((v128_t)vec_perm((vsx_v_u8)(a), (vsx_v_u8)(b), (vsx_v_u8){ \
    (c0), (c1), (c2), (c3), (c4), (c5), (c6), (c7), \
    (c8), (c9), (c10),(c11),(c12),(c13),(c14),(c15) \
  }))

// 16-bit Shuffle (Multiplies element index by 2 to get byte offsets)
#define vsx_i16x8_shuffle(a, b, c0,c1,c2,c3,c4,c5,c6,c7) \
  ((v128_t)vec_perm((vsx_v_u8)(a), (vsx_v_u8)(b), (vsx_v_u8){ \
    (c0)*2, (c0)*2+1,  (c1)*2, (c1)*2+1, \
    (c2)*2, (c2)*2+1,  (c3)*2, (c3)*2+1, \
    (c4)*2, (c4)*2+1,  (c5)*2, (c5)*2+1, \
    (c6)*2, (c6)*2+1,  (c7)*2, (c7)*2+1  \
  }))

// 32-bit Shuffle (Multiplies element index by 4 to get byte offsets)
#define vsx_i32x4_shuffle(a, b, c0,c1,c2,c3) \
  ((v128_t)vec_perm((vsx_v_u8)(a), (vsx_v_u8)(b), (vsx_v_u8){ \
    (c0)*4, (c0)*4+1, (c0)*4+2, (c0)*4+3, \
    (c1)*4, (c1)*4+1, (c1)*4+2, (c1)*4+3, \
    (c2)*4, (c2)*4+1, (c2)*4+2, (c2)*4+3, \
    (c3)*4, (c3)*4+1, (c3)*4+2, (c3)*4+3  \
  }))

// 64-bit Shuffle (Multiplies element index by 8 to get byte offsets)
#define vsx_i64x2_shuffle(a, b, c0,c1) \
  ((v128_t)vec_perm((vsx_v_u8)(a), (vsx_v_u8)(b), (vsx_v_u8){ \
    (c0)*8, (c0)*8+1, (c0)*8+2, (c0)*8+3,  \
    (c0)*8+4, (c0)*8+5, (c0)*8+6, (c0)*8+7, \
    (c1)*8, (c1)*8+1, (c1)*8+2, (c1)*8+3,  \
    (c1)*8+4, (c1)*8+5, (c1)*8+6, (c1)*8+7  \
  }))

//---------------------------------------------------------------------------
// swizzle: runtime byte-table lookup; lanes with index > 15 give 0
//---------------------------------------------------------------------------
static inline v128_t vsx_i8x16_swizzle(v128_t a, v128_t idx)
{
  v128_t r = vec_perm(a, a, idx);
  v128_t oob = (v128_t)vec_cmpgt((vsx_v_u8)idx,
                                 vec_splats((unsigned char)15));
  return vec_andc(r, oob);
}

//---------------------------------------------------------------------------
// bitmask: MSB of each byte lane -> bit of result, lane 0 -> bit 0
// (vbpermq gathers the 16 selected bits into bits 48..63 of the
// big-endian first doubleword, which is doubleword 1 on ppc64le)
//---------------------------------------------------------------------------
static inline int vsx_i8x16_bitmask(v128_t a)
{
#if defined(__POWER10_VECTOR__)
  return (int)vec_extractm(a);   // ISA 3.1 native movemask
#else
  const vsx_v_u8 perm = { 120, 112, 104, 96, 88, 80, 72, 64,
                            56,  48,  40, 32, 24, 16,  8,  0 };
  vsx_v_u64 r = (vsx_v_u64)vec_bperm(a, perm);
  return (int)r[1];
#endif
}

#endif // OJPH_SIMD_VSX_H
