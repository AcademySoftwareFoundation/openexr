//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_STRING_ATTRIBUTE_H
#define INCLUDED_IMF_STRING_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class StringAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include <string>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<std::string> StringAttribute;

template <>
IMF_EXPORT
const char *StringAttribute::staticTypeName ();

template <>
IMF_EXPORT
void StringAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                    int) const;

template <>
IMF_EXPORT
void StringAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                     int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
