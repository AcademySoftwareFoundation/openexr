//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_RESIZE_IMAGE_H
#define INCLUDED_RESIZE_IMAGE_H

//-----------------------------------------------------------------------------
//
//	resizeLatLong(), resizeCube() -- functions that resample
//	an environment map and convert it to latitude-longitude or
//	cube-face format.
//
//-----------------------------------------------------------------------------

#include "EnvmapImage.h"


void
resizeLatLong (const EnvmapImage &image1,
               EnvmapImage &image2,
               const IMATH::Box2i &image2DataWindow,
               float filterRadius,
               int numSamples);

void
resizeCube (const EnvmapImage &image1,
            EnvmapImage &image2,
            const IMATH::Box2i &image2DataWindow,
            float filterRadius,
            int numSamples);


#endif
