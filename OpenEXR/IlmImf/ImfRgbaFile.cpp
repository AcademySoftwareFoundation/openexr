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




//-----------------------------------------------------------------------------
//
//	class Rgba
//	class RgbaOutputFile
//	class RgbaInputFile
//
//-----------------------------------------------------------------------------

#include <ImfRgbaFile.h>
#include <ImfOutputFile.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>

namespace Imf {

using Imath::Box2i;
using Imath::V2i;

namespace {

RgbaChannels
rgbaChannels (const ChannelList &ch)
{
    int i = 0;

    if (ch.findChannel ("R"))
	i |= WRITE_R;
    if (ch.findChannel ("G"))
	i |= WRITE_G;
    if (ch.findChannel ("B"))
	i |= WRITE_B;
    if (ch.findChannel ("A"))
	i |= WRITE_A;

    return RgbaChannels (i);
}

} // namespace


RgbaOutputFile::RgbaOutputFile (const char name[],
				const Header &header,
				RgbaChannels rgbaChannels):
    _outputFile (0)
{
    Header hd (header);
    ChannelList ch;

    if (rgbaChannels & WRITE_R)
	ch.insert ("R", Channel (HALF, 1, 1));
    if (rgbaChannels & WRITE_G)
	ch.insert ("G", Channel (HALF, 1, 1));
    if (rgbaChannels & WRITE_B)
	ch.insert ("B", Channel (HALF, 1, 1));
    if (rgbaChannels & WRITE_A)
	ch.insert ("A", Channel (HALF, 1, 1));

    hd.channels() = ch;
    _outputFile = new OutputFile (name, hd);
}


RgbaOutputFile::RgbaOutputFile (const char name[],
				const Imath::Box2i &displayWindow,
				const Imath::Box2i &dataWindow,
				RgbaChannels rgbaChannels,
				float pixelAspectRatio,
				const Imath::V2f screenWindowCenter,
				float screenWindowWidth,
				LineOrder lineOrder,
				Compression compression):
    _outputFile (0)
{
    Header hd (displayWindow,
	       dataWindow.isEmpty()? displayWindow: dataWindow,
	       pixelAspectRatio,
	       screenWindowCenter,
	       screenWindowWidth,
	       lineOrder,
	       compression);

    ChannelList ch;

    if (rgbaChannels & WRITE_R)
	ch.insert ("R", Channel (HALF, 1, 1));
    if (rgbaChannels & WRITE_G)
	ch.insert ("G", Channel (HALF, 1, 1));
    if (rgbaChannels & WRITE_B)
	ch.insert ("B", Channel (HALF, 1, 1));
    if (rgbaChannels & WRITE_A)
	ch.insert ("A", Channel (HALF, 1, 1));

    hd.channels() = ch;
    _outputFile = new OutputFile (name, hd);
}


RgbaOutputFile::RgbaOutputFile (const char name[],
				int width,
				int height,
				RgbaChannels rgbaChannels,
				float pixelAspectRatio,
				const Imath::V2f screenWindowCenter,
				float screenWindowWidth,
				LineOrder lineOrder,
				Compression compression):
    _outputFile (0)
{
    Header hd (width,
	       height,
	       pixelAspectRatio,
	       screenWindowCenter,
	       screenWindowWidth,
	       lineOrder,
	       compression);

    ChannelList ch;

    if (rgbaChannels & WRITE_R)
	ch.insert ("R", Channel (HALF, 1, 1));
    if (rgbaChannels & WRITE_G)
	ch.insert ("G", Channel (HALF, 1, 1));
    if (rgbaChannels & WRITE_B)
	ch.insert ("B", Channel (HALF, 1, 1));
    if (rgbaChannels & WRITE_A)
	ch.insert ("A", Channel (HALF, 1, 1));

    hd.channels() = ch;
    _outputFile = new OutputFile (name, hd);
}


RgbaOutputFile::~RgbaOutputFile ()
{
    delete _outputFile;
}


void
RgbaOutputFile::setFrameBuffer (const Rgba *base,
				size_t xStride,
				size_t yStride)
{
    size_t xs = xStride * sizeof (Rgba);
    size_t ys = yStride * sizeof (Rgba);

    FrameBuffer fb;

    fb.insert ("R", Slice (HALF, (char *) base + offsetof (Rgba, r), xs, ys));
    fb.insert ("G", Slice (HALF, (char *) base + offsetof (Rgba, g), xs, ys));
    fb.insert ("B", Slice (HALF, (char *) base + offsetof (Rgba, b), xs, ys));
    fb.insert ("A", Slice (HALF, (char *) base + offsetof (Rgba, a), xs, ys));

    _outputFile->setFrameBuffer (fb);
}


void	
RgbaOutputFile::writePixels (int numScanLines)
{
    _outputFile->writePixels (numScanLines);
}


int	
RgbaOutputFile::currentScanLine () const
{
    return _outputFile->currentScanLine();
}


const Header &
RgbaOutputFile::header () const
{
    return _outputFile->header();
}


const FrameBuffer &
RgbaOutputFile::frameBuffer () const
{
    return _outputFile->frameBuffer();
}


const Imath::Box2i &
RgbaOutputFile::displayWindow () const
{
    return _outputFile->header().displayWindow();
}


const Imath::Box2i &
RgbaOutputFile::dataWindow () const
{
    return _outputFile->header().dataWindow();
}


float	
RgbaOutputFile::pixelAspectRatio () const
{
    return _outputFile->header().pixelAspectRatio();
}


const Imath::V2f
RgbaOutputFile::screenWindowCenter () const
{
    return _outputFile->header().screenWindowCenter();
}


float	
RgbaOutputFile::screenWindowWidth () const
{
    return _outputFile->header().screenWindowWidth();
}


LineOrder
RgbaOutputFile::lineOrder () const
{
    return _outputFile->header().lineOrder();
}


Compression
RgbaOutputFile::compression () const
{
    return _outputFile->header().compression();
}


RgbaChannels
RgbaOutputFile::channels () const
{
    return rgbaChannels (_outputFile->header().channels());
}


void		
RgbaOutputFile::updatePreviewImage (const PreviewRgba newPixels[])
{
    _outputFile->updatePreviewImage (newPixels);
}


RgbaInputFile::RgbaInputFile (const char name[]):
    _inputFile (new InputFile (name))
{
    // empty
}


RgbaInputFile::~RgbaInputFile ()
{
    delete _inputFile;
}


void	
RgbaInputFile::setFrameBuffer (Rgba *base, size_t xStride, size_t yStride)
{
    size_t xs = xStride * sizeof (Rgba);
    size_t ys = yStride * sizeof (Rgba);

    FrameBuffer fb;

    fb.insert ("R", Slice (HALF,
			   (char *) base + offsetof (Rgba, r),
			   xs, ys,
			   1, 1,	// xSampling, ySampling
			   0.0));	// fillValue

    fb.insert ("G", Slice (HALF,
			   (char *) base + offsetof (Rgba, g),
			   xs, ys,
			   1, 1,	// xSampling, ySampling
			   0.0));	// fillValue

    fb.insert ("B", Slice (HALF,
			   (char *) base + offsetof (Rgba, b),
			   xs, ys,
			   1, 1,	// xSampling, ySampling
			   0.0));	// fillValue

    fb.insert ("A", Slice (HALF,
			   (char *) base + offsetof (Rgba, a),
			   xs, ys,
			   1, 1,	// xSampling, ySampling
			   1.0));	// fillValue

    _inputFile->setFrameBuffer (fb);
}


void	
RgbaInputFile::readPixels (int scanLine1, int scanLine2)
{
    _inputFile->readPixels (scanLine1, scanLine2);
}


void	
RgbaInputFile::readPixels (int scanLine)
{
    _inputFile->readPixels (scanLine);
}


const Header &
RgbaInputFile::header () const
{
    return _inputFile->header();
}


const char *
RgbaInputFile::fileName () const
{
    return _inputFile->fileName();
}


const FrameBuffer &	
RgbaInputFile::frameBuffer () const
{
    return _inputFile->frameBuffer();
}


const Imath::Box2i &
RgbaInputFile::displayWindow () const
{
    return _inputFile->header().displayWindow();
}


const Imath::Box2i &
RgbaInputFile::dataWindow () const
{
    return _inputFile->header().dataWindow();
}


float	
RgbaInputFile::pixelAspectRatio () const
{
    return _inputFile->header().pixelAspectRatio();
}


const Imath::V2f	
RgbaInputFile::screenWindowCenter () const
{
    return _inputFile->header().screenWindowCenter();
}


float	
RgbaInputFile::screenWindowWidth () const
{
    return _inputFile->header().screenWindowWidth();
}


LineOrder
RgbaInputFile::lineOrder () const
{
    return _inputFile->header().lineOrder();
}


Compression
RgbaInputFile::compression () const
{
    return _inputFile->header().compression();
}


RgbaChannels	
RgbaInputFile::channels () const
{
    return rgbaChannels (_inputFile->header().channels());
}


int
RgbaInputFile::version () const
{
    return _inputFile->version();
}


} // namespace Imf
