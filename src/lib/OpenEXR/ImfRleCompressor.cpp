//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class RleCompressor
//
//-----------------------------------------------------------------------------

#include "ImfRleCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

RleCompressor::RleCompressor (const Header& hdr, size_t maxScanLineSize)
    : Compressor (hdr, EXR_COMPRESSION_RLE, maxScanLineSize, 1)
{
}

RleCompressor::~RleCompressor ()
{
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
