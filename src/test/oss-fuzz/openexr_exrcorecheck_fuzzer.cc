
//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

/*
 * This fuzzer uses the checkOpenEXRFile utility to validate the OpenEXR Core (C) API.
 * It passes the raw fuzzer input directly to the library's internal check
 * routine, specifically instructing it to use only the Core API read paths.
 */

#include "ImfNamespace.h"
#include "ImfCheckFile.h"
#include <stdint.h>

using OPENEXR_IMF_NAMESPACE::checkOpenEXRFile;
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    checkOpenEXRFile ((const char*) data , size , true , true, true);
    return 0;
}

