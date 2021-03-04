//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_FLOAT_ATTRIBUTE_H
#define INCLUDED_IMF_FLOAT_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class FloatAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<float> FloatAttribute;
template <> IMF_EXPORT const char *FloatAttribute::staticTypeName ();


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
