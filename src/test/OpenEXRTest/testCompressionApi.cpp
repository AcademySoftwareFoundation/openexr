//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// unset NDEBUG to make assert() operational in release mode.
#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "ImfChannelList.h"
#include "ImfCompression.h"
#include "ImfCompressor.h"
#include "ImfHeader.h"
#include "ImfTileDescriptionAttribute.h"
#include "openexr_compression.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;

// To run this test only:
// > cd build
// > ctest -R testCompressionApi

void
testCompressionApi (const string& tempDir)
{
    try
    {
        cout << "Testing compression API functions." << endl;

        // update this if you add a new compressor.
        string codecList = "none/rle/zips/zip/piz/pxr24/b44/b44a/dwaa/dwab/htj2k256/htj2k32/lj2k/zstd";

        int numMethods = static_cast<int> (NUM_COMPRESSION_METHODS);
        // update this if you add a new compressor.
        assert (numMethods == 14);

        for (int i = 0; i < numMethods; i++)
        {
            assert (isValidCompression (i) == true);

            Compression c = static_cast<Compression> (i);
            Compression id;
            string      name, desc;

            getCompressionNameFromId (c, name);
            assert (codecList.find (name) != string::npos);

            getCompressionIdFromName (name, id);
            assert (id >= NO_COMPRESSION && id < NUM_COMPRESSION_METHODS);
            assert (id == c);

            getCompressionDescriptionFromId (c, desc);
            assert (!desc.empty ());

            assert (isValidCompression (id) == true);

            assert (getCompressionNumScanlines (c) > 0);

            // update this if you add a new compressor.
            switch (c)
            {
                case NO_COMPRESSION:
                case RLE_COMPRESSION:
                case ZIPS_COMPRESSION:
                case ZIP_COMPRESSION:
                case PIZ_COMPRESSION:
                case ZSTD_COMPRESSION:
                case HTJ2K256_COMPRESSION:
                case HTJ2K32_COMPRESSION:
                    assert (isLossyCompression (c) == false);
                    break;

                default: assert (isLossyCompression (c) == true); break;
            }

            // update this if you add a new deep compressor.
            switch (c)
            {
                case NO_COMPRESSION:
                case RLE_COMPRESSION:
                case ZIPS_COMPRESSION:
                case ZSTD_COMPRESSION:
                    assert (isValidDeepCompression (c) == true);
                    break;

                default: assert (isValidDeepCompression (c) == false); break;
            }
        }

        string codecs;
        getCompressionNamesString ("/", codecs);
        assert (codecs == codecList);

        cout << "Testing Compressor instantiation" << endl;

        const int zstdLinesPerChunk = getCompressionNumScanlines (ZSTD_COMPRESSION);
        assert (zstdLinesPerChunk == exr_compression_lines_per_chunk (EXR_COMPRESSION_ZSTD));

        struct CompressorTestCase
        {
            Compression         compression;
            exr_compression_t   exrCompression;
            int                 expectedScanlines;
            bool                expectsCompressor;
        };

        CompressorTestCase testCases[] = {
            {NO_COMPRESSION,     EXR_COMPRESSION_NONE,    1,   false},
            {RLE_COMPRESSION,    EXR_COMPRESSION_RLE,     1,   true},
            {ZIPS_COMPRESSION,   EXR_COMPRESSION_ZIPS,    1,   true},
            {ZIP_COMPRESSION,    EXR_COMPRESSION_ZIP,     16,  true},
            {PIZ_COMPRESSION,    EXR_COMPRESSION_PIZ,     32,  true},
            {PXR24_COMPRESSION,  EXR_COMPRESSION_PXR24,   16,  true},
            {B44_COMPRESSION,    EXR_COMPRESSION_B44,     32,  true},
            {B44A_COMPRESSION,   EXR_COMPRESSION_B44A,    32,  true},
            {DWAA_COMPRESSION,   EXR_COMPRESSION_LAST_TYPE,    32,  true},
            {DWAB_COMPRESSION,   EXR_COMPRESSION_LAST_TYPE,   256, true},
            {HTJ2K256_COMPRESSION,  EXR_COMPRESSION_LAST_TYPE, 256, true},
            {HTJ2K32_COMPRESSION,   EXR_COMPRESSION_LAST_TYPE,  32, true},
            {LJ2K_COMPRESSION, EXR_COMPRESSION_LAST_TYPE, 256, true},
            {ZSTD_COMPRESSION,   EXR_COMPRESSION_ZSTD,    zstdLinesPerChunk, true},
        };

        const int    width           = 32;
        const int    height          = 32;
        const size_t maxScanLineSize = width * 3 * sizeof (uint16_t);

        for (const auto& tc : testCases)
        {
            Header hdr (width, height);
            hdr.compression () = tc.compression;
            hdr.channels ().insert ("R", Channel (HALF));
            hdr.channels ().insert ("G", Channel (HALF));
            hdr.channels ().insert ("B", Channel (HALF));

            std::unique_ptr<Compressor> comp (
                newCompressor (tc.compression, maxScanLineSize, hdr));

            if (tc.expectsCompressor)
            {
                assert (comp != nullptr);
                assert (comp->numScanLines () == tc.expectedScanlines);
                assert (comp->compressionType () == tc.exrCompression);
                int coreScanlines = exr_compression_lines_per_chunk (exr_compression_t(tc.compression));
                assert (coreScanlines == tc.expectedScanlines);

                const int chunkLines = std::min (height, comp->numScanLines ());
                const int chunkBytes =
                    static_cast<int> (chunkLines * maxScanLineSize);
                std::vector<char> inBuf (chunkBytes, 0);

                const char* compressedPtr  = nullptr;
                int         compressedSize = comp->compress (
                    inBuf.data (), chunkBytes, 0, compressedPtr);
                assert (compressedSize > 0);
                assert (compressedPtr != nullptr);

                std::vector<char> compressedCopy (
                    compressedPtr, compressedPtr + compressedSize);
                const char* uncompressedPtr  = nullptr;
                int         uncompressedSize = comp->uncompress (
                    compressedCopy.data (), compressedSize, 0, uncompressedPtr);
                assert (uncompressedSize == chunkBytes);
                assert (uncompressedPtr != nullptr);

                // Also verify the raw-chunk fallback path when packed_size == unpacked_size.
                uncompressedPtr  = nullptr;
                uncompressedSize = comp->uncompress (
                    inBuf.data (), chunkBytes, 0, uncompressedPtr);
                assert (uncompressedSize == chunkBytes);
                assert (uncompressedPtr != nullptr);

                if (tc.compression == HTJ2K256_COMPRESSION ||
                    tc.compression == HTJ2K32_COMPRESSION ||
                    tc.compression == LJ2K_COMPRESSION)
                {
                    comp->setStorageType (EXR_STORAGE_DEEP_SCANLINE);
                    comp->setExpectedSize (static_cast<size_t> (chunkBytes + 2));
                    bool threw = false;
                    try
                    {
                        comp->uncompress (
                            compressedCopy.data (),
                            compressedSize,
                            0,
                            uncompressedPtr);
                    }
                    catch (const std::exception&)
                    {
                        threw = true;
                    }
                    assert (threw);
                }

                Header tileHdr (width, height);
                tileHdr.compression () = tc.compression;
                tileHdr.channels ().insert ("R", Channel (HALF));
                tileHdr.channels ().insert ("G", Channel (HALF));
                tileHdr.channels ().insert ("B", Channel (HALF));
                tileHdr.setTileDescription (
                    TileDescription (width, height, ONE_LEVEL));

                std::unique_ptr<Compressor> tileComp (newTileCompressor (
                    tc.compression, maxScanLineSize, height, tileHdr));
                assert (tileComp != nullptr);

                const int tileBytes =
                    static_cast<int> (height * maxScanLineSize);
                std::vector<char>      tileInBuf (tileBytes, 0);
                IMATH_NAMESPACE::Box2i tileRange (
                    IMATH_NAMESPACE::V2i (0, 0),
                    IMATH_NAMESPACE::V2i (width - 1, height - 1));

                compressedPtr  = nullptr;
                compressedSize = tileComp->compressTile (
                    tileInBuf.data (), tileBytes, tileRange, compressedPtr);
                assert (compressedSize > 0);
                assert (compressedPtr != nullptr);

                compressedCopy.assign (
                    compressedPtr, compressedPtr + compressedSize);
                uncompressedPtr  = nullptr;
                uncompressedSize = tileComp->uncompressTile (
                    compressedCopy.data (),
                    compressedSize,
                    tileRange,
                    uncompressedPtr);
                assert (uncompressedSize == tileBytes);
                assert (uncompressedPtr != nullptr);
            }
            else
            {
                assert (comp == nullptr);
            }
        }

        cout << "ok" << endl;
    }
    catch (const exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
