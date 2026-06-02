//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

MemoryMappedIStream::~MemoryMappedIStream()
{
    munmap (_buffer, _fileLength);
}
