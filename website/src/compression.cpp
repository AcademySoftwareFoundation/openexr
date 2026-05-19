//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//
#include "ImfHeader.h"
#include "ImfChannelList.h"
#include "ImfCompression.h"

using namespace OPENEXR_IMF_NAMESPACE;

int main()
{
    int width = 1;
    int height = 1;

    Header header(width, height);

    header.channels().insert("G", Channel(HALF));
    header.channels().insert("Z", Channel(FLOAT));

    header.compression() = ZIP_COMPRESSION;
    header.zipCompressionLevel() = 6;

    setDefaultZipCompressionLevel(6);
    setDefaultDwaCompressionLevel(45.0f);

    return 0;
}
