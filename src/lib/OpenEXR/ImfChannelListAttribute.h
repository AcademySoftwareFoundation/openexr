//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_CHANNEL_LIST_ATTRIBUTE_H
#define INCLUDED_IMF_CHANNEL_LIST_ATTRIBUTE_H

//-----------------------------------------------------------------------------
//
//	class ChannelListAttribute
//
//-----------------------------------------------------------------------------

#include "ImfAttribute.h"
#include "ImfChannelList.h"
#include "ImfExport.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


typedef TypedAttribute<OPENEXR_IMF_INTERNAL_NAMESPACE::ChannelList> ChannelListAttribute;

template <>
IMF_EXPORT
const char *ChannelListAttribute::staticTypeName ();

template <>
IMF_EXPORT
void ChannelListAttribute::writeValueTo (OPENEXR_IMF_INTERNAL_NAMESPACE::OStream &,
                                         int) const;

template <>
IMF_EXPORT
void ChannelListAttribute::readValueFrom (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &,
                                          int, int);


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT


#endif

