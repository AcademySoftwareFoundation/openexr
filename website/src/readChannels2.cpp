//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// [begin directAccess]
#include <ImfInputFile.h>
#include <ImfChannelList.h>

using namespace OPENEXR_IMF_NAMESPACE;

void 
readChannels (const char fileName[])
{
    
    InputFile file (fileName);

    const ChannelList &channels = file.header().channels();

    const Channel &channel = channels["G"];

    const Channel *channelPtr = channels.findChannel("G");
}
// [end directAccess]
