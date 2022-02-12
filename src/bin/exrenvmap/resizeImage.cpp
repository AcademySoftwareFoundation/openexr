//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	resizeLatLong(), resizeCube() -- functions that resample
//	an environment map and convert it to latitude-longitude or
//	cube-face format.
//
//-----------------------------------------------------------------------------

#include <resizeImage.h>

#include "Iex.h"
#include <string.h>

#include "namespaceAlias.h"
using namespace IMF;
using namespace std;
using namespace IMATH;

void
resizeLatLong (
    const EnvmapImage& image1,
    EnvmapImage&       image2,
    const Box2i&       image2DataWindow,
    float              filterRadius,
    int                numSamples)
{
    int   w      = image2DataWindow.max.x - image2DataWindow.min.x + 1;
    int   h      = image2DataWindow.max.y - image2DataWindow.min.y + 1;
    float radius = 0.5f * 2 * M_PI * filterRadius / w;

    image2.resize (ENVMAP_LATLONG, image2DataWindow);
    image2.clear ();

    Array2D<Rgba>& pixels = image2.pixels ();

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            V3f dir      = LatLongMap::direction (image2DataWindow, V2f (x, y));
            pixels[y][x] = image1.filteredLookup (dir, radius, numSamples);
        }
    }
}

void
resizeCube (
    const EnvmapImage& image1,
    EnvmapImage&       image2,
    const Box2i&       image2DataWindow,
    float              filterRadius,
    int                numSamples)
{
    if (image1.type () == ENVMAP_CUBE &&
        image1.dataWindow () == image2DataWindow)
    {
        //
        // Special case - the input image is a cube-face environment
        // map with the same size as the output image.  We can copy
        // the input image without resampling.
        //

        image2.resize (ENVMAP_CUBE, image2DataWindow);

        int w = image2DataWindow.max.x - image2DataWindow.min.x + 1;
        int h = image2DataWindow.max.y - image2DataWindow.min.y + 1;

        memcpy (
            &(image2.pixels ()[0][0]),
            &(image1.pixels ()[0][0]),
            sizeof (Rgba) * w * h);

        return;
    }

    //
    // Resampe the input image
    //

    int   sof    = CubeMap::sizeOfFace (image2DataWindow);
    float radius = 1.5f * filterRadius / sof;

    image2.resize (ENVMAP_CUBE, image2DataWindow);
    image2.clear ();

    Array2D<Rgba>& pixels = image2.pixels ();

    for (int f = CUBEFACE_POS_X; f <= CUBEFACE_NEG_Z; ++f)
    {
        CubeMapFace face = CubeMapFace (f);

        for (int y = 0; y < sof; ++y)
        {
            for (int x = 0; x < sof; ++x)
            {
                V2f posInFace (x, y);

                V3f dir =
                    CubeMap::direction (face, image2DataWindow, posInFace);

                V2f pos =
                    CubeMap::pixelPosition (face, image2DataWindow, posInFace);

                pixels[int (pos.y + 0.5f)][int (pos.x + 0.5f)] =
                    image1.filteredLookup (dir, radius, numSamples);
            }
        }
    }
}
