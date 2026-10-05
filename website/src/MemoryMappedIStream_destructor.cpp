//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "MemoryMappedIStream.h"

#include <sys/mman.h>

MemoryMappedIStream::~MemoryMappedIStream()
{
    munmap (_buffer, _fileLength);
}
