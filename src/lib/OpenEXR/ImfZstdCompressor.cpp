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

ZstdCompressor::ZstdCompressor (const Header& hdr)
    : Compressor (hdr), _outBuffer ()
{}

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
    outPtr = (char*) malloc (inSize);
    {
        std::lock_guard<std::mutex> lock (g_mutex);
        _outBuffer.push_back (raw_ptr ((char*) outPtr, &free));
    }
    auto fullSize =
        exr_compress_zstd ((char*) (inPtr), inSize, (void*) outPtr, inSize);
    return fullSize;
}

int
ZstdCompressor::uncompress (
    const char* inPtr, int inSize, int minY, const char*& outPtr)
{
    auto  read  = (const char*) inPtr;
    void* write = nullptr;
    auto  ret   = exr_uncompress_zstd (read, inSize, &write, 0);
    {
        std::lock_guard<std::mutex> lock (g_mutex);
        _outBuffer.push_back (raw_ptr ((char*) write, &free));
    }
    outPtr = (const char*) write;
    return ret;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT