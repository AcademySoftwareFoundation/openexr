//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//



//-----------------------------------------------------------------------------
//
//	class LineOrderAttribute
//
//-----------------------------------------------------------------------------

#include "ImfLineOrderAttribute.h"


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;

template <>
const char *
LineOrderAttribute::staticTypeName ()
{
    return "lineOrder";
}


template <>
void
LineOrderAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    unsigned char tmp = _value;
    Xdr::write <StreamIO> (os, tmp);
}


template <>
void
LineOrderAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    unsigned char tmp;
    Xdr::read <StreamIO> (is, tmp);

    //
    // prevent invalid values being written to LineOrder enum
    // by forcing all unknown types to NUM_LINEORDERS which is also an invalid
    // value but is a legal enum. Note that Header::sanityCheck will
    // throw an exception when files with invalid lineOrders 
    //
    
    if (tmp != INCREASING_Y &&
        tmp != DECREASING_Y &&
        tmp != RANDOM_Y)
        tmp = NUM_LINEORDERS;

    _value = LineOrder (tmp);
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
