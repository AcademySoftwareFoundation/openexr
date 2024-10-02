//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_B44_COMPRESSOR_H
#define INCLUDED_IMF_B44_COMPRESSOR_H

//-----------------------------------------------------------------------------
//
//	class B44Compressor -- lossy compression of 4x4 pixel blocks
//
//-----------------------------------------------------------------------------

#include "ImfForward.h"

#include "ImfCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class B44Compressor : public Compressor
{
public:
    B44Compressor (
        const Header& hdr,
        size_t        maxScanLineSize,
        int           numScanLines,
        bool          optFlatFields);

    virtual ~B44Compressor ();

    B44Compressor (const B44Compressor& other)            = delete;
    B44Compressor& operator= (const B44Compressor& other) = delete;
    B44Compressor (B44Compressor&& other)                 = delete;
    B44Compressor& operator= (B44Compressor&& other)      = delete;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
