//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class KeyCodeAttribute
//
//-----------------------------------------------------------------------------

#include <ImfKeyCodeAttribute.h>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;

template <>
const char *
KeyCodeAttribute::staticTypeName ()
{
    return "keycode";
}


template <>
void
KeyCodeAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    Xdr::write <StreamIO> (os, _value.filmMfcCode());
    Xdr::write <StreamIO> (os, _value.filmType());
    Xdr::write <StreamIO> (os, _value.prefix());
    Xdr::write <StreamIO> (os, _value.count());
    Xdr::write <StreamIO> (os, _value.perfOffset());
    Xdr::write <StreamIO> (os, _value.perfsPerFrame());
    Xdr::write <StreamIO> (os, _value.perfsPerCount());
}


template <>
void
KeyCodeAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    int tmp;

    Xdr::read <StreamIO> (is, tmp);
    _value.setFilmMfcCode (tmp);

    Xdr::read <StreamIO> (is, tmp);
    _value.setFilmType (tmp);

    Xdr::read <StreamIO> (is, tmp);
    _value.setPrefix (tmp);

    Xdr::read <StreamIO> (is, tmp);
    _value.setCount (tmp);

    Xdr::read <StreamIO> (is, tmp);
    _value.setPerfOffset (tmp);

    Xdr::read <StreamIO> (is, tmp);
    _value.setPerfsPerFrame (tmp);

    Xdr::read <StreamIO> (is, tmp);
    _value.setPerfsPerCount (tmp);
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
