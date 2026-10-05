//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// [begin readComments]
#include <ImfRgbaFile.h>
#include <ImfStandardAttributes.h>

using namespace OPENEXR_IMF_NAMESPACE;

void
readComments (const char fileName[], string &comments)
{
    RgbaInputFile file (fileName);

    comments = file.header().typedAttribute<StringAttribute>("comments").value();
}
// [end readComments]
