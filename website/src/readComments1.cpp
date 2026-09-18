//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// [begin readCommentsError]
#include <ImfRgbaFile.h>
#include <ImfStandardAttributes.h>

using namespace OPENEXR_IMF_NAMESPACE;

void
readComments (const char fileName[], const StringAttribute *&comments)
{
    // error: comments pointer is invalid after this function returns

    RgbaInputFile file (fileName);

    comments = file.header().findTypedAttribute <StringAttribute> ("comments");
}
// [end readCommentsError]
