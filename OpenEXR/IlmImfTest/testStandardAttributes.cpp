///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2003, Industrial Light & Magic, a division of Lucas
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

#include <ImfRgbaFile.h>
#include <ImfStandardAttributes.h>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <assert.h>

using namespace Imf;
using namespace Imath;
using namespace std;

namespace {

void
convertRGBtoXYZ ()
{
    cout << "conversion from RGB to XYZ" << endl;

    Chromaticities c;
    float Y = 100;
    M44f M1 = RGBtoXYZ (c, Y);

    V3f R1 = V3f (1, 0, 0) * M1;
    V3f G1 = V3f (0, 1, 0) * M1;
    V3f B1 = V3f (0, 0, 1) * M1;
    V3f W1 = V3f (1, 1, 1) * M1;

    cout << "red   XYZ = " << R1 << endl;
    cout << "green XYZ = " << G1 << endl;
    cout << "blue  XYZ = " << B1 << endl;
    cout << "white XYZ = " << W1 << endl;

    V2f r1 (R1.x / (R1.x + R1.y + R1.z), R1.y / (R1.x + R1.y + R1.z));
    V2f g1 (G1.x / (G1.x + G1.y + G1.z), G1.y / (G1.x + G1.y + G1.z));
    V2f b1 (B1.x / (B1.x + B1.y + B1.z), B1.y / (B1.x + B1.y + B1.z));
    V2f w1 (W1.x / (W1.x + W1.y + W1.z), W1.y / (W1.x + W1.y + W1.z));

    cout << "red   xy = " << r1 << endl;
    cout << "green xy = " << g1 << endl;
    cout << "blue  xy = " << b1 << endl;
    cout << "white xy = " << w1 << endl;

    assert (equalWithRelError (W1.y, Y, 1e-5F));
    assert (r1.equalWithAbsError (c.red, 1e-5F));
    assert (g1.equalWithAbsError (c.green, 1e-5F));
    assert (b1.equalWithAbsError (c.blue, 1e-5F));
    assert (w1.equalWithAbsError (c.white, 1e-5F));

    cout << "conversion from XYZ to RGB" << endl;

    M44f M2 = XYZtoRGB (c, Y);

    V3f R2 = R1 * M2;
    V3f G2 = G1 * M2;
    V3f B2 = B1 * M2;
    V3f W2 = W1 * M2;

    cout << "red   RGB = " << R2 << endl;
    cout << "green RGB = " << G2 << endl;
    cout << "blue  RGB = " << B2 << endl;
    cout << "white RGB = " << W2 << endl;

    assert (R2.equalWithAbsError (V3f (1, 0, 0), 1e-3F));
    assert (G2.equalWithAbsError (V3f (0, 1, 0), 1e-3F));
    assert (B2.equalWithAbsError (V3f (0, 0, 1), 1e-3F));
    assert (W2.equalWithAbsError (V3f (1, 1, 1), 1e-3F));
}


void
writeReadChromaticities (const char fileName[])
{
    cout << "chromaticities attribute" << endl;

    cout << "writing, ";

    Chromaticities c1 (V2f (1, 2), V2f (3, 4), V2f (5, 6), V2f (7, 8));
    static const int W = 100;
    static const int H = 100;

    Header header (W, H);
    assert (hasChromaticities (header) == false);

    addChromaticities (header, c1);
    assert (hasChromaticities (header) == true);

    {
	RgbaOutputFile out (fileName, header);
	Rgba pixels[W];
	out.setFrameBuffer (pixels, 1, 0);
	out.writePixels (H);
    }

    cout << "reading, comparing" << endl;

    {
	RgbaInputFile in (fileName);
	const Chromaticities &c2 = chromaticities (in.header());

	assert (hasChromaticities (in.header()) == true);
	assert (c1.red == c2.red);
	assert (c1.green == c2.green);
	assert (c1.blue == c2.blue);
	assert (c1.white == c2.white);
    }

    remove (fileName);
}


void
generatedFunctions ()
{
    //
    // Most optional standard attributes are of type string, float,
    // etc.  The attribute types are already being tested elsewhere
    // (testAttributes.cpp), and the convenience functions to access
    // the standard attributes are all generated via macros.  Here
    // we just verify that all the convenience functions exist
    // (that is, ImfStandardAttributes.cpp and ImfStandardAttributes.h
    // contain the right macro invocations).  If any functions are
    // missing, we should get an error during compiling or linking.
    //

    cout << "automatically generated functions" << endl;

    Header header;

    assert (hasChromaticities (header) == false);
    assert (hasWhiteLuminance (header) == false);
    assert (hasXDensity (header) == false);
    assert (hasOwner (header) == false);
    assert (hasComments (header) == false);
    assert (hasCapDate (header) == false);
    assert (hasutcOffset (header) == false);
    assert (hasLongitude (header) == false);
    assert (hasLatitude (header) == false);
    assert (hasAltitude (header) == false);
    assert (hasFocus (header) == false);
    assert (hasExpTime (header) == false);
    assert (hasAperture (header) == false);
    assert (hasIsoSpeed (header) == false);
}


} // namespace


void
testStandardAttributes ()
{
    try
    {
	cout << "Testing optional standard attributes" << endl;

	convertRGBtoXYZ();
	writeReadChromaticities ("/var/tmp/imf_test_chromaticities.exr");
	generatedFunctions();

	cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
