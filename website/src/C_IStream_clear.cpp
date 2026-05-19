//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//
#include <cstdio>
#include "C_IStream.cpp"
void
C_IStream::clear ()
{
    clearerr (_file);
}
