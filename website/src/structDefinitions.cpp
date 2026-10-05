//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#include <ImfArray.h>

using namespace IMATH_NAMESPACE;
using namespace OPENEXR_IMF_NAMESPACE;

// [Rgba definition begin]
struct Rgba
{
    half r; // red
    half g; // green
    half b; // blue
    half a; // alpha (opacity)
};
// [Rgba definition end]

// [GZ definition begin]
struct GZ
{
    half g;
    float z;
};
// [GZ definition end]
