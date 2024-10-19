//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class HTKCompressor
//
//-----------------------------------------------------------------------------

#include "ImfHTKCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

HTKCompressor::HTKCompressor (
    const Header& hdr, size_t maxScanLineSize, int numScanLines)
    : Compressor (
          hdr,
          EXR_COMPRESSION_LAST_TYPE,
          maxScanLineSize,
          numScanLines > 0 ? numScanLines : 16000)
{}

HTKCompressor::~HTKCompressor ()
{}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
