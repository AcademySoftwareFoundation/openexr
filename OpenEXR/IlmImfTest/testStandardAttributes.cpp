///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004, Industrial Light & Magic, a division of Lucas
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


#include <tmpDir.h>

#include <ImfRgbaFile.h>
#include <ImfStandardAttributes.h>
#include <ImfArray.h>
#include <ImathRandom.h>
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

	for (int i = 0; i < W; ++i)
	{
	    pixels[i].r = 1;
	    pixels[i].g = 1;
	    pixels[i].b = 1;
	    pixels[i].a = 1;
	}

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
latLongMap (const char fileName1[], const char fileName2[])
{
    cout << "latitude-longitude environment map" << endl;
    const int W = 360;
    const int H = 180;

    Header header (W, H);
    addEnvmap (header, ENVMAP_LATLONG);

    V2f pos;

    pos = LatLongMap::latLong (V3f (0, 1, 0));
    assert (equalWithAbsError (pos.x, float (M_PI/2), 1e-6f));

    pos = LatLongMap::latLong (V3f (0, -1, 0));
    assert (equalWithAbsError (pos.x, float (-M_PI/2), 1e-6f));

    pos = LatLongMap::latLong (V3f (0, 0, 1));
    assert (pos.equalWithAbsError (V2f (0, 0), 1e-6f));

    pos = LatLongMap::latLong (V3f (1, 0, 0));
    assert (pos.equalWithAbsError (V2f (0, M_PI/2), 1e-6f));

    pos = LatLongMap::latLong (V3f (-1, 0, 0));
    assert (pos.equalWithAbsError (V2f (0, -M_PI/2), 1e-6f));

    pos = LatLongMap::latLong (V3f (0, 1, 1));
    assert (pos.equalWithAbsError (V2f (M_PI/4, 0), 1e-6f));

    pos = LatLongMap::latLong (V3f (0, -1, 1));
    assert (pos.equalWithAbsError (V2f (-M_PI/4, 0), 1e-6f));

    pos = LatLongMap::pixelPosition (header.dataWindow(), V2f (M_PI/2, M_PI));
    assert (pos.equalWithAbsError (V2f (0, 0), 1e-6f * W));

    pos = LatLongMap::pixelPosition(header.dataWindow(), V2f (-M_PI/2, -M_PI));
    assert (pos.equalWithAbsError (V2f (header.dataWindow().max), 1e-6f * W));

    Array2D<Rgba> pixels (H, W);

    for (int y = 0; y < H; ++y)
    {
	for (int x = 0; x < W; ++x)
	{
	    Rgba &p = pixels[y][x];
	    V3f dir = LatLongMap::direction (header.dataWindow(), V2f (x, y));

	    p.r = dir.x + 1;
	    p.g = dir.y + 1;
	    p.b = dir.z + 1;
	}
    }

    {
	RgbaOutputFile out (fileName1, header, WRITE_RGB);
	out.setFrameBuffer (&pixels[0][0], 1, W);
	out.writePixels (H);
    }

    Rand48 rand (0);

    for (int i = 0; i < W * H * 3; ++i)
    {
	V3f dir = hollowSphereRand<V3f> (rand);
	V2f pos = LatLongMap::pixelPosition (header.dataWindow(), dir);

	Rgba &p = pixels[int (pos.y + 0.5)][int (pos.x + 0.5)];

	p.r = (dir.x + 1) * 0.8;
	p.g = (dir.y + 1) * 0.8;
	p.b = (dir.z + 1) * 0.8;

	V3f dir1 = LatLongMap::direction (header.dataWindow(), pos);
	assert (dir.equalWithAbsError (dir1.normalized(), 1e-5f));
    }

    {
	RgbaOutputFile out (fileName2, header, WRITE_RGB);
	out.setFrameBuffer (&pixels[0][0], 1, W);
	out.writePixels (H);
    }


    remove (fileName1);
    remove (fileName2);
}


void
cubeMap (const char fileName1[], const char fileName2[])
{
    cout << "cube environment map" << endl;
    const int N = 128;
    const int W = N;
    const int H = N * 6;

    Header header (W, H);
    addEnvmap (header, ENVMAP_CUBE);

    int sof = CubeMap::sizeOfFace (header.dataWindow());

    assert (sof == N);

    for (int face1 = 0; face1 < 6; ++face1)
    {
	Box2i dw1 = CubeMap::dataWindowForFace (CubeMapFace (face1),
						header.dataWindow());

	assert (dw1.max.x - dw1.min.x == sof - 1);
	assert (dw1.max.y - dw1.min.y == sof - 1);
	assert (header.dataWindow().intersects (dw1.min));
	assert (header.dataWindow().intersects (dw1.max));

	for (int face2 = face1 + 1; face2 < 6; ++face2)
	{
	    Box2i dw2 = CubeMap::dataWindowForFace (CubeMapFace (face2),
						    header.dataWindow());
	    assert (!dw1.intersects (dw2));
	}
    }

    CubeMapFace face;
    V2f pos;

    CubeMap::faceAndPixelPosition (V3f (1, 0, 0),
				   header.dataWindow(),
				   face, pos);
    
    assert (face == CUBEFACE_POS_X);
    assert (pos.equalWithAbsError (V2f ((sof - 1), (sof - 1)) / 2, 1e-6 * W));

    CubeMap::faceAndPixelPosition (V3f (-1, 0, 0),
	                           header.dataWindow(),
				   face, pos);
    
    assert (face == CUBEFACE_NEG_X);
    assert (pos.equalWithAbsError (V2f ((sof - 1), (sof - 1)) / 2, 1e-6 * W));

    CubeMap::faceAndPixelPosition (V3f (0, 1, 0),
	    			   header.dataWindow(),
				   face, pos);
    
    assert (face == CUBEFACE_POS_Y);
    assert (pos.equalWithAbsError (V2f ((sof - 1), (sof - 1)) / 2, 1e-6 * W));

    CubeMap::faceAndPixelPosition (V3f (0, -1, 0),
	    			   header.dataWindow(),
				   face, pos);
    
    assert (face == CUBEFACE_NEG_Y);
    assert (pos.equalWithAbsError (V2f ((sof - 1), (sof - 1)) / 2, 1e-6 * W));

    CubeMap::faceAndPixelPosition (V3f (0, 0, 1),
	    			   header.dataWindow(),
				   face, pos);
    
    assert (face == CUBEFACE_POS_Z);
    assert (pos.equalWithAbsError (V2f ((sof - 1), (sof - 1)) / 2, 1e-6 * W));

    CubeMap::faceAndPixelPosition (V3f (0, 0, -1),
	    			   header.dataWindow(),
				   face, pos);
    
    assert (face == CUBEFACE_NEG_Z);
    assert (pos.equalWithAbsError (V2f ((sof - 1), (sof - 1)) / 2, 1e-6 * W));

    Array2D<Rgba> pixels (H, W);

    for (int y = 0; y < H; ++y)
    {
	for (int x = 0; x < W; ++x)
	{
	    Rgba &p = pixels[y][x];
	    p.r = p.g = p.b = 0;
	}
    }

    for (int face = 0; face < 6; ++face)
    {
	for (int y = 0; y < sof; ++y)
	{
	    for (int x = 0; x < sof; ++x)
	    {
		V2f px = CubeMap::pixelPosition (CubeMapFace (face),
						 header.dataWindow(),
						 V2f (x, y));

		Rgba &p = pixels[int (px.y + 0.5)][int (px.x + 0.5)];

		V3f dir = CubeMap::direction (CubeMapFace (face),
			                      header.dataWindow(),
					      V2f (x, y));
		dir.normalize();

		p.r = dir.x + 1;
		p.g = dir.y + 1;
		p.b = dir.z + 1;
	    }
	}
    }

    {
	RgbaOutputFile out (fileName1, header, WRITE_RGB);
	out.setFrameBuffer (&pixels[0][0], 1, W);
	out.writePixels (H);
    }

    for (int y = 0; y < H; ++y)
    {
	for (int x = 0; x < W; ++x)
	{
	    Rgba &p = pixels[y][x];
	    assert (p.r != 0 || p.g != 0 || p.b != 0);
	}
    }

    Rand48 rand (0);

    for (int i = 0; i < W * H * 3; ++i)
    {
	V3f dir = hollowSphereRand<V3f> (rand);

	CubeMapFace face;
	V2f pif;

	CubeMap::faceAndPixelPosition (dir, header.dataWindow(), face, pif);

	V2f pos = CubeMap::pixelPosition (face, header.dataWindow(), pif);

	Rgba &p = pixels[int (pos.y + 0.5)][int (pos.x + 0.5)];

	p.r = (dir.x + 1) * 0.8;
	p.g = (dir.y + 1) * 0.8;
	p.b = (dir.z + 1) * 0.8;

	V3f dir1 = CubeMap::direction (face, header.dataWindow(), pif);
	assert (dir.equalWithAbsError (dir1.normalized(), 1e-6f));
    }

    {
	RgbaOutputFile out (fileName2, header, WRITE_RGB);
	out.setFrameBuffer (&pixels[0][0], 1, W);
	out.writePixels (H);
    }

    remove (fileName1);
    remove (fileName2);
}


void
generatedFunctions ()
{
    //
    // Most optional standard attributes are of type string, float,
    // etc.  The attribute types are already being tested elsewhere
    // (testAttributes.C), and the convenience functions to access
    // the standard attributes are all generated via macros.  Here
    // we just verify that all the convenience functions exist
    // (that is, ImfStandardAttributes.C and ImfStandardAttributes.h
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
    assert (hasEnvmap (header) == false);
}


} // namespace


void
testStandardAttributes ()
{
    try
    {
	cout << "Testing optional standard attributes" << endl;

	convertRGBtoXYZ();

	{
	    const char *filename = IMF_TMP_DIR "imf_test_chromaticities.exr";
	    writeReadChromaticities (filename);
	}

	{
	    const char *fn1 = IMF_TMP_DIR "imf_test_latlong1.exr";
	    const char *fn2 = IMF_TMP_DIR "imf_test_latlong2.exr";
	    latLongMap (fn1, fn2);
	}

	{
	    const char *fn1 = IMF_TMP_DIR "imf_test_cube1.exr";
	    const char *fn2 = IMF_TMP_DIR "imf_test_cube2.exr";
	    cubeMap (fn1, fn2);
	}

	generatedFunctions();

	cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
