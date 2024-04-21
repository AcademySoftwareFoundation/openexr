//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IMFINPUTPARTDATA_H_
#define IMFINPUTPARTDATA_H_

#include "ImfForward.h"

#include <vector>

#include "ImfHeader.h"

#include "ImfContext.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

struct InputPartData
{
    // TODO: reconsider / update
    Header                header;

    int                   numThreads;
    int                   partNumber;
    Context               context;

    InputPartData () = default;

    InputPartData (
        const Context& ctxt,
        int            partNumber,
        int            numThreads
        // TODO: remove
        , InputStreamMutex *mx
                   );

    // TODO: remove these
    int                   version;
    InputStreamMutex*     mutex;
    std::vector<uint64_t> chunkOffsets;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif /* IMFINPUTPARTDATA_H_ */
