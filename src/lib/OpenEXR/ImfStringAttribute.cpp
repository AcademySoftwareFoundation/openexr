//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	class StringAttribute
//
//-----------------------------------------------------------------------------

#include <ImfStringAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;

template <>
const char *
StringAttribute::staticTypeName ()
{
    return "string";
}


template <>
void
StringAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    int size = _value.size();

    for (int i = 0; i < size; i++)
	Xdr::write <StreamIO> (os, _value[i]);
}


template <>
void
StringAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    _value.resize (size);

    for (int i = 0; i < size; i++)
	Xdr::read <StreamIO> (is, _value[i]);
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
