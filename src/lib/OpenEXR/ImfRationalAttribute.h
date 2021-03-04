//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_RATIONAL_ATTRIBUTE_H
#define INCLUDED_IMF_RATIONAL_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class RationalAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfRational.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::Rational> RationalAttribute;

template <>
IMF_EXPORT
const char *RationalAttribute::staticTypeName ();

template <>
IMF_EXPORT
void RationalAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                      int) const;

template <>
IMF_EXPORT
void RationalAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                       int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
