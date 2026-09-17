//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <ImfInputFile.h>
#include <ImfRgbaFile.h>
#include <ImfEnvmap.h>
#include <ImfStandardAttributes.h>

using namespace OPENEXR_IMF_NAMESPACE;

void
envmap1 ()
{
    char fileName[] = "";
    // [begin hasEnvmap]
    RgbaInputFile file (fileName);

    if (hasEnvmap (file.header()))
    {
        Envmap type = envmap (file.header());
        // ...
    }
    // [end hasEnvmap]

}
