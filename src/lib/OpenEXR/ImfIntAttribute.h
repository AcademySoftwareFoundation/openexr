//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_INT_ATTRIBUTE_H
#define INCLUDED_IMF_INT_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class IntAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfNamespace.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<int> IntAttribute;
template <> IMF_EXPORT const char *IntAttribute::staticTypeName ();


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
