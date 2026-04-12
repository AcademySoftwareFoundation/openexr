///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

// Primary authors:
//     Florian Kainz <kainz@ilm.com>
//     Rod Bogart <rgb@ilm.com>

//--------------------------------------------------------------------------
//
//	half -- a 16-bit floating point number class:
//
//	Type half can represent positive and negative numbers whose
//	magnitude is between roughly 6.1e-5 and 6.5e+4 with a relative
//	error of 9.8e-4; numbers smaller than 6.1e-5 can be represented
//	with an absolute error of 6.0e-8.  All integers from -2048 to
//	+2048 can be represented exactly.
//
//	Type half behaves (almost) like the built-in C++ floating point
//	types.  In arithmetic expressions, half, float and double can be
//	mixed freely.
//
//	Conversions from half to float are lossless; all half numbers
//	are exactly representable as floats.
//
//	Conversions from float to half may not preserve a float's value
//	exactly.  If a float is not representable as a half, then the
//	float value is rounded to the nearest representable half.
//
//--------------------------------------------------------------------------

#pragma once

#ifndef OPENEXR_HALF_H
#define OPENEXR_HALF_H

#include "halfExport.h"    // for definition of HALF_EXPORT
#include <iostream>

namespace Imath {

class half
{
  public:

    //-------------
    // Constructors
    //-------------

    half () = default;			// no initialization
    half (float f);
    // rule of 5
    ~half () = default;
    half (const half &) = default;
    half (half &&) noexcept = default;

    //--------------------
    // Conversion to float
    //--------------------

    operator		float () const;

    //------------
    // Unary minus
    //------------

    half		operator - () const;

    //-----------
    // Assignment
    //-----------

    half &		operator = (const half  &h) = default;
    half &		operator = (half  &&h) noexcept = default;
    half &		operator = (float f);

    half &		operator += (half  h);
    half &		operator += (float f);

    half &		operator -= (half  h);
    half &		operator -= (float f);

    half &		operator *= (half  h);
    half &		operator *= (float f);

    half &		operator /= (half  h);
    half &		operator /= (float f);

    //---------------------------------------------------------
    // Round to n-bit precision (n should be between 0 and 10).
    //---------------------------------------------------------

    half		round (unsigned int n) const;

    //--------------------------------------------------------------------
    // Classification:
    //--------------------------------------------------------------------

    bool		isFinite () const;
    bool		isNormalized () const;
    bool		isDenormalized () const;
    bool		isZero () const;
    bool		isNan () const;
    bool		isInfinity () const;
    bool		isNegative () const;

    //--------------------------------------------
    // Special values
    //--------------------------------------------

    static half		posInf ();
    static half		negInf ();
    static half		qNan ();
    static half		sNan ();

    //--------------------------------------
    // Access to the internal representation
    //--------------------------------------

    HALF_EXPORT unsigned short	bits () const;
    HALF_EXPORT void		setBits (unsigned short bits);

  public:

    union uif
    {
	unsigned int	i;
	float		f;
    };

  private:

    HALF_EXPORT static short                  convert (int i);
    HALF_EXPORT static float                  overflow ();

    unsigned short                            _h;

    HALF_EXPORT static const uif              _toFloat[1 << 16];
    HALF_EXPORT static const unsigned short   _eLut[1 << 9];
};

//-----------
// Stream I/O
//-----------

HALF_EXPORT std::ostream &      operator << (std::ostream &os, half  h);
HALF_EXPORT std::istream &      operator >> (std::istream &is, half &h);

//----------
// Debugging
//----------

HALF_EXPORT void        printBits   (std::ostream &os, half  h);
HALF_EXPORT void        printBits   (std::ostream &os, float f);
HALF_EXPORT void        printBits   (char  c[19], half  h);
HALF_EXPORT void        printBits   (char  c[35], float f);

} // namespace Imath

// Backward compatibility: allow 'half' to be used without Imath:: prefix
using Imath::half;

//-------------------------------------------------------------------------
// Limits
//-------------------------------------------------------------------------

#if (defined _WIN32 || defined _WIN64) && defined _MSC_VER

  #define HALF_MIN	5.96046448e-08f
  #define HALF_NRM_MIN	6.10351562e-05f
  #define HALF_MAX	65504.0f
  #define HALF_EPSILON	0.00097656f
#else

  #define HALF_MIN	5.96046448e-08
  #define HALF_NRM_MIN	6.10351562e-05
  #define HALF_MAX	65504.0
  #define HALF_EPSILON	0.00097656
#endif

#define HALF_MANT_DIG	11
#define HALF_DIG	3
#define HALF_DECIMAL_DIG	5
#define HALF_RADIX	2
#define HALF_MIN_EXP	-13
#define HALF_MAX_EXP	16
#define HALF_MIN_10_EXP	-4
#define HALF_MAX_10_EXP	4

//--------------------------------------------------------------------------
// Implementation
//--------------------------------------------------------------------------

//----------------------------
// Half-from-float constructor
//----------------------------

inline
half::half (float f)
{
    uif x;

    x.f = f;

    if (f == 0)
    {
	_h = (x.i >> 16);
    }
    else
    {
	int e = (x.i >> 23) & 0x000001ff;
	e = _eLut[e];

	if (e)
	{
	    int m = x.i & 0x007fffff;
	    _h = e + ((m + 0x00000fff + ((m >> 13) & 1)) >> 13);
	}
	else
	{
	    _h = convert (x.i);
	}
    }
}

//------------------------------------------
// Half-to-float conversion via table lookup
//------------------------------------------

inline
half::operator float () const
{
    return _toFloat[_h].f;
}

//-------------------------
// Round to n-bit precision
//-------------------------

inline half
half::round (unsigned int n) const
{
    if (n >= 10)
	return *this;

    unsigned short s = _h & 0x8000;
    unsigned short e = _h & 0x7fff;

    e >>= 9 - n;
    e  += e & 1;
    e <<= 9 - n;

    if (e >= 0x7c00)
    {
	e = _h;
	e >>= 10 - n;
	e <<= 10 - n;
    }

    half h;
    h._h = s | e;
    return h;
}

//-----------------------
// Other inline functions
//-----------------------

inline half	
half::operator - () const
{
    half h;
    h._h = _h ^ 0x8000;
    return h;
}

inline half &
half::operator = (float f)
{
    *this = half (f);
    return *this;
}

inline half &
half::operator += (half h)
{
    *this = half (float (*this) + float (h));
    return *this;
}

inline half &
half::operator += (float f)
{
    *this = half (float (*this) + f);
    return *this;
}

inline half &
half::operator -= (half h)
{
    *this = half (float (*this) - float (h));
    return *this;
}

inline half &
half::operator -= (float f)
{
    *this = half (float (*this) - f);
    return *this;
}

inline half &
half::operator *= (half h)
{
    *this = half (float (*this) * float (h));
    return *this;
}

inline half &
half::operator *= (float f)
{
    *this = half (float (*this) * f);
    return *this;
}

inline half &
half::operator /= (half h)
{
    *this = half (float (*this) / float (h));
    return *this;
}

inline half &
half::operator /= (float f)
{
    *this = half (float (*this) / f);
    return *this;
}

inline bool	
half::isFinite () const
{
    unsigned short e = (_h >> 10) & 0x001f;
    return e < 31;
}

inline bool
half::isNormalized () const
{
    unsigned short e = (_h >> 10) & 0x001f;
    return e > 0 && e < 31;
}

inline bool
half::isDenormalized () const
{
    unsigned short e = (_h >> 10) & 0x001f;
    unsigned short m =  _h & 0x3ff;
    return e == 0 && m != 0;
}

inline bool
half::isZero () const
{
    return (_h & 0x7fff) == 0;
}

inline bool
half::isNan () const
{
    unsigned short e = (_h >> 10) & 0x001f;
    unsigned short m =  _h & 0x3ff;
    return e == 31 && m != 0;
}

inline bool
half::isInfinity () const
{
    unsigned short e = (_h >> 10) & 0x001f;
    unsigned short m =  _h & 0x3ff;
    return e == 31 && m == 0;
}

inline bool	
half::isNegative () const
{
    return (_h & 0x8000) != 0;
}

inline half
half::posInf ()
{
    half h;
    h._h = 0x7c00;
    return h;
}

inline half
half::negInf ()
{
    half h;
    h._h = 0xfc00;
    return h;
}

inline half
half::qNan ()
{
    half h;
    h._h = 0x7fff;
    return h;
}

inline half
half::sNan ()
{
    half h;
    h._h = 0x7dff;
    return h;
}

inline unsigned short
half::bits () const
{
    return _h;
}

inline void
half::setBits (unsigned short bits)
{
    _h = bits;
}

#endif
