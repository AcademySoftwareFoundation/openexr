//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//
// example of using an IDManifest to locate given objects in a deep image with IDs
// demonstrates how to use multivariate IDs, and 64 bit IDs spread across two channels
// deepidexample will create images that can be used as input
// (though this tool is intended to support images from other sources)
//

#include <ImfChannelList.h>
#include <ImfMultiPartInputFile.h>
#include <ImfDeepScanLineInputPart.h>
#include <ImfMultiPartOutputFile.h>
#include <ImfDeepScanLineOutputPart.h>
#include <ImfOutputPart.h>
#include <ImfDeepFrameBuffer.h>
#include <ImfIDManifest.h>
#include <ImfStandardAttributes.h>
#include <ImfPartType.h>
#include <iomanip>
#include <list>
#include <map>
#include <vector>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;

using std::cerr;
using std::dec;
using std::endl;
using std::hex;
using std::list;
using std::map;
using std::set;
using std::stoi;
using std::vector;

struct match
{
    int      channel1; // index of first channel to look up ID in
    uint32_t id1;      // first ID
    int      channel2; // index of second channel, or -1 if only one channel
    uint32_t id2;      // second ID, ignored channel2==-1
};

//
// setIds parses the matches arguments, and populates lists of matching IDs. If there are '--and' statements in the matches
// ids are entered in a new list
//
void setIds (
    const IDManifest&       mfst,
    list<list<match>>&      ids,
    const char*             matches[],
    int                     numMatches,
    const map<string, int>& channelToPos);

int
main (int argc, const char* argv[])
{
    if (argc < 4)
    {
        cerr
            << "syntax: [--mask] input.exr match [match...] [--and match [match...]] output.exr\n"
            << " if --mask specified, writes a shallow EXR with a mask of the selected object(s) in the 'A' channel\n"
            << " otherwise, writes a deep EXR only containing the selected object(s)\n"
            << '\n'
            << " matches can be:\n"
            << "   searchstring - match any component of any channel\n"
            << "   componentname:searchstring - only match given component\n"
            << "   channelname:number - match specified numeric ID in given channel\n"
            << '\n'
            << "\"A B --and C D\" means \"(must match either A or B) and also (must match either C or D)\"\n"
            << " e.g:\n"
            << "  input.deep.exr blue output.deep.exr\n"
            << "  input.deep.exr material:blue --and model:blob output.deep.exr\n"
            << "  input.deep.exr material:blue material:red --and model:blob output.deep.exr\n"
            << "  input.deep.exr particleid:3 output.deep.exr\n";
        return 1;
    }

    bool         mask              = false;
    int          numMatchArguments = argc - 2;
    const char** matchArguments    = argv + 2;
    const char*  inputFile         = argv[1];
    if (strcmp (argv[1], "--mask") == 0)
    {
        mask = true;
        numMatchArguments--;
        matchArguments++;
        inputFile = argv[2];
    }

    MultiPartInputFile input (inputFile);

    if (!hasIDManifest (input.header (0)))
    {
        cerr << "deepidselect requires an ID manifest in the EXR header\n";
        return 1;
    }

    for (int i = 0; i < input.parts (); ++i)
    {
        if (input.header (i).type () != DEEPSCANLINE)
        {
            cerr
                << "deepidselect currently only supports files which are entirely deep scanline files\n";
            return 1;
        }
    }

    //
    // build output headers. For deep output, this is easy: just copy them over
    // for masks, build scanline images of the same dimensions, each with a single alpha channel
    //
    vector<Header> hdrs (input.parts ());
    if (mask)
    {
        for (int h = 0; h < input.parts (); ++h)
        {
            const Header& inHdr   = input.header (h);
            hdrs[h].dataWindow () = inHdr.dataWindow ();
            hdrs[h].setType (SCANLINEIMAGE);
            hdrs[h].displayWindow () = inHdr.displayWindow ();
            if (inHdr.hasView ()) { hdrs[h].setView (inHdr.view ()); }
            if (inHdr.hasName ()) { hdrs[h].setName (inHdr.name ()); }
            hdrs[h].channels ().insert ("A", Channel (HALF));
        }
    }
    else
    {
        for (int h = 0; h < input.parts (); ++h)
        {
            hdrs[h] = input.header (h);
        }
    }

    MultiPartOutputFile output (argv[argc - 1], hdrs.data (), input.parts ());

    // process each part individually

    for (int pt = 0; pt < input.parts (); ++pt)
    {
        const Header&      inputHeader = input.header (pt);
        const ChannelList& inputChans  = inputHeader.channels ();
        Box2i              dataWindow  = inputHeader.dataWindow ();
        int                width = dataWindow.max.x + 1 - dataWindow.min.x;

        map<string, int>
            channelToPos; // index of each channel as stored in scanLine object

        int       alphaChannel     = -1;
        PixelType alphaChannelType = HALF;

        int channels = 0;
        for (ChannelList::ConstIterator i = inputChans.begin ();
             i != inputChans.end ();
             ++i)
        {
            channelToPos[i.name ()] = channels;
            if (strcmp (i.name (), "A") == 0)
            {
                alphaChannel     = channels;
                alphaChannelType = i.channel ().type;
            }
            channels++;
        }

        // if the part has a manifest, use this part's manifest
        // otherwise use part 0's manifest.
        // (but reparse the manifest for every part in case the channel list has changed)
        //
        int               manifestPart = hasIDManifest (inputHeader) ? pt : 0;
        list<list<match>> ids;
        IDManifest        mfst = idManifest (input.header (manifestPart));

        setIds (mfst, ids, matchArguments, numMatchArguments, channelToPos);

        // store for an individual deep scanline. Accessed using scanLine[channelIndex][pixelIndex][sampleIndex]
        // where pixelIndex is 0 for the leftmost pixel (even if the dataWindow doesn't start at 0)
        vector<vector<vector<uint32_t>>> scanLine (channels);

        // pointers to the data in each channel for FrameBuffer
        vector<vector<uint32_t*>> scanPointers (channels);

        for (int i = 0; i < channels; ++i)
        {
            scanLine[i].resize (width);
            scanPointers[i].resize (width);
        }

        vector<int>  pixelCounts (width);
        vector<half> outputAlpha (
            width); // only required for --mask mode: stores output

        DeepFrameBuffer buf;
        buf.insertSampleCountSlice (Slice (
            UINT,
            (char*) pixelCounts.data () - dataWindow.min.x,
            sizeof (int),
            0));
        int c = 0;

        //
        // read all channels as their native type, so they round trip when writing deep
        //
        for (ChannelList::ConstIterator i = inputChans.begin ();
             i != inputChans.end ();
             ++i, ++c)
        {
            buf.insert (
                i.name (),
                DeepSlice (
                    i.channel ().type,
                    (char*) scanPointers[c].data () - dataWindow.min.x,
                    sizeof (char*),
                    0,
                    sizeof (int32_t)));
        }

        DeepScanLineInputPart inPart (input, pt);
        inPart.setFrameBuffer (buf);

        //
        // for mask, create an alpha channel and initialize a FrameBuffer with that data
        // otherwise, can use the deep frame buffer for both input and output, since the data is processed in-place
        //
        if (mask)
        {
            FrameBuffer outBuf;
            outBuf.insert (
                "A",
                Slice (
                    HALF,
                    (char*) outputAlpha.data () - dataWindow.min.x,
                    sizeof (half),
                    0));
            OutputPart outPart (output, pt);
            outPart.setFrameBuffer (outBuf);
        }
        else
        {
            DeepScanLineOutputPart outPart (output, pt);
            outPart.setFrameBuffer (buf);
        }

        for (int y = dataWindow.min.y; y <= dataWindow.max.y; ++y)
        {
            inPart.readPixelSampleCounts (y);
            for (int c = 0; c < channels; ++c)
            {
                for (int x = 0; x < width; ++x)
                {
                    scanLine[c][x].resize (pixelCounts[x]);
                    scanPointers[c][x] = scanLine[c][x].data ();
                }
            }

            inPart.readPixels (y);

            for (int x = 0; x < width; ++x)
            {
                int outputSample = 0;

                float totalAlpha = 0.f;
                float maskAlpha  = 0.f;

                for (int s = 0; s < pixelCounts[x]; ++s)
                {
                    //
                    // should sample s be retained?
                    // look for an entry in each 'and group' where all the required channels match their
                    // corresponding values
                    //

                    bool good = true;
                    for (list<list<match>>::const_iterator idGroup =
                             ids.begin ();
                         idGroup != ids.end () && good;
                         ++idGroup)
                    {
                        good = false;
                        for (list<match>::const_iterator i = idGroup->begin ();
                             i != idGroup->end () && !good;
                             ++i)
                        {
                            if (scanLine[i->channel1][x][s] == i->id1 &&
                                (i->channel2 == -1 ||
                                 scanLine[i->channel2][x][s] == i->id2))
                            {
                                good = true;
                            }
                        }
                    }

                    //
                    // deep output mode:
                    //   delete unwanted samples
                    // mask output node:
                    //   composite together wanted sample's alpha
                    //

                    if (mask)
                    {
                        //cast alpha to float, and composite
                        float alpha = 0.f;
                        switch (alphaChannelType)
                        {
                            case FLOAT:
                                alpha = *(float*) (&scanLine[alphaChannel][x]
                                                        .data ()[s]);
                                break;
                            case HALF:
                                alpha = *(half*) (&scanLine[alphaChannel][x]
                                                       .data ()[s]);
                                break;
                            case UINT:
                                alpha = scanLine[alphaChannel][x][s];
                                break; //wat! this is a weird thing to do,but whatever...
                            case NUM_PIXELTYPES: break;
                        }

                        if (good) { maskAlpha += (1.0 - totalAlpha) * alpha; }
                        totalAlpha += (1.0 - totalAlpha) * alpha;
                    }

                    else if (good)
                    {
                        // keep Sample: copy from original position into output position
                        // (so overwrite any samples that are to be deleted)
                        for (int c = 0; c < channels; ++c)
                        {
                            scanLine[c][x][outputSample] = scanLine[c][x][s];
                        }
                        outputSample++;
                    }
                }

                if (mask)
                {
                    if (totalAlpha > 0.f) { maskAlpha /= totalAlpha; }
                    outputAlpha[x] = maskAlpha;
                }
                else
                {
                    // update total count of samples
                    pixelCounts[x] = outputSample;
                }
            }

            //
            // write data out
            //
            if (mask)
            {
                OutputPart outPart (output, pt);
                outPart.writePixels (1);
            }
            else
            {
                DeepScanLineOutputPart outPart (output, pt);
                outPart.writePixels (1);
            }
        }
    }
}

void
setIds (
    const IDManifest&       mfst,
    list<list<match>>&      ids,
    const char*             matches[],
    int                     numMatches,
    const map<string, int>& channelToPos)
{

    //
    // initially one single list
    //
    ids.clear ();
    ids.push_back (list<match> ());

    //
    // check each manifest, each entry, against each matching expression
    //
    for (int c = 0; c < numMatches; ++c)
    {

        //
        // an 'and' argument means proceed to the next group of ids
        // must find a match in every such group
        //
        if (strcmp (matches[c], "--and") == 0)
        {
            ids.push_back (list<match> ());
            continue;
        }

        string matchString (matches[c]);

        //
        // handle strings of the form component:searchstring
        // and channel:idnumber
        //
        string            componentName;
        string::size_type pos = matchString.find (':');
        if (pos != string::npos)
        {
            componentName = matchString.substr (0, pos);
            matchString   = matchString.substr (pos + 1);
        }

        if (matchString.find_first_not_of ("0123456789") == string::npos)
        {
            map<string, int>::const_iterator chan =
                channelToPos.find (componentName);
            if (chan != channelToPos.end ())
            {
                match m;
                m.channel1 = chan->second;
                m.id1      = stoi (matchString);
                m.channel2 = -1;
                ids.back ().push_back (m);
            }
            continue; // skip parsing the manifests for this string
        }

        // check the manifest for each group of channels
        for (size_t i = 0; i < mfst.size (); ++i)
        {

            for (IDManifest::ChannelGroupManifest::ConstIterator it =
                     mfst[i].begin ();
                 it != mfst[i].end ();
                 ++it)
            {
                for (size_t stringIndex = 0; stringIndex < it.text ().size ();
                     ++stringIndex)
                {
                    if (componentName == "" ||
                        mfst[i].getComponents ()[stringIndex] == componentName)
                    {
                        // simple substring matching only: could do wildcards or regexes here instead
                        if (it.text ()[stringIndex].find (matchString) !=
                            string::npos)
                        {
                            // a match is found - add it to the corresponding channels
                            if (mfst[i].getEncodingScheme () ==
                                IDManifest::ID_SCHEME)
                            {
                                // simple scheme: the ID channel has to match
                                for (const string& s: mfst[i].getChannels ())
                                {
                                    map<string, int>::const_iterator chan =
                                        channelToPos.find (s);
                                    if (chan != channelToPos.end ())
                                    {
                                        //could support matching the ID against a specific channel
                                        //that check would happen here

                                        match m;
                                        m.channel1 = chan->second;
                                        m.id1      = uint32_t (it.id ());
                                        m.channel2 = -1;
                                        ids.back ().push_back (m);
                                        cerr << "adding match " << hex
                                             << it.id () << dec
                                             << " for string "
                                             << it.text ()[stringIndex]
                                             << " in channel " << chan->second
                                             << '(' << chan->first << ")\n";
                                    }
                                }
                            }
                            else if (
                                mfst[i].getEncodingScheme () ==
                                IDManifest::ID2_SCHEME)
                            {
                                // 64 bit IDs are spread across two channels, with the least significant bits
                                // in the first channel (alphabetically) and the most significant bits in the second
                                // so process the channel set in pairs

                                set<string>::const_iterator chanLow =
                                    mfst[i].getChannels ().begin ();
                                set<string>::const_iterator end =
                                    mfst[i].getChannels ().end ();

                                while (chanLow != end)
                                {
                                    set<string>::const_iterator chanHigh =
                                        chanLow;
                                    ++chanHigh;

                                    if (chanHigh != end)
                                    {
                                        map<string, int>::const_iterator
                                            chanIdxLow =
                                                channelToPos.find (*chanLow);
                                        map<string, int>::const_iterator
                                            chanIdxHigh =
                                                channelToPos.find (*chanHigh);

                                        if (chanIdxLow != channelToPos.end () &&
                                            chanIdxHigh != channelToPos.end ())
                                        {
                                            // to match against specific channels, check at least one channel matches here
                                            match m;
                                            m.channel1 = chanIdxLow->second;
                                            m.id1      = it.id () & 0xFFFFFFFF;
                                            m.channel2 = chanIdxHigh->second;
                                            m.id2      = it.id () >> 32;
                                            ids.back ().push_back (m);

                                            cerr << "adding match " << hex
                                                 << it.id () << dec
                                                 << " for string "
                                                 << it.text ()[stringIndex]
                                                 << ": " << hex << m.id1
                                                 << " in channel " << m.channel1
                                                 << '(' << chanIdxLow->first
                                                 << "), " << hex << m.id2
                                                 << " in channel " << m.channel2
                                                 << '(' << chanIdxHigh->first
                                                 << ")\n";
                                        }

                                        ++chanLow;
                                        if (chanLow != end) { ++chanLow; }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
