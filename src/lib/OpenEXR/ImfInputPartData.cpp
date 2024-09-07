//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "ImfInputPartData.h"
#include "ImfNamespace.h"

#include <string.h>
#include <iostream>
OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

InputPartData::InputPartData (
        const Context& ctxt,
        int            partNumber,
        int            numThreads
                              )
    : header (ctxt.header (partNumber))
    , numThreads (numThreads)
    , partNumber (partNumber)
    , context (ctxt)
{
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
