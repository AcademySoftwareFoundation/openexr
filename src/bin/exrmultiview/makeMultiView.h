//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_MAKE_MULTI_VIEW_H
#define INCLUDED_MAKE_MULTI_VIEW_H

//----------------------------------------------------------------------------
//
//	Combine multiple single-view images
//	into one multi-view image.
//
//----------------------------------------------------------------------------

#include "namespaceAlias.h"
#include <ImfCompression.h>
#include <string>
#include <vector>

void makeMultiView (
    const std::vector<std::string>& viewNames,
    const std::vector<const char*>& inFileNames,
    const char*                     outFileName,
    IMF::Compression                compression,
    bool                            verbose);

#endif
