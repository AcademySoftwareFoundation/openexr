//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// unset NDEBUG to make assert() operational in release mode.
#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "ImfCompression.h"
#include "ImfCompressor.h"
#include "ImfHeader.h"
#include "openexr_compression.h"

#include <cassert>
#include <iostream>
#include <memory>

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
        string codecList = "none/rle/zips/zip/piz/pxr24/b44/b44a/dwaa/dwab/htj2k256/htj2k32";

        int numMethods = static_cast<int> (NUM_COMPRESSION_METHODS);
        // update this if you add a new compressor.
        assert (numMethods == 12);

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

            // update this if you add a new lossy compressor.
            switch (c)
            {
                case NO_COMPRESSION:
                case RLE_COMPRESSION:
                case ZIPS_COMPRESSION:
                case ZIP_COMPRESSION:
                case PIZ_COMPRESSION:
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
                    assert (isValidDeepCompression (c) == true);
                    break;

                default: assert (isValidDeepCompression (c) == false); break;
            }
        }

        string codecs;
        getCompressionNamesString ("/", codecs);
        assert (codecs == codecList);

        cout << "Testing Compressor instantiation" << endl;

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
            {HTJ2K256_COMPRESSION, EXR_COMPRESSION_LAST_TYPE, 256, true},
            {HTJ2K32_COMPRESSION,  EXR_COMPRESSION_LAST_TYPE,  32,  true},
        };

        const size_t maxScanLineSize = 1024;

        for (const auto& tc : testCases)
        {
            Header hdr;
            hdr.compression () = tc.compression;

            std::unique_ptr<Compressor> comp (
                newCompressor (tc.compression, maxScanLineSize, hdr));

            if (tc.expectsCompressor)
            {
                assert (comp != nullptr);
                assert (comp->numScanLines () == tc.expectedScanlines);
                assert (comp->compressionType () == tc.exrCompression);
                int coreScanlines = exr_compression_lines_per_chunk (exr_compression_t(tc.compression));
                assert (coreScanlines == tc.expectedScanlines);
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
