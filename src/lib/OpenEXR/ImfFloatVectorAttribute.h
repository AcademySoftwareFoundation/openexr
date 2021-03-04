//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Weta Digital, Ltd and Contributors to the OpenEXR Project.
//



#ifndef INCLUDED_IMF_FLOATVECTOR_ATTRIBUTE_H
#define INCLUDED_IMF_FLOATVECTOR_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class FloatVectorAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfNamespace.h"

#include <vector>


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

typedef std::vector<float>
    FloatVector;

typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::FloatVector>
    FloatVectorAttribute;

template <>
IMF_EXPORT
const char *FloatVectorAttribute::staticTypeName ();

template <>
IMF_EXPORT
void FloatVectorAttribute::writeValueTo
    (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &, int) const;

template <>
IMF_EXPORT
void FloatVectorAttribute::readValueFrom
    (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &, int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
