//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	class FloatAttribute
//
//-----------------------------------------------------------------------------

#include <ImfFloatAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


template <>
const char *
FloatAttribute::staticTypeName ()
{
    return "float";
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
