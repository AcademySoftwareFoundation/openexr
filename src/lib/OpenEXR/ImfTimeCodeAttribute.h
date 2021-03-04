//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_TIME_CODE_ATTRIBUTE_H
#define INCLUDED_IMF_TIME_CODE_ATTRIBUTE_H


//-----------------------------------------------------------------------------
//
//	class TimeCodeAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfTimeCode.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::TimeCode> TimeCodeAttribute;

template <>
IMF_EXPORT
const char *TimeCodeAttribute::staticTypeName ();

template <>
IMF_EXPORT
void TimeCodeAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                      int) const;

template <>
IMF_EXPORT
void TimeCodeAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                       int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT




#endif
