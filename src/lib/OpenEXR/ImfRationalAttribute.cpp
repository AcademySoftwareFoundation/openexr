//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class RationalAttribute
//
//-----------------------------------------------------------------------------

#include <ImfRationalAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;

template <>
const char *
RationalAttribute::staticTypeName ()
{
    return "rational";
}


template <>
void
RationalAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    Xdr::write <StreamIO> (os, _value.n);
    Xdr::write <StreamIO> (os, _value.d);
}


template <>
void
RationalAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    Xdr::read <StreamIO> (is, _value.n);
    Xdr::read <StreamIO> (is, _value.d);
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
