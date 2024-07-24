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
    const Header& hdr, size_t tileLineSize, size_t numTileLines)
    : Compressor (hdr)
    , m_tileLineSize (tileLineSize)
    , m_numTileLines (numTileLines)
    , _outBuffer ()
{
    // we only need this for deep images to compute the sample count table size.
    m_isTiled = hdr.hasTileDescription ();
    if (m_isTiled)
    {
        auto td = hdr.tileDescription ();
        m_tileArea = td.xSize * td.ySize;
    }
}

int
ZstdCompressor::numScanLines () const
{
    // Needs to be in sync with ImfCompressor::numLinesInBuffer
    return (int) exr_get_zstd_lines_per_chunk ();
}

int
ZstdCompressor::compress (
    const char* inPtr, int inSize, int minY, const char*& outPtr)
{
    if (inSize == 0)
    {
        outPtr = nullptr;
        return 0;
    }

    auto             totalChannelSize = 0;
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
        // We received fewer data than expected. It probably is because we are
        // processing the sampleCounts for DeepExr
        sizePerChannel = {sampleCountTableSize ()};
    }

    outPtr = (char*) malloc (inSize);
    {
        std::lock_guard<std::mutex> lock (g_mutex);
        _outBuffer.push_back (data_ptr ((char*) outPtr, &free));
    }

    const auto fullSize = exr_compress_zstd (
        const_cast<char*> (inPtr),
        inSize,
        numSamples,
        sizePerChannel.data (),
        sizePerChannel.size (),
        (void*) outPtr,
        inSize);
    return fullSize;
}

int
ZstdCompressor::uncompress (
    const char* inPtr, int inSize, int minY, const char*& outPtr)
{
    if (inSize == 0)
    {
        outPtr = nullptr;
        return 0;
    }

    auto  read  = (const char*) inPtr;
    void* write = nullptr;
    auto  ret   = exr_uncompress_zstd (read, inSize, &write, 0);
    {
        std::lock_guard<std::mutex> lock (g_mutex);
        _outBuffer.push_back (data_ptr ((char*) write, &free));
    }
    outPtr = (const char*) write;
    return ret;
}

int
ZstdCompressor::sampleCountTableSize ()
{
    if (m_isTiled) { return m_tileArea * sizeof (int); }
    const Header& hdr = header ();
    return ((hdr.dataWindow ().max.x + 1) - hdr.dataWindow ().min.x) *
           sizeof (int);
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT