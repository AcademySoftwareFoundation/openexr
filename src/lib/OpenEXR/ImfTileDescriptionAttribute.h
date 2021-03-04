//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_TILE_DESCRIPTION_ATTRIBUTE_H
#define INCLUDED_IMF_TILE_DESCRIPTION_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class TileDescriptionAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfTileDescription.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::TileDescription> TileDescriptionAttribute;

template <>
IMF_EXPORT 
const char *
TileDescriptionAttribute::staticTypeName ();

template <>
IMF_EXPORT 
void
TileDescriptionAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                        int) const;

template <>
IMF_EXPORT 
void
TileDescriptionAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                         int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
