//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_ENVMAP_ATTRIBUTE_H
#define INCLUDED_IMF_ENVMAP_ATTRIBUTE_H


//-----------------------------------------------------------------------------
//
//	class EnvmapAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfEnvmap.h"
#include "ImfExport.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::Envmap> EnvmapAttribute;

template <> IMF_EXPORT const char *EnvmapAttribute::staticTypeName ();

template <> IMF_EXPORT
void EnvmapAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                    int) const;

template <> IMF_EXPORT
void EnvmapAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                     int,
                                     int);
template <> IMF_EXPORT
void
EnvmapAttribute::copyValueFrom (const OPENEXR_IMF_INTERNAL_NAMESPACE::Attribute &other);

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
