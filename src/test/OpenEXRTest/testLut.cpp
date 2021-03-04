//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <ImfLut.h>
#include <ImfArray.h>
#include "ImathRandom.h"
#include <iostream>

#include <assert.h>


using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;
using namespace IMATH_NAMESPACE;


namespace {

half
one (half)
{
    return 1;
}


void
testHalfLut ()
{
    const int NX = 67;
    const int NY = 31;

    Array2D<half> h (NY, NX);
    HalfLut lut (one);

    //
    // apply (data, nData, stride);
    //

    for (int y = 0; y < NY; ++y)
	for (int x = 0; x < NX; ++x)
	    h[y][x] = 0;

    lut.apply (&h[0][0], NX * NY, 1);

    for (int y = 0; y < NY; ++y)
	for (int x = 0; x < NX; ++x)
	    assert (h[y][x] == 1);

    //
    // apply (slice, dataWindow);
    //

    for (int y = 0; y < NY; ++y)
	for (int x = 0; x < NX; ++x)
	    h[y][x] = 0;

    Slice s (HALF,			// type
	     (char *) &h[0][0],		// base
	     sizeof (h[0][0]),		// xStride
	     sizeof (h[0][0]) * NX,	// yStride
	     1, 1); 			// xSampling, ySampling

    Box2i dw (V2i (3, 5), V2i (45, 27));

    lut.apply (s, dw);

    for (int y = 0; y < NY; ++y)
	for (int x = 0; x < NX; ++x)
	    if (dw.intersects (V2i (x, y)))
		assert (h[y][x] == 1);
	    else
		assert (h[y][x] == 0);
}


void
testRgbaLut ()
{
    const int NX = 67;
    const int NY = 31;

    Array2D<Rgba> rgba (NY, NX);
    RgbaLut lut (one, WRITE_RGB);

    //
    // apply (data, nData, stride);
    //

    for (int y = 0; y < NY; ++y)
    {
	for (int x = 0; x < NX; ++x)
	{
	    rgba[y][x].r = 0;
	    rgba[y][x].g = 0;
	    rgba[y][x].b = 0;
	    rgba[y][x].a = 0;
	}
    }

    lut.apply (&rgba[0][0], NX * NY, 1);

    for (int y = 0; y < NY; ++y)
    {
	for (int x = 0; x < NX; ++x)
	{
	    assert (rgba[y][x].r == 1);
	    assert (rgba[y][x].g == 1);
	    assert (rgba[y][x].b == 1);
	    assert (rgba[y][x].a == 0);
	}
    }

    //
    // apply (base, xStride, yStride, dataWindow);
    //

    for (int y = 0; y < NY; ++y)
    {
	for (int x = 0; x < NX; ++x)
	{
	    rgba[y][x].r = 0;
	    rgba[y][x].g = 0;
	    rgba[y][x].b = 0;
	    rgba[y][x].a = 0;
	}
    }

    Box2i dw (V2i (3, 5), V2i (45, 27));

    lut.apply (&rgba[0][0], 1, NX, dw);

    for (int y = 0; y < NY; ++y)
    {
	for (int x = 0; x < NX; ++x)
	{
	    if (dw.intersects (V2i (x, y)))
	    {
		assert (rgba[y][x].r == 1);
		assert (rgba[y][x].g == 1);
		assert (rgba[y][x].b == 1);
		assert (rgba[y][x].a == 0);
	    }
	    else
	    {
		assert (rgba[y][x].r == 0);
		assert (rgba[y][x].g == 0);
		assert (rgba[y][x].b == 0);
		assert (rgba[y][x].a == 0);
	    }
	}
    }
}


void
testRounding ()
{
    //
    // For each rounding function, f,
    // f(f(x)) == f(x) must be true.
    //

    Rand32 rand;

    for (int i = 0; i < 10000; ++i)
    {
	half h = rand.nextf (HALF_MIN, HALF_MAX);
	assert (round12log (h) == round12log (round12log (h)));
    }

    for (int n = 0; n <= 10; ++n)
    {
	roundNBit rn (n);

	for (int i = 0; i < 10000; ++i)
	{
	    half h = rand.nextf (HALF_MIN, HALF_MAX);
	    assert (rn (h) == rn (rn (h)));
	}
    }

    //
    // Special cases:
    //

    assert (round12log (-1) == 0);
    assert (round12log (0) == 0);
    assert (round12log (0.5) == 0.5);
    assert (round12log (1) == 1);
    assert (round12log (2) == 2);

    roundNBit r3 (3);

    assert (r3 (-1) == -1);
    assert (r3 (0) == 0);
    assert (r3 (0.5) == 0.5);
    assert (r3 (1) == 1);
    assert (r3 (2) == 2);
}


} // namespace


void
testLut (const std::string&)
{
    try
    {
	cout << "Testing lookup tables" << endl;

	testHalfLut();
	testRgbaLut();
	testRounding();

	cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
