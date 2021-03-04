//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IMFINPUTPARTDATA_H_
#define IMFINPUTPARTDATA_H_

#include <vector>

#include "ImfInputStreamMutex.h"
#include "ImfHeader.h"
#include "ImfNamespace.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


struct InputPartData
{
        Header                  header;
        int                     numThreads;
        int                     partNumber;
        int                     version;
        InputStreamMutex*       mutex;
        std::vector<uint64_t>   chunkOffsets;
        bool                    completed;

        IMF_EXPORT
        InputPartData(InputStreamMutex* mutex, const Header &header,
                      int partNumber, int numThreads, int version);

};


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT





#endif /* IMFINPUTPARTDATA_H_ */
