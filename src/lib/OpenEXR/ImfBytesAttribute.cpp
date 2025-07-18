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

BytesAttribute::BytesAttribute (
    size_t      size,
    const void* data,
    const std::string& typeHint)
    : _data (size), typeHint (typeHint)
{
    if (size > 0 && ! data)
    {
        THROW (IEX_NAMESPACE::ArgExc, "Invalid data pointer.");
    }
    memcpy (
        (unsigned char*) _data,
        (const unsigned char*) data,
        size);
}

BytesAttribute::BytesAttribute (const BytesAttribute& other)
    : _data (other._data.size ()), typeHint (other.typeHint)
{
    memcpy (
        (unsigned char*) _data,
        (const unsigned char*) other._data,
        other._data.size ());
}

// Note the destructor is defined here and not in the header file to
// allow the Windows build to clearly separate definition from
// declaration with respect to the DLL export macros defined in
// ImfExport.h.
BytesAttribute::~BytesAttribute () = default;

bool
BytesAttribute::operator==(const BytesAttribute& other) const
{
    if (_data.size() != other._data.size())
        return false;

    if (memcmp(_data, other._data, _data.size()) != 0)
        return false;

    return typeHint == other.typeHint;
}

void
BytesAttribute::setData (const unsigned char* data, size_t size)
{
    _data.resizeErase(size);
    if (data)
    {
        memcpy((unsigned char*) _data, data, size);
    }
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
    const unsigned int hintLength = typeHint.length();
    // Note that even if unsigned int is not 32 bits on the platform
    // write<>() will write exactly 32 bits via bit shifting.
    Xdr::write<StreamIO> (os, hintLength);
    if (hintLength)
        Xdr::write<StreamIO> (os, typeHint.c_str(), hintLength);
    Xdr::writeUnsignedChars<StreamIO> (os, _data, _data.size ());
}

void
BytesAttribute::readValueFrom (
    OPENEXR_IMF_INTERNAL_NAMESPACE::IStream& is, int size, int version)
{
    unsigned int hintLength = 0;
    Xdr::read<StreamIO> (is, hintLength);

    typeHint.resize(hintLength);
    if (hintLength)
        Xdr::read<StreamIO> (is, &typeHint[0], hintLength);

    const size_t dataSize = size - sizeof(hintLength) - hintLength; 
    _data.resizeErase (dataSize);
    // NOTE that readUnsignedChars will not propagate any error value
    // returned by the underlying stream read, so if the underlying
    // stream read fails, the _data will be left in an erased state.
    // This undefined behavior is consistent with other readValueFrom
    // implementations in the OpenEXR library. An error in the underlying
    // stream read is very rare, it's much more likely that an exception
    // will be thrown.
    Xdr::readUnsignedChars<StreamIO> (is, _data, dataSize);
}

void
BytesAttribute::copyValueFrom (const Attribute& other)
{
    const BytesAttribute* oa = dynamic_cast<const BytesAttribute*> (&other);

    if (!oa)
    {
        THROW (
            IEX_NAMESPACE::TypeExc,
            "Cannot copy the value of an "
            "image file attribute of type "
            "\"" << other.typeName ()
                 << "\" "
                    "to an attribute of type \"bytes\".");
    }

    typeHint = oa->typeHint;
    _data.resizeErase (oa->_data.size ());
    memcpy (
        (unsigned char*) _data,
        (const unsigned char*) oa->_data,
        oa->_data.size ());
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
