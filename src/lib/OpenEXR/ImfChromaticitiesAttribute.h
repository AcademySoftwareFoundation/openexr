//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_CHROMATICITIES_ATTRIBUTE_H
#define INCLUDED_IMF_CHROMATICITIES_ATTRIBUTE_H


//-----------------------------------------------------------------------------
//
//	class ChromaticitiesAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfChromaticities.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::Chromaticities> ChromaticitiesAttribute;

template <>
IMF_EXPORT
const char *ChromaticitiesAttribute::staticTypeName ();

template <>
IMF_EXPORT
void ChromaticitiesAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                            int) const;

template <>
IMF_EXPORT
void ChromaticitiesAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                             int,
                                             int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT


#endif
