
//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//
// this file is found by the oss-fuzz project to generate a fuzzer. It is not part of
// OpenEXR's internal OpenEXRFuzzTest suite
//

#include <ImfNamespace.h>
#include <ImfCheckFile.h>
#include <stdint.h>

using OPENEXR_IMF_NAMESPACE::checkOpenEXRFile;
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    checkOpenEXRFile ((const char*) data , size , true , true, true);
    return 0;
}

