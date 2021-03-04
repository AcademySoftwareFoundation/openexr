//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_PREVIEW_IMAGE_ATTRIBUTE_H
#define INCLUDED_IMF_PREVIEW_IMAGE_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class PreviewImageAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfPreviewImage.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::PreviewImage> PreviewImageAttribute;

template <>
IMF_EXPORT
const char *PreviewImageAttribute::staticTypeName ();

template <>
IMF_EXPORT
void PreviewImageAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                          int) const;

template <>
IMF_EXPORT
void PreviewImageAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                           int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
