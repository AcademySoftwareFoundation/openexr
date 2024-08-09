//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <cstring>
#include <mutex>
#include "openexr_compression.h"
#include "ImfZstdCompressor.h"

#include "IlmThreadPool.h"
#include "ImfChannelList.h"
#include "ImfMisc.h"

namespace
{
std::mutex g_mutex;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

ZstdCompressor::ZstdCompressor (
    const Header& hdr, size_t maxBytesPerLine, size_t numTileLines)
    : Compressor (hdr)
    , m_maxBytesPerLine (maxBytesPerLine)
    , m_numTileLines (numTileLines)
    , _outBuffer ()
    , m_outBuffer (0)
{
    int lineCount = m_numTileLines > 0 ? m_numTileLines : numScanLines ();
    m_outBuffer   = new char[m_maxBytesPerLine * lineCount];
}

ZstdCompressor::~ZstdCompressor ()
{
    delete[] m_outBuffer;
}

int
ZstdCompressor::numScanLines () const
{
    // Needs to be in sync with ImfCompressor::numLinesInBuffer
    return (int) exr_get_zstd_lines_per_chunk ();
}

int
ZstdCompressor::compress (
    const char*  inPtr,
    int          inSize,
    const int*   inSampleCountPerLine,
    int          minY,
    const char*& outPtr)
{
#if (1)
    int lineCount = m_numTileLines > 0 ? m_numTileLines : numScanLines ();
    exr_attr_box2i_t range = {
        {header ().dataWindow ().min.x, minY},
        {header ().dataWindow ().max.x, minY + lineCount - 1}};

    auto             channels = header ().channels ();
    std::vector<int> bytesPerChannel;
    for (auto ch = channels.begin (); ch != channels.end (); ++ch)
    {
        bytesPerChannel.push_back (pixelTypeSize (ch.channel ().type));
    }

    outPtr      = m_outBuffer;

    const int compressedSize = exr_compress_zstd_v2 (
        const_cast<char*> (inPtr),
        inSize,
        range,
        bytesPerChannel.size (),
        bytesPerChannel.data (),
        inSampleCountPerLine,
        (void*) outPtr);

#else

    // extract per-channel data size and total data size.
    // NOTE: This is NOT OK for deep imnages.
    int              lineCount        = m_isTiled ? m_numTileLines : 1;
    int              totalChannelSize = 0;
    std::vector<int> sizePerChannel;
    auto             hdr      = header ();
    auto             channels = hdr.channels ();
    for (auto ch = channels.begin (); ch != channels.end (); ++ch)
    {
        const auto size = pixelTypeSize (ch.channel ().type);
        sizePerChannel.push_back (size);
        totalChannelSize += size;
    }

    const auto numSamples = inSize / totalChannelSize;

    if (numSamples * totalChannelSize != inSize)
    {
        // std::cout << "Tile: BAD" << std::endl;
        // We received fewer data than expected. It probably is because we are processing
        // the sampleCounts for DeepExr
        sizePerChannel = {4}; // we compress it as an int
    }

    outPtr = (char*) malloc (inSize);
    {
        std::lock_guard<std::mutex> lock (g_mutex);
        _outBuffer.push_back (data_ptr ((char*) outPtr, &free));
    }

    const auto compressedSize = exr_compress_zstd (
        const_cast<char*> (inPtr),
        inSize,
        numSamples,
        sizePerChannel.data (),
        sizePerChannel.size (),
        (void*) outPtr,
        inSize);
#endif

    return compressedSize;
}

int
ZstdCompressor::uncompress (
    const char*  inPtr,
    int          inSize,
    const int*   inSampleCountPerLine,
    int          minY,
    const char*& outPtr)
{
    if (inSize == 0)
    {
        outPtr = nullptr;
        return 0;
    }

    outPtr = m_outBuffer;
    int  lineCount = m_numTileLines > 0 ? m_numTileLines : numScanLines ();
    auto channels  = header ().channels ();
    std::vector<int> bytesPerChannel;
    for (auto ch = channels.begin (); ch != channels.end (); ++ch)
    {
        bytesPerChannel.push_back (pixelTypeSize (ch.channel ().type));
    }

    auto decompressedSize = exr_uncompress_zstd_v2 (
        inPtr,
        inSize,
        bytesPerChannel.size (),
        bytesPerChannel.data (),
        lineCount,
        inSampleCountPerLine,
        (char*) outPtr);

    return decompressedSize;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT