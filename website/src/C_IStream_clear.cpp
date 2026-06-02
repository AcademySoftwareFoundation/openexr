//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

void
C_IStream::clear ()
{
    clearerr (_file);
}
