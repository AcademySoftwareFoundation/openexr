//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	class DoubleAttribute
//
//-----------------------------------------------------------------------------

#include <ImfDoubleAttribute.h>


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


template <>
const char *
DoubleAttribute::staticTypeName ()
{
    return "double";
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT 
