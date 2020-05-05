//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <iostream>
#include <iomanip>
#include <half.h>
#include <assert.h>
#include <cmath>

using namespace std;

//
// This test uses the code that generates the toFLoat.h header to
// validate the the tabel values are correct.
//

//---------------------------------------------------
// Interpret an unsigned short bit pattern as a half,
// and convert that half to the corresponding float's
// bit pattern.
//---------------------------------------------------

unsigned int
halfToFloat (unsigned short y)
{

    int s = (y >> 15) & 0x00000001;
    int e = (y >> 10) & 0x0000001f;
    int m =  y        & 0x000003ff;

    if (e == 0)
    {
	if (m == 0)
	{
	    //
	    // Plus or minus zero
	    //

	    return s << 31;
	}
	else
	{
	    //
	    // Denormalized number -- renormalize it
	    //

	    while (!(m & 0x00000400))
	    {
		m <<= 1;
		e -=  1;
	    }

	    e += 1;
	    m &= ~0x00000400;
	}
    }
    else if (e == 31)
    {
	if (m == 0)
	{
	    //
	    // Positive or negative infinity
	    //

	    return (s << 31) | 0x7f800000;
	}
	else
	{
	    //
	    // Nan -- preserve sign and significand bits
	    //

	    return (s << 31) | 0x7f800000 | (m << 13);
	}
    }

    //
    // Normalized number
    //

    e = e + (127 - 15);
    m = m << 13;

    //
    // Assemble s, e and m.
    //

    return (s << 31) | (e << 23) | m;
}

union HalfShort
{
    half           h;
    unsigned short s;
};

void
testToFloat ()
{
    std::cout << "running testToFloat" << std::endl;
    
    constexpr int iMax = (1 << 16);

    //
    // for each 16-bit bit pattern...
    //
    
    for (int s = 0; s < iMax; s++)
    {
        HalfShort hs;
        hs.s = s;
        
        //
        // cast these bits to a float, using the cast-to-float
        // operator.
        //
        
        float f = float (hs.h); // = _toFloat[s]

        //
        // Cast that float back to a half.
        //
        
        half h = half (f);

        //
        // halfToFloat() above is what generated the _toFloat table.
        // The i value return is the integer bit pattern of the corresponding
        // float.
        //
        
        half::uif uif;
        uif.i = halfToFloat (s);

        //
        // Equality operators fail for inf and nan, so handle them
        // specially.
        //
        
        if (isnan (f))
        {
            assert (h.isNan());
            assert (isnan (uif.f));
        }
        else if (isinf (f))
        {
            assert (h.isInfinity());
            assert (isinf (uif.f));
        }
        else
        {
            assert (h == hs.h);
            assert (f == uif.f);
        }
    }

    std::cout << "ok" << std::endl;
}
