//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Weta Digital, Ltd and Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	class StringVectorAttribute
//
//-----------------------------------------------------------------------------

#include <ImfStringVectorAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;


template <>
const char *
StringVectorAttribute::staticTypeName ()
{
    return "stringvector";
}


template <>
void
StringVectorAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &os, int version) const
{
    int size = _value.size();

    for (int i = 0; i < size; i++)
    {
        int strSize = _value[i].size();
        Xdr::write <StreamIO> (os, strSize);
	Xdr::write <StreamIO> (os, &_value[i][0], strSize);
    }
}


template <>
void
StringVectorAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int size, int version)
{
    int read = 0;

    while (read < size)
    {   
       int strSize;
       Xdr::read <StreamIO> (is, strSize);
       read += Xdr::size<int>();       

       // check there is enough space remaining in attribute to
       // contain claimed string length
       if( strSize < 0 ||  strSize > size - read)
       {
           throw IEX_NAMESPACE::InputExc("Invalid size field reading stringvector attribute");
       }

       std::string str;
       str.resize (strSize);
  
       if( strSize>0 )
       {
           Xdr::read<StreamIO> (is, &str[0], strSize);
       }
       
       read += strSize;

       _value.push_back (str);
    }
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
