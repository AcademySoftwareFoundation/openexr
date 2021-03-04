//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_KEY_CODE_ATTRIBUTE_H
#define INCLUDED_IMF_KEY_CODE_ATTRIBUTE_H


//-----------------------------------------------------------------------------
//
//	class KeyCodeAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfKeyCode.h"
#include "ImfExport.h"


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::KeyCode> KeyCodeAttribute;

template <>
IMF_EXPORT
const char *KeyCodeAttribute::staticTypeName ();

template <>
IMF_EXPORT
void KeyCodeAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                     int) const;

template <>
IMF_EXPORT
void KeyCodeAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                      int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
