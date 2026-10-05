//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// [begin useIterator]
#include <ImfInputFile.h>
#include <ImfChannelList.h>

using namespace OPENEXR_IMF_NAMESPACE;

void
readChannels (const char fileName[])
{
    InputFile file (fileName);

    const ChannelList &channels = file.header().channels();

    for (ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i)
    {
        const Channel &channel = i.channel();
        // ...
    }

}
// [end useIterator]
