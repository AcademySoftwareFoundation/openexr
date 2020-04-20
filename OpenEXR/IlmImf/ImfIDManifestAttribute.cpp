///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2017, Weta Digital Ltd
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Weta Digital nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
//
//        class StringVectorAttribute
//
//-----------------------------------------------------------------------------

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