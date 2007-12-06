///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2007, Industrial Light & Magic, a division of Lucas
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
//	function readInputImage() --
//	reads an image file and constructs an EnvMapImage object
//
//-----------------------------------------------------------------------------

#include <makeCubeMap.h>

#include <ImfRgbaFile.h>
#include <ImfStandardAttributes.h>
#include <EnvmapImage.h>
#include "Iex.h"
#include <iostream>
//#include <algorithm>


using namespace std;
using namespace Imf;
using namespace Imath;


void
readInputImage (const char inFileName[],
		float padTop,
		float padBottom,
		bool verbose,
		EnvmapImage &image,
		Header &header,
		RgbaChannels &channels)
{
    //
    // Read the input image, and if necessary,
    // pad the image at the top and bottom.
    //

    RgbaInputFile in (inFileName);

    if (verbose)
	cout << "reading file " << inFileName << endl;

    header = in.header();
    channels = in.channels();

    Envmap type = ENVMAP_LATLONG;

    if (hasEnvmap (in.header()))
	type = envmap (in.header());

    const Box2i &dw = in.dataWindow();
    int w = dw.max.x - dw.min.x + 1;
    int h = dw.max.y - dw.min.y + 1;

    int pt = 0;
    int pb = 0;

    if (type == ENVMAP_LATLONG)
    {
	pt = int (padTop * h + 0.5f);
	pb = int (padBottom * h + 0.5f);
    }

    Box2i paddedDw (V2i (dw.min.x, dw.min.y - pt),
		    V2i (dw.max.x, dw.max.y + pb));
    
    image.resize (type, paddedDw);
    Array2D<Rgba> &pixels = image.pixels();

    in.setFrameBuffer (&pixels[-paddedDw.min.y][-paddedDw.min.x], 1, w);
    in.readPixels (dw.min.y, dw.max.y);

    for (int y = 0; y < pt; ++y)
	for (int x = 0; x < w; ++x)
	    pixels[y][x] = pixels[pt][x];

    for (int y = h + pt; y < h + pt + pb; ++y)
    {
	for (int x = 0; x < w; ++x)
	    pixels[y][x] = pixels[h + pt - 1][x];
    }
}
