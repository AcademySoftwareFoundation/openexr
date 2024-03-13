//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_COMPRESSION_H
#define INCLUDED_IMF_COMPRESSION_H

//-----------------------------------------------------------------------------
//
//  enum Compression
//
// This file enumerates available compression methods and defines a simple API
// to query them.
//
// ----------------------------------------------------------------------------
/*
    HOW TO ADD NEW COMPRESSION TECHNIQUE
    ====================================

    1. Creating your C++ compression class

        1. Implement a new src/lib/OpenEXR/Imf*Compressor.{h, cpp} class deriving
           from ImfCompressor.

        2. Update the build systems
            * Update src/lib/OpenEXR/CMakeLists.txt to build your codec with cmake.
            * Update BUILD.bazel to build your codec with bazel.
            * NOTE: Both build methods must be implemented !

    2. C++ API integration

        1. src/lib/OpenEXR/ImfCompression.h
            * Add your method at the end of the Compression enum below.

        2. src/lib/OpenEXR/ImfCompression.cpp
            * Add your CompressionDesc(s) to IdToDesc[]. This is used by the API
              to check its capabilities and requirements.
              NOTE: the order MUST MATCH Compression's order, as we access the
                       descriptions by index.
            * Update CompressionNameToId to allow the API to map from your
              compressor's identifier(s) to its/their Compression value(s).

        3. openexr/src/lib/OpenEXR/ImfCompressor.cpp
            * Include your Imf*Compressor.h.
            * Add your instantiation calls to newCompressor().
            * Add your instantiation calls to newTileCompressor().

        4. src/lib/OpenEXR/ImfCRgbaFile.h
            * Add a IMF_*_COMPRESSION define for your compression method, making
              sure to update IMF_NUM_COMPRESSION_METHODS with the proper index.

        5. Tests

            1. src/test/OpenEXRTest/testCompressionApi.cpp
                * See the comments to update that file and ensure your method is
                properly registered.

    3. C API integration

        1. src/lib/OpenEXRCore/openexr_attr.h
            * Add your EXR_COMPRESSION_* to the exr_compression_t enum.

        2. src/lib/OpenEXRCore/encoding.c
            * Update default_compress_chunk().

        3. src/lib/OpenEXRCore/decoding.c
            * Update decompress_data().

        4. src/lib/OpenEXRCore/parse_header.c
            * Update internal_exr_compute_chunk_offset_size().

        5. src/lib/OpenEXRCore/validation.c
            * Update validate_deep_data()

        6. Tests

            1. src/test/OpenEXRCoreTest/compression.h
                * Declare your own test*Compression().

            2. src/test/OpenEXRCoreTest/compression.cpp
                * Update doWriteRead().
                * Implement your test*Compression() function at the end.

            3. src/test/OpenEXRCoreTest/main.cpp
                * Call your test function in main().

            4. src/test/OpenEXRCoreTest/CMakeLists.txt
                * Add your test function name to define_openexrcore_tests().

    5. Documentation

        1. Compression methods are listed or described in many places throughout
           the docs and you will have to update all relevant *.rst files.

----------------------------------------------------------------------------- */

#include "ImfForward.h"
#include <string>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

// All available compression methods.
// NOTE: Must be extended to add a new codec. Ids must be continuous.
enum IMF_EXPORT_ENUM Compression
{
    NO_COMPRESSION = 0, // no compression.

    RLE_COMPRESSION = 1, // run length encoding.

    ZIPS_COMPRESSION = 2, // zlib compression, one scan line at a time.

    ZIP_COMPRESSION = 3, // zlib compression, in blocks of 16 scan lines.

    PIZ_COMPRESSION = 4, // piz-based wavelet compression.

    PXR24_COMPRESSION = 5, // lossy 24-bit float compression

    B44_COMPRESSION = 6, // lossy 4-by-4 pixel block compression,
                         // fixed compression rate.

    B44A_COMPRESSION = 7, // lossy 4-by-4 pixel block compression,
                          // flat fields are compressed more.

    DWAA_COMPRESSION = 8, // lossy DCT based compression, in blocks
                          // of 32 scanlines. More efficient for partial
                          // buffer access.

    DWAB_COMPRESSION = 9, // lossy DCT based compression, in blocks
                          // of 256 scanlines. More efficient space
                          // wise and faster to decode full frames
                          // than DWAA_COMPRESSION.

    NUM_COMPRESSION_METHODS // number of different compression methods.
};

/// Returns a codec ID's short name (lowercase).
IMF_EXPORT void getCompressionNameFromId (Compression id, std::string& name);

/// Returns a codec ID's short description (lowercase).
IMF_EXPORT void
getCompressionDescriptionFromId (Compression id, std::string& desc);

/// Returns the codec name's ID, NUM_COMPRESSION_METHODS if not found.
IMF_EXPORT void
getCompressionIdFromName (const std::string& name, Compression& id);

/// Return true if a compression id exists.
IMF_EXPORT bool isValidCompression (int id);

/// Return a string enumerating all compression names, with a custom separator.
IMF_EXPORT void
getCompressionNamesString (const std::string& separator, std::string& in);

/// Return the number of scan lines expected by a given compression method.
IMF_EXPORT int getCompressionNumScanlines (Compression id);

/// Return true is the compression method exists and doesn't preserves data integrity.
IMF_EXPORT bool isLossyCompression (Compression id);

/// Return true is the compression method exists and supports deep data.
IMF_EXPORT bool isValidDeepCompression (Compression id);

/// Controls the default zip compression level used. Zip is used for
/// the 2 zip levels as well as some modes of the DWAA/B compression.
IMF_EXPORT void setDefaultZipCompressionLevel (int level);

/// Controls the default quality level for the DWA lossy compression
IMF_EXPORT void setDefaultDwaCompressionLevel (float level);

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
