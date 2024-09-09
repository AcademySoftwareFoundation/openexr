
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
#include "ImfTiledMisc.h"
#include "ImfTiledOutputPart.h"

#include <chrono>
#include <ctime>
#include <list>
#include <stdexcept>
#include <vector>
#include <sys/stat.h>

using namespace Imf;
using Imath::Box2i;

using std::cerr;
using namespace std::chrono;
using std::chrono::steady_clock;
using std::cout;
using std::endl;
using std::list;
using std::runtime_error;
using std::string;
using std::to_string;
using std::vector;

double
timing (steady_clock::time_point start, steady_clock::time_point end)
{
    return std::chrono::duration<double>(end-start).count();
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
copyScanLine (InputPart& in, OutputPart& out)
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

    steady_clock::time_point startRead = steady_clock::now();
    in.readPixels (dw.min.y, dw.max.y);
    steady_clock::time_point endRead = steady_clock::now();

    steady_clock::time_point startWrite = steady_clock::now();
    out.writePixels (height);
    steady_clock::time_point endWrite = steady_clock::now();

    cout << "   \"read time\": " << timing (startRead, endRead) << ",\n";
    cout << "   \"write time\": " << timing (startWrite, endWrite) << ",\n";
    cout << "   \"pixel count\": " << numPixels << ",\n";
    cout << "   \"raw size\": " << numPixels * pixelSize << ",\n";
}

void
copyTiled (TiledInputPart& in, TiledOutputPart& out)
{
    int             numChans = channelCount (in.header ());
    TileDescription tiling   = in.header ().tileDescription ();

    Box2i imageDw = in.header ().dataWindow ();
    int   totalLevels;
    switch (tiling.mode)
    {
        case ONE_LEVEL: totalLevels = 1; //break;
        case MIPMAP_LEVELS: totalLevels = in.numLevels (); break;
        case RIPMAP_LEVELS:
            totalLevels = in.numXLevels () * in.numYLevels ();
            break;
        case NUM_LEVELMODES: throw runtime_error ("unknown tile mode");
    }

    vector<vector<vector<char>>> pixelData (totalLevels);
    vector<FrameBuffer>          frameBuffer (totalLevels);

    int    levelIndex  = 0;
    int    pixelSize   = 0;
    size_t totalPixels = 0;

    //
    // allocate memory and initialize frameBuffer for each level
    //
    for (int xLevel = 0; xLevel < in.numXLevels (); ++xLevel)
    {
        for (int yLevel = 0; yLevel < in.numYLevels (); ++yLevel)
        {
            if (tiling.mode == RIPMAP_LEVELS || xLevel == yLevel)
            {
                Box2i dw = dataWindowForLevel (
                    tiling,
                    imageDw.min.x,
                    imageDw.max.x,
                    imageDw.min.y,
                    imageDw.max.y,
                    xLevel,
                    yLevel);
                uint64_t width     = dw.max.x + 1 - dw.min.x;
                uint64_t height    = dw.max.y + 1 - dw.min.y;
                uint64_t numPixels = width * height;
                uint64_t offsetToOrigin =
                    width * static_cast<uint64_t> (dw.min.y) +
                    static_cast<uint64_t> (dw.min.x);
                int channelNumber = 0;
                pixelSize         = 0;

                pixelData[levelIndex].resize (numChans);

                for (ChannelList::ConstIterator i =
                         out.header ().channels ().begin ();
                     i != out.header ().channels ().end ();
                     ++i)
                {
                    int samplesize = pixelTypeSize (i.channel ().type);
                    pixelData[levelIndex][channelNumber].resize (
                        numPixels * samplesize);

                    frameBuffer[levelIndex].insert (
                        i.name (),
                        Slice (
                            i.channel ().type,
                            pixelData[levelIndex][channelNumber].data () -
                                offsetToOrigin * samplesize,
                            samplesize,
                            samplesize * width));
                    ++channelNumber;
                    pixelSize += samplesize;
                }
                totalPixels += numPixels;
                ++levelIndex;
            }
        }
    }

    steady_clock::time_point startRead = steady_clock::now();
    levelIndex        = 0;

    for (int xLevel = 0; xLevel < in.numXLevels (); ++xLevel)
    {
        for (int yLevel = 0; yLevel < in.numYLevels (); ++yLevel)
        {
            if (tiling.mode == RIPMAP_LEVELS || xLevel == yLevel)
            {
                in.setFrameBuffer (frameBuffer[levelIndex]);
                in.readTiles (
                    0,
                    in.numXTiles (xLevel) - 1,
                    0,
                    in.numYTiles (yLevel) - 1,
                    xLevel,
                    yLevel);
                ++levelIndex;
            }
        }
    }

    steady_clock::time_point endRead = steady_clock::now();

    steady_clock::time_point startWrite = steady_clock::now();
    levelIndex         = 0;
    int tileCount      = 0;

    for (int xLevel = 0; xLevel < in.numXLevels (); ++xLevel)
    {
        for (int yLevel = 0; yLevel < in.numYLevels (); ++yLevel)
        {
            if (tiling.mode == RIPMAP_LEVELS || xLevel == yLevel)
            {
                out.setFrameBuffer (frameBuffer[levelIndex]);
                out.writeTiles (
                    0,
                    in.numXTiles (xLevel) - 1,
                    0,
                    in.numYTiles (yLevel) - 1,
                    xLevel,
                    yLevel);
                tileCount += in.numXTiles (xLevel) * in.numYTiles (yLevel);
                ++levelIndex;
            }
        }
    }
    steady_clock::time_point endWrite = steady_clock::now();

    cout << "   \"read time\": " << timing (startRead, endRead) << ",\n";
    cout << "   \"write time\": " << timing (startWrite, endWrite) << ",\n";
    cout << "   \"total tiles\": " << tileCount << ",\n";
    cout << "   \"pixel count\": " << totalPixels << ",\n";
    cout << "   \"raw size\": " << totalPixels * pixelSize << ",\n";
}

void
copyDeepScanLine (DeepScanLineInputPart& in, DeepScanLineOutputPart& out)
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

    steady_clock::time_point startCountRead = steady_clock::now();
    in.readPixelSampleCounts (dw.min.y, dw.max.y);
    steady_clock::time_point endCountRead = steady_clock::now();

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

    steady_clock::time_point startSampleRead = steady_clock::now();
    in.readPixels (dw.min.y, dw.max.y);
    steady_clock::time_point endSampleRead = steady_clock::now();


    steady_clock::time_point startWrite = steady_clock::now();
    out.writePixels (height);
    steady_clock::time_point endWrite = steady_clock::now();


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
copyDeepTiled (DeepTiledInputPart& in, DeepTiledOutputPart& out)
{

    TileDescription tiling = in.header ().tileDescription ();

    if (tiling.mode == MIPMAP_LEVELS)
    {
        throw runtime_error (
            "exrmetrics does not support mipmapped deep tiled parts");
    }

    if (tiling.mode == RIPMAP_LEVELS)
    {
        throw runtime_error (
            "exrmetrics does not support ripmapped deep tiled parts");
    }

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

    steady_clock::time_point startCountRead = steady_clock::now();

    in.readPixelSampleCounts (
        0, in.numXTiles (0) - 1, 0, in.numYTiles (0) - 1, 0, 0);
    steady_clock::time_point endCountRead = steady_clock::now();


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

    steady_clock::time_point startSampleRead = steady_clock::now();
    in.readTiles (0, in.numXTiles (0) - 1, 0, in.numYTiles (0) - 1, 0, 0);
    steady_clock::time_point endSampleRead = steady_clock::now();

    steady_clock::time_point startWrite = steady_clock::now();
    out.writeTiles (0, in.numXTiles (0) - 1, 0, in.numYTiles (0) - 1, 0, 0);
    steady_clock::time_point endWrite = steady_clock::now();


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
    int              halfMode)
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
            copyTiled (inpart, outpart);
        }
        else if (type == SCANLINEIMAGE)
        {
            InputPart  inpart (in, part);
            OutputPart outpart (out, 0);
            copyScanLine (inpart, outpart);
        }
        else if (type == DEEPSCANLINE)
        {
            DeepScanLineInputPart  inpart (in, part);
            DeepScanLineOutputPart outpart (out, 0);
            copyDeepScanLine (inpart, outpart);
        }
        else if (type == DEEPTILE)
        {
            DeepTiledInputPart  inpart (in, part);
            DeepTiledOutputPart outpart (out, 0);
            copyDeepTiled (inpart, outpart);
        }
        else
        {
            throw runtime_error (
                (inFileName + string (" contains unknown part type ") + type)
                    .c_str ());
        }
    }
    struct stat instats, outstats;
    stat (inFileName, &instats);
    stat (outFileName, &outstats);
    cout << "   \"input file size\": " << instats.st_size << ",\n";
    cout << "   \"output file size\": " << outstats.st_size << "\n";
    cout << "}\n";
}
