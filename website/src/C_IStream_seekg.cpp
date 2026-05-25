//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <cstdint>
#include <cstdio>
#include "C_IStream.h"

void
C_IStream::seekg (uint64_t pos)
{
    clearerr (_file);
    fseek (_file, pos, SEEK_SET);
}

