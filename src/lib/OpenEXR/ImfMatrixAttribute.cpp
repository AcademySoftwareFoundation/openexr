//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	class M33fAttribute
//	class M33dAttribute
//	class M44fAttribute
//	class M44dAttribute
//
//-----------------------------------------------------------------------------

#include <ImfMatrixAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;


template <>
const char *
M33fAttribute::staticTypeName ()
{
    return "m33f";
}


template <>
void
M33fAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    Xdr::write <StreamIO> (os, _value[0][0]);
    Xdr::write <StreamIO> (os, _value[0][1]);
    Xdr::write <StreamIO> (os, _value[0][2]);

    Xdr::write <StreamIO> (os, _value[1][0]);
    Xdr::write <StreamIO> (os, _value[1][1]);
    Xdr::write <StreamIO> (os, _value[1][2]);

    Xdr::write <StreamIO> (os, _value[2][0]);
    Xdr::write <StreamIO> (os, _value[2][1]);
    Xdr::write <StreamIO> (os, _value[2][2]);
}


template <>
void
M33fAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    Xdr::read <StreamIO> (is, _value[0][0]);
    Xdr::read <StreamIO> (is, _value[0][1]);
    Xdr::read <StreamIO> (is, _value[0][2]);

    Xdr::read <StreamIO> (is, _value[1][0]);
    Xdr::read <StreamIO> (is, _value[1][1]);
    Xdr::read <StreamIO> (is, _value[1][2]);

    Xdr::read <StreamIO> (is, _value[2][0]);
    Xdr::read <StreamIO> (is, _value[2][1]);
    Xdr::read <StreamIO> (is, _value[2][2]);
}


template <>
const char *
M33dAttribute::staticTypeName ()
{
    return "m33d";
}


template <>
void
M33dAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    Xdr::write <StreamIO> (os, _value[0][0]);
    Xdr::write <StreamIO> (os, _value[0][1]);
    Xdr::write <StreamIO> (os, _value[0][2]);

    Xdr::write <StreamIO> (os, _value[1][0]);
    Xdr::write <StreamIO> (os, _value[1][1]);
    Xdr::write <StreamIO> (os, _value[1][2]);

    Xdr::write <StreamIO> (os, _value[2][0]);
    Xdr::write <StreamIO> (os, _value[2][1]);
    Xdr::write <StreamIO> (os, _value[2][2]);
}


template <>
void
M33dAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    Xdr::read <StreamIO> (is, _value[0][0]);
    Xdr::read <StreamIO> (is, _value[0][1]);
    Xdr::read <StreamIO> (is, _value[0][2]);

    Xdr::read <StreamIO> (is, _value[1][0]);
    Xdr::read <StreamIO> (is, _value[1][1]);
    Xdr::read <StreamIO> (is, _value[1][2]);

    Xdr::read <StreamIO> (is, _value[2][0]);
    Xdr::read <StreamIO> (is, _value[2][1]);
    Xdr::read <StreamIO> (is, _value[2][2]);
}


template <>
const char *
M44fAttribute::staticTypeName ()
{
    return "m44f";
}


template <>
void
M44fAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    Xdr::write <StreamIO> (os, _value[0][0]);
    Xdr::write <StreamIO> (os, _value[0][1]);
    Xdr::write <StreamIO> (os, _value[0][2]);
    Xdr::write <StreamIO> (os, _value[0][3]);

    Xdr::write <StreamIO> (os, _value[1][0]);
    Xdr::write <StreamIO> (os, _value[1][1]);
    Xdr::write <StreamIO> (os, _value[1][2]);
    Xdr::write <StreamIO> (os, _value[1][3]);

    Xdr::write <StreamIO> (os, _value[2][0]);
    Xdr::write <StreamIO> (os, _value[2][1]);
    Xdr::write <StreamIO> (os, _value[2][2]);
    Xdr::write <StreamIO> (os, _value[2][3]);

    Xdr::write <StreamIO> (os, _value[3][0]);
    Xdr::write <StreamIO> (os, _value[3][1]);
    Xdr::write <StreamIO> (os, _value[3][2]);
    Xdr::write <StreamIO> (os, _value[3][3]);
}


template <>
void
M44fAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    Xdr::read <StreamIO> (is, _value[0][0]);
    Xdr::read <StreamIO> (is, _value[0][1]);
    Xdr::read <StreamIO> (is, _value[0][2]);
    Xdr::read <StreamIO> (is, _value[0][3]);

    Xdr::read <StreamIO> (is, _value[1][0]);
    Xdr::read <StreamIO> (is, _value[1][1]);
    Xdr::read <StreamIO> (is, _value[1][2]);
    Xdr::read <StreamIO> (is, _value[1][3]);

    Xdr::read <StreamIO> (is, _value[2][0]);
    Xdr::read <StreamIO> (is, _value[2][1]);
    Xdr::read <StreamIO> (is, _value[2][2]);
    Xdr::read <StreamIO> (is, _value[2][3]);

    Xdr::read <StreamIO> (is, _value[3][0]);
    Xdr::read <StreamIO> (is, _value[3][1]);
    Xdr::read <StreamIO> (is, _value[3][2]);
    Xdr::read <StreamIO> (is, _value[3][3]);
}


template <>
const char *
M44dAttribute::staticTypeName ()
{
    return "m44d";
}


template <>
void
M44dAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    Xdr::write <StreamIO> (os, _value[0][0]);
    Xdr::write <StreamIO> (os, _value[0][1]);
    Xdr::write <StreamIO> (os, _value[0][2]);
    Xdr::write <StreamIO> (os, _value[0][3]);

    Xdr::write <StreamIO> (os, _value[1][0]);
    Xdr::write <StreamIO> (os, _value[1][1]);
    Xdr::write <StreamIO> (os, _value[1][2]);
    Xdr::write <StreamIO> (os, _value[1][3]);

    Xdr::write <StreamIO> (os, _value[2][0]);
    Xdr::write <StreamIO> (os, _value[2][1]);
    Xdr::write <StreamIO> (os, _value[2][2]);
    Xdr::write <StreamIO> (os, _value[2][3]);

    Xdr::write <StreamIO> (os, _value[3][0]);
    Xdr::write <StreamIO> (os, _value[3][1]);
    Xdr::write <StreamIO> (os, _value[3][2]);
    Xdr::write <StreamIO> (os, _value[3][3]);
}


template <>
void
M44dAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    Xdr::read <StreamIO> (is, _value[0][0]);
    Xdr::read <StreamIO> (is, _value[0][1]);
    Xdr::read <StreamIO> (is, _value[0][2]);
    Xdr::read <StreamIO> (is, _value[0][3]);

    Xdr::read <StreamIO> (is, _value[1][0]);
    Xdr::read <StreamIO> (is, _value[1][1]);
    Xdr::read <StreamIO> (is, _value[1][2]);
    Xdr::read <StreamIO> (is, _value[1][3]);

    Xdr::read <StreamIO> (is, _value[2][0]);
    Xdr::read <StreamIO> (is, _value[2][1]);
    Xdr::read <StreamIO> (is, _value[2][2]);
    Xdr::read <StreamIO> (is, _value[2][3]);

    Xdr::read <StreamIO> (is, _value[3][0]);
    Xdr::read <StreamIO> (is, _value[3][1]);
    Xdr::read <StreamIO> (is, _value[3][2]);
    Xdr::read <StreamIO> (is, _value[3][3]);
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
