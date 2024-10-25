//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "ImfZstdCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

ZstdCompressor::ZstdCompressor (const Header& hdr, size_t maxScanLineSize, int scanLines)
    : Compressor (hdr, EXR_COMPRESSION_ZSTD, maxScanLineSize, scanLines)
{}

ZstdCompressor::~ZstdCompressor ()
{}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT