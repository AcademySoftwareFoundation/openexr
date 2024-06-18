
//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "exrmetrics.h"

#include "ImfChannelList.h"
#include "ImfDeepFrameBuffer.h"
#include "ImfDeepScanLineInputPart.h"
#include "ImfDeepScanLineOutputPart.h"
#include "ImfDeepTiledInputPart.h"
#include "ImfDeepTiledOutputPart.h"
#include "ImfHeader.h"
#include "ImfInputPart.h"
#include "ImfMisc.h"
#include "ImfMultiPartInputFile.h"
#include "ImfMultiPartOutputFile.h"
#include "ImfOutputPart.h"
#include "ImfPartType.h"
#include "ImfTiledInputPart.h"
#include "ImfTiledOutputPart.h"

#include <ctime>
#include <list>
#include <stdexcept>
#include <vector>
#include <sys/stat.h>

using namespace Imf;
using Imath::Box2i;

using std::cerr;
using std::clock;
using std::cout;
using std::list;
using std::runtime_error;
using std::string;
using std::to_string;
using std::vector;

double
timing (clock_t start, clock_t end)
{
    return double (end - start) / double (CLOCKS_PER_SEC);
}

int
channelCount (const Header& h)
{
    int channels = 0;
    for (ChannelList::ConstIterator i = h.channels ().begin ();
         i != h.channels ().end ();
         ++i)
    {
        ++channels;
    }
    return channels;
}

void
copyDeepTiled (DeepTiledInputPart& in, DeepTiledOutputPart& out, bool verbose)
{
    throw runtime_error ("deep tiled parts are not yet supported");
}

void
copyScanLine (InputPart& in, OutputPart& out, bool verbose)
{
    Box2i    dw        = in.header ().dataWindow ();
    uint64_t width     = dw.max.x + 1 - dw.min.x;
    uint64_t height    = dw.max.y + 1 - dw.min.y;
    uint64_t numPixels = width * height;
    int      numChans  = channelCount (in.header ());

    vector<vector<char>> pixelData (numChans);
    uint64_t offsetToOrigin = width * static_cast<uint64_t> (dw.min.y) +
                              static_cast<uint64_t> (dw.min.x);

    int         channelNumber = 0;
    int         pixelSize     = 0;
    FrameBuffer buf;

    for (ChannelList::ConstIterator i = out.header ().channels ().begin ();
         i != out.header ().channels ().end ();
         ++i)
    {
        int samplesize = pixelTypeSize (i.channel ().type);
        pixelData[channelNumber].resize (numPixels * samplesize);

        buf.insert (
            i.name (),
            Slice (
                i.channel ().type,
                pixelData[channelNumber].data () - offsetToOrigin * samplesize,
                samplesize,
                samplesize * width));
        ++channelNumber;
        pixelSize += samplesize;
    }

    in.setFrameBuffer (buf);
    out.setFrameBuffer (buf);
    clock_t startRead = clock ();
    in.readPixels (dw.min.y, dw.max.y);
    clock_t endRead = clock ();

    clock_t startWrite = clock ();
    out.writePixels (height);
    clock_t endWrite = clock ();

    cout << "   \"read time\": " << timing (startRead, endRead) << ",\n";
    cout << "   \"write time\": " << timing (startWrite, endWrite) << ",\n";
    cout << "   \"pixel count\": " << numPixels << ",\n";
    cout << "   \"raw size\": " << numPixels * pixelSize << ",\n";
}

void
copyTiled (const TiledInputPart& in, TiledOutputPart& out, bool verbose)
{
    throw runtime_error ("tiled parts are not yet supported");
}

void
copyDeepScanLine (
    DeepScanLineInputPart& in, DeepScanLineOutputPart& out, bool verbose)
{
    Box2i       dw        = in.header ().dataWindow ();
    uint64_t    width     = dw.max.x + 1 - dw.min.x;
    uint64_t    height    = dw.max.y + 1 - dw.min.y;
    uint64_t    numPixels = width * height;
    int         numChans  = channelCount (in.header ());
    vector<int> sampleCount (numPixels);

    uint64_t offsetToOrigin = width * static_cast<uint64_t> (dw.min.y) +
                              static_cast<uint64_t> (dw.min.x);
    vector<vector<char*>> pixelPtrs (numChans);

    DeepFrameBuffer buffer;

    buffer.insertSampleCountSlice (Slice (
        UINT,
        (char*) (sampleCount.data () - offsetToOrigin),
        sizeof (int),
        sizeof (int) * width));
    int channelNumber  = 0;
    int bytesPerSample = 0;
    for (ChannelList::ConstIterator i = out.header ().channels ().begin ();
         i != out.header ().channels ().end ();
         ++i)
    {
        pixelPtrs[channelNumber].resize (numPixels);
        int samplesize = pixelTypeSize (i.channel ().type);
        buffer.insert (
            i.name (),
            DeepSlice (
                i.channel ().type,
                (char*) (pixelPtrs[channelNumber].data () - offsetToOrigin),
                sizeof (char*),
                sizeof (char*) * width,
                samplesize));
        ++channelNumber;
        bytesPerSample += samplesize;
    }

    in.setFrameBuffer (buffer);
    out.setFrameBuffer (buffer);

    clock_t startCountRead = clock ();
    in.readPixelSampleCounts (dw.min.y, dw.max.y);
    clock_t endCountRead = clock ();

    size_t totalSamples = 0;

    for (int i: sampleCount)
    {
        totalSamples += i;
    }

    vector<vector<char>> sampleData (numChans);
    channelNumber = 0;
    for (ChannelList::ConstIterator i = in.header ().channels ().begin ();
         i != in.header ().channels ().end ();
         ++i)
    {
        int samplesize = pixelTypeSize (i.channel ().type);
        sampleData[channelNumber].resize (samplesize * totalSamples);
        int offset = 0;
        for (int p = 0; p < numPixels; ++p)
        {
            pixelPtrs[channelNumber][p] =
                sampleData[channelNumber].data () + offset * samplesize;
            offset += sampleCount[p];
        }

        ++channelNumber;
    }

    clock_t startSampleRead = clock ();
    in.readPixels (dw.min.y, dw.max.y);
    clock_t endSampleRead = clock ();

    clock_t startWrite = clock ();
    out.writePixels (height);
    clock_t endWrite = clock ();

    cout << "   \"count read time\": " << timing (startCountRead, endCountRead)
         << ",\n";
    cout << "   \"sample read time\": "
         << timing (startSampleRead, endSampleRead) << ",\n";
    cout << "   \"write time\": " << timing (startWrite, endWrite) << ",\n";
    cout << "   \"pixel count\": " << numPixels << ",\n";
    cout << "   \"raw size\": "
         << totalSamples * bytesPerSample + numPixels * sizeof (int) << ",\n";
}

void
exrmetrics (
    const char       inFileName[],
    const char       outFileName[],
    int              part,
    Imf::Compression compression,
    float            level,
    int              halfMode,
    bool             verbose)
{
    MultiPartInputFile in (inFileName);
    if (part >= in.parts ())
    {
        throw runtime_error ((string (inFileName) + " only contains " +
                              to_string (in.parts ()) +
                              " parts. Cannot copy part " + to_string (part))
                                 .c_str ());
    }
    Header outHeader = in.header (part);

    if (compression < NUM_COMPRESSION_METHODS)
    {
        outHeader.compression () = compression;
    }
    else { compression = outHeader.compression (); }

    if (!isinf (level) && level >= -1)
    {
        switch (outHeader.compression ())
        {
            case DWAA_COMPRESSION:
            case DWAB_COMPRESSION:
                outHeader.dwaCompressionLevel () = level;
                break;
            case ZIP_COMPRESSION:
            case ZIPS_COMPRESSION:
                outHeader.zipCompressionLevel () = level;
                break;
                //            case ZSTD_COMPRESSION :
                //                outHeader.zstdCompressionLevel()=level;
                //                break;
            default:
                throw runtime_error (
                    "-l option only works for DWAA/DWAB,ZIP/ZIPS or ZSTD compression");
        }
    }

    if (halfMode > 0)
    {
        for (ChannelList::Iterator i = outHeader.channels ().begin ();
             i != outHeader.channels ().end ();
             ++i)
        {
            if (halfMode == 2 || !strcmp (i.name (), "R") ||
                !strcmp (i.name (), "G") || !strcmp (i.name (), "B") ||
                !strcmp (i.name (), "A"))
            {
                i.channel ().type = HALF;
            }
        }
    }

    string inCompress, outCompress;
    getCompressionNameFromId (in.header (part).compression (), inCompress);
    getCompressionNameFromId (outHeader.compression (), outCompress);
    cout << "{\n";
    cout << "   \"input file\": \"" << inFileName << "\",\n";
    cout << "   \"output file\": \"" << outFileName << "\",\n";
    cout << "   \"input compression\": \"" << inCompress << "\",\n";
    cout << "   \"output compression\": \"" << outCompress << "\",\n";
    if (compression == ZIP_COMPRESSION || compression == ZIPS_COMPRESSION)
    {
        cout << "   \"zipCompressionLevel\": "
             << outHeader.zipCompressionLevel () << ",\n";
    }

    if (compression == DWAA_COMPRESSION || compression == DWAB_COMPRESSION)
    {
        cout << "   \"dwaCompressionLevel\": "
             << outHeader.dwaCompressionLevel () << ",\n";
    }

    std::string type = outHeader.type ();
    cout << "   \"part type\": \"" << type << "\",\n";

    if (type == SCANLINEIMAGE)
    {
        cout << "   \"scanlines per chunk:\" : "
             << getCompressionNumScanlines (compression) << ",\n";
    }

    {
        MultiPartOutputFile out (outFileName, &outHeader, 1);

        if (type == TILEDIMAGE)
        {
            TiledInputPart  inpart (in, part);
            TiledOutputPart outpart (out, 0);
            copyTiled (inpart, outpart, verbose);
        }
        else if (type == SCANLINEIMAGE)
        {
            InputPart  inpart (in, part);
            OutputPart outpart (out, 0);
            copyScanLine (inpart, outpart, verbose);
        }
        else if (type == DEEPSCANLINE)
        {
            DeepScanLineInputPart  inpart (in, part);
            DeepScanLineOutputPart outpart (out, 0);
            copyDeepScanLine (inpart, outpart, verbose);
        }
        else if (type == DEEPTILE)
        {
            DeepTiledInputPart  inpart (in, part);
            DeepTiledOutputPart outpart (out, 0);
            copyDeepTiled (inpart, outpart, verbose);
        }
        else
        {
            throw runtime_error (
                (inFileName + string (" contains unknown part type ") + type)
                    .c_str ());
        }
    }
#ifdef __APPLE__
    struct stat instats, outstats;
    stat (inFileName, &instats);
    stat (outFileName, &outstats);
#else
    struct stat64 instats, outstats;
    stat64 (inFileName, &instats);
    stat64 (outFileName, &outstats);
#endif
    cout << "   \"input file size\": " << instats.st_size << ",\n";
    cout << "   \"output file size\": " << outstats.st_size << "\n";
    cout << "}\n";
}
