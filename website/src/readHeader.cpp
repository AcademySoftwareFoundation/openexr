//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// [begin readHeader]
#include <ImfRgbaFile.h>
#include <ImfStandardAttributes.h>

#include <iostream>

using std::cout;
using std::endl;
using std::flush;

using namespace OPENEXR_IMF_NAMESPACE;

void
readHeader (const char fileName[])
{
    RgbaInputFile file (fileName);

    const StringAttribute* comments =
        file.header ().findTypedAttribute<StringAttribute> ("comments");

    const M44fAttribute* cameraTransform =
        file.header ().findTypedAttribute<M44fAttribute> ("cameraTransform");

    if (comments) cout << "comments " << comments->value () << endl;

    if (cameraTransform)
        cout << "cameraTransform " << cameraTransform->value () << flush;
}
// [end readHeader]
