//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//  class GdeflateCompressor
//
//-----------------------------------------------------------------------------

#include "ImfGdeflateCompressor.h"
#include "OpenEXRConfigInternal.h"

#ifndef OPENEXR_ENABLE_GDEFLATE
#include "IexBaseExc.h"
#endif

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

#ifdef OPENEXR_ENABLE_GDEFLATE

GdeflateCompressor::GdeflateCompressor (
    const Header& hdr, size_t maxScanLineSize, int numScanLines)
    : Compressor (
          hdr,
          EXR_COMPRESSION_GDEFLATE,
          maxScanLineSize,
          numScanLines)
{
}

GdeflateCompressor::~GdeflateCompressor ()
{
}

#else

GdeflateCompressor::GdeflateCompressor (
    const Header& hdr, size_t maxScanLineSize, int numScanLines)
    : Compressor (hdr, EXR_COMPRESSION_LAST_TYPE, maxScanLineSize, numScanLines)
{
    throw IEX_NAMESPACE::NoImplExc (
        "Gdeflate support is not enabled in this build of OpenEXR.");
}

GdeflateCompressor::~GdeflateCompressor ()
{
}

#endif

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT


