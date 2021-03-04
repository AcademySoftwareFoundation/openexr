//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_COMPRESSION_ATTRIBUTE_H
#define INCLUDED_IMF_COMPRESSION_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class CompressionAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfCompression.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::Compression> CompressionAttribute;
template <> IMF_EXPORT const char *CompressionAttribute::staticTypeName ();
template <> IMF_EXPORT void CompressionAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                                     int) const;
template <> IMF_EXPORT void CompressionAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                                      int,
                                                      int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT


#endif
