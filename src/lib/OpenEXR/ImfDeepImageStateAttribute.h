//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_DEEPIMAGESTATE_ATTRIBUTE_H
#define INCLUDED_IMF_DEEPIMAGESTATE_ATTRIBUTE_H


//-----------------------------------------------------------------------------
//
//	class DeepImageStateAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfDeepImageState.h"
#include "ImfExport.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::DeepImageState>
    DeepImageStateAttribute;

template <> IMF_EXPORT const char *DeepImageStateAttribute::staticTypeName ();

template <> IMF_EXPORT
void DeepImageStateAttribute::writeValueTo
    (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;

template <> IMF_EXPORT
void DeepImageStateAttribute::readValueFrom
    (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);

template <> IMF_EXPORT
void DeepImageStateAttribute::copyValueFrom (const OPENEXR_IMF_INTERNAL_NAMESPACE::Attribute &other);

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
