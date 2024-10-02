//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_ZIP_COMPRESSOR_H
#define INCLUDED_IMF_ZIP_COMPRESSOR_H

//-----------------------------------------------------------------------------
//
//	class ZipCompressor -- performs zlib-style compression
//
//-----------------------------------------------------------------------------

#include "ImfCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class ZipCompressor : public Compressor
{
public:
    ZipCompressor (
        const Header& hdr, size_t maxScanLineSize, int numScanLines);

    virtual ~ZipCompressor ();
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
