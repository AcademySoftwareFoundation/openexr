void
readChannels(const char fileName[])
{
    InputFile file (fileName);
   
    // [begin useIterator]
    const ChannelList &channels = file.header().channels();

    for (ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i)
    {
        const Channel &channel = i.channel();
        // ...
    }
    // [end useIterator]
   
    // [begin directAccess]
    // const ChannelList &channels = file.header().channels();

    const Channel &channel = channels["G"];

    const Channel *channelPtr = channels.findChannel("G");
    // [end directAccess]
    
}

void
readLayers (const char fileName[])
{
    InputFile file (fileName);

    // [begin layers]
    const ChannelList &channels = file.header().channels(); ;

    std::set<string> layerNames;

    channels.layers (layerNames);

    for (std::set<std::string>::const_iterator i = layerNames.begin(); i != layerNames.end(); ++i)
    {
        cout << "layer " << *i << endl;

        ChannelList::ConstIterator layerBegin, layerEnd;
        channels.channelsInLayer (*i, layerBegin, layerEnd);
        for (ChannelList::ConstIterator j = layerBegin; j != layerEnd; ++j)
        {
            cout << "tchannel " << j.name() << endl;
        }
    }
    // [end layers]
}



