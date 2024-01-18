//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) DreamWorks Animation LLC and Contributors of the OpenEXR Project
//

// Mandatory section to register this compression method in OpenEXR
// ----------------------------------------------------------------
// DWAA_COMPRESSION = 8
// DWAA_COMPRESSION name dwaa
// DWAA_COMPRESSION desc lossy DCT based compression, in blocks of 32 scanlines. More efficient for partial buffer access.
// DWAA_COMPRESSION scanlines 32
// DWAA_COMPRESSION lossy true
// DWAA_COMPRESSION deep false
// DWAA_COMPRESSION newscan DwaCompressor (hdr, static_cast<int> (maxScanLineSize), 32, DwaCompressor::STATIC_HUFFMAN)
// DWAA_COMPRESSION newtile DwaCompressor (hdr, static_cast<int> (tileLineSize), static_cast<int> (numTileLines), DwaCompressor::DEFLATE);
// ----------------------------------------------------------------
// DWAB_COMPRESSION = 9
// DWAB_COMPRESSION name dwab
// DWAB_COMPRESSION desc lossy DCT based compression, in blocks of 256 scanlines. More efficient space wise and faster to decode full frames than DWAA_COMPRESSION.
// DWAB_COMPRESSION scanlines 256
// DWAB_COMPRESSION lossy true
// DWAB_COMPRESSION deep false
// DWAB_COMPRESSION newscan DwaCompressor (hdr, static_cast<int> (maxScanLineSize), 256, DwaCompressor::STATIC_HUFFMAN)
// DWAB_COMPRESSION newtile DwaCompressor (hdr, static_cast<int> (tileLineSize), static_cast<int> (numTileLines), DwaCompressor::STATIC_HUFFMAN)

#ifndef INCLUDED_IMF_DWA_COMRESSOR_H
#define INCLUDED_IMF_DWA_COMRESSOR_H

//------------------------------------------------------------------------------
//
// class DwaCompressor -- Store lossy RGB data by quantizing DCT components.
//
//------------------------------------------------------------------------------

#include "ImfCompressor.h"

#include "ImfChannelList.h"
#include "ImfZip.h"

#include <half.h>

#include <cstdint>
#include <vector>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class DwaCompressor : public Compressor
{
public:
    enum AcCompression
    {
        STATIC_HUFFMAN,
        DEFLATE,
    };

    DwaCompressor (
        const Header& hdr,
        int           maxScanLineSize,
        int           numScanLines, // ideally is a multiple of 8
        AcCompression acCompression);

    virtual ~DwaCompressor ();

    DwaCompressor (const DwaCompressor& other) = delete;
    DwaCompressor& operator= (const DwaCompressor& other) = delete;
    DwaCompressor (DwaCompressor&& other)                 = delete;
    DwaCompressor& operator= (DwaCompressor&& other) = delete;

    virtual int numScanLines () const;

    virtual OPENEXR_IMF_NAMESPACE::Compressor::Format format () const;

    virtual int
    compress (const char* inPtr, int inSize, int minY, const char*& outPtr);

    virtual int compressTile (
        const char*            inPtr,
        int                    inSize,
        IMATH_NAMESPACE::Box2i range,
        const char*&           outPtr);

    virtual int
    uncompress (const char* inPtr, int inSize, int minY, const char*& outPtr);

    virtual int uncompressTile (
        const char*            inPtr,
        int                    inSize,
        IMATH_NAMESPACE::Box2i range,
        const char*&           outPtr);

    static void initializeFuncs ();

private:
    struct ChannelData;
    struct CscChannelSet;
    struct Classifier;

    class LossyDctDecoderBase;
    class LossyDctDecoder;
    class LossyDctDecoderCsc;

    class LossyDctEncoderBase;
    class LossyDctEncoder;
    class LossyDctEncoderCsc;

    enum CompressorScheme
    {
        UNKNOWN = 0,
        LOSSY_DCT,
        RLE,

        NUM_COMPRESSOR_SCHEMES
    };

    //
    // Per-chunk compressed data sizes, one value per chunk
    //

    enum DataSizesSingle
    {
        VERSION = 0, // Version number:
                     //   0: classic
                     //   1: adds "end of block" to the AC RLE

        UNKNOWN_UNCOMPRESSED_SIZE, // Size of leftover data, uncompressed.
        UNKNOWN_COMPRESSED_SIZE,   // Size of leftover data, zlib compressed.

        AC_COMPRESSED_SIZE,    // AC RLE + Huffman size
        DC_COMPRESSED_SIZE,    // DC + Deflate size
        RLE_COMPRESSED_SIZE,   // RLE + Deflate data size
        RLE_UNCOMPRESSED_SIZE, // RLE'd data size
        RLE_RAW_SIZE,          // Un-RLE'd data size

        AC_UNCOMPRESSED_COUNT, // AC RLE number of elements
        DC_UNCOMPRESSED_COUNT, // DC number of elements

        AC_COMPRESSION, // AC compression strategy
        NUM_SIZES_SINGLE
    };

    AcCompression _acCompression;

    int _maxScanLineSize;
    int _numScanLines;
    int _min[2], _max[2];

    ChannelList                _channels;
    std::vector<ChannelData>   _channelData;
    std::vector<CscChannelSet> _cscSets;
    std::vector<Classifier>    _channelRules;

    char*    _packedAcBuffer;
    uint64_t _packedAcBufferSize;
    char*    _packedDcBuffer;
    uint64_t _packedDcBufferSize;
    char*    _rleBuffer;
    uint64_t _rleBufferSize;
    char*    _outBuffer;
    uint64_t _outBufferSize;
    char*    _planarUncBuffer[NUM_COMPRESSOR_SCHEMES];
    uint64_t _planarUncBufferSize[NUM_COMPRESSOR_SCHEMES];

    Zip*  _zip;
    int   _zipLevel;
    float _dwaCompressionLevel;

    int compress (
        const char*            inPtr,
        int                    inSize,
        IMATH_NAMESPACE::Box2i range,
        const char*&           outPtr);

    int uncompress (
        const char*            inPtr,
        int                    inSize,
        IMATH_NAMESPACE::Box2i range,
        const char*&           outPtr);

    void initializeBuffers (size_t&);
    void initializeDefaultChannelRules ();
    void initializeLegacyChannelRules ();

    void relevantChannelRules (std::vector<Classifier>&) const;

    //
    // Populate our cached version of the channel data with
    // data from the real channel list. We want to
    // copy over attributes, determine compression schemes
    // relevant for the channel type, and find sets of
    // channels to be compressed from Y'CbCr data instead
    // of R'G'B'.
    //

    void classifyChannels (
        ChannelList                 channels,
        std::vector<ChannelData>&   chanData,
        std::vector<CscChannelSet>& cscData);

    //
    // Compute various buffer pointers for each channel
    //

    void setupChannelData (int minX, int minY, int maxX, int maxY);
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
