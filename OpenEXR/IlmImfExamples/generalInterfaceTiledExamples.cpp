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
//	Code examples that show how class TiledInputFile and class TiledOutputFile
//	can be used to read and write OpenEXR image files with an arbitrary
//	set of channels.
//
//-----------------------------------------------------------------------------


#include <ImfTiledOutputFile.h>
#include <ImfTiledInputFile.h>
#include <ImfChannelList.h>
#include <ImfStringAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfArray.h>
#include <drawImage.h>

#include <iostream>

using namespace std;
using namespace Imf;
using namespace Imath;


void
writeTiled1 (const char fileName[],
             Array2D<half> &gPixels,
             int width,
             int height,
             int xSize, int ySize)
{
    Header header (width, height);
    header.channels().insert ("G", Channel (HALF));
    header.setTileDescription(TileDescription(xSize, ySize,
                                              ONE_LEVEL));
    
    TiledOutputFile out (fileName, header);

    FrameBuffer frameBuffer;

    frameBuffer.insert ("G",                                // name
                        Slice (HALF,                        // type
                        (char *) &gPixels[0][0],            // base
                        sizeof (gPixels[0][0]) * 1,         // xStride
                        sizeof (gPixels[0][0]) * width));   // yStride

    out.setFrameBuffer (frameBuffer);

    for (int tileY = 0; tileY < out.numYTiles (); ++tileY)
        for (int tileX = 0; tileX < out.numXTiles (); ++tileX)
            out.writeTile (tileX, tileY);
}


void
readTiled1 (const char fileName[],
            Array2D<half> &gPixels,
            int &width, int &height)
{
    TiledInputFile in (fileName);

    Box2i dw = in.header().dataWindow();
    width  = dw.max.x - dw.min.x + 1;
    height = dw.max.y - dw.min.y + 1;
    int dx = dw.min.x;
    int dy = dw.min.y;

    gPixels.resizeErase (height, width);

    FrameBuffer frameBuffer;

    frameBuffer.insert ("G",                              // name
                        Slice (HALF,                      // type
                        (char *) &gPixels[-dy][-dx],      // base
                        sizeof (gPixels[0][0]) * 1,       // xStride
                        sizeof (gPixels[0][0]) * width,   // yStride
                        1, 1,                             // x/y sampling
                        0.0));                            // fillValue

    in.setFrameBuffer (frameBuffer);

    for (int tileY = 0; tileY < in.numYTiles(); ++tileY)
        for (int tileX = 0; tileX < in.numXTiles(); ++tileX)
            in.readTile (tileX, tileY);
}


void
generalInterfaceTiledExamples ()
{
    cout << "\nG (green) tiled images\n" << endl;
    cout << "drawing image" << endl;

    int w = 800;
    int h = 600;

    Array2D<half>  gp (h, w);
    drawImage6 (gp, w, h);

    cout << "writing entire image" << endl;

    writeTiled1 ("gTiled1.exr", gp, w, h, 64, 64);


    cout << "reading file" << endl;

    Array2D<half> gp2 (1, 1);
    readTiled1 ("gTiled1.exr", gp2, w, h);

}
