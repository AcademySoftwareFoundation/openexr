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
        , InputStreamMutex *mx
                              )
    : header (ctxt.header (partNumber))
    , numThreads (numThreads)
    , partNumber (partNumber)
    , context (ctxt)
    , version (ctxt.version ())
    , mutex (mx)
{
    uint64_t *ctable;
    int32_t ccount;
    if (EXR_ERR_SUCCESS == exr_get_chunk_table (ctxt, partNumber, &ctable, &ccount))
    {
        chunkOffsets.resize (ccount);
        memcpy (chunkOffsets.data (), ctable, size_t (ccount) * sizeof(uint64_t));
    }
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
