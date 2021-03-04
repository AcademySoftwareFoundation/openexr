//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_BOX_ATTRIBUTE_H
#define INCLUDED_IMF_BOX_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class Box2iAttribute
//	class Box2fAttribute
//
//-----------------------------------------------------------------------------

#include "ImfForward.h"
#include "ImfExport.h"
#include "ImfAttribute.h"
#include "ImathBox.h"
#include "ImfNamespace.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<IMATH_NAMESPACE::Box2i> Box2iAttribute;

template <>
IMF_EXPORT
const char *Box2iAttribute::staticTypeName ();
template <>
IMF_EXPORT
void Box2iAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                   int) const;
template <>
IMF_EXPORT
void Box2iAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                    int, int);


typedef TypedAttribute<IMATH_NAMESPACE::Box2f> Box2fAttribute;
template <>
IMF_EXPORT
const char *Box2fAttribute::staticTypeName ();
template <>
IMF_EXPORT
void Box2fAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                   int) const;
template <>
IMF_EXPORT
void Box2fAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                    int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
