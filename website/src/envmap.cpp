//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <ImfInputFile.h>
#include <ImfRgbaFile.h>
#include <ImfEnvmap.h>

using namespace OPENEXR_IMF_NAMESPACE;

bool hasEnvmap(Header header)
{
    return true;
}

Envmap
envmap (Header header)
{
    
}

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
