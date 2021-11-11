//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_BLUR_IMAGE_H
#define INCLUDED_BLUR_IMAGE_H

//-----------------------------------------------------------------------------
//
//	function blurImage() -- performs a hemispherical blur
//
//	An environment map image is blurred by applying a 180-degree-wide
//	filter kernel, such that point-sampling the blurred image at a
//	location that corresponds to 3D direction N returns the color that
//	a white diffuse reflector with surface normal N would have if it
//	was illuminated using the original non-blurred image.
//
//-----------------------------------------------------------------------------

#include <readInputImage.h>


void
blurImage (EnvmapImage &image, bool verbose);


#endif
