//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_COMPRESSOR_DEEP_INTERNAL_H
#define INCLUDED_IMF_COMPRESSOR_DEEP_INTERNAL_H

//-----------------------------------------------------------------------------
//
// Internal helpers for deep scanline/tile output. libOpenEXR only.
//
//-----------------------------------------------------------------------------

#include "ImfForward.h"

#include <Imath/ImathBox.h>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class Compressor;

int compressWithSampleCountTable (
    Compressor&  compressor,
    const char*  inPtr,
    int          inSize,
    int          minY,
    const char*& outPtr,
    const char*  sampleCountTable,
    int          sampleCountTableSize);

int compressTileWithSampleCountTable (
    Compressor&            compressor,
    const char*            inPtr,
    int                    inSize,
    IMATH_NAMESPACE::Box2i range,
    const char*&           outPtr,
    const char*            sampleCountTable,
    int                    sampleCountTableSize);

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
