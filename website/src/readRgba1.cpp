//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <ImfRgbaFile.h>
#include <ImfRgba.h>
#include <ImfArray.h>

using namespace IMATH_NAMESPACE;
using namespace OPENEXR_IMF_NAMESPACE;

void
readRgba1 (
    const char fileName[], Array2D<Rgba>& pixels, int& width, int& height)
{
    RgbaInputFile file (fileName);
    Box2i         dw = file.dataWindow ();

    width  = dw.max.x - dw.min.x + 1;
    height = dw.max.y - dw.min.y + 1;
    pixels.resizeErase (height, width);

    file.setFrameBuffer (&pixels[0][0] - dw.min.x - dw.min.y * width, 1, width);
    file.readPixels (dw.min.y, dw.max.y);
}
