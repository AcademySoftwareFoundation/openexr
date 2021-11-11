//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	Code examples that show how to add preview images
//	(also known as thumbnais) to OpenEXR image files.
//
//-----------------------------------------------------------------------------

#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <ImfPreviewImage.h>
#include "ImathFun.h"

#include "drawImage.h"

#include <iostream>
#include <algorithm>

#include "namespaceAlias.h"
using namespace IMF;
using namespace std;
using namespace IMATH_NAMESPACE;


unsigned char
gamma (float x)
{
    //
    // Convert a floating-point pixel value to an 8-bit gamma-2.2
    // preview pixel.  (This routine is a simplified version of
    // how the exrdisplay program transforms floating-point pixel
    // values in order to display them on the screen.)
    //

    x = pow (5.5555f * max (0.f, x), 0.4545f) * 84.66f;
    return (unsigned char) IMATH_NAMESPACE::clamp (x, 0.f, 255.f);
}


void
makePreviewImage (const Array2D <Rgba> &pixels,
		  int width,
		  int height,
		  Array2D <PreviewRgba> &previewPixels,
		  int &previewWidth,
		  int &previewHeight)
{
    const int N = 8;

    previewWidth  = width / N;
    previewHeight = height / N;
    previewPixels.resizeErase (previewHeight, previewWidth);

    for (int y = 0; y < previewHeight; ++y)
    {
	for (int x = 0; x < previewWidth; ++x)
	{
	    const Rgba  &inPixel = pixels[y * N][x * N];
	    PreviewRgba &outPixel = previewPixels[y][x];

	    outPixel.r = gamma (inPixel.r);
	    outPixel.g = gamma (inPixel.g);
	    outPixel.b = gamma (inPixel.b);
	    outPixel.a = int (IMATH_NAMESPACE::clamp (inPixel.a * 255.f, 0.f, 255.f) + 0.5f);
	}
    }
}


void
writeRgbaWithPreview1 (const char fileName[],
		       const Array2D <Rgba> &pixels,
		       int width,
		       int height)
{
    //
    // Write an image file with a preview image, version 1:
    //
    // - generate the preview image by subsampling the main image
    // - generate a file header
    // - add the preview image to the file header
    // - open the file (this stores the header with the
    //   preview image in the file)
    // - describe the memory layout of the main image's pixels
    // - store the main image's pixels in the file
    //

    Array2D <PreviewRgba> previewPixels;
    int previewWidth;
    int previewHeight;

    makePreviewImage (pixels, width, height,
		      previewPixels, previewWidth, previewHeight);

    Header header (width, height);

    header.setPreviewImage
	(PreviewImage (previewWidth, previewHeight, &previewPixels[0][0]));

    RgbaOutputFile file (fileName, header, WRITE_RGBA);
    file.setFrameBuffer (&pixels[0][0], 1, width);
    file.writePixels (height);
}


void
writeRgbaWithPreview2 (const char fileName[],
		       int width,
		       int height)
{
    //
    // Write an image file with a preview image, version 2:
    //
    // - generate a file header
    // - add a dummy preview image to the file header
    // - open the file (this stores the header with the dummy
    //   preview image in the file)
    // - render the main image's pixels one scan line at a time,
    //   and store each scan line in the file before rendering
    //   the next scan line
    // - generate the preview image on the fly, while the main
    //   image is being rendered
    // - once the main image has been rendered, store the preview
    //   image in the file, overwriting the dummy preview
    //   

    Array <Rgba> pixels (width);

    const int N = 8;

    int previewWidth = width / N;
    int previewHeight = height / N;
    Array2D <PreviewRgba> previewPixels (previewHeight, previewWidth);

    Header header (width, height);
    header.setPreviewImage (PreviewImage (previewWidth, previewHeight));

    RgbaOutputFile file (fileName, header, WRITE_RGBA);
    file.setFrameBuffer (pixels, 1, 0);

    for (int y = 0; y < height; ++y)
    {
	drawImage7 (pixels, width, height, y);
	file.writePixels (1);

	if (y % N == 0)
	{
	    for (int x = 0; x < width; x += N)
	    {
		const Rgba  &inPixel = pixels[x];
		PreviewRgba &outPixel = previewPixels[y / N][x / N];

		outPixel.r = gamma (inPixel.r);
		outPixel.g = gamma (inPixel.g);
		outPixel.b = gamma (inPixel.b);
		outPixel.a = int (IMATH_NAMESPACE::clamp (inPixel.a * 255.f, 0.f, 255.f) + 0.5f);
	    }
	}
    }

    file.updatePreviewImage (&previewPixels[0][0]);
}


void
previewImageExamples ()
{
    cout << "\nfiles with preview images\n" << endl;

    cout << "drawing image then writing file" << endl;

    int w = 800;
    int h = 600;

    Array2D<Rgba> p (h, w);
    drawImage1 (p, w, h);
    writeRgbaWithPreview1 ("rgbaWithPreview1.exr", p, w, h);

    cout << "drawing image while writing file" << endl;

    writeRgbaWithPreview2 ("rgbaWithPreview2.exr", w, h);

    cout << endl;
}
