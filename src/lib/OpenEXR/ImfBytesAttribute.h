//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_BYTES_ATTRIBUTE_H
#define INCLUDED_IMF_BYTES_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class BytesAttribute
// 
//  BytesAttribute allows arbitrary binary data to be stored as an attribute.
//  It holds a sequence of bytes, with the length specified by the `size` arg.
//
//  Unlike OpaqueAttribute, which also stores raw bytes, BytesAttribute is
//  explicitly intended for use when the data is known to be a byte sequence.
//  OpaqueAttribute should remain semantically uninterpreted, whereas
//  BytesAttribute conveys an explicit intent to store binary data.
//
//-----------------------------------------------------------------------------

#include "ImfExport.h"
#include "ImfNamespace.h"

#include "ImfArray.h"
#include "ImfAttribute.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class IMF_EXPORT_TYPE BytesAttribute : public Attribute
{
public:
    //----------------------------
    // Constructors and destructor
    //----------------------------

    IMF_EXPORT BytesAttribute ();
    IMF_EXPORT BytesAttribute (
        size_t      size,
        const void* data,
        const std::string& typeHint = "");

    IMF_EXPORT BytesAttribute (const BytesAttribute& other);
    IMF_EXPORT virtual ~BytesAttribute ();

    //----------
    // Operators
    //----------

    IMF_EXPORT bool operator==(const BytesAttribute& other) const;

    //-------------------------------
    // Get this attribute's type name
    //-------------------------------

    IMF_EXPORT const char* typeName () const override;

    //------------------------------
    // Make a copy of this attribute
    //------------------------------

    IMF_EXPORT Attribute* copy () const override;

    //----------------
    // I/O and copying
    //----------------

    IMF_EXPORT void writeValueTo (
        OPENEXR_IMF_INTERNAL_NAMESPACE::OStream& os, int version) const override;

    IMF_EXPORT void readValueFrom (
        OPENEXR_IMF_INTERNAL_NAMESPACE::IStream& is, int size, int version) override;

    IMF_EXPORT void copyValueFrom (const Attribute& other) override;

    size_t size () const { return _data.size (); }
    const Array<unsigned char>& data () const { return _data; }
    IMF_EXPORT void setData(const unsigned char* data, size_t size);
    std::string typeHint; 

    //--------------------------------
    // Methods to support registration
    //--------------------------------

    //---------------------------------------------------------
    // Static version of typeName()
    //---------------------------------------------------------

    static const char* staticTypeName ();

    //---------------------
    // Make a new attribute
    //---------------------

    static Attribute* makeNewAttribute ();

    //---------------------------------------------------------------
    // Register this attribute type so that Attribute::newAttribute()
    // knows how to make objects of this type.
    //
    // Note that this function is not thread-safe because it modifies
    // a global variable in the IlmIlm library.  A thread in a multi-
    // threaded program may call registerAttributeType() only when no
    // other thread is accessing any functions or classes in the
    // OpenEXR library.
    //
    //---------------------------------------------------------------

    static void registerAttributeType ();

    //-----------------------------------------------------
    // Un-register this attribute type (for debugging only)
    //-----------------------------------------------------

    static void unRegisterAttributeType ();

private:
    Array<unsigned char> _data;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
