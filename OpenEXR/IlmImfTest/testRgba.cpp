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
#include <ImfArray.h>
#include <string>
#include <stdio.h>
#include <assert.h>

using namespace Imf;
using namespace Imath;
using namespace std;

namespace {

void
rgbaMethods ()
{
    //
    // Verify that the constructors and the assignment
    // operator for struct Rgba work.
    //

    Rgba x (2.f, 3.f, 4.f);
    assert (x.r == 2.f && x.g == 3.f && x.b == 4.f && x.a == 1.f);

    Rgba y (5.f, 6.f, 7.f, 0.f);
    assert (y.r == 5.f && y.g == 6.f && y.b == 7.f && y.a == 0.f);

    Rgba z;

    z = x;
    assert (z.r == 2.f && z.g == 3.f && z.b == 4.f && z.a == 1.f);

    z = y;
    assert (z.r == 5.f && z.g == 6.f && z.b == 7.f && z.a == 0.f);

    Rgba w (z);
    assert (w.r == 5.f && w.g == 6.f && w.b == 7.f && w.a == 0.f);
}


void
fillPixels (Array2D<Rgba> &pixels, int w, int h)
{
    for (int y = 0; y < h; ++y)
    {
	for (int x = 0; x < w; ++x)
	{
	    Rgba &p = pixels[y][x];

	    p.r = 0.5 + 0.5 * sin (0.1 * x + 0.1 * y);
	    p.g = 0.5 + 0.5 * sin (0.1 * x + 0.2 * y);
	    p.b = 0.5 + 0.5 * sin (0.1 * x + 0.3 * y);
	    p.a = (p.r + p.b + p.g) / 3.0;
	}
    }
}


void
writeReadRGBA (const char fileName[],
	       int width,
	       int height,
	       const Array2D<Rgba> &p1,
	       RgbaChannels channels,
	       LineOrder lorder,
	       Compression comp)
{
    //
    // Save the selected channels of RGBA image p1; save the
    // scan lines in the specified order.  Read the image back
    // from the file, and compare the data with the orignal.
    //

    cout << "channels " <<
	    ((channels & WRITE_R)? "R": "") <<
	    ((channels & WRITE_G)? "G": "") <<
	    ((channels & WRITE_B)? "B": "") <<
	    ((channels & WRITE_A)? "A": "") <<
	    ", line order " << lorder <<
	    ", compression " << comp << endl;

    Header header (width, height);
    header.lineOrder() = lorder;
    header.compression() = comp;

    {
	remove (fileName);
	RgbaOutputFile out (fileName, header, channels);
	out.setFrameBuffer (&p1[0][0], 1, width);
	out.writePixels (height);
    }

    {
	RgbaInputFile in (fileName);
	const Box2i &dw = in.dataWindow();
	
	int w = dw.max.x - dw.min.x + 1;
	int h = dw.max.y - dw.min.y + 1;
	int dx = dw.min.x;
	int dy = dw.min.y;

	Array2D<Rgba> p2 (h, w);
	in.setFrameBuffer (&p2[-dy][-dx], 1, w);
	in.readPixels (dw.min.y, dw.max.y);

	assert (in.displayWindow() == header.displayWindow());
	assert (in.dataWindow() == header.dataWindow());
	assert (in.pixelAspectRatio() == header.pixelAspectRatio());
	assert (in.screenWindowCenter() == header.screenWindowCenter());
	assert (in.screenWindowWidth() == header.screenWindowWidth());
	assert (in.lineOrder() == header.lineOrder());
	assert (in.compression() == header.compression());
	assert (in.channels() == channels);

	for (int y = 0; y < h; ++y)
	{
	    for (int x = 0; x < w; ++x)
	    {
		if (channels & WRITE_R)
		    assert (p2[y][x].r == p1[y][x].r);
		else
		    assert (p2[y][x].r == 0);

		if (channels & WRITE_G)
		    assert (p2[y][x].g == p1[y][x].g);
		else
		    assert (p2[y][x].g == 0);

		if (channels & WRITE_B)
		    assert (p2[y][x].b == p1[y][x].b);
		else
		    assert (p2[y][x].b == 0);

		if (channels & WRITE_A)
		    assert (p2[y][x].a == p1[y][x].a);
		else
		    assert (p2[y][x].a == 1);
	    }
	}
    }

    remove (fileName);
}


} // namespace


void
testRgba ()
{
    try
    {
	cout << "Testing the RGBA image interface" << endl;

	rgbaMethods ();

	const int W = 237;
	const int H = 119;

	Array2D<Rgba> p1 (H, W);
	fillPixels (p1, W, H);

	for (int lorder = 0; lorder < RANDOM_Y; ++lorder)
	{
	    for (int comp = 0; comp < NUM_COMPRESSION_METHODS; ++comp)
	    {
		writeReadRGBA (IMF_TMP_DIR "imf_test_rgba.exr",
			       W, H, p1,
			       WRITE_RGBA,
			       LineOrder (lorder),
			       Compression (comp));

		writeReadRGBA (IMF_TMP_DIR "imf_test_rgba.exr",
			       W, H, p1,
			       WRITE_RGB,
			       LineOrder (lorder),
			       Compression (comp));

		writeReadRGBA ("imf_test_rgba.exr",
			       W, H, p1,
			       WRITE_A,
			       LineOrder (lorder),
			       Compression (comp));

		writeReadRGBA ("imf_test_rgba.exr",
			       W, H, p1,
			       RgbaChannels (WRITE_R | WRITE_B),
			       LineOrder (lorder),
			       Compression (comp));
	    }
	}

	cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
