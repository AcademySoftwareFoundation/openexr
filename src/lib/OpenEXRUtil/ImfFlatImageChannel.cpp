//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//----------------------------------------------------------------------------
//
//      class FlatImageChannel
//
//----------------------------------------------------------------------------

#include "ImfFlatImageChannel.h"
#include "ImfFlatImageLevel.h"
#include <Iex.h>

using namespace IMATH_NAMESPACE;
using namespace IEX_NAMESPACE;
using namespace std;

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


FlatImageChannel::FlatImageChannel
    (FlatImageLevel &level,
     int xSampling,
     int ySampling,
     bool pLinear)
:
    ImageChannel (level, xSampling, ySampling, pLinear)
{
    // empty
}


FlatImageChannel::~FlatImageChannel ()
{
    // empty
}

FlatImageLevel &
FlatImageChannel::flatLevel ()
{
    return static_cast <FlatImageLevel &> (level());
}


const FlatImageLevel &
FlatImageChannel::flatLevel () const
{
    return static_cast <const FlatImageLevel &> (level());
}


void
FlatImageChannel::resize ()
{
    ImageChannel::resize();
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
