//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_DOUBLE_ATTRIBUTE_H
#define INCLUDED_IMF_DOUBLE_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class DoubleAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfExport.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<double> DoubleAttribute;
template <> IMF_EXPORT const char *DoubleAttribute::staticTypeName ();


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT


#endif
