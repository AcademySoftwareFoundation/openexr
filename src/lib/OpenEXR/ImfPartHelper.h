//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Weta Digital, Ltd and Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_PARTHELPER_H
#define INCLUDED_IMF_PARTHELPER_H

//-----------------------------------------------------------------------------
//
//	Functions to help split channels into separate parts: provide a list of
//      channels, with desired views. call SplitChannels to assign a part to each
//      layer, or correct the name of the channel.
//      Also can enumerate the parts in a file and list which parts channels are in
//
//      This is a good way to offer a 'create Multipart file' checkbox to the user in a
//      write dialog box: Populate a list of MultiViewChannelName objects,
//      call SplitChannels with whether single or multipart files are required.
//      Then write the number of parts it specifies, using internal_name for the channel
//      names in the ChannelList and FrameBuffer objects. There should be no need
//      for different codepaths for single part and multipart files
//
//      Similarly, on reading a file as a MultiPartInputFile, use GetChannelsInMultiPartFile to
//      enumerate all channels in the file, using internal_name in FrameBuffer objects
//      to read the channel
//
//
//-----------------------------------------------------------------------------

#include "ImfChannelList.h"
#include "ImfExport.h"
#include "ImfForward.h"
#include "ImfMultiPartInputFile.h"
#include "ImfMultiView.h"
#include "ImfNamespace.h"
#include "ImfStandardAttributes.h"
#include "ImfStringVectorAttribute.h"

#include <map>
#include <set>
#include <string>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

struct MultiViewChannelName
{

public:
    std::string name; ///< name of channel
    std::string view; ///< view for channel

    int part_number; ///< part number: updated by SplitChannels
    std::string
        internal_name; ///< name used in headers: in singlepart mode, may contain viewname

    //return layer for this channel, or "" if no layer
    std::string getLayer () const
    {
        std::size_t q = name.rfind ('.');
        if (q == name.npos) { return ""; }
        return name.substr (0, q);
    }

    std::string getSuffix () const
    {
        std::size_t q = name.rfind ('.');
        if (q == name.npos) { return name; }
        return name.substr (q + 1);
    }
};

//
///\brief assigns individual channels to different parts based on their layer and view name
///       input is an array, list, vector etc of MultiViewChannelName objects
///       on entry, each MultiViewChannelName name/view must be set (view can be empty if not multiview)
///
///       if singlepart set, then on exit part_number will be zero, and internal_name will have view name inserted
///       otherwise, each channel will be assigned to a different part based on its layer name and view name
///
/// @param begin pointer to first MultiViewChannelName item
/// @param end   pointer to end of MultiViewChannelName item array
/// @return      total number of parts required
//

template <typename T>
inline int
SplitChannels (
    const T&           begin,
    const T&           end,
    bool               multipart = true,
    const std::string& heroView  = std::string ())
{
    if (!multipart)
    {
        for (T i = begin; i != end; i++)
        {
            i->part_number = 0;

            //does this have a view name set?
            if (i->view == "") { i->internal_name = i->name; }
            else
            {

                std::string lname = i->getLayer ();

                // no layer, only non-hero views get view name in layer name

                if (lname == "")
                {
                    if (i->view == heroView) { i->internal_name = i->name; }
                    else { i->internal_name = i->view + "." + i->name; }
                }
                else
                {
                    i->internal_name =
                        lname + "." + i->view + "." + i->getSuffix ();
                }
            }
        }
        // single part created
        return 1;
    }
    else
    {
        // step 1: extract individual layers and parts
        // for each layer, enumerate which views are active

        std::map<std::string, std::set<std::string>> viewsInLayers;
        for (T i = begin; i != end; i++)
        {
            viewsInLayers[i->getLayer ()].insert (i->view);
        }

        // step 2: assign a part number to each layer/view

        std::map<std::pair<std::string, std::string>, int> layerToPart;

        int partCount = 0;

        for (std::map<std::string, std::set<std::string>>::const_iterator
                 layer = viewsInLayers.begin ();
             layer != viewsInLayers.end ();
             layer++)
        {
            // if this layer has a heroView, insert that first
            bool layer_has_hero =
                layer->second.find (heroView) != layer->second.end ();
            if (layer_has_hero)
            {
                layerToPart[std::make_pair (layer->first, heroView)] =
                    partCount++;
            }

            // insert other layers which aren't the hero view
            for (std::set<std::string>::const_iterator view =
                     layer->second.begin ();
                 view != layer->second.end ();
                 view++)
            {
                if (*view != heroView)
                {
                    layerToPart[std::make_pair (layer->first, *view)] =
                        partCount++;
                }
            }
        }

        // step 3: update part number of each provided channel

        for (T i = begin; i != end; i++)
        {
            i->internal_name = i->name;
            i->part_number =
                layerToPart[std::make_pair (i->getLayer (), i->view)];
        }

        // return number of parts created
        return partCount;
    }
}

//
// populate the chans vector<MultiViewChannelName> with a list of channels in the file
// and their corresponding part number
//
template <class T>
inline void
GetChannelsInMultiPartFile (const MultiPartInputFile& file, T& chans)
{
    bool         has_multiview = false;
    StringVector mview;
    if (file.parts () == 1)
    {
        if (hasMultiView (file.header (0)))
        {
            mview         = multiView (file.header (0));
            has_multiview = true;
        }
    }

    for (int p = 0; p < file.parts (); p++)
    {
        const ChannelList& c = file.header (p).channels ();

        std::string view = "";
        if (file.header (p).hasView ()) { view = file.header (p).view (); }
        for (ChannelList::ConstIterator i = c.begin (); i != c.end (); i++)
        {
            MultiViewChannelName m;
            m.name          = std::string (i.name ());
            m.internal_name = m.name;

            if (has_multiview)
            {
                m.view = viewFromChannelName (m.name, mview);
                m.name = removeViewName (m.internal_name, m.view);
            }
            else { m.view = view; }
            m.part_number = p;
            chans.push_back (m);
        }
    }
}

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
