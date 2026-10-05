//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "C_IStream.h"

#include <ImfArray.h>
#include <ImfInputFile.h>
#include <ImfRgbaFile.h>

using namespace IMATH_NAMESPACE;
using namespace OPENEXR_IMF_NAMESPACE;

void
readRgbaFILE (
    FILE*          cfile,
    const char     fileName[],
    Array2D<Rgba>& pixels,
    int&           width,
    int&           height)
{

   C_IStream istr (cfile, fileName);

    RgbaInputFile file (istr);

    Box2i dw = file.dataWindow ();

    width  = dw.max.x - dw.min.x + 1;
    height = dw.max.y - dw.min.y + 1;
    pixels.resizeErase (height, width);

    file.setFrameBuffer (&pixels[0][0] - dw.min.x - dw.min.y * width, 1, width);

    file.readPixels (dw.min.y, dw.max.y);
}
