// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include "ImfIDManifestAttribute.h"

#include <stdlib.h>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;

template <>
const char*
IDManifestAttribute::staticTypeName()
{
   return "idmanifest";
}


template <>
void
IDManifestAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    Xdr::write<StreamIO>(os,_value._uncompressedDataSize);
    const char* output = (const char*) _value._data;
    Xdr::write <StreamIO> (os, output,_value._compressedDataSize);

}


template <>
void
IDManifestAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    int read = 0;

    Xdr::read<StreamIO>(is,_value._uncompressedDataSize);
    _value._data = (unsigned char*) malloc(size-4);
    _value._compressedDataSize = size-4;
    char* input = (char*) _value._data;
    Xdr::read<StreamIO>(is,input,_value._compressedDataSize);
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
