
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

using namespace OPENEXR_IMF_NAMESPACE;
using IMATH_NAMESPACE::Box2i;

using std::cerr;
using namespace std::chrono;
using std::cout;
using std::endl;
using std::list;
using std::min;
using std::runtime_error;
using std::string;
using std::to_string;
using std::vector;
using std::chrono::steady_clock;
using std::isinf;

double
timing (steady_clock::time_point start, steady_clock::time_point end)
{
    return std::chrono::duration<double> (end - start).count ();
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

// allocate pixelData and FrameBuffer to read scanline data from Input
partSizeData
initScanLine (
    vector<vector<char>>& pixelData,
    FrameBuffer&          buf,
    InputPart&            in,
    const Header&         outHeader)
{
    Box2i    dw        = in.header ().dataWindow ();
    uint64_t width     = dw.max.x + 1 - dw.min.x;
    uint64_t height    = dw.max.y + 1 - dw.min.y;
    uint64_t numPixels = width * height;
    int      numChans  = channelCount (in.header ());

    pixelData.resize (numChans);
    uint64_t offsetToOrigin = width * static_cast<uint64_t> (dw.min.y) +
                              static_cast<uint64_t> (dw.min.x);

    int    channelNumber = 0;
    size_t rawSize       = 0;

    for (ChannelList::ConstIterator i = outHeader.channels ().begin ();
         i != outHeader.channels ().end ();
         ++i)
    {
        int    samplesize      = pixelTypeSize (i.channel ().type);
        size_t pixelsInChannel = (width / i.channel ().xSampling) *
                                 (height / i.channel ().ySampling);
        rawSize += pixelsInChannel * samplesize;
        pixelData[channelNumber].resize (numPixels * samplesize);

        buf.insert (
            i.name (),
            Slice (
                i.channel ().type,
                pixelData[channelNumber].data () - offsetToOrigin * samplesize,
                samplesize,
                samplesize * width,
                i.channel ().xSampling,
                i.channel ().ySampling));
        ++channelNumber;
    }

    partSizeData data;
    data.rawSize      = rawSize;
    data.pixelCount   = width * height;
    data.partType     = in.header ().type ();
    data.compression  = in.header ().compression ();
    data.channelCount = numChans;
    return data;
}

void
readScanLine (InputPart& in, FrameBuffer& buf, vector<double>& perf)
{
    steady_clock::time_point start = steady_clock::now ();
    Box2i                    dw    = in.header ().dataWindow ();
    in.setFrameBuffer (buf);
    in.readPixels (dw.min.y, dw.max.y);
    steady_clock::time_point end = steady_clock::now ();
    perf.push_back (timing (start, end));
}

void
writeScanLine (OutputPart& out, FrameBuffer& buf, vector<double>* perf)
{
    Box2i dw = out.header ().dataWindow ();
    out.setFrameBuffer (buf);
    steady_clock::time_point start = steady_clock::now ();
    out.writePixels (dw.max.y - dw.min.y + 1);
    if (perf)
    {
        steady_clock::time_point end = steady_clock::now ();
        perf->push_back (timing (start, end));
    }
}

partSizeData
initTiled (
    vector<vector<vector<char>>>& pixelData,
    vector<FrameBuffer>&          buf,
    TiledInputPart&               in,
    const Header&                 outHeader)
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
        case NUM_LEVELMODES:
        default: throw runtime_error ("unknown tile mode");
    }
    pixelData.resize (totalLevels);
    buf.resize (totalLevels);

    int     levelIndex  = 0;
    int64_t pixelSize   = 0;
    size_t  totalPixels = 0;
    int     tileCount   = 0;

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

                pixelData[levelIndex].resize (numChans);
                pixelSize = 0;
                for (ChannelList::ConstIterator i =
                         outHeader.channels ().begin ();
                     i != outHeader.channels ().end ();
                     ++i)
                {
                    int samplesize = pixelTypeSize (i.channel ().type);
                    pixelData[levelIndex][channelNumber].resize (
                        numPixels * samplesize);

                    buf[levelIndex].insert (
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
            tileCount += in.numXTiles (xLevel) * in.numYTiles (yLevel);
        }
    }
    partSizeData data;
    data.rawSize      = pixelSize * totalPixels;
    data.pixelCount   = totalPixels;
    data.tileCount    = tileCount;
    data.isTiled      = true;
    data.partType     = in.header ().type ();
    data.compression  = in.header ().compression ();
    data.channelCount = numChans;
    return data;
}

void
readTiled (TiledInputPart& in, vector<FrameBuffer>& buf, vector<double>& perf)
{
    TileDescription          tiling     = in.header ().tileDescription ();
    int                      levelIndex = 0;
    steady_clock::time_point start      = steady_clock::now ();

    for (int xLevel = 0; xLevel < in.numXLevels (); ++xLevel)
    {
        for (int yLevel = 0; yLevel < in.numYLevels (); ++yLevel)
        {
            if (tiling.mode == RIPMAP_LEVELS || xLevel == yLevel)
            {
                in.setFrameBuffer (buf[levelIndex]);
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

    steady_clock::time_point end = steady_clock::now ();
    perf.push_back (timing (start, end));
}

void
writeTiled (
    TiledOutputPart& out, vector<FrameBuffer>& buf, vector<double>* perf)
{
    int levelIndex = 0;

    TileDescription          tiling = out.header ().tileDescription ();
    steady_clock::time_point start  = steady_clock::now ();

    for (int xLevel = 0; xLevel < out.numXLevels (); ++xLevel)
    {
        for (int yLevel = 0; yLevel < out.numYLevels (); ++yLevel)
        {
            if (tiling.mode == RIPMAP_LEVELS || xLevel == yLevel)
            {
                out.setFrameBuffer (buf[levelIndex]);
                out.writeTiles (
                    0,
                    out.numXTiles (xLevel) - 1,
                    0,
                    out.numYTiles (yLevel) - 1,
                    xLevel,
                    yLevel);
                ++levelIndex;
            }
        }
    }
    if (perf)
    {
        steady_clock::time_point end = steady_clock::now ();
        perf->push_back (timing (start, end));
    }
}

//
// allocate arrays, assign frame buffer, read sample counts and
// main buffer.
//
// Also used to allocate buffers for re-reading.
// In that case, the inputSampleCount stores the per-pixel counts,
// so no need to do actual reading (performance counters not updated)
//
partSizeData
initAndReadDeepScanLine (
    vector<int>&           sampleCount,
    vector<vector<char>>&  sampleData,
    vector<vector<char*>>& pixelPtrs,
    DeepFrameBuffer&       buf,
    DeepScanLineInputPart& in,
    const vector<int>*     inputSampleCount,
    const Header&          outHeader,
    vector<double>&        countPerf,
    vector<double>&        samplePerf)
{
    Box2i    dw        = in.header ().dataWindow ();
    uint64_t width     = dw.max.x + 1 - dw.min.x;
    uint64_t height    = dw.max.y + 1 - dw.min.y;
    uint64_t numPixels = width * height;
    int      numChans  = channelCount (in.header ());
    sampleCount.resize (numPixels);

    uint64_t offsetToOrigin = width * static_cast<uint64_t> (dw.min.y) +
                              static_cast<uint64_t> (dw.min.x);

    pixelPtrs.resize (numChans);

    buf.insertSampleCountSlice (Slice (
        UINT,
        (char*) (sampleCount.data () - offsetToOrigin),
        sizeof (int),
        sizeof (int) * width));
    int channelNumber  = 0;
    int bytesPerSample = 0;
    for (ChannelList::ConstIterator i = outHeader.channels ().begin ();
         i != outHeader.channels ().end ();
         ++i)
    {
        pixelPtrs[channelNumber].resize (numPixels);
        int samplesize = pixelTypeSize (i.channel ().type);
        buf.insert (
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

    const vector<int>& samples = inputSampleCount ? *inputSampleCount
                                                  : sampleCount;
    if (!inputSampleCount)
    {
        in.setFrameBuffer (buf);

        steady_clock::time_point startCountRead = steady_clock::now ();
        in.readPixelSampleCounts (dw.min.y, dw.max.y);
        steady_clock::time_point endCountRead = steady_clock::now ();

        countPerf.push_back (timing (startCountRead, endCountRead));
    }

    size_t totalSamples = 0;

    for (int i: samples)
    {
        totalSamples += i;
    }

    sampleData.resize (numChans);
    channelNumber = 0;
    for (ChannelList::ConstIterator i = in.header ().channels ().begin ();
         i != in.header ().channels ().end ();
         ++i)
    {
        int samplesize = pixelTypeSize (i.channel ().type);
        sampleData[channelNumber].resize (samplesize * totalSamples);
        int offset = 0;
        for (uint64_t p = 0; p < numPixels; ++p)
        {
            pixelPtrs[channelNumber][p] =
                sampleData[channelNumber].data () + offset * samplesize;
            offset += samples[p];
        }

        ++channelNumber;
    }

    if (!inputSampleCount)
    {

        steady_clock::time_point startSampleRead = steady_clock::now ();
        in.readPixels (dw.min.y, dw.max.y);
        steady_clock::time_point endSampleRead = steady_clock::now ();
        samplePerf.push_back (timing (startSampleRead, endSampleRead));
    }

    partSizeData data;
    data.pixelCount = numPixels;

    //raw size includes the sample count table
    data.rawSize     = totalSamples * bytesPerSample + numPixels * sizeof (int);
    data.isDeep      = true;
    data.partType    = in.header ().type ();
    data.compression = in.header ().compression ();
    data.channelCount = numChans;

    return data;
}

void
readDeepScanLine (
    DeepScanLineInputPart& in,
    DeepFrameBuffer&       buf,
    vector<double>&        samplePerf,
    vector<double>&        countPerf)
{

    in.setFrameBuffer (buf);
    Box2i dw = in.header ().dataWindow ();

    steady_clock::time_point startCountRead = steady_clock::now ();
    in.readPixelSampleCounts (dw.min.y, dw.max.y);
    steady_clock::time_point endCountRead = steady_clock::now ();
    countPerf.push_back (timing (startCountRead, endCountRead));

    steady_clock::time_point startSampleRead = steady_clock::now ();
    in.readPixels (dw.min.y, dw.max.y);
    steady_clock::time_point endSampleRead = steady_clock::now ();
    samplePerf.push_back (timing (startSampleRead, endSampleRead));
}

void
writeDeepScanLine (
    DeepScanLineOutputPart& out, DeepFrameBuffer& buf, vector<double>* perf)
{
    out.setFrameBuffer (buf);
    Box2i dw = out.header ().dataWindow ();

    steady_clock::time_point start = steady_clock::now ();
    out.writePixels (dw.max.y - dw.min.y + 1);
    if (perf)
    {
        steady_clock::time_point end = steady_clock::now ();
        perf->push_back (timing (start, end));
    }
}

partSizeData
initAndReadDeepTiled (
    vector<int>&           sampleCount,
    vector<vector<char>>&  sampleData,
    vector<vector<char*>>& pixelPtrs,
    DeepFrameBuffer&       buf,
    DeepTiledInputPart&    in,
    const vector<int>*     inputSampleCount,
    const Header&          outHeader,
    vector<double>&        countPerf,
    vector<double>&        samplePerf)
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

    Box2i    dw        = in.header ().dataWindow ();
    uint64_t width     = dw.max.x + 1 - dw.min.x;
    uint64_t height    = dw.max.y + 1 - dw.min.y;
    uint64_t numPixels = width * height;
    int      numChans  = channelCount (in.header ());

    uint64_t offsetToOrigin = width * static_cast<uint64_t> (dw.min.y) +
                              static_cast<uint64_t> (dw.min.x);

    pixelPtrs.resize (numChans);
    sampleCount.resize (numPixels);

    buf.insertSampleCountSlice (Slice (
        UINT,
        (char*) (sampleCount.data () - offsetToOrigin),
        sizeof (int),
        sizeof (int) * width));
    int channelNumber  = 0;
    int bytesPerSample = 0;

    for (ChannelList::ConstIterator i = outHeader.channels ().begin ();
         i != outHeader.channels ().end ();
         ++i)
    {
        pixelPtrs[channelNumber].resize (numPixels);
        int samplesize = pixelTypeSize (i.channel ().type);
        buf.insert (
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

    const vector<int>& samples = inputSampleCount ? *inputSampleCount
                                                  : sampleCount;

    if (!inputSampleCount)
    {
        in.setFrameBuffer (buf);

        steady_clock::time_point startCountRead = steady_clock::now ();

        in.readPixelSampleCounts (
            0, in.numXTiles (0) - 1, 0, in.numYTiles (0) - 1, 0, 0);
        steady_clock::time_point endCountRead = steady_clock::now ();

        countPerf.push_back (timing (startCountRead, endCountRead));
    }
    size_t totalSamples = 0;

    for (int i: samples)
    {
        totalSamples += i;
    }

    sampleData.resize (numChans);
    channelNumber = 0;

    for (ChannelList::ConstIterator i = in.header ().channels ().begin ();
         i != in.header ().channels ().end ();
         ++i)
    {
        int samplesize = pixelTypeSize (i.channel ().type);
        sampleData[channelNumber].resize (samplesize * totalSamples);
        int offset = 0;
        for (uint64_t p = 0; p < numPixels; ++p)
        {
            pixelPtrs[channelNumber][p] =
                sampleData[channelNumber].data () + offset * samplesize;
            offset += samples[p];
        }

        ++channelNumber;
    }

    if (!inputSampleCount)
    {
        steady_clock::time_point startSampleRead = steady_clock::now ();
        in.readTiles (0, in.numXTiles (0) - 1, 0, in.numYTiles (0) - 1, 0, 0);
        steady_clock::time_point endSampleRead = steady_clock::now ();

        samplePerf.push_back (timing (startSampleRead, endSampleRead));
    }

    partSizeData data;
    data.rawSize    = totalSamples * bytesPerSample + numPixels * sizeof (int);
    data.pixelCount = numPixels;
    data.isDeep     = true;
    data.isTiled    = true;
    data.channelCount = numChans;
    return data;
}

void
readDeepTiled (
    DeepTiledInputPart& in,
    DeepFrameBuffer&    buf,
    vector<double>&     countPerf,
    vector<double>&     samplePerf)
{
    in.setFrameBuffer (buf);

    steady_clock::time_point startCountRead = steady_clock::now ();

    in.readPixelSampleCounts (
        0, in.numXTiles (0) - 1, 0, in.numYTiles (0) - 1, 0, 0);
    steady_clock::time_point endCountRead = steady_clock::now ();

    countPerf.push_back (timing (startCountRead, endCountRead));

    steady_clock::time_point startSampleRead = steady_clock::now ();
    in.readTiles (0, in.numXTiles (0) - 1, 0, in.numYTiles (0) - 1, 0, 0);
    steady_clock::time_point endSampleRead = steady_clock::now ();

    samplePerf.push_back (timing (startSampleRead, endSampleRead));
}

void
writeDeepTiled (
    DeepTiledOutputPart& out, DeepFrameBuffer& buf, vector<double>* perf)
{
    out.setFrameBuffer (buf);
    steady_clock::time_point startWrite = steady_clock::now ();
    out.writeTiles (0, out.numXTiles (0) - 1, 0, out.numYTiles (0) - 1, 0, 0);

    if (perf)
    {
        steady_clock::time_point endWrite = steady_clock::now ();
        perf->push_back (timing (startWrite, endWrite));
    }
}

//
// memory allocation to hold all the pixels for a part, and a buffer that represents it
//
struct partBuffers
{
    vector<vector<char>>
        scanlinePixelData; // pixel data for scanline input images - an array per channel
    vector<vector<vector<char>>>
        tilePixelData; // pixel data for tiled input images - an array per level per channel
    vector<int> deepSampleCount; // per-pixel sample counts
    vector<vector<char>>
        deepSampleData; // actual sample data for deep scanline and deep single level deep tiled data
    vector<vector<char*>>
        deepSamplePtrs; // pointers to deep sample data, a pointer per pixel per channel
    FrameBuffer         scanlineBuf;
    vector<FrameBuffer> tiledBuf; //
    DeepFrameBuffer     deepbuf;
};

struct partData
{
    partBuffers readBuf;
    partBuffers rereadBuf;
};

void
initAndReadFile (
    MultiPartInputFile&   in,
    const vector<Header>& outHeaders,
    int                   part,
    vector<partData>&     parts,
    fileMetrics&          metrics,
    bool                  reread)
{

    for (size_t p = 0; p < parts.size (); ++p)
    {

        int    readPart = (part == -1 ? p : part);
        string type     = in.header (readPart).type ();

        if (type == SCANLINEIMAGE)
        {
            InputPart inpart (in, readPart);
            metrics.stats[p].sizeData = initScanLine (
                parts[p].readBuf.scanlinePixelData,
                parts[p].readBuf.scanlineBuf,
                inpart,
                outHeaders[p]);
            if (reread)
            {
                initScanLine (
                    parts[p].rereadBuf.scanlinePixelData,
                    parts[p].rereadBuf.scanlineBuf,
                    inpart,
                    outHeaders[p]);
            }
            readScanLine (
                inpart,
                parts[p].readBuf.scanlineBuf,
                metrics.stats[p].readPerf);
        }
        else if (type == TILEDIMAGE)
        {
            TiledInputPart inpart (in, readPart);
            metrics.stats[p].sizeData = initTiled (
                parts[p].readBuf.tilePixelData,
                parts[p].readBuf.tiledBuf,
                inpart,
                outHeaders[p]);
            if (reread)
            {
                initTiled (
                    parts[p].rereadBuf.tilePixelData,
                    parts[p].rereadBuf.tiledBuf,
                    inpart,
                    outHeaders[p]);
            }
            readTiled (
                inpart, parts[p].readBuf.tiledBuf, metrics.stats[p].readPerf);
        }
        else if (type == DEEPSCANLINE)
        {
            DeepScanLineInputPart inpart (in, readPart);
            metrics.stats[p].sizeData = initAndReadDeepScanLine (
                parts[p].readBuf.deepSampleCount,
                parts[p].readBuf.deepSampleData,
                parts[p].readBuf.deepSamplePtrs,
                parts[p].readBuf.deepbuf,
                inpart,
                nullptr,
                outHeaders[p],
                metrics.stats[p].countReadPerf,
                metrics.stats[p].readPerf);

            if (reread)
            {
                metrics.stats[p].sizeData = initAndReadDeepScanLine (
                    parts[p].rereadBuf.deepSampleCount,
                    parts[p].rereadBuf.deepSampleData,
                    parts[p].rereadBuf.deepSamplePtrs,
                    parts[p].rereadBuf.deepbuf,
                    inpart,
                    &parts[p].readBuf.deepSampleCount,
                    outHeaders[p],
                    metrics.stats[p].countReadPerf,
                    metrics.stats[p].readPerf);
            }
        }
        else if (type == DEEPTILE)
        {
            DeepTiledInputPart inpart (in, readPart);
            metrics.stats[p].sizeData = initAndReadDeepTiled (
                parts[p].readBuf.deepSampleCount,
                parts[p].readBuf.deepSampleData,
                parts[p].readBuf.deepSamplePtrs,
                parts[p].readBuf.deepbuf,
                inpart,
                nullptr,
                outHeaders[p],
                metrics.stats[p].countReadPerf,
                metrics.stats[p].readPerf);
            if (reread)
            {
                initAndReadDeepTiled (
                    parts[p].rereadBuf.deepSampleCount,
                    parts[p].rereadBuf.deepSampleData,
                    parts[p].rereadBuf.deepSamplePtrs,
                    parts[p].rereadBuf.deepbuf,
                    inpart,
                    &parts[p].readBuf.deepSampleCount,
                    outHeaders[p],
                    metrics.stats[p].countReadPerf,
                    metrics.stats[p].readPerf);
            }
        }
    }
}

void
writeFile (
    MultiPartOutputFile& out,
    vector<partData>&    parts,
    fileMetrics&         metrics,
    bool                 logPerformance)
{
    //
    // Call initialization functions, Read image from source.
    //
    for (size_t p = 0; p < parts.size (); ++p)
    {
        string type = out.header (p).type ();

        if (type == SCANLINEIMAGE)
        {
            OutputPart outpart (out, p);
            writeScanLine (
                outpart,
                parts[p].readBuf.scanlineBuf,
                logPerformance ? &metrics.stats[p].writePerf : nullptr);
        }
        else if (type == TILEDIMAGE)
        {
            TiledOutputPart outpart (out, p);
            writeTiled (
                outpart,
                parts[p].readBuf.tiledBuf,
                logPerformance ? &metrics.stats[p].writePerf : nullptr);
        }
        else if (type == DEEPSCANLINE)
        {
            DeepScanLineOutputPart outpart (out, p);
            writeDeepScanLine (
                outpart,
                parts[p].readBuf.deepbuf,
                logPerformance ? &metrics.stats[p].writePerf : nullptr);
        }
        else if (type == DEEPTILE)
        {
            DeepTiledOutputPart outpart (out, p);
            writeDeepTiled (
                outpart,
                parts[p].readBuf.deepbuf,
                logPerformance ? &metrics.stats[p].writePerf : nullptr);
        }
    }
}

void
rereadFile (
    MultiPartInputFile& in, vector<partData>& parts, fileMetrics& metrics)
{
    //
    // Call initialization functions, Read image from source.
    //
    for (size_t p = 0; p < parts.size (); ++p)
    {
        string type = in.header (p).type ();

        if (type == SCANLINEIMAGE)
        {
            InputPart inpart (in, p);
            readScanLine (
                inpart,
                parts[p].rereadBuf.scanlineBuf,
                metrics.stats[p].rereadPerf);
        }
        else if (type == TILEDIMAGE)
        {
            TiledInputPart inpart (in, p);
            readTiled (
                inpart,
                parts[p].rereadBuf.tiledBuf,
                metrics.stats[p].rereadPerf);
        }
        else if (type == DEEPSCANLINE)
        {
            DeepScanLineInputPart inpart (in, p);
            readDeepScanLine (
                inpart,
                parts[p].rereadBuf.deepbuf,
                metrics.stats[p].rereadPerf,
                metrics.stats[p].countRereadPerf);
        }
        else if (type == DEEPTILE)
        {
            DeepTiledInputPart inpart (in, p);
            readDeepTiled (
                inpart,
                parts[p].rereadBuf.deepbuf,
                metrics.stats[p].rereadPerf,
                metrics.stats[p].countRereadPerf);
        }
    }
}

// stream that doesn't write data, just logs file size
class DummyOStream : public OStream
{
    uint64_t streamptr = 0;

public:
    DummyOStream () : OStream ("<dummy>") {}

    void     write (const char c[], int n) override { streamptr += n; }
    void     seekp (uint64_t pos) override { streamptr = pos; }
    uint64_t tellp () override { return streamptr; }
};

//
// write file to preallocated memory: must be initialized with enough space
// to store file
//
class MemOStream : public OStream
{

    uint64_t streamptr = 0;

public:
    vector<char> data;
    MemOStream (uint64_t size) : OStream ("<memory>"), data (size) {}

    void write (const char c[], int n) override
    {
        if (n + streamptr > data.size ())
        {
            throw runtime_error ("attempt to write beyond preallocated memory");
        }
        memcpy (data.data () + streamptr, c, n);
        streamptr += n;
    }
    void     seekp (uint64_t pos) override { streamptr = pos; }
    uint64_t tellp () override { return streamptr; }
};

class MemIStream : public IStream
{
    uint64_t          streamptr = 0;
    const MemOStream& ostream;

public:
    MemIStream (const MemOStream& ostream)
        : IStream ("<memory>"), ostream (ostream)
    {}

    bool isMemoryMapped () const override { return true; }

    char* readMemoryMapped (int n) override
    {
        if (n + streamptr > ostream.data.size ())
        {
            throw runtime_error ("attempt to read past end of file");
        }
        uint64_t oldStreamptr = streamptr;
        streamptr += n;
        return const_cast<char*> (ostream.data.data () + oldStreamptr);
    }

    bool read (char c[], int n) override
    {
        int bytesToRead =
            min (n, static_cast<int> (ostream.data.size () - streamptr));
        memcpy (c, ostream.data.data () + streamptr, bytesToRead);
        streamptr += n;
        return bytesToRead < n;
    }

    void seekg (uint64_t pos) override { streamptr = pos; }

    uint64_t tellg () override { return streamptr; }
};

// add each entry in input to the corresponding value in output
// if output has fewer entries than input, resize it to be the same size
void
accumulate (vector<double>& output, const vector<double>& input)
{
    if (output.size () < input.size ()) { output.resize (input.size ()); }
    for (size_t x = 0; x < input.size (); ++x)
    {
        output[x] += input[x];
    }
}

string
modeName (PixelMode p)
{
    switch (p)
    {
        case PIXELMODE_ALL_FLOAT: return "float";
        case PIXELMODE_ALL_HALF: return "half";
        case PIXELMODE_MIXED_HALF_FLOAT: return "mixed";
        case PIXELMODE_ORIGINAL: return "original";
    }
    throw runtime_error ("bad pixelmode");
}

fileMetrics
exrmetrics (
    const char*                        inFileName,
    const char*                        outFileName,
    int                                part,
    OPENEXR_IMF_NAMESPACE::Compression compression,
    float                              level,
    int                                passes,
    bool                               write,
    bool                               reread,
    PixelMode                          pixelMode,
    bool                               verbose)
{

    if (verbose)
    {
        cerr << "read " << inFileName;
        cerr << " as " << modeName (pixelMode) << "... ";
        cerr.flush ();
    }

    MultiPartInputFile in (inFileName);
    if (part != -1 && part >= in.parts ())
    {
        throw runtime_error ((string (inFileName) + " only contains " +
                              to_string (in.parts ()) +
                              " parts. Cannot copy part " + to_string (part))
                                 .c_str ());
    }
    fileMetrics metrics;

    //write all parts if part==-1, otherwise write single part specified
    vector<Header> outHeaders (part == -1 ? in.parts () : 1);
    if (part == -1)
    {
        for (int p = 0; p < in.parts (); ++p)
        {
            outHeaders[p] = in.header (p);
        }
    }
    else { outHeaders[0] = in.header (part); }

    bool compressionSet = false;

    for (int p = 0; p < in.parts (); ++p)
    {
        if (compression < NUM_COMPRESSION_METHODS)
        {
            outHeaders[p].compression () = compression;
        }

        if (!isinf (level) && level >= -1)
        {
            switch (outHeaders[p].compression ())
            {
                case DWAA_COMPRESSION:
                case DWAB_COMPRESSION:
                    outHeaders[p].dwaCompressionLevel () = level;
                    compressionSet                       = true;
                    break;
                case ZIP_COMPRESSION:
                case ZIPS_COMPRESSION:
                    outHeaders[p].zipCompressionLevel () = level;
                    compressionSet                       = true;
                    break;
                    //            case ZSTD_COMPRESSION :
                    //                outHeader.zstdCompressionLevel()=level;
                    //                break;
                default: break;
            }
        }

        if (pixelMode != PIXELMODE_ORIGINAL)
        {
            for (ChannelList::Iterator i = outHeaders[p].channels ().begin ();
                 i != outHeaders[p].channels ().end ();
                 ++i)
            {
                // find channel suffix within full channel name (so 'R' in 'layer.R')
                const char* name = i.name ();
                const char* dot  = strrchr (name, 'r');
                if (dot) { name = dot + 1; }

                if (pixelMode == PIXELMODE_ALL_HALF ||
                    (pixelMode == PIXELMODE_MIXED_HALF_FLOAT &&
                     (!strcmp (name, "R") || !strcmp (name, "G") ||
                      !strcmp (name, "B") || !strcmp (name, "A"))))
                {
                    i.channel ().type = HALF;
                }
                else if (
                    pixelMode == PIXELMODE_ALL_FLOAT ||
                    pixelMode == PIXELMODE_MIXED_HALF_FLOAT)
                {
                    i.channel ().type = FLOAT;
                }
            }
        }
    }

    // abort if level was set but no parts had a compression type with a level
    if (!isinf (level) && level >= -1 && !compressionSet)
    {
        throw runtime_error (
            "-l option only works for DWAA/DWAB,ZIP/ZIPS or ZSTD compression");
    }

    vector<partData> parts (part == -1 ? in.parts () : 1);
    metrics.stats.resize (parts.size ());

    initAndReadFile (in, outHeaders, part, parts, metrics, reread);

    if (write)
    {

        //
        // when NOT writing to file, write to preallocated block of data
        // precompute the total datasize, so no memory reallocation is required
        //
        uint64_t fileSize = 0;
        if (!outFileName)
        {

            DummyOStream        tmp;
            MultiPartOutputFile out (
                tmp, outHeaders.data (), outHeaders.size ());
            writeFile (out, parts, metrics, false);
            fileSize = tmp.tellp ();
        }

        //
        // write to output; re-read output back to input
        //

        if (verbose)
        {
            cerr << " write ";
            if (compression != NUM_COMPRESSION_METHODS)
            {
                string name;
                getCompressionNameFromId (compression, name);
                cerr << "compression " << name;
            }
            cerr << "... ";
            cerr.flush ();
        }

        for (int i = 0; i < passes; ++i)
        {
            if (verbose && passes > 1)
            {
                cerr << i << ' ';
                cerr.flush ();
            }
            MemOStream           ostream (fileSize);
            MultiPartOutputFile* out;
            if (outFileName)
            {
                out = new MultiPartOutputFile (
                    outFileName, outHeaders.data (), outHeaders.size ());
            }
            else
            {
                out = new MultiPartOutputFile (
                    ostream, outHeaders.data (), outHeaders.size ());
            }
            writeFile (*out, parts, metrics, true);
            delete out;

            if (reread)
            {
                MemIStream          istream (ostream);
                MultiPartInputFile* in;
                if (outFileName) { in = new MultiPartInputFile (outFileName); }
                else { in = new MultiPartInputFile (istream); }

                rereadFile (*in, parts, metrics);

                delete in;
            }
        }

        struct stat instats, outstats;
        stat (inFileName, &instats);
        metrics.inputFileSize = instats.st_size;
        if (outFileName)
        {
            stat (outFileName, &outstats);
            metrics.outputFileSize = outstats.st_size;
        }
        else { metrics.outputFileSize = fileSize; }
    }

    //
    // sum across all parts
    //

    metrics.totalStats = metrics.stats[0];
    for (size_t i = 1; i < metrics.stats.size (); ++i)
    {
        accumulate (metrics.totalStats.readPerf, metrics.stats[i].readPerf);
        accumulate (
            metrics.totalStats.countReadPerf, metrics.stats[i].countReadPerf);
        accumulate (metrics.totalStats.writePerf, metrics.stats[i].writePerf);
        accumulate (metrics.totalStats.rereadPerf, metrics.stats[i].rereadPerf);
        accumulate (
            metrics.totalStats.countRereadPerf,
            metrics.stats[i].countRereadPerf);

        metrics.totalStats.sizeData.pixelCount +=
            metrics.stats[i].sizeData.pixelCount;
        metrics.totalStats.sizeData.channelCount +=
            metrics.stats[i].sizeData.channelCount;
        metrics.totalStats.sizeData.rawSize +=
            metrics.stats[i].sizeData.rawSize;
        metrics.totalStats.sizeData.tileCount +=
            metrics.stats[i].sizeData.tileCount;

        metrics.totalStats.sizeData.isDeep |= metrics.stats[i].sizeData.isDeep;
        metrics.totalStats.sizeData.isTiled |=
            metrics.stats[i].sizeData.isTiled;

        //check for mixed compression or part types within file

        if (metrics.stats[i].sizeData.compression !=
            metrics.totalStats.sizeData.compression)
        {
            metrics.totalStats.sizeData.compression = NUM_COMPRESSION_METHODS;
        }
        if (metrics.stats[i].sizeData.partType !=
            metrics.totalStats.sizeData.partType)
        {
            metrics.totalStats.sizeData.partType = "";
        }
    }

    if (verbose) { cerr << endl; }
    return metrics;
}
