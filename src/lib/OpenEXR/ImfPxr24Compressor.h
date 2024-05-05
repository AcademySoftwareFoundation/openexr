//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Pixar Animation Studios and Contributors of the OpenEXR Project
//

#ifndef INCLUDED_IMF_PXR24_COMPRESSOR_H
#define INCLUDED_IMF_PXR24_COMPRESSOR_H

//-----------------------------------------------------------------------------
//
//	class Pxr24Compressor -- Loren Carpenter's 24-bit float compressor
//
//-----------------------------------------------------------------------------

#include "ImfCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class Pxr24Compressor : public Compressor
{
public:
    Pxr24Compressor (
        const Header& hdr, size_t maxScanLineSize, int numScanLines);

    virtual ~Pxr24Compressor ();

    Pxr24Compressor (const Pxr24Compressor& other)            = delete;
    Pxr24Compressor& operator= (const Pxr24Compressor& other) = delete;
    Pxr24Compressor (Pxr24Compressor&& other)                 = delete;
    Pxr24Compressor& operator= (Pxr24Compressor&& other)      = delete;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
