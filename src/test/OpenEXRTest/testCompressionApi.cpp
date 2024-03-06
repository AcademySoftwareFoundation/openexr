//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// unset NDEBUG to make assert() operational in release mode.
#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "ImfCompression.h"

#include <cassert>
#include <iostream>

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
        string codecList = "none/rle/zips/zip/piz/pxr24/b44/b44a/dwaa/dwab";

        int numMethods = static_cast<int> (NUM_COMPRESSION_METHODS);
        // update this if you add a new compressor.
        assert (numMethods == 10);

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
    }
    catch (const exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
