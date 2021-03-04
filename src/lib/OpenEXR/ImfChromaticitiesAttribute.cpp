//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class ChromaticitiesAttribute
//
//-----------------------------------------------------------------------------

#include <ImfChromaticitiesAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;

template <>
const char *
ChromaticitiesAttribute::staticTypeName ()
{
    return "chromaticities";
}


template <>
void
ChromaticitiesAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    Xdr::write <StreamIO> (os, _value.red.x);
    Xdr::write <StreamIO> (os, _value.red.y);
    Xdr::write <StreamIO> (os, _value.green.x);
    Xdr::write <StreamIO> (os, _value.green.y);
    Xdr::write <StreamIO> (os, _value.blue.x);
    Xdr::write <StreamIO> (os, _value.blue.y);
    Xdr::write <StreamIO> (os, _value.white.x);
    Xdr::write <StreamIO> (os, _value.white.y);
}


template <>
void
ChromaticitiesAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    Xdr::read <StreamIO> (is, _value.red.x);
    Xdr::read <StreamIO> (is, _value.red.y);
    Xdr::read <StreamIO> (is, _value.green.x);
    Xdr::read <StreamIO> (is, _value.green.y);
    Xdr::read <StreamIO> (is, _value.blue.x);
    Xdr::read <StreamIO> (is, _value.blue.y);
    Xdr::read <StreamIO> (is, _value.white.x);
    Xdr::read <StreamIO> (is, _value.white.y);
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
