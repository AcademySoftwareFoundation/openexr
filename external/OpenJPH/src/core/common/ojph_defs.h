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
// File: ojph_defs.h
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/


#ifndef OJPH_TYPES_H
#define OJPH_TYPES_H

#include <cstdint>
#include "ojph_version.h"

namespace ojph {

/////////////////////////////////////////////////////////////////////////////
//                               types
/////////////////////////////////////////////////////////////////////////////
typedef uint8_t ui8;
typedef int8_t si8;
typedef uint16_t ui16;
typedef int16_t si16;
typedef uint32_t ui32;
typedef int32_t si32;
typedef uint64_t ui64;
typedef int64_t si64;

/////////////////////////////////////////////////////////////////////////////
#define OJPH_INT_STRINGIFY(I) #I
#define OJPH_INT_TO_STRING(I) OJPH_INT_STRINGIFY(I)

/////////////////////////////////////////////////////////////////////////////
// number of fractional bits for 16 bit representation
// for 32 bits, it is NUM_FRAC_BITS + 16
// All numbers are in the range of [-0.5, 0.5)
const int NUM_FRAC_BITS = 13;

/////////////////////////////////////////////////////////////////////////////
#define ojph_div_ceil(a, b) (((a) + (b) - 1) / (b))

/////////////////////////////////////////////////////////////////////////////
#define ojph_max(a, b) (((a) > (b)) ? (a) : (b))

/////////////////////////////////////////////////////////////////////////////
#define ojph_min(a, b) (((a) < (b)) ? (a) : (b))

#define ojph_unused(x) (void)(x)


}

#endif // !OJPH_TYPES_H
