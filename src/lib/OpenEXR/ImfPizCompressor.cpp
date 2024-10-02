//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class PizCompressor
//
//-----------------------------------------------------------------------------

#include "ImfPizCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

PizCompressor::PizCompressor (
    const Header& hdr, size_t maxScanLineSize, int numScanLines)
    : Compressor (hdr, EXR_COMPRESSION_PIZ, maxScanLineSize, numScanLines)
{
}

PizCompressor::~PizCompressor ()
{
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
