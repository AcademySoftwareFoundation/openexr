//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) DreamWorks Animation LLC and Contributors of the OpenEXR Project
//

#ifndef COMPARE_DWA_H_INCLUDED
#define COMPARE_DWA_H_INCLUDED

#include "ImfArray.h"
#include "ImfNamespace.h"
#include "ImfRgba.h"

void compareDwa (
    int                                                                width,
    int                                                                height,
    const OPENEXR_IMF_NAMESPACE::Array2D<OPENEXR_IMF_NAMESPACE::Rgba>& src,
    const OPENEXR_IMF_NAMESPACE::Array2D<OPENEXR_IMF_NAMESPACE::Rgba>& test,
    OPENEXR_IMF_NAMESPACE::RgbaChannels channels);

#endif
