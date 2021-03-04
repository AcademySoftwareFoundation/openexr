//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class FloatVectorAttribute
//
//-----------------------------------------------------------------------------

#include <ImfFloatVectorAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;


template <>
const char *
FloatVectorAttribute::staticTypeName ()
{
    return "floatvector";
}


template <>
void
FloatVectorAttribute::writeValueTo
    (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    int n = _value.size();

    for (int i = 0; i < n; ++i)
        Xdr::write <StreamIO> (os, _value[i]);
}


template <>
void
FloatVectorAttribute::readValueFrom
    (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    int n = size / Xdr::size<float>();
    _value.resize (n);

    for (int i = 0; i < n; ++i)
       Xdr::read <StreamIO> (is, _value[i]);
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
