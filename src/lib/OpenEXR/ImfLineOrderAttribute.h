//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_LINE_ORDER_ATTRIBUTE_H
#define INCLUDED_IMF_LINE_ORDER_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class LineOrderAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfLineOrder.h"
#include "ImfExport.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::LineOrder> LineOrderAttribute;

template <> 
IMF_EXPORT 
const char *LineOrderAttribute::staticTypeName ();

template <> 
IMF_EXPORT 
void LineOrderAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                       int) const;

template <> 
IMF_EXPORT 
void LineOrderAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                        int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
