///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006, Industrial Light & Magic, a division of Lucas
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

//-----------------------------------------------------------------------------
//
//	Rational numbers
//
//-----------------------------------------------------------------------------

#include <ImfRational.h>

namespace Imf {


Rational::Rational (double x)
{
    int sign;

    if (x >= 0)
    {
	sign = 1;	// positive
    }
    else if (x < 0)
    {
	sign = -1;	// negative
	x = -x;
    }
    else
    {
	n = 0;		// NaN
	d = 0;
	return;
    }

    if (x >= (1U << 31) - 0.5)
    {
	n = sign;	// infinity
	d = 0;
	return;
    }

    d = 1;
    n = int (x * d + 0.5);

    while (n < (1 << 30) && d < (1U << 31))
    {
	d <<= 1;
	n = int (x * d + 0.5);
    }

    if (n == 0)
    {
	d = 1;		// zero
	return;
    }

    while (!(n & 1) && d > 1)
    {
	n >>= 1;
	d >>= 1;
    }
    
    n *= sign;
}


} // namespace Imf
