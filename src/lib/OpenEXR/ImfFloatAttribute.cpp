//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	class FloatAttribute
//
//-----------------------------------------------------------------------------

#define COMPILING_IMF_FLOAT_ATTRIBUTE

#include <ImfFloatAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


template <>
IMF_EXPORT const char *
FloatAttribute::staticTypeName ()
{
    return "float";
}

template class IMF_EXPORT_TEMPLATE_INSTANCE TypedAttribute<float>;


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
