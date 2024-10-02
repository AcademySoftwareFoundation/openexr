//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class ZipCompressor
//
//-----------------------------------------------------------------------------

#include "ImfZipCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

ZipCompressor::ZipCompressor (
    const Header& hdr, size_t maxScanLineSize, int numScanLines)
    : Compressor (hdr,
                  (numScanLines == 16) ? EXR_COMPRESSION_ZIPS : EXR_COMPRESSION_ZIP,
                  maxScanLineSize,
                  numScanLines)
{
}

ZipCompressor::~ZipCompressor ()
{
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
