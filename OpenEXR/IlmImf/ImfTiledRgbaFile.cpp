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


//-----------------------------------------------------------------------------
//
//	class TiledRgbaOutputFile
//	class TiledRgbaInputFile
//
//-----------------------------------------------------------------------------

#include <ImfTiledRgbaFile.h>
#include <ImfRgbaFile.h>
#include <ImfTiledOutputFile.h>
#include <ImfTiledInputFile.h>
#include <ImfChannelList.h>
#include <ImfTileDescriptionAttribute.h>

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



TiledRgbaOutputFile::TiledRgbaOutputFile
    (const char name[],
     const Header &header,
     RgbaChannels rgbaChannels,
     int tileXSize,
     int tileYSize,
     LevelMode mode)
:
    _outputFile(0)
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
    hd.setTileDescription (TileDescription (tileXSize, tileYSize, mode));
    
    _outputFile = new TiledOutputFile (name, hd);
}



TiledRgbaOutputFile::TiledRgbaOutputFile
    (OStream &os,
     const Header &header,
     RgbaChannels rgbaChannels,
     int tileXSize,
     int tileYSize,
     LevelMode mode)
:
    _outputFile(0)
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
    hd.setTileDescription (TileDescription (tileXSize, tileYSize, mode));
    
    _outputFile = new TiledOutputFile (os, hd);
}



TiledRgbaOutputFile::TiledRgbaOutputFile
    (const char name[],
     int tileXSize,
     int tileYSize,
     LevelMode mode,
     const Imath::Box2i &displayWindow,
     const Imath::Box2i &dataWindow,
     RgbaChannels rgbaChannels,
     float pixelAspectRatio,
     const Imath::V2f screenWindowCenter,
     float screenWindowWidth,
     LineOrder lineOrder,
     Compression compression)
:
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
    hd.setTileDescription (TileDescription (tileXSize, tileYSize, mode));
    
    _outputFile = new TiledOutputFile (name, hd);
}


TiledRgbaOutputFile::TiledRgbaOutputFile
    (const char name[],
     int width,
     int height,
     int tileXSize,
     int tileYSize,
     LevelMode mode,
     RgbaChannels rgbaChannels,
     float pixelAspectRatio,
     const Imath::V2f screenWindowCenter,
     float screenWindowWidth,
     LineOrder lineOrder,
     Compression compression)
:
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
    hd.setTileDescription (TileDescription (tileXSize, tileYSize, mode));
    _outputFile = new TiledOutputFile (name, hd);
}


TiledRgbaOutputFile::~TiledRgbaOutputFile ()
{
    delete _outputFile;
}


void
TiledRgbaOutputFile::setFrameBuffer (const Rgba *base,
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


const Header &
TiledRgbaOutputFile::header () const
{
    return _outputFile->header();
}


const FrameBuffer &
TiledRgbaOutputFile::frameBuffer () const
{
    return _outputFile->frameBuffer();
}


const Imath::Box2i &
TiledRgbaOutputFile::displayWindow () const
{
    return _outputFile->header().displayWindow();
}


const Imath::Box2i &
TiledRgbaOutputFile::dataWindow () const
{
    return _outputFile->header().dataWindow();
}


float	
TiledRgbaOutputFile::pixelAspectRatio () const
{
    return _outputFile->header().pixelAspectRatio();
}


const Imath::V2f
TiledRgbaOutputFile::screenWindowCenter () const
{
    return _outputFile->header().screenWindowCenter();
}


float	
TiledRgbaOutputFile::screenWindowWidth () const
{
    return _outputFile->header().screenWindowWidth();
}


LineOrder
TiledRgbaOutputFile::lineOrder () const
{
    return _outputFile->header().lineOrder();
}


Compression
TiledRgbaOutputFile::compression () const
{
    return _outputFile->header().compression();
}


RgbaChannels
TiledRgbaOutputFile::channels () const
{
    return rgbaChannels (_outputFile->header().channels());
}


unsigned int
TiledRgbaOutputFile::tileXSize () const
{
     return _outputFile->tileXSize();
}


unsigned int
TiledRgbaOutputFile::tileYSize () const
{
     return _outputFile->tileYSize();
}


LevelMode
TiledRgbaOutputFile::levelMode () const
{
     return _outputFile->levelMode();
}


int
TiledRgbaOutputFile::numLevels () const
{
     return _outputFile->numLevels();
}


int
TiledRgbaOutputFile::numXLevels () const
{
     return _outputFile->numXLevels();
}


int
TiledRgbaOutputFile::numYLevels () const
{
     return _outputFile->numYLevels();
}


bool
TiledRgbaOutputFile::isValidLevel (int lx, int ly) const
{
    return _outputFile->isValidLevel (lx, ly);
}


int
TiledRgbaOutputFile::levelWidth (int lx) const
{
     return _outputFile->levelWidth (lx);
}


int
TiledRgbaOutputFile::levelHeight (int ly) const
{
     return _outputFile->levelHeight (ly);
}


int
TiledRgbaOutputFile::numXTiles (int lx) const
{
     return _outputFile->numXTiles (lx);
}


int
TiledRgbaOutputFile::numYTiles (int ly) const
{
     return _outputFile->numYTiles (ly);
}


Imath::Box2i
TiledRgbaOutputFile::dataWindowForLevel (int l) const
{
     return _outputFile->dataWindowForLevel (l);
}


Imath::Box2i
TiledRgbaOutputFile::dataWindowForLevel (int lx, int ly) const
{
     return _outputFile->dataWindowForLevel (lx, ly);
}


Imath::Box2i
TiledRgbaOutputFile::dataWindowForTile (int dx, int dy, int l) const
{
     return _outputFile->dataWindowForTile (dx, dy, l);
}


Imath::Box2i
TiledRgbaOutputFile::dataWindowForTile (int dx, int dy, int lx, int ly) const
{
     return _outputFile->dataWindowForTile (dx, dy, lx, ly);
}


void
TiledRgbaOutputFile::writeTile (int dx, int dy, int l)
{
     _outputFile->writeTile (dx, dy, l);
}


void
TiledRgbaOutputFile::writeTile (int dx, int dy, int lx, int ly)
{
     _outputFile->writeTile (dx, dy, lx, ly);
}


TiledRgbaInputFile::TiledRgbaInputFile (const char name[]):
    _inputFile (new TiledInputFile (name))
{
    // empty
}


TiledRgbaInputFile::TiledRgbaInputFile (IStream &is):
    _inputFile (new TiledInputFile (is))
{
    // empty
}


TiledRgbaInputFile::~TiledRgbaInputFile ()
{
    delete _inputFile;
}


void	
TiledRgbaInputFile::setFrameBuffer (Rgba *base, size_t xStride, size_t yStride)
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


const Header &
TiledRgbaInputFile::header () const
{
    return _inputFile->header();
}


const char *
TiledRgbaInputFile::fileName () const
{
    return _inputFile->fileName();
}


const FrameBuffer &	
TiledRgbaInputFile::frameBuffer () const
{
    return _inputFile->frameBuffer();
}


const Imath::Box2i &
TiledRgbaInputFile::displayWindow () const
{
    return _inputFile->header().displayWindow();
}


const Imath::Box2i &
TiledRgbaInputFile::dataWindow () const
{
    return _inputFile->header().dataWindow();
}


float	
TiledRgbaInputFile::pixelAspectRatio () const
{
    return _inputFile->header().pixelAspectRatio();
}


const Imath::V2f	
TiledRgbaInputFile::screenWindowCenter () const
{
    return _inputFile->header().screenWindowCenter();
}


float	
TiledRgbaInputFile::screenWindowWidth () const
{
    return _inputFile->header().screenWindowWidth();
}


LineOrder
TiledRgbaInputFile::lineOrder () const
{
    return _inputFile->header().lineOrder();
}


Compression
TiledRgbaInputFile::compression () const
{
    return _inputFile->header().compression();
}


RgbaChannels	
TiledRgbaInputFile::channels () const
{
    return rgbaChannels (_inputFile->header().channels());
}


int
TiledRgbaInputFile::version () const
{
    return _inputFile->version();
}


unsigned int
TiledRgbaInputFile::tileXSize () const
{
     return _inputFile->tileXSize();
}


unsigned int
TiledRgbaInputFile::tileYSize () const
{
     return _inputFile->tileYSize();
}


LevelMode
TiledRgbaInputFile::levelMode () const
{
     return _inputFile->levelMode();
}


int
TiledRgbaInputFile::numLevels () const
{
     return _inputFile->numLevels();
}


int
TiledRgbaInputFile::numXLevels () const
{
     return _inputFile->numXLevels();
}


int
TiledRgbaInputFile::numYLevels () const
{
     return _inputFile->numYLevels();
}


bool
TiledRgbaInputFile::isValidLevel (int lx, int ly) const
{
    return _inputFile->isValidLevel (lx, ly);
}


int
TiledRgbaInputFile::levelWidth (int lx) const
{
     return _inputFile->levelWidth (lx);
}


int
TiledRgbaInputFile::levelHeight (int ly) const
{
     return _inputFile->levelHeight (ly);
}


int
TiledRgbaInputFile::numXTiles (int lx) const
{
     return _inputFile->numXTiles(lx);
}


int
TiledRgbaInputFile::numYTiles (int ly) const
{
     return _inputFile->numYTiles(ly);
}


Imath::Box2i
TiledRgbaInputFile::dataWindowForLevel (int l) const
{
     return _inputFile->dataWindowForLevel (l);
}


Imath::Box2i
TiledRgbaInputFile::dataWindowForLevel (int lx, int ly) const
{
     return _inputFile->dataWindowForLevel (lx, ly);
}


Imath::Box2i
TiledRgbaInputFile::dataWindowForTile (int dx, int dy, int l) const
{
     return _inputFile->dataWindowForTile (dx, dy, l);
}


Imath::Box2i
TiledRgbaInputFile::dataWindowForTile (int dx, int dy, int lx, int ly) const
{
     return _inputFile->dataWindowForTile (dx, dy, lx, ly);
}


void
TiledRgbaInputFile::readTile (int dx, int dy, int l)
{
     _inputFile->readTile (dx, dy, l);
}


void
TiledRgbaInputFile::readTile (int dx, int dy, int lx, int ly)
{
     _inputFile->readTile (dx, dy, lx, ly);
}

void		
TiledRgbaOutputFile::updatePreviewImage (const PreviewRgba newPixels[])
{
    _outputFile->updatePreviewImage (newPixels);
}


} // namespace Imf
