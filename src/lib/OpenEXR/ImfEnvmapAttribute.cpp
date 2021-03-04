//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class EnvmapAttribute
//
//-----------------------------------------------------------------------------

#include <ImfEnvmapAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;

template <>
const char *
EnvmapAttribute::staticTypeName ()
{
    return "envmap";
}


template <>
void
EnvmapAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    unsigned char tmp = _value;
    Xdr::write <StreamIO> (os, tmp);
}


template <>
void
EnvmapAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    unsigned char tmp;
    Xdr::read <StreamIO> (is, tmp);
    _value = Envmap (tmp);
}

template <>
void
EnvmapAttribute::copyValueFrom (const OPENEXR_IMF_INTERNAL_NAMESPACE::Attribute &other)
{
    _value = cast(other).value();

}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
