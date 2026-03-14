//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

void
C_IStream::seekg (uint64_t pos)
{
    clearerr (_file);
    fseek (_file, pos, SEEK_SET);
}

