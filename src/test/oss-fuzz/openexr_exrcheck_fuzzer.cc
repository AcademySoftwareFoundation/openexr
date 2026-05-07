
//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

/*
 * This fuzzer uses the checkOpenEXRFile utility to validate the OpenEXR C++ API.
 * It passes the raw fuzzer input directly to the library's internal check
 * routine, which attempts to read it using various C++ API paths.
 */

#include "ImfNamespace.h"
#include "ImfCheckFile.h"
#include <stdint.h>

using OPENEXR_IMF_NAMESPACE::checkOpenEXRFile;
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    checkOpenEXRFile ((const char*) data , size , true , true, false);
    return 0;
}
