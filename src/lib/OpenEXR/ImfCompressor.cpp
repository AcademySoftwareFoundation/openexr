//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class Compressor
//
//-----------------------------------------------------------------------------

#include "ImfCheckedArithmetic.h"
#include "ImfNamespace.h"
#include "ImfCompressor.h"
#include "ImfB44Compressor.h"
#include "ImfDwaCompressor.h"
#include "ImfPizCompressor.h"
#include "ImfPxr24Compressor.h"
#include "ImfRleCompressor.h"
#include "ImfZipCompressor.h"
#include "ImfZip.h"

#include <algorithm>
#include <stdexcept>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

using IMATH_NAMESPACE::Box2i;

Compressor::Compressor (
    const Header& hdr,
    exr_compression_t compression_type,
    size_t maxScanLineSize,
    int numScanLines)
    : _ctxt ("<compression>",
             ContextInitializer(),
             Context::temp_mode_t
             {})
    , _header (hdr)
    , _maxScanLineSize (maxScanLineSize)
    , _numScanLines (numScanLines)
    , _comp_type (compression_type)
{
    if (maxScanLineSize > std::numeric_limits<int>::max ())
    {
        throw IEX_NAMESPACE::OverflowExc (
            "ScanLine size too large for RleCompressor");
    }
    _ctxt.setLongNameSupport (true);
    _ctxt.addHeader (0, hdr);

    _store_type = _ctxt.storage (0);

    exr_set_zip_compression_level (_ctxt, 0, hdr.zipCompressionLevel ());
    exr_set_dwa_compression_level (_ctxt, 0, hdr.dwaCompressionLevel ());

    exr_compression_t hdrcomp;
    if (EXR_ERR_SUCCESS != exr_get_compression (_ctxt, 0, &hdrcomp))
        throw IEX_NAMESPACE::ArgExc ("Unable to initialize compression type");

    if (hdrcomp != compression_type &&
        compression_type != EXR_COMPRESSION_LAST_TYPE)
    {
        // no idea why this would fail, but also do need to apply it
        // prior to the chunk initialization to have lines per chunk, etc.
        // be correct
        if (EXR_ERR_SUCCESS != exr_set_compression (_ctxt, 0, compression_type))
            throw IEX_NAMESPACE::ArgExc ("Unable to initialize compression type");
    }
 }

Compressor::~Compressor ()
{
    if (_decoder_init)
        exr_decoding_destroy(_ctxt, &_decoder);
    if (_encoder_init)
        exr_encoding_destroy(_ctxt, &_encoder);
}

Compressor::Format
Compressor::format () const
{
    return XDR;
}

int
Compressor::numScanLines () const
{
    return _numScanLines;
}

int
Compressor::compress (
    const char* inPtr, int inSize, int minY, const char*& outPtr)
{
    IMATH_NAMESPACE::Box2i range = _header.dataWindow ();

    range.min.y = minY;
    range.max.y = minY + _numScanLines - 1;

    return static_cast<int> (
        runEncodeStep (inPtr, inSize, range, outPtr));
}

int
Compressor::uncompress (
    const char* inPtr, int inSize, int minY, const char*& outPtr)
{
    IMATH_NAMESPACE::Box2i range = _header.dataWindow ();

    range.min.y = minY;
    range.max.y = minY + _numScanLines - 1;

    return static_cast<int> (
        runDecodeStep (inPtr, inSize, range, outPtr));
}

int
Compressor::compressTile (
    const char* inPtr, int inSize, Box2i range, const char*& outPtr)
{
    return static_cast<int> (
        runEncodeStep (inPtr, inSize, range, outPtr));
}

int
Compressor::uncompressTile (
    const char* inPtr, int inSize, Box2i range, const char*& outPtr)
{
    return static_cast<int> (
        runDecodeStep (inPtr, inSize, range, outPtr));
}

uint64_t
Compressor::runEncodeStep (
    const char* inPtr,
    int inSize,
    IMATH_NAMESPACE::Box2i range,
    const char*& outPtr)
{
    // special case
    if (inSize == 0)
    {
        outPtr = inPtr;
        return 0;
    }

    exr_chunk_info_t cinfo = {0};

    if (EXR_ERR_SUCCESS != exr_chunk_default_initialize (
            _ctxt, 0, (const exr_attr_box2i_t*) &range, _levelX, _levelY, &cinfo))
        throw IEX_NAMESPACE::ArgExc ("Unable to initialize chunk information");

    cinfo.type = _store_type;
    if (!_encoder_init)
    {
        if (EXR_ERR_SUCCESS != exr_encoding_initialize(_ctxt, 0, &cinfo, &_encoder))
            throw IEX_NAMESPACE::ArgExc ("Unable to initialize encoder type");

        _encoder_init = true;
    }
    else
    {
        if (EXR_ERR_SUCCESS != exr_encoding_update(_ctxt, 0, &cinfo, &_encoder))
            throw IEX_NAMESPACE::ArgExc ("Unable to update encoder type");
    }

    _encoder.packed_buffer = const_cast<char*> (inPtr);
    _encoder.packed_bytes = inSize;

    if (EXR_ERR_SUCCESS != exr_compress_chunk(&_encoder))
        throw IEX_NAMESPACE::ArgExc ("Unable to run compression routine");

    outPtr = (const char*) _encoder.compressed_buffer;

    _encoder.packed_buffer = nullptr;
    _encoder.packed_bytes = 0;

    return _encoder.compressed_bytes;
}

////////////////////////////////////////

uint64_t
Compressor::runDecodeStep (
    const char* inPtr,
    int inSize,
    IMATH_NAMESPACE::Box2i range,
    const char*& outPtr)
{
    // special case
    if (inSize == 0)
    {
        if (!_memory_buffer)
        {
            _buf_sz = _maxScanLineSize * _numScanLines;
            _memory_buffer.reset (new char[_buf_sz]);
        }
        outPtr = _memory_buffer.get ();
        return 0;
    }

    exr_result_t rv;
    exr_chunk_info_t cinfo = {0};
    uint64_t expectedOutputSize;

    if (EXR_ERR_SUCCESS != exr_chunk_default_initialize (
            _ctxt, 0, (const exr_attr_box2i_t*) &range, _levelX, _levelY, &cinfo))
        throw IEX_NAMESPACE::ArgExc ("Unable to initialize chunk information");

    if (_store_type == EXR_STORAGE_DEEP_SCANLINE ||
        _store_type == EXR_STORAGE_DEEP_TILED)
    {
        if (_expectedSize != 0)
        {
            expectedOutputSize = _expectedSize;
            cinfo.unpacked_size = _expectedSize;
        }
        else
        {
            expectedOutputSize = _maxScanLineSize;
            cinfo.unpacked_size = _maxScanLineSize;
        }
    }
    else
    {
        expectedOutputSize = cinfo.unpacked_size;
    }

    cinfo.packed_size = inSize;
    cinfo.type = _store_type;
    if (_buf_sz < expectedOutputSize)
    {
        uint64_t fullbuf = _maxScanLineSize * _numScanLines;
        _buf_sz = std::max (fullbuf, expectedOutputSize);
        _memory_buffer.reset (new char[_buf_sz]);
    }

    if (!_decoder_init)
    {
        if (EXR_ERR_SUCCESS != exr_decoding_initialize(_ctxt, 0, &cinfo, &_decoder))
            throw IEX_NAMESPACE::ArgExc ("Unable to initialize decoder type");

        _decoder_init = true;
    }
    else
    {
        if (EXR_ERR_SUCCESS != exr_decoding_update(_ctxt, 0, &cinfo, &_decoder))
            throw IEX_NAMESPACE::ArgExc ("Unable to update decoder");
    }

//    if (_store_type == EXR_STORAGE_DEEP_SCANLINE ||
//        _store_type == EXR_STORAGE_DEEP_TILED)
//    {
//        _decoder.decode_flags |= EXR_DECODE_ALLOW_SHORT_DECODE;
//    }

    _decoder.packed_buffer = const_cast<char*> (inPtr);
    _decoder.unpacked_buffer = _memory_buffer.get();
    _decoder.unpacked_alloc_size = _buf_sz;

    rv = exr_uncompress_chunk(&_decoder);

    _decoder.packed_buffer   = nullptr;
    _decoder.unpacked_buffer = nullptr;
    _decoder.unpacked_alloc_size = 0;

    outPtr = _memory_buffer.get ();

    if (EXR_ERR_SUCCESS != rv)
        throw IEX_NAMESPACE::ArgExc ("Unable to run compression routine");

    return _decoder.bytes_decompressed;
}


Compressor*
newCompressor (Compression c, size_t maxScanLineSize, const Header& hdr)
{
    Compressor* ret = nullptr;

    // clang-format off
    switch (c)
    {
        case RLE_COMPRESSION:

            ret = new RleCompressor (hdr, maxScanLineSize);
            break;

        case ZIPS_COMPRESSION:

            ret = new ZipCompressor (hdr, maxScanLineSize, 1);
            break;

        case ZIP_COMPRESSION:

            ret = new ZipCompressor (hdr, maxScanLineSize, 16);
            break;

        case PIZ_COMPRESSION:

            ret = new PizCompressor (hdr, maxScanLineSize, 32);
            break;

        case PXR24_COMPRESSION:

            ret = new Pxr24Compressor (hdr, maxScanLineSize, 16);
            break;

        case B44_COMPRESSION:

            ret = new B44Compressor (hdr, maxScanLineSize, 32, false);
            break;

        case B44A_COMPRESSION:

            ret = new B44Compressor (hdr, maxScanLineSize, 32, true);
            break;

        case DWAA_COMPRESSION:

            ret = new DwaCompressor (
                hdr,
                static_cast<int> (maxScanLineSize),
                32,
                DwaCompressor::STATIC_HUFFMAN);
            break;

        case DWAB_COMPRESSION:

            ret = new DwaCompressor (
                hdr,
                static_cast<int> (maxScanLineSize),
                256,
                DwaCompressor::STATIC_HUFFMAN);
            break;

        default: break;
    }
    // clang-format on

    if (ret && ret->storageType () == EXR_STORAGE_LAST_TYPE)
        ret->setStorageType (EXR_STORAGE_SCANLINE);

    return ret;
}

// for a given compression type, return the number of scanlines
// compressed into a single chunk
// TODO add to API and move to ImfCompressor.cpp
int
numLinesInBuffer (Compression comp)
{
    int numScanlines = getCompressionNumScanlines (comp);
    if (exr_compression_lines_per_chunk ((exr_compression_t)comp) != numScanlines)
        throw IEX_NAMESPACE::ArgExc ("Mismatch in compression lines per chunk");
    if (numScanlines < 1)
        throw IEX_NAMESPACE::ArgExc ("Unknown compression type");
    return numScanlines;
}

Compressor*
newTileCompressor (
    Compression c, size_t tileLineSize, size_t numTileLines, const Header& hdr)
{
    Compressor* ret = nullptr;
    // clang-format off
    switch (c)
    {
        case RLE_COMPRESSION:

            ret = new RleCompressor (hdr, uiMult (tileLineSize, numTileLines));
            break;

        case ZIPS_COMPRESSION:
        case ZIP_COMPRESSION:

            ret = new ZipCompressor (hdr, tileLineSize, numTileLines);
            break;

        case PIZ_COMPRESSION:

            ret = new PizCompressor (hdr, tileLineSize, numTileLines);
            break;

        case PXR24_COMPRESSION:

            ret = new Pxr24Compressor (hdr, tileLineSize, numTileLines);
            break;

        case B44_COMPRESSION:

            ret = new B44Compressor (hdr, tileLineSize, numTileLines, false);
            break;

        case B44A_COMPRESSION:

            ret = new B44Compressor (hdr, tileLineSize, numTileLines, true);
            break;

        case DWAA_COMPRESSION:

            ret = new DwaCompressor (
                hdr,
                static_cast<int> (tileLineSize),
                static_cast<int> (numTileLines),
                DwaCompressor::DEFLATE);
            break;

        case DWAB_COMPRESSION:

            ret = new DwaCompressor (
                hdr,
                static_cast<int> (tileLineSize),
                static_cast<int> (numTileLines),
                DwaCompressor::STATIC_HUFFMAN);
            break;

        default: break;
    }
    // clang-format on

    if (ret && ret->storageType () == EXR_STORAGE_LAST_TYPE)
        ret->setStorageType (EXR_STORAGE_TILED);

    return ret;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
