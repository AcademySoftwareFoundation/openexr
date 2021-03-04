//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "ImfNamespace.h"

#include <ImfRgba.h>
#include <ImfArray.h>

bool withinB44ErrorBounds (const half A[4][4], const half B[4][4]);

void compareB44 (int width,
		 int height,
		 const OPENEXR_IMF_NAMESPACE::Array2D<half> &p1,
		 const OPENEXR_IMF_NAMESPACE::Array2D<half> &p2);

void compareB44 (int width,
		 int height,
		 const OPENEXR_IMF_NAMESPACE::Array2D<OPENEXR_IMF_NAMESPACE::Rgba> &p1,
		 const OPENEXR_IMF_NAMESPACE::Array2D<OPENEXR_IMF_NAMESPACE::Rgba> &p2,
		 OPENEXR_IMF_NAMESPACE::RgbaChannels channels);
