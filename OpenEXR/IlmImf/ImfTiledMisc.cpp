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
//	Miscellaneous stuff related to tiled files
//
//-----------------------------------------------------------------------------

#include <ImfTiledMisc.h>
#include <Iex.h>
#include <ImfMisc.h>
#include <ImfChannelList.h>

#if defined PLATFORM_WIN32 && _MSC_VER < 1300
namespace
{
template<class T>
inline T min (const T &a, const T &b) { return (a <= b) ? a : b; }

template<class T>
inline T wmax (const T &a, const T &b) { return (a >= b) ? a : b; }
}
#endif

namespace Imf {

using Imath::Box2i;
using Imath::V2i;


int
levelSize (int min, int max, int l)
{
    if (l < 0)
	throw Iex::ArgExc ("Argument not in valid range.");

#if defined PLATFORM_WIN32 && _MSC_VER < 1300
    return wmax((max - min + 1) / (1 << l), 1);
#else
    return std::max((max - min + 1) / (1 << l), 1);
#endif
}


Box2i
dataWindowForLevel (int minX, int maxX,
		    int minY, int maxY,
		    int lx, int ly)
{
    V2i levelMin = V2i (minX, minY);
    V2i levelMax = levelMin + V2i (levelSize (minX, maxX, lx) - 1,
				   levelSize (minY, maxY, ly) - 1);

    return Box2i(levelMin, levelMax);
}


Box2i
dataWindowForTile (int minX, int maxX,
		   int minY, int maxY,
		   int tileXSize, int tileYSize,
		   int dx, int dy,
		   int lx, int ly)
{
    V2i tileMin = V2i (minX + dx * tileXSize,
		       minY + dy * tileYSize);

    V2i tileMax = tileMin + V2i (tileXSize - 1, tileYSize - 1);

    V2i levelMax = dataWindowForLevel (minX, maxX, minY, maxY, lx, ly).max;

#if defined PLATFORM_WIN32 && _MSC_VER < 1300
    tileMax = V2i (min (tileMax[0], levelMax[0]),
		   min (tileMax[1], levelMax[1]));
#else
    tileMax = V2i (std::min (tileMax[0], levelMax[0]),
		   std::min (tileMax[1], levelMax[1]));
#endif

    return Box2i (tileMin, tileMax);
}


size_t
calculateBytesPerPixel (const Header &header)
{
    const ChannelList &channels = header.channels();

    size_t bytesPerPixel = 0;

    for (ChannelList::ConstIterator c = channels.begin();
	 c != channels.end();
	 ++c)
    {
	bytesPerPixel += pixelTypeSize (c.channel().type);
    }

    return bytesPerPixel;
}


namespace {

int
floorLog2 (int x)
{
    //
    // For x > 0, floorLog2(y) returns floor(log(x)/log(2)).
    //

    int y = 0;

    while (x > 1)
    {
	y +=  1;
	x >>= 1;
    }

    return y;
}


int
calculateNumXLevels (const TileDescription& tileDesc,
		     int minX, int maxX,
		     int minY, int maxY)
{
    int num = 0;

    switch (tileDesc.mode)
    {
      case ONE_LEVEL:

	num = 1;
	break;

      case MIPMAP_LEVELS:

	{
	  int w = maxX - minX + 1;
	  int h = maxY - minY + 1;

#if defined PLATFORM_WIN32 && _MSC_VER < 1300
	  num = floorLog2 (wmax (w, h)) + 1;
#else
	  num = floorLog2 (std::max (w, h)) + 1;
#endif
	}
        break;

      case RIPMAP_LEVELS:

	{
	  int w = maxX - minX + 1;
	  num = floorLog2 (w) + 1;
	}
	break;

      default:

	throw Iex::ArgExc ("Unknown LevelMode format.");
    }

    return num;
}


int
calculateNumYLevels (const TileDescription& tileDesc,
		     int minX, int maxX,
		     int minY, int maxY)
{
    int num = 0;

    switch (tileDesc.mode)
    {
      case ONE_LEVEL:

	num = 1;
	break;

      case MIPMAP_LEVELS:

	{
	  int w = maxX - minX + 1;
	  int h = maxY - minY + 1;

#if defined PLATFORM_WIN32 && _MSC_VER < 1300
	  num = floorLog2 (wmax (w, h)) + 1;
#else
	  num = floorLog2 (std::max (w, h)) + 1;
#endif
	}
        break;

      case RIPMAP_LEVELS:

	{
	  int h = maxY - minY + 1;
	  num = floorLog2 (h) + 1;
	}
	break;

      default:

	throw Iex::ArgExc ("Unknown LevelMode format.");
    }

    return num;
}


void
calculateNumXTiles (int *numXTiles,
		    int numXLevels,
		    int minX, int maxX,
		    int xSize)
{
    for (int i = 0; i < numXLevels; i++)
    {
	numXTiles[i] = (levelSize (minX, maxX, i) + xSize - 1) / xSize;
    }
}


void
calculateNumYTiles (int *numYTiles,
		    int numYLevels,
		    int minY, int maxY,
		    int ySize)
{
    for (int i = 0; i < numYLevels; i++)
    {
	numYTiles[i] = (levelSize (minY, maxY, i) + ySize - 1) / ySize;
    }
}

} // namespace


void
precalculateTileInfo (const TileDescription& tileDesc,
		      int minX, int maxX,
		      int minY, int maxY,
		      int *&numXTiles, int *&numYTiles,
		      int &numXLevels, int &numYLevels)
{
    numXLevels = calculateNumXLevels(tileDesc, minX, maxX, minY, maxY);
    numYLevels = calculateNumYLevels(tileDesc, minX, maxX, minY, maxY);
    
    numXTiles = new int[numXLevels];
    numYTiles = new int[numYLevels];
    calculateNumXTiles(numXTiles, numXLevels, minX, maxX, tileDesc.xSize);
    calculateNumYTiles(numYTiles, numYLevels, minY, maxY, tileDesc.ySize);
}


} // namespace Imf
