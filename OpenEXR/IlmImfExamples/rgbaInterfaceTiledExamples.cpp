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
//	Code examples that show how class TiledRgbaInputFile and
//	class TiledRgbaOutputFile can be used to read and write
//	OpenEXR image files with 16-bit floating-point red,
//	green, blue and alpha channels.
//
//-----------------------------------------------------------------------------


#include <ImfTiledRgbaFile.h>
#include <ImfStringAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfArray.h>
#include <drawImage.h>

#include <iostream>

using namespace std;
using namespace Imf;
using namespace Imath;

#if defined PLATFORM_WIN32 && _MSC_VER < 1300
namespace
{
template<class T>
inline T min (const T &a, const T &b) { return (a <= b) ? a : b; }

template<class T>
inline T max (const T &a, const T &b) { return (a >= b) ? a : b; }
}
#endif

void
writeTiledRgbaONE1 (const char fileName[],
                    Array2D<Rgba> &p,
                    int width,
                    int height,
                    int xSize, int ySize)
{
    //
    // Write a Tiled image with one level using image-sized framebuffers.
    //

    TiledRgbaOutputFile out (fileName,
                             width, height, // width, height
                             xSize, ySize,  // tile size
                             ONE_LEVEL,     // level mode
                             ROUND_DOWN,    // rounding mode
                             WRITE_RGB);    // channels in file

    out.setFrameBuffer (&p[0][0], 1, width);

    drawImage3 (p, width, height, 0, width, 0, height, 0);

    for (int tileY = 0; tileY < out.numYTiles (); ++tileY)
        for (int tileX = 0; tileX < out.numXTiles (); ++tileX)
            out.writeTile (tileX, tileY);
}


void
writeTiledRgbaONE2 (const char fileName[],
                    Array2D<Rgba> &p,
                    int width, int height,
                    int xSize, int ySize)
{
    //
    // Write a Tiled image with one level using tile-sized framebuffers.
    //

    TiledRgbaOutputFile out (fileName,
                             width, height, // width, height
                             xSize, ySize,  // tile size
                             ONE_LEVEL,     // level mode
                             ROUND_DOWN,    // rounding mode
                             WRITE_RGB);    // channels in file

    for (int tileY = 0; tileY < out.numYTiles (); ++tileY)
    {
        for (int tileX = 0; tileX < out.numXTiles (); ++tileX)
        {
            Box2i range = out.dataWindowForTile (tileX, tileY);

            drawImage3 (p, width, height,
                        range.min.x, range.max.x+1,
                        range.min.y, range.max.y+1,
                        0);

            out.setFrameBuffer (&p[-range.min.y][-range.min.x],
                                1,        // xStride
                                xSize);   // yStride

            out.writeTile (tileX, tileY);
        }
    }
}



void
writeTiledRgbaMIP1 (const char fileName[],
                    Array2D<Rgba> &p,
                    int width, int height,
                    int xSize, int ySize)
{
    //
    // Write a Tiled image with mipmap levels using image-sized framebuffers.
    //

    TiledRgbaOutputFile out (fileName,
                             width, height, // width, height
                             xSize, ySize,  // tile size
                             MIPMAP_LEVELS, // level mode
                             ROUND_DOWN,    // rounding mode
                             WRITE_RGB);    // channels in file

    out.setFrameBuffer (&p[0][0], 1, width);

    for (int level = 0; level < out.numLevels (); ++level)
    {
        drawImage4 (p, out.levelWidth(level), out.levelHeight(level),
                    0, out.levelWidth(level), 0, out.levelHeight(level),
                    level);

        for (int tileY = 0; tileY < out.numYTiles (level); ++tileY)
            for (int tileX = 0; tileX < out.numXTiles (level); ++tileX)
                out.writeTile (tileX, tileY, level);
    }
}


void
writeTiledRgbaMIP2 (const char fileName[],
                    Array2D<Rgba> &p,
                    int width, int height,
                    int xSize, int ySize)
{
    //
    // Write a Tiled image with mipmap levels using tile-sized framebuffers.
    //

    TiledRgbaOutputFile out (fileName,
                             width, height, // width, height
                             xSize, ySize,  // tile size
                             MIPMAP_LEVELS, // level mode
                             ROUND_DOWN,    // rounding mode
                             WRITE_RGB);    // channels in file

    for (int level = 0; level < out.numLevels (); ++level)
    {
        for (int tileY = 0; tileY < out.numYTiles (level); ++tileY)
        {
            for (int tileX = 0; tileX < out.numXTiles (level); ++tileX)
            {
                Box2i range = out.dataWindowForTile (tileX, tileY, level);

                drawImage4 (p, out.levelWidth(level), out.levelHeight(level),
                            range.min.x, range.max.x+1,
                            range.min.y, range.max.y+1,
                            level);

                out.setFrameBuffer (&p[-range.min.y][-range.min.x],
                                    1,        // xStride
                                    xSize);   // yStride

                out.writeTile (tileX, tileY, level);
            }
        }
    }
}



void
writeTiledRgbaRIP1 (const char fileName[],
                    Array2D<Rgba> &p,
                    int width, int height,
                    int xSize, int ySize)
{
    //
    // Write a Tiled image with ripmap levels using image-sized framebuffers.
    //

    TiledRgbaOutputFile out (fileName,
                             width, height, // width, height
                             xSize, ySize,  // tile size
                             RIPMAP_LEVELS, // level mode
                             ROUND_DOWN,    // rounding mode
                             WRITE_RGB);    // channels in file

    out.setFrameBuffer (&p[0][0], 1, width);

    for (int ylevel = 0; ylevel < out.numYLevels (); ++ylevel)
    {
        for (int xlevel = 0; xlevel < out.numXLevels (); ++xlevel)
        {
            drawImage5 (p, out.levelWidth(xlevel), out.levelHeight(ylevel),
                        0, out.levelWidth(xlevel), 0, out.levelHeight(ylevel));

            for (int tileY = 0; tileY < out.numYTiles (ylevel); ++tileY)
                for (int tileX = 0; tileX < out.numXTiles (xlevel); ++tileX)
                    out.writeTile (tileX, tileY, xlevel, ylevel);
        }
    }
}


void
writeTiledRgbaRIP2 (const char fileName[],
                    Array2D<Rgba> &p,
                    int width, int height,
                    int xSize, int ySize)
{
    //
    // Write a Tiled image with ripmap levels using tile-sized framebuffers.
    //

    TiledRgbaOutputFile out (fileName,
                             width, height, // width, height
                             xSize, ySize,  // tile size
                             RIPMAP_LEVELS, // level mode
                             ROUND_DOWN,    // rounding mode
                             WRITE_RGB);    // channels in file

    for (int ylevel = 0; ylevel < out.numYLevels (); ++ylevel)
    {
        for (int xlevel = 0; xlevel < out.numXLevels (); ++xlevel)
        {
            for (int tileY = 0; tileY < out.numYTiles (ylevel); ++tileY)
            {
                for (int tileX = 0; tileX < out.numXTiles (xlevel); ++tileX)
                {
                    Box2i range = out.dataWindowForTile (tileX, tileY, xlevel, ylevel);

                    drawImage5 (p, out.levelWidth(xlevel), out.levelHeight(ylevel),
                                range.min.x, range.max.x+1,
                                range.min.y, range.max.y+1,
                                min(xlevel,ylevel));

                    out.setFrameBuffer (&p[-range.min.y][-range.min.x],
                                        1,        // xStride
                                        xSize);   // yStride

                    out.writeTile (tileX, tileY, xlevel, ylevel);
                }
            }
        }
    }
}


void
readTiledRgba1 (const char fileName[],
                Array2D<Rgba> &pixels,
                int &width,
                int &height)
{
    TiledRgbaInputFile in (fileName);
    Box2i dw = in.dataWindow();

    width  = dw.max.x - dw.min.x + 1;
    height = dw.max.y - dw.min.y + 1;
    int dx = dw.min.x;
    int dy = dw.min.y;

    pixels.resizeErase (height, width);

    in.setFrameBuffer (&pixels[-dy][-dx], 1, width);

    for (int tileY = 0; tileY < in.numYTiles(); ++tileY)
        for (int tileX = 0; tileX < in.numXTiles(); ++tileX)
            in.readTile (tileX, tileY);
}


void
rgbaInterfaceTiledExamples ()
{
    cout << "\nRGBA tiled images\n" << endl;
    cout << "drawing image" << endl;

    int w = 512;
    int h = 512;
    int tx = 100;
    int ty = 75;

    Array2D<Rgba> p1 (h, w);
    Array2D<Rgba> p2 (ty, tx);

    cout << "writing tiled image with image-size framebuffer" << endl;

    writeTiledRgbaONE1 ("tiledrgba1.exr", p1, w, h, tx, ty);

    cout << "writing tiled image with tile-size framebuffer" << endl;

    writeTiledRgbaONE2 ("tiledrgba2.exr", p2, w, h, tx, ty);

    cout << "writing tiled mipmap image with image-size framebuffer" << endl;

    writeTiledRgbaMIP1 ("tiledrgba3.exr", p1, w, h, tx, ty);

    cout << "writing tiled mipmap image with tile-size framebuffer" << endl;

    writeTiledRgbaMIP2 ("tiledrgba4.exr", p2, w, h, tx, ty);

    cout << "writing tiled ripmap image with image-size framebuffer" << endl;

    writeTiledRgbaRIP1 ("tiledrgba5.exr", p1, w, h, tx, ty);

    cout << "writing tiled ripmap image with tile-size framebuffer" << endl;

    writeTiledRgbaRIP2 ("tiledrgba6.exr", p2, w, h, tx, ty);

    cout << "reading tiled rgba file" << endl;

    readTiledRgba1 ("tiledrgba1.exr", p1, w, h);

}
