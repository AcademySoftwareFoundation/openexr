//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.
//


#include <iostream>
#include <iomanip>
#include <half.h>
#include <assert.h>

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

    HalfShort hs;

    for (int i = 0; i < iMax; i++)
    {
        float f = halfToFloat (i);
        half h1 = (half) f;
        half h2 (f);
        
        hs.s = i;
        assert (hs.h == h1);
        assert (hs.h == h2);
    }

    std::cout << "ok" << std::endl;
}
