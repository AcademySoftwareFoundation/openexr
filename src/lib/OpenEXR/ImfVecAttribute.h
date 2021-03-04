//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_VEC_ATTRIBUTE_H
#define INCLUDED_IMF_VEC_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class V2iAttribute
//	class V2fAttribute
//	class V2dAttribute
//	class V3iAttribute
//	class V3fAttribute
//	class V3dAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImathVec.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

typedef TypedAttribute<IMATH_NAMESPACE::V2i> V2iAttribute;
template <> IMF_EXPORT const char *V2iAttribute::staticTypeName ();
template <> IMF_EXPORT void V2iAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;
template <> IMF_EXPORT void V2iAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);


typedef TypedAttribute<IMATH_NAMESPACE::V2f> V2fAttribute;
template <> IMF_EXPORT const char *V2fAttribute::staticTypeName ();
template <> IMF_EXPORT void V2fAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;
template <> IMF_EXPORT void V2fAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);


typedef TypedAttribute<IMATH_NAMESPACE::V2d> V2dAttribute;
template <> IMF_EXPORT const char *V2dAttribute::staticTypeName ();
template <> IMF_EXPORT void V2dAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;
template <> IMF_EXPORT void V2dAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);


typedef TypedAttribute<IMATH_NAMESPACE::V3i> V3iAttribute;
template <> IMF_EXPORT const char *V3iAttribute::staticTypeName ();
template <> IMF_EXPORT void V3iAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;
template <> IMF_EXPORT void V3iAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);


typedef TypedAttribute<IMATH_NAMESPACE::V3f> V3fAttribute;
template <> IMF_EXPORT const char *V3fAttribute::staticTypeName ();
template <> IMF_EXPORT void V3fAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;
template <> IMF_EXPORT void V3fAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);


typedef TypedAttribute<IMATH_NAMESPACE::V3d> V3dAttribute;
template <> IMF_EXPORT const char *V3dAttribute::staticTypeName ();
template <> IMF_EXPORT void V3dAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;
template <> IMF_EXPORT void V3dAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT


#if defined (OPENEXR_IMF_INTERNAL_NAMESPACE_AUTO_EXPOSE)
namespace Imf { using namespace OPENEXR_IMF_INTERNAL_NAMESPACE; }

#endif

#endif
