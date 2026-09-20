//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// [begin writeRgbaWithPreview1]
#include <ImfHeader.h>
#include <ImfRgba.h>
#include <ImfArray.h>
#include <ImfRgbaFile.h>
#include <ImfPreviewImage.h>

using namespace OPENEXR_IMF_NAMESPACE;

void
makePreviewImage (
    const Array2D<Rgba>&  pixels,
    int                   width,
    int                   height,
    Array2D<PreviewRgba>& previewPixels,
    int&                  previewWidth,
    int&                  previewHeight);

void
writeRgbaWithPreview1 (
    const char fileName[], const Array2D<Rgba>& pixels, int width, int height)
{
    Array2D<PreviewRgba> previewPixels;                                     // 1

    int previewWidth;                                                       // 2
    int previewHeight;                                                      // 3

    makePreviewImage (
        pixels,
        width,
        height,                         
        previewPixels,
        previewWidth,
        previewHeight);                                                     // 4

    Header header (width, height);                                          // 5
    header.setPreviewImage (
        PreviewImage (previewWidth, previewHeight, &previewPixels[0][0]));  // 6

    RgbaOutputFile file (fileName, header, WRITE_RGBA);                     // 7
    file.setFrameBuffer (&pixels[0][0], 1, width);                          // 8
    file.writePixels (height);                                              // 9
}
// [end writeRgbaWithPreview1]
