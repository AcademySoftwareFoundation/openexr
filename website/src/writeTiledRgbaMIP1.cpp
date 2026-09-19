//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <ImfTiledRgbaFile.h>
#include <ImfRgba.h>
#include <ImfArray.h>

using namespace IMATH_NAMESPACE;
using namespace OPENEXR_IMF_NAMESPACE;

void
writeTiledRgbaMIP1 (
    const char fileName[], int width, int height, int tileWidth, int tileHeight)
{
    TiledRgbaOutputFile out (
        fileName,
        width,
        height,
        tileWidth,
        tileHeight,
        MIPMAP_LEVELS,
        ROUND_DOWN,
        WRITE_RGBA);                                        // 1

    Array2D<Rgba> pixels (height, width);                   // 2

    out.setFrameBuffer (&pixels[0][0], 1, width);           // 3

    for (int level = 0; level < out.numLevels (); ++level)  // 4
    {
        generatePixels (pixels, width, height, level);      // 5

        out.writeTiles (
            0,
            out.numXTiles (level) - 1,                      
            0,
            out.numYTiles (level) - 1,
            level);                                         // 6
    }
}
