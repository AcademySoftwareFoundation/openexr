//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	class IntAttribute
//
//-----------------------------------------------------------------------------

#define COMPILING_IMF_INT_ATTRIBUTE
#include "ImfIntAttribute.h"


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

//#if defined(__MINGW32__)
//template <>
//IMF_EXPORT
//TypedAttribute<int>::~TypedAttribute ()
//{
//}
//#endif

template <>
IMF_EXPORT const char *
IntAttribute::staticTypeName ()
{
    return "int";
}

template class IMF_EXPORT_TEMPLATE_INSTANCE TypedAttribute<int>;


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
