//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <cstdint>
#include <cstdio>
#include "C_IStream.h"

uint64_t
C_IStream::tellg ()
{
    return ftell (_file);
}

