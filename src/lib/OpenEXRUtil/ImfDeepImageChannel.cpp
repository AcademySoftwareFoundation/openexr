//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//----------------------------------------------------------------------------
//
//      class DeepImageChannel
//
//----------------------------------------------------------------------------

#include "ImfDeepImageChannel.h"
#include "ImfDeepImageLevel.h"
#include <Iex.h>

using namespace IMATH_NAMESPACE;
using namespace IEX_NAMESPACE;
using namespace std;

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


DeepImageChannel::DeepImageChannel
    (DeepImageLevel &level,
     bool pLinear)
:
    ImageChannel (level, 1, 1, pLinear)
{
    // empty
}


DeepImageChannel::~DeepImageChannel ()
{
    // empty
}

DeepImageLevel &
DeepImageChannel::deepLevel ()
{
    return static_cast <DeepImageLevel &> (level());
}


const DeepImageLevel &
DeepImageChannel::deepLevel () const
{
    return static_cast <const DeepImageLevel &> (level());
}


SampleCountChannel &
DeepImageChannel::sampleCounts ()
{
    return deepLevel().sampleCounts();
}


const SampleCountChannel &
DeepImageChannel::sampleCounts () const
{
    return deepLevel().sampleCounts();
}


void
DeepImageChannel::resize ()
{
    ImageChannel::resize();
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
