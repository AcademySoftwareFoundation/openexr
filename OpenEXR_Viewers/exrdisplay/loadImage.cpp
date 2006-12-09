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
//	Load an OpenEXR image into a pixel array.
//
//----------------------------------------------------------------------------

#include <loadImage.h>
#include <ImfRgbaFile.h>
#include <ImfTiledRgbaFile.h>
#include <ImfInputFile.h>
#include <ImfTiledInputFile.h>
#include <ImfPreviewImage.h>
#include <ImfChannelList.h>
#include <Iex.h>

using namespace Imath;
using namespace Imf;
using namespace std;

namespace {

void
loadImage (const char fileName[],
	   Header &header,
	   Array<Rgba> &pixels)
{
    RgbaInputFile in (fileName);

    header = in.header();

    Box2i &dataWindow = header.dataWindow();
    int dw = dataWindow.max.x - dataWindow.min.x + 1;
    int dh = dataWindow.max.y - dataWindow.min.y + 1;
    int dx = dataWindow.min.x;
    int dy = dataWindow.min.y;

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
}


void
loadTiledImage (const char fileName[],
		int lx,
		int ly,
		Header &header,
	        Array<Rgba> &pixels)
{
    TiledRgbaInputFile in (fileName);

    if (!in.isValidLevel (lx, ly))
    {
	THROW (Iex::InputExc, "Level (" << lx << ", " << ly << ") does "
			      "not exist in file " << fileName << ".");
    }

    header = in.header();
    header.dataWindow() = in.dataWindowForLevel (lx, ly);
    header.displayWindow() = header.dataWindow();

    Box2i &dataWindow = header.dataWindow();
    int dw = dataWindow.max.x - dataWindow.min.x + 1;
    int dh = dataWindow.max.y - dataWindow.min.y + 1;
    int dx = dataWindow.min.x;
    int dy = dataWindow.min.y;

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
}


void
loadPreviewImage (const char fileName[],
		  Header &header,
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

    header = in.header();

    header.displayWindow() = Box2i (V2i (0, 0), V2i (w-1, h-1));
    header.dataWindow() = header.displayWindow();
    header.pixelAspectRatio() = 1;

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
		  Header &header,
		  Array<Rgba> &pixels)
{
    InputFile in (fileName);

    header = in.header();

    Box2i &dataWindow = header.dataWindow();
    int dw = dataWindow.max.x - dataWindow.min.x + 1;
    int dh = dataWindow.max.y - dataWindow.min.y + 1;
    int dx = dataWindow.min.x;
    int dy = dataWindow.min.y;

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
		       Header &header,
		       Array<Rgba> &pixels)
{
    TiledInputFile in (fileName);

    if (!in.isValidLevel (lx, ly))
    {
	THROW (Iex::InputExc, "Level (" << lx << ", " << ly << ") does "
			      "not exist in file " << fileName << ".");
    }

    header = in.header();
    header.dataWindow() = in.dataWindowForLevel (lx, ly);
    header.displayWindow() = header.dataWindow();

    Box2i &dataWindow = header.dataWindow();
    int dw = dataWindow.max.x - dataWindow.min.x + 1;
    int dh = dataWindow.max.y - dataWindow.min.y + 1;
    int dx = dataWindow.min.x;
    int dy = dataWindow.min.y;

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


} // namespace


void
loadImage (const char fileName[],
	   const char channel[],
	   bool preview,
	   int lx,
	   int ly,
	   Header &header,
	   Array<Rgba> &pixels)
{
    if (preview)
    {
	loadPreviewImage (fileName,
			  header,
			  pixels);
    }
    else if (lx >= 0 || ly >= 0)
    {
	if (channel)
	{
	    loadTiledImageChannel (fileName,
				   channel,
				   lx, ly,
				   header,
				   pixels);
	}
	else
	{
	    loadTiledImage (fileName,
			    lx, ly,
			    header,
			    pixels);
	}
    }
    else
    {
	if (channel)
	{
	    loadImageChannel (fileName,
			      channel,
			      header,
			      pixels);
	}
	else
	{
	    loadImage (fileName,
		       header,
		       pixels);
	}
    }
}
