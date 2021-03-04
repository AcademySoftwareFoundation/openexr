//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	class IntAttribute
//
//-----------------------------------------------------------------------------

#include <ImfIntAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


template <>
const char *
IntAttribute::staticTypeName ()
{
    return "int";
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
