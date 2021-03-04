//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class TimeCodeAttribute
//
//-----------------------------------------------------------------------------

#include <ImfTimeCodeAttribute.h>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;

template <>
const char *
TimeCodeAttribute::staticTypeName ()
{
    return "timecode";
}


template <>
void
TimeCodeAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    Xdr::write <StreamIO> (os, _value.timeAndFlags());
    Xdr::write <StreamIO> (os, _value.userData());
}


template <>
void
TimeCodeAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    unsigned int tmp;

    Xdr::read <StreamIO> (is, tmp);
    _value.setTimeAndFlags (tmp);

    Xdr::read <StreamIO> (is, tmp);
    _value.setUserData (tmp);
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
