//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	class Box2iAttribute
//	class Box2fAttribute
//
//-----------------------------------------------------------------------------

#include <ImfBoxAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;

template <>
const char *
Box2iAttribute::staticTypeName ()
{
    return "box2i";
}


template <>
void
Box2iAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    Xdr::write <StreamIO> (os, _value.min.x);
    Xdr::write <StreamIO> (os, _value.min.y);
    Xdr::write <StreamIO> (os, _value.max.x);
    Xdr::write <StreamIO> (os, _value.max.y);
}


template <>
void
Box2iAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    Xdr::read <StreamIO> (is, _value.min.x);
    Xdr::read <StreamIO> (is, _value.min.y);
    Xdr::read <StreamIO> (is, _value.max.x);
    Xdr::read <StreamIO> (is, _value.max.y);
}


template <>
const char *
Box2fAttribute::staticTypeName ()
{
    return "box2f";
}


template <>
void
Box2fAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    Xdr::write <StreamIO> (os, _value.min.x);
    Xdr::write <StreamIO> (os, _value.min.y);
    Xdr::write <StreamIO> (os, _value.max.x);
    Xdr::write <StreamIO> (os, _value.max.y);
}


template <>
void
Box2fAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    Xdr::read <StreamIO> (is, _value.min.x);
    Xdr::read <StreamIO> (is, _value.min.y);
    Xdr::read <StreamIO> (is, _value.max.x);
    Xdr::read <StreamIO> (is, _value.max.y);
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
