//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004, Industrial Light & Magic, a division of Lucasfilm
// Entertainment Company Ltd.  Portions contributed and copyright held by
// others as indicated.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided with
//       the distribution.
//
//     * Neither the name of Industrial Light & Magic nor the names of
//       any other contributors to this software may be used to endorse or
//       promote products derived from this software without specific prior
//       written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//
//	Functions to load OpenEXR images into a pixel array.
//
//----------------------------------------------------------------------------

#include <loadImage.h>
#include <ImfRgbaFile.h>
#include <ImfTiledRgbaFile.h>
#include <ImfInputFile.h>
#include <ImfTiledInputFile.h>
#include <ImfPreviewImage.h>
#include <ImfChannelList.h>
#include <ImfStandardAttributes.h>
#include <Iex.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

using namespace Imath;
using namespace Imf;
using namespace std;

namespace {

Chromaticities
fileChromaticities (const Header &header)
{
    Chromaticities c;  // default-initialized according to Rec. 709

    if (hasChromaticities (header))
	c = chromaticities (header);

    return c;
}


Chromaticities
displayChromaticities ()
{
    static const char chromaticitiesEnv[] = "CTL_DISPLAY_CHROMATICITIES";
    Chromaticities c;  // default-initialized according to Rec. 709

    if (const char *chromaticities = getenv (chromaticitiesEnv))
    {
	Chromaticities tmp;

	int n = sscanf (chromaticities,
			" red %f %f green %f %f blue %f %f white %f %f",
			&tmp.red.x, &tmp.red.y,
			&tmp.green.x, &tmp.green.y,
			&tmp.blue.x, &tmp.blue.y,
			&tmp.white.x, &tmp.white.y);

	if (n == 8)
	{
	    c = tmp;
	}
	else
	{
	    cerr << "Warning: cannot parse environment variable " <<
		     chromaticitiesEnv << "; using default value "
		     "(chromaticities according to Rec. ITU-R BT.709)." <<
		     endl;
	}
    }

    return c;
}


void
adjustChromaticities (Array<Rgba> &pixels,
		      size_t numPixels,
		      const Chromaticities &fileChroma,
		      const Chromaticities &displayChroma)
{
    //
    // If the chromaticities of the RGB pixel loaded from a file
    // are not the same as the chromaticities of the display,
    // then transform the pixels from the file's to the display's
    // RGB coordinate system.
    //

    if (fileChroma.red   == displayChroma.red &&
	fileChroma.green == displayChroma.green &&
	fileChroma.blue  == displayChroma.blue &&
	fileChroma.white == displayChroma.white)
    {
	return;
    }

    M44f M = RGBtoXYZ (fileChroma, 1) * XYZtoRGB (displayChroma, 1);

    for (size_t i = 0; i < numPixels; ++i)
    {
	Rgba &p = pixels[i];
	V3f rgb = V3f (p.r, p.g, p.b) * M;

	p.r = rgb[0];
	p.g = rgb[1];
	p.b = rgb[2];
    }
}

} // namespace


void
loadImage (const char fileName[],
	   Box2i &displayWindow,
	   Box2i &dataWindow,
	   float &pixelAspectRatio,
	   Array<Rgba> &pixels)
{
    RgbaInputFile in (fileName);

    displayWindow = in.displayWindow();
    dataWindow = in.dataWindow();
    pixelAspectRatio = in.pixelAspectRatio();

    int dw, dh, dx, dy;

    dw = dataWindow.max.x - dataWindow.min.x + 1;
    dh = dataWindow.max.y - dataWindow.min.y + 1;
    dx = dataWindow.min.x;
    dy = dataWindow.min.y;

    pixels.resizeErase (dw * dh);
    in.setFrameBuffer (pixels - dx - dy * dw, 1, dw);

    try
    {
	in.readPixels (dataWindow.min.y, dataWindow.max.y);
    }
    catch (const exception &e)
    {
	//
	// If some of the pixels in the file cannot be read,
	// print an error message, and return a partial image
	// to the caller.
	//

	cerr << e.what() << endl;
    }

    adjustChromaticities (pixels,
			  dw * dh,
		          fileChromaticities (in.header()),
			  displayChromaticities ());
}


void
loadTiledImage (const char fileName[],
		int lx,
		int ly,
		Box2i &displayWindow,
		Box2i &dataWindow,
	        float &pixelAspectRatio,
	        Array<Rgba> &pixels)
{
    TiledRgbaInputFile in (fileName);

    if (!in.isValidLevel (lx, ly))
    {
	THROW (Iex::InputExc, "Level (" << lx << ", " << ly << ") does "
			      "not exist in file " << fileName << ".");
    }

    dataWindow = in.dataWindowForLevel (lx, ly);
    displayWindow = dataWindow;
    pixelAspectRatio = in.pixelAspectRatio();

    int dw, dh, dx, dy;

    dw = dataWindow.max.x - dataWindow.min.x + 1;
    dh = dataWindow.max.y - dataWindow.min.y + 1;
    dx = dataWindow.min.x;
    dy = dataWindow.min.y;

    pixels.resizeErase (dw * dh);
    in.setFrameBuffer (pixels - dx - dy * dw, 1, dw);

    try
    {
	int tx = in.numXTiles (lx);
	int ty = in.numYTiles (ly);

	//
	// For maximum speed, try to read the tiles in
	// the same order as they are stored in the file.
	//

	if (in.lineOrder() == INCREASING_Y)
	{
	    for (int y = 0; y < ty; ++y)
		for (int x = 0; x < tx; ++x)
		    in.readTile (x, y, lx, ly);
	}
	else
	{
	    for (int y = ty - 1; y >= 0; --y)
		for (int x = 0; x < tx; ++x)
		    in.readTile (x, y, lx, ly);
	}
    }
    catch (const exception &e)
    {
	//
	// If some of the tiles in the file cannot be read,
	// print an error message, and return a partial image
	// to the caller.
	//

	cerr << e.what() << endl;
    }

    adjustChromaticities (pixels,
			  dw * dh,
		          fileChromaticities (in.header()),
			  displayChromaticities ());
}


void
loadPreviewImage (const char fileName[],
	          Box2i &displayWindow,
	          Box2i &dataWindow,
		  float &pixelAspectRatio,
		  Array<Rgba> &pixels)
{
    RgbaInputFile in (fileName);

    if (!in.header().hasPreviewImage())
    {
	THROW (Iex::InputExc, "File " << fileName << " "
			      "contains no preview image.");
    }

    const PreviewImage &preview = in.header().previewImage();
    int w = preview.width();
    int h = preview.height();

    displayWindow = Box2i (V2i (0, 0), V2i (w-1, h-1));
    dataWindow = displayWindow;
    pixelAspectRatio = 1;

    pixels.resizeErase (w * h);

    //
    // Convert the 8-bit gamma-2.2 preview pixels
    // into linear 16-bit floating-point pixels.
    //

    for (int i = 0; i < w * h; ++i)
    {
	Rgba &p = pixels[i];
	const PreviewRgba &q = preview.pixels()[i];

	p.r = 2.f * pow (q.r / 255.f, 2.2f);
	p.g = 2.f * pow (q.g / 255.f, 2.2f);
	p.b = 2.f * pow (q.b / 255.f, 2.2f);
	p.a = q.a / 255.f;
    }
}


void
loadImageChannel (const char fileName[],
		  const char channelName[],
	          Box2i &displayWindow,
	          Box2i &dataWindow,
		  float &pixelAspectRatio,
		  Array<Rgba> &pixels)
{
    InputFile in (fileName);

    displayWindow = in.header().displayWindow();
    dataWindow = in.header().dataWindow();
    pixelAspectRatio = in.header().pixelAspectRatio();

    int dw, dh, dx, dy;

    dw = dataWindow.max.x - dataWindow.min.x + 1;
    dh = dataWindow.max.y - dataWindow.min.y + 1;
    dx = dataWindow.min.x;
    dy = dataWindow.min.y;

    pixels.resizeErase (dw * dh);

    for (int i = 0; i < dw * dh; ++i)
    {
	pixels[i].r = half::qNan();
	pixels[i].g = half::qNan();
	pixels[i].b = half::qNan();
    }

    if (const Channel *ch = in.header().channels().findChannel (channelName))
    {
	FrameBuffer fb;

	fb.insert (channelName,
		   Slice (HALF,
			  (char *) &pixels[-dx - dy * dw].g,
			  sizeof (Rgba) * ch->xSampling,
			  sizeof (Rgba) * ch->ySampling * dw,
			  ch->xSampling,
			  ch->ySampling));

	in.setFrameBuffer (fb);
    }
    else
    {
	cerr << "Image file \"" << fileName << "\" has no "
		"channel named \"" << channelName << "\"." << endl;
	return;
    }

    try
    {
	in.readPixels (dataWindow.min.y, dataWindow.max.y);
    }
    catch (const exception &e)
    {
	//
	// If some of the pixels in the file cannot be read,
	// print an error message, and return a partial image
	// to the caller.
	//

	cerr << e.what() << endl;
    }

    for (int i = 0; i < dw * dh; ++i)
    {
	pixels[i].r = pixels[i].g;
	pixels[i].b = pixels[i].g;
    }
}


void
loadTiledImageChannel (const char fileName[],
		       const char channelName[],
		       int lx,
		       int ly,
		       Box2i &displayWindow,
		       Box2i &dataWindow,
		       float &pixelAspectRatio,
		       Array<Rgba> &pixels)
{
    TiledInputFile in (fileName);

    if (!in.isValidLevel (lx, ly))
    {
	THROW (Iex::InputExc, "Level (" << lx << ", " << ly << ") does "
			      "not exist in file " << fileName << ".");
    }

    dataWindow = in.dataWindowForLevel (lx, ly);
    displayWindow = dataWindow;
    pixelAspectRatio = in.header().pixelAspectRatio();

    int dw, dh, dx, dy;

    dw = dataWindow.max.x - dataWindow.min.x + 1;
    dh = dataWindow.max.y - dataWindow.min.y + 1;
    dx = dataWindow.min.x;
    dy = dataWindow.min.y;

    pixels.resizeErase (dw * dh);

    for (int i = 0; i < dw * dh; ++i)
    {
	pixels[i].r = half::qNan();
	pixels[i].g = half::qNan();
	pixels[i].b = half::qNan();
    }

    if (const Channel *ch = in.header().channels().findChannel (channelName))
    {
	FrameBuffer fb;

	fb.insert (channelName,
		   Slice (HALF,
			  (char *) &pixels[-dx - dy * dw].g,
			  sizeof (Rgba) * ch->xSampling,
			  sizeof (Rgba) * ch->ySampling * dw,
			  ch->xSampling,
			  ch->ySampling));

	in.setFrameBuffer (fb);
    }
    else
    {
	cerr << "Image file \"" << fileName << "\" has no "
		"channel named \"" << channelName << "\"." << endl;
	return;
    }

    try
    {
	int tx = in.numXTiles (lx);
	int ty = in.numYTiles (ly);

	//
	// For maximum speed, try to read the tiles in
	// the same order as they are stored in the file.
	//

	if (in.header().lineOrder() == INCREASING_Y)
	{
	    for (int y = 0; y < ty; ++y)
		for (int x = 0; x < tx; ++x)
		    in.readTile (x, y, lx, ly);
	}
	else
	{
	    for (int y = ty - 1; y >= 0; --y)
		for (int x = 0; x < tx; ++x)
		    in.readTile (x, y, lx, ly);
	}
    }
    catch (const exception &e)
    {
	//
	// If some of the tiles in the file cannot be read,
	// print an error message, and return a partial image
	// to the caller.
	//

	cerr << e.what() << endl;
    }

    for (int i = 0; i < dw * dh; ++i)
    {
	pixels[i].r = pixels[i].g;
	pixels[i].b = pixels[i].g;
    }
}
