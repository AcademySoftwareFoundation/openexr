//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//      function readInputImage() --
//      reads an image file and constructs an EnvMapImage object
//
//-----------------------------------------------------------------------------

#include <makeCubeMap.h>

#include "Iex.h"
#include "IexMacros.h"
#include <EnvmapImage.h>
#include <ImfRgbaFile.h>
#include <ImfStandardAttributes.h>
#include <iostream>
#include <string.h>
#include <string>

#include "namespaceAlias.h"
using namespace IMF;
using namespace std;
using namespace IMATH;

namespace
{

void
readSingleImage (
    const char    inFileName[],
    float         padTop,
    float         padBottom,
    Envmap        overrideType,
    bool          verbose,
    EnvmapImage&  image,
    Header&       header,
    RgbaChannels& channels)
{
    //
    // Read the input image, and if necessary,
    // pad the image at the top and bottom.
    //

    RgbaInputFile in (inFileName);

    if (verbose) cout << "reading file " << inFileName << endl;

    header   = in.header ();
    channels = in.channels ();

    Envmap type = ENVMAP_LATLONG;

    if (overrideType == ENVMAP_LATLONG || overrideType == ENVMAP_CUBE)
    {
        type = overrideType;
        addEnvmap (header, overrideType);
    }
    else if (hasEnvmap (in.header ()))
    {
        // validate type is known before using
        const Envmap& typeInFile = envmap (in.header ());

        if (typeInFile != ENVMAP_LATLONG && typeInFile != ENVMAP_CUBE)
        {
            THROW (IEX::InputExc, "unknown envmap type " << int (typeInFile));
        }
        type = typeInFile;
    }

    const Box2i& dw = in.dataWindow ();
    int          w  = dw.max.x - dw.min.x + 1;
    int          h  = dw.max.y - dw.min.y + 1;

    int pt = 0;
    int pb = 0;

    if (type == ENVMAP_LATLONG)
    {
        pt = int (padTop * h + 0.5f);
        pb = int (padBottom * h + 0.5f);
    }

    Box2i paddedDw (
        V2i (dw.min.x, dw.min.y - pt), V2i (dw.max.x, dw.max.y + pb));

    image.resize (type, paddedDw);
    Array2D<Rgba>& pixels = image.pixels ();

    in.setFrameBuffer (&pixels[-paddedDw.min.y][-paddedDw.min.x], 1, w);
    in.readPixels (dw.min.y, dw.max.y);

    for (int y = 0; y < pt; ++y)
        for (int x = 0; x < w; ++x)
            pixels[y][x] = pixels[pt][x];

    for (int y = h + pt; y < h + pt + pb; ++y)
    {
        for (int x = 0; x < w; ++x)
            pixels[y][x] = pixels[h + pt - 1][x];
    }
}

void
readSixImages (
    const char    inFileName[],
    bool          verbose,
    EnvmapImage&  image,
    Header&       header,
    RgbaChannels& channels)
{
    //
    // Generate six file names by replacing the first '%' character in
    // inFileName with +X, -X, ... -Z.  Interpreting the corresponding
    // image files as the six sides of a cube, assemble a single cube-
    // face map image.
    //

    static const char* faceNames[] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};

    size_t pos  = strchr (inFileName, '%') - inFileName;
    string name = string (inFileName).replace (pos, 1, faceNames[0]);

    Box2i dw;
    int   w, h;

    {
        RgbaInputFile in (name.c_str ());

        if (verbose)
            cout << "reading cube face size from file " << name << endl;

        dw = in.dataWindow ();
        w  = dw.max.x - dw.min.x + 1;
        h  = dw.max.y - dw.min.y + 1;

        if (w != h)
        {
            THROW (
                IEX::InputExc, "Cube face image " << name << " is not square.");
        }

        header   = in.header ();
        channels = in.channels ();
        addEnvmap (header, ENVMAP_CUBE);
    }

    const Box2i imageDw (V2i (0, 0), V2i (w - 1, 6 * h - 1));

    image.resize (ENVMAP_CUBE, imageDw);
    Rgba* pixels = &(image.pixels ()[0][0]);

    for (int i = 0; i < 6; ++i)
    {
        string name = string (inFileName).replace (pos, 1, faceNames[i]);

        RgbaInputFile in (name.c_str ());

        if (verbose) cout << "reading file " << name << endl;

        if (in.dataWindow () != dw)
        {
            THROW (
                IEX::InputExc,
                "The data window of cube face "
                    << name
                    << " differs "
                       "from the data window of other cube faces.");
        }

        in.setFrameBuffer (ComputeBasePointer (pixels, dw), 1, w);
        in.readPixels (dw.min.y, dw.max.y);

        pixels += w * h;
    }
}

} // namespace

void
readInputImage (
    const char    inFileName[],
    float         padTop,
    float         padBottom,
    Envmap        overrideType,
    bool          verbose,
    EnvmapImage&  image,
    Header&       header,
    RgbaChannels& channels)
{
    if (strchr (inFileName, '%'))
    {
        readSixImages (inFileName, verbose, image, header, channels);
    }
    else
    {
        readSingleImage (
            inFileName,
            padTop,
            padBottom,
            overrideType,
            verbose,
            image,
            header,
            channels);
    }
}
