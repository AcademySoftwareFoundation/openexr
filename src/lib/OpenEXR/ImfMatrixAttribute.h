//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_MATRIX_ATTRIBUTE_H
#define INCLUDED_IMF_MATRIX_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class M33fAttribute
//	class M33dAttribute
//	class M44fAttribute
//	class M44dAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImathMatrix.h"
#include "ImfExport.h"


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<IMATH_NAMESPACE::M33f> M33fAttribute;
template <> IMF_EXPORT const char *M33fAttribute::staticTypeName ();
template <> IMF_EXPORT void M33fAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;
template <> IMF_EXPORT void M33fAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);


typedef TypedAttribute<IMATH_NAMESPACE::M33d> M33dAttribute;
template <> IMF_EXPORT const char *M33dAttribute::staticTypeName ();
template <> IMF_EXPORT void M33dAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;
template <> IMF_EXPORT void M33dAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);


typedef TypedAttribute<IMATH_NAMESPACE::M44f> M44fAttribute;
template <> IMF_EXPORT const char *M44fAttribute::staticTypeName ();
template <> IMF_EXPORT void M44fAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;
template <> IMF_EXPORT void M44fAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);


typedef TypedAttribute<IMATH_NAMESPACE::M44d> M44dAttribute;
template <> IMF_EXPORT const char *M44dAttribute::staticTypeName ();
template <> IMF_EXPORT void M44dAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;
template <> IMF_EXPORT void M44dAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
