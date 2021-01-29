// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#ifndef INCLUDED_IMF_IDMANIFEST_ATTRIBUTE_H
#define INCLUDED_IMF_IDMANIFEST_ATTRIBUTE_H

#include "ImfAttribute.h"
#include "ImfNamespace.h"
#include "ImfIDManifest.h"
#include <vector>


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER



typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::CompressedIDManifest>
    IDManifestAttribute;

template <>
IMF_EXPORT
const char *IDManifestAttribute::staticTypeName ();

template <>
IMF_EXPORT
void IDManifestAttribute::writeValueTo
    (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;

template <>
IMF_EXPORT
void IDManifestAttribute::readValueFrom
    (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
