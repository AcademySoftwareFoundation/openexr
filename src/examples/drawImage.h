//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <OpenEXRConfig.h>
#include <ImfRgbaFile.h>
#include <ImfArray.h>

#include "namespaceAlias.h"


struct GZ
{
    half  g;
    float z;
};


void drawImage1 (IMF::Array2D<IMF::Rgba> &pixels,
		 int width,
		 int height);

void drawImage2 (IMF::Array2D<half>  &gPixels,
		 IMF::Array2D<float> &zPixels,
		 int width,
		 int height);

void drawImage3 (IMF::Array2D<IMF::Rgba> &pixels,
                 int width,
                 int height,
                 int xMin, int xMax,
                 int yMin, int yMax,
                 int xLevel = 0, int yLevel = 0);

void drawImage4 (IMF::Array2D<IMF::Rgba> &pixels,
                 int width,
                 int height,
                 int xMin, int xMax,
                 int yMin, int yMax,
                 int xLevel = 0, int yLevel = 0);

void drawImage5 (IMF::Array2D<IMF::Rgba> &pixels,
                 int width,
                 int height,
                 int xMin, int xMax,
                 int yMin, int yMax,
                 int xLevel = 0, int yLevel = 0);

void drawImage6 (IMF::Array2D<GZ> &pixels,
		 int width,
		 int height);

void drawImage7 (IMF::Array<IMF::Rgba> &pixels,
		 int width,
		 int height,
		 int y);
