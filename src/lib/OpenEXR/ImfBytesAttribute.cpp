//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class BytesAttribute
//
//-----------------------------------------------------------------------------

#include "Iex.h"
#include "ImfNamespace.h"
#include "ImfBytesAttribute.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

#if defined(_MSC_VER)
// suppress warning about non-exported base classes
#    pragma warning(disable : 4251)
#    pragma warning(disable : 4275)
#endif

BytesAttribute::BytesAttribute ()
{}

BytesAttribute::BytesAttribute (long        dataSize,
                                const void* data)
    : _data (dataSize)
{
    memcpy ((char*) _data, (const char*) data, dataSize);
}

BytesAttribute::BytesAttribute (const BytesAttribute& other)
    : _data (other._data.size ())
{
    memcpy ((char*) _data, (const char*) other._data, other._data.size ());
}


const char*
BytesAttribute::staticTypeName ()
{
    return "bytes";
}

const char*
BytesAttribute::typeName () const
{
    return staticTypeName ();
}

Attribute*
BytesAttribute::copy () const
{
    return new BytesAttribute (*this);
}

void
BytesAttribute::writeValueTo (
    OPENEXR_IMF_INTERNAL_NAMESPACE::OStream& os, int version) const
{
    Xdr::write<StreamIO> (os, _data, _data.size ());
}

void
BytesAttribute::readValueFrom (
    OPENEXR_IMF_INTERNAL_NAMESPACE::IStream& is, int size, int version)
{
    _data.resizeErase (size);
    Xdr::read<StreamIO> (is, _data, size);
}

void
BytesAttribute::copyValueFrom (const Attribute& other)
{
    const BytesAttribute* oa = dynamic_cast<const BytesAttribute*> (&other);

    if (oa == 0)
    {
        THROW (
            IEX_NAMESPACE::TypeExc,
            "Cannot copy the value of an "
            "image file attribute of type "
            "\"" << other.typeName ()
                 << "\" "
                    "to an attribute of type \"bytes\".");
    }

    _data.resizeErase (oa->_data.size ());
    memcpy ((char*) _data, (const char*) oa->_data, oa->_data.size ());
}

Attribute*
BytesAttribute::makeNewAttribute ()
{
    return new BytesAttribute ();
}

void
BytesAttribute::registerAttributeType ()
{
    Attribute::registerAttributeType ( staticTypeName (), makeNewAttribute);
}

void
BytesAttribute::unRegisterAttributeType ()
{
    Attribute::unRegisterAttributeType (staticTypeName ());
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
