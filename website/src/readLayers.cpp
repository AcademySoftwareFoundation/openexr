//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// [begin readLayers]
#include <ImfInputFile.h>
#include <ImfChannelList.h>

using std::cout;
using std::set;
using std::string;
using std::endl;

using namespace OPENEXR_IMF_NAMESPACE;

void
readLayers (const char fileName[])
{
    InputFile file (fileName);

    const ChannelList &channels = file.header().channels(); ;

    set<string> layerNames;

    channels.layers (layerNames);

    for (set<string>::const_iterator i = layerNames.begin(); i != layerNames.end(); ++i)
    {
        cout << "layer " << *i << endl;

        ChannelList::ConstIterator layerBegin, layerEnd;
        channels.channelsInLayer (*i, layerBegin, layerEnd);
        for (ChannelList::ConstIterator j = layerBegin; j != layerEnd; ++j)
        {
            cout << "tchannel " << j.name() << endl;
        }
    }
}
// [end readLayers]
