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



#include <ImfOutputFile.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfArray.h>
#include <half.h>

#include <ImfBoxAttribute.h>
#include <ImfChannelListAttribute.h>
#include <ImfCompressionAttribute.h>
#include <ImfFloatAttribute.h>
#include <ImfDoubleAttribute.h>
#include <ImfIntAttribute.h>
#include <ImfLineOrderAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfOpaqueAttribute.h>
#include <ImfStringAttribute.h>
#include <ImfVecAttribute.h>

#include <stdio.h>
#include <assert.h>

using namespace std;
using namespace Imath;
using namespace Imf;

namespace {

void
fillPixels (Array2D<float> &pf, int width, int height)
{
    for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x)
	    pf[y][x] = x % 10 + 10 * (y % 17);
}


void
writeReadAttr (const Array2D<float> &pf1,
	       const char fileName[],
	       int width,
	       int height)
{
    //
    // We don't test ChannelList, LineOrder, Compression and opaque
    // attributes here; those types are covered by other tests.
    //

    Box2i  a1  (V2i (1, 2), V2i (3, 4));
    Box2f  a2  (V2f (1.5, 2.5), V2f (3.5, 4.5));
    float  a3  (3.14159);
    int    a4  (17);
    M33f   a5  (11, 12, 13, 14, 15, 16, 17, 18, 19);
    M44f   a6  (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    string a7  ("extensive rebuilding by Nebuchadrezzar has left");
    V2i    a8  (27, 28);
    V2f    a9  (27.5, 28.5);
    V3i    a10 (37, 38, 39);
    V3f    a11 (37.5, 38.5, 39.5);
    double a12 (7.12342341419);

    //
    // Write an image file with extra attributes in the header
    //

    {
	Header hdr (width, height);

	hdr.insert ("a1",  Box2iAttribute  (a1));
	hdr.insert ("a2",  Box2fAttribute  (a2));
	hdr.insert ("a3",  FloatAttribute  (a3));
	hdr.insert ("a4",  IntAttribute    (a4));
	hdr.insert ("a5",  M33fAttribute   (a5));
	hdr.insert ("a6",  M44fAttribute   (a6));
	hdr.insert ("a7",  StringAttribute (a7));
	hdr.insert ("a8",  V2iAttribute    (a8));
	hdr.insert ("a9",  V2fAttribute    (a9));
	hdr.insert ("a10", V3iAttribute    (a10));
	hdr.insert ("a11", V3fAttribute    (a11));
	hdr.insert ("a12", DoubleAttribute (a12));

	hdr.channels().insert ("F",			// name
			       Channel (FLOAT,		// type
					1,		// xSampling
					1)		// ySampling
			      );

	FrameBuffer fb; 

	fb.insert ("F",					// name
		   Slice (FLOAT,			// type
			  (char *) &pf1[0][0],		// base
			  sizeof (pf1[0][0]), 		// xStride
			  sizeof (pf1[0][0]) * width,	// yStride
			  1,				// xSampling
			  1)				// ySampling
		  );

	cout << "writing" << flush;

	remove (fileName);
	OutputFile out (fileName, hdr);
	out.setFrameBuffer (fb);
	out.writePixels (height);
    }

    //
    // Read the header back from the file, and see if the
    // values of the extra attributes come back correctly.
    //

    {
	cout << " reading" << flush;

	InputFile in (fileName);

	cout << " (version " << in.version() << ")" << flush;

	const Header &hdr = in.header();

	assert (hdr.typedAttribute <Box2iAttribute>  ("a1").value()  == a1);
	assert (hdr.typedAttribute <Box2fAttribute>  ("a2").value()  == a2);
	assert (hdr.typedAttribute <FloatAttribute>  ("a3").value()  == a3);
	assert (hdr.typedAttribute <IntAttribute>    ("a4").value()  == a4);
	assert (hdr.typedAttribute <M33fAttribute>   ("a5").value()  == a5);
	assert (hdr.typedAttribute <M44fAttribute>   ("a6").value()  == a6);
	assert (hdr.typedAttribute <StringAttribute> ("a7").value()  == a7);
	assert (hdr.typedAttribute <V2iAttribute>    ("a8").value()  == a8);
	assert (hdr.typedAttribute <V2fAttribute>    ("a9").value()  == a9);
	assert (hdr.typedAttribute <V3iAttribute>    ("a10").value() == a10);
	assert (hdr.typedAttribute <V3fAttribute>    ("a11").value() == a11);
	assert (hdr.typedAttribute <DoubleAttribute> ("a12").value() == a12);
    }

    remove (fileName);
    cout << endl;
}


} // namespace


void
testAttributes ()
{
    try
    {
	cout << "Testing built-in attributes" << endl;

	const int W = 217;
	const int H = 197;

	Array2D<float> pf (H, W);
	fillPixels (pf, W, H);

#ifdef PLATFORM_WIN32
	const char * filename = "imf_test_attr.exr";
#else
	const char * filename = "/var/tmp/imf_test_attr.exr";
#endif

	writeReadAttr (pf, filename, W, H);

	cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
