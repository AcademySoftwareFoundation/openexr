//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class HTCompressor
//
//-----------------------------------------------------------------------------

#include "ImfHTCompressor.h"
#include "ImfAutoArray.h"
#include "ImfChannelList.h"
#include "ImfCheckedArithmetic.h"
#include "ImfHeader.h"
#include "ImfIO.h"
#include "ImfMisc.h"
#include "ImfNamespace.h"
#include "ImfXdr.h"
#include <assert.h>
#include <string.h>

#include <ImathBox.h>

using IMATH_NAMESPACE::Box2i;

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

HTCompressor::HTCompressor (
    const Header& hdr, size_t maxScanLineSize, int numScanLines)
    : Compressor (
          hdr,
          EXR_COMPRESSION_LAST_TYPE,
          maxScanLineSize,
          numScanLines > 0 ? numScanLines : 16000)
{}

HTCompressor::~HTCompressor ()
{}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
