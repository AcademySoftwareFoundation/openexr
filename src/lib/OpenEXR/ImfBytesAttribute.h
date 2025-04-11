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
    IMF_EXPORT BytesAttribute (long        dataSize,
                               const void* data);
    IMF_EXPORT BytesAttribute (const BytesAttribute& other);
    IMF_EXPORT virtual ~BytesAttribute ();

    //-------------------------------
    // Get this attribute's type name
    //-------------------------------

    IMF_EXPORT virtual const char* typeName () const;

    //------------------------------
    // Make a copy of this attribute
    //------------------------------

    IMF_EXPORT virtual Attribute* copy () const;

    //----------------
    // I/O and copying
    //----------------

    IMF_EXPORT virtual void writeValueTo (
        OPENEXR_IMF_INTERNAL_NAMESPACE::OStream& os, int version) const;

    IMF_EXPORT virtual void readValueFrom (
        OPENEXR_IMF_INTERNAL_NAMESPACE::IStream& is, int size, int version);

    IMF_EXPORT virtual void copyValueFrom (const Attribute& other);

    int                dataSize () const { return _dataSize; }
    const Array<char>& data () const { return _data; }

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
    long        _dataSize;
    Array<char> _data;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
