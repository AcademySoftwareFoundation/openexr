//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	class DoubleAttribute
//
//-----------------------------------------------------------------------------

#define COMPILING_IMF_DOUBLE_ATTRIBUTE
#include "ImfDoubleAttribute.h"


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


template <>
IMF_EXPORT const char *
DoubleAttribute::staticTypeName ()
{
    return "double";
}

template class IMF_EXPORT_TEMPLATE_INSTANCE TypedAttribute<double>;


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
