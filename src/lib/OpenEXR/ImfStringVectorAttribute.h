//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Weta Digital, Ltd and Contributors to the OpenEXR Project.
//



#ifndef INCLUDED_IMF_STRINGVECTOR_ATTRIBUTE_H
#define INCLUDED_IMF_STRINGVECTOR_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class StringVectorAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfNamespace.h"

#include <string>
#include <vector>


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

typedef std::vector<std::string> StringVector;
typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::StringVector> StringVectorAttribute;

template <>
IMF_EXPORT
const char *StringVectorAttribute::staticTypeName ();

template <>
IMF_EXPORT
void StringVectorAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                          int) const;

template <>
IMF_EXPORT
void StringVectorAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                           int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
