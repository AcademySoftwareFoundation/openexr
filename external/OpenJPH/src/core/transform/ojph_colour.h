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
// File: ojph_colour.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_COLOR_H
#define OJPH_COLOR_H

namespace ojph {

  // defined elsewhere
  class line_buf;

  namespace local {

  ////////////////////////////////////////////////////////////////////////////
  void init_colour_transform_functions();

  ////////////////////////////////////////////////////////////////////////////
  extern void (*rev_convert)
    (const line_buf *src_line, const ui32 src_line_offset, 
     line_buf *dst_line, const ui32 dst_line_offset, 
     si64 shift, ui32 width);

  ////////////////////////////////////////////////////////////////////////////
  extern void (*rev_convert_nlt_type3)
    (const line_buf *src_line, const ui32 src_line_offset, 
     line_buf *dst_line, const ui32 dst_line_offset, 
     si64 shift, ui32 width);


  ////////////////////////////////////////////////////////////////////////////
  extern void (*irv_convert_to_integer) (
    const line_buf *src_line, line_buf *dst_line, ui32 dst_line_offset,
    ui32 bit_depth, bool is_signed, ui32 width);

  ////////////////////////////////////////////////////////////////////////////
  extern void (*irv_convert_to_float) (
    const line_buf *src_line, ui32 src_line_offset,
    line_buf *dst_line, ui32 bit_depth, bool is_signed, ui32 width);

  ////////////////////////////////////////////////////////////////////////////
  extern void (*irv_convert_to_integer_nlt_type3) (
    const line_buf *src_line, line_buf *dst_line, ui32 dst_line_offset,
    ui32 bit_depth, bool is_signed, ui32 width);

  ////////////////////////////////////////////////////////////////////////////
  extern void (*irv_convert_to_float_nlt_type3) (
    const line_buf *src_line, ui32 src_line_offset,
    line_buf *dst_line, ui32 bit_depth, bool is_signed, ui32 width);

  ////////////////////////////////////////////////////////////////////////////
  extern void (*rct_forward)
    (const line_buf *r, const line_buf *g, const line_buf *b,
     line_buf *y, line_buf *cb, line_buf *cr, ui32 repeat);

  ////////////////////////////////////////////////////////////////////////////
  extern void (*rct_backward)
    (const line_buf *y, const line_buf *cb, const line_buf *cr,
     line_buf *r, line_buf *g, line_buf *b, ui32 repeat);

  ////////////////////////////////////////////////////////////////////////////
  extern void (*ict_forward)
    (const float *r, const float *g, const float *b,
     float *y, float *cb, float *cr, ui32 repeat);

  ////////////////////////////////////////////////////////////////////////////
  extern void (*ict_backward)
    (const float *y, const float *cb, const float *cr,
     float *r, float *g, float *b, ui32 repeat);
  }
}



#endif // !OJPH_COLOR_H
