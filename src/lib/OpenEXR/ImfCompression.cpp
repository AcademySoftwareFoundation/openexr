//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//
// -----------------------------------------------------------------------------
//
//  HOW TO ADD NEW COMPRESSION TECHNIQUE
//  ====================================
//
//  1. Creating your C++ compression class
//
//      1. Implement a new src/lib/OpenEXR/Imf*Compressor.{h, cpp} class deriving
//         from ImfCompressor.
//
//      2. Update the build systems
//          * Update src/lib/OpenEXR/CMakeLists.txt to build your codec with cmake.
//          * Update BUILD.bazel to build your codec with bazel.
//          * NOTE: Both build methods must be implemented !
//
//  2. C API integration
//
//      1. src/lib/OpenEXRCore/openexr_attr.h
//          * Add your EXR_COMPRESSION_* to the exr_compression_t enum.
//
//      2. src/lib/OpenEXRCore/compression.c
//          * Update exr_compression_lines_per_chunk().
//          * Update exr_compress_chunk().
//          * Update exr_uncompress_chunk().
//
//      5. src/lib/OpenEXRCore/validation.c
//          * Update validate_deep_data()
//
//      6. Tests
//
//          1. src/test/OpenEXRCoreTest/compression.h
//              * Declare your own test*Compression().
//
//          2. src/test/OpenEXRCoreTest/compression.cpp
//              * Update doWriteRead().
//              * Implement your test*Compression() function at the end.
//
//          3. src/test/OpenEXRCoreTest/main.cpp
//              * Call your test function in main().
//
//          4. src/test/OpenEXRCoreTest/CMakeLists.txt
//              * Add your test function name to define_openexrcore_tests().
//
//  3. C++ API integration
//
//      1. src/lib/OpenEXR/ImfCompression.h
//         * Add your method at the end of the Compression enum below.
//
//      2. src/lib/OpenEXR/ImfCompression.cpp
//          * Add your CompressionDesc(s) to IdToDesc[]. This is used by the API
//             to check its capabilities and requirements.
//             NOTE: the order MUST MATCH Compression's order, as we access the
//                   descriptions by index.
//           * Update CompressionNameToId to allow the API to map from your
//             compressor's identifier(s) to its/their Compression value(s).
//
//      3. openexr/src/lib/OpenEXR/ImfCompressor.cpp
//           * Include your Imf*Compressor.h.
//           * Add your instantiation calls to newCompressor().
//           * Add your instantiation calls to newTileCompressor().
//
//      4. src/lib/OpenEXR/ImfCRgbaFile.h
//          * Add a IMF_*_COMPRESSION define for your compression method, making
//            sure to update IMF_NUM_COMPRESSION_METHODS with the proper index.
//
//      5. Tests
//
//          1. src/test/OpenEXRTest/testCompressionApi.cpp
//              * See the comments to update that file and ensure your method is
//                properly registered.
//
//  4. Documentation
//
//      1. Compression methods are listed or described in many places throughout
//         the docs and you will have to update all relevant *.rst files.
//
// -----------------------------------------------------------------------------

#include "ImfNamespace.h"
#include "ImfCompression.h"
#include <map>
#include <cctype>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

/// Store codec properties so they may be queried in various places.
struct CompressionDesc
{
    std::string name;         // short name
    std::string desc;         // method description
    int         numScanlines; // number of scanlines required
    bool        lossy;        // true if lossy algorithm
    bool        deep;         // true is capable of compressing deep data

    CompressionDesc (
        std::string _name,
        std::string _desc,
        int         _scanlines,
        bool        _lossy,
        bool        _deep)
    {
        name         = _name;
        desc         = _desc;
        numScanlines = _scanlines;
        lossy        = _lossy;
        deep         = _deep;
    }
};

// NOTE: IdToDesc order MUST match Imf::Compression enum.
// clang-format off
static const CompressionDesc IdToDesc[] = {
    CompressionDesc (
        "none",
        "no compression.",
        1,
        false,
        true),
    CompressionDesc (
        "rle",
        "run-length encoding.",
        1,
        false,
        true),
    CompressionDesc (
        "zips",
        "zlib compression, one scan line at a time.",
        1,
        false,
        true),
    CompressionDesc (
        "zip",
        "zlib compression, in blocks of 16 scan lines.",
        16,
        false,
        false),
    CompressionDesc (
        "piz",
        "piz-based wavelet compression, in blocks of 32 scan lines.",
        32,
        false,
        false),
    CompressionDesc (
        "pxr24",
        "lossy 24-bit float compression, in blocks of 16 scan lines.",
        16,
        true,
        false),
    CompressionDesc (
        "b44",
        "lossy 4-by-4 pixel block compression, fixed compression rate.",
        32,
        true,
        false),
    CompressionDesc (
        "b44a",
        "lossy 4-by-4 pixel block compression, flat fields are compressed more.",
        32,
        true,
        false),
    CompressionDesc (
        "dwaa",
        "lossy DCT based compression, in blocks of 32 scanlines. More efficient "
        "for partial buffer access.",
        32,
        true,
        false),
    CompressionDesc (
        "dwab",
        "lossy DCT based compression, in blocks of 256 scanlines. More efficient "
        "space wise and faster to decode full frames than DWAA_COMPRESSION.",
        256,
        true,
        false),
};
// clang-format on

// NOTE: CompressionNameToId order MUST match Imf::Compression enum.
static const std::map<std::string, Compression> CompressionNameToId = {
    {"no", Compression::NO_COMPRESSION},
    {"none", Compression::NO_COMPRESSION},
    {"rle", Compression::RLE_COMPRESSION},
    {"zips", Compression::ZIPS_COMPRESSION},
    {"zip", Compression::ZIP_COMPRESSION},
    {"piz", Compression::PIZ_COMPRESSION},
    {"pxr24", Compression::PXR24_COMPRESSION},
    {"b44", Compression::B44_COMPRESSION},
    {"b44a", Compression::B44A_COMPRESSION},
    {"dwaa", Compression::DWAA_COMPRESSION},
    {"dwab", Compression::DWAB_COMPRESSION},
};

#define UNKNOWN_COMPRESSION_ID_MSG "INVALID COMPRESSION ID"

/// Returns a codec ID's short name (lowercase).
void
getCompressionNameFromId (Compression id, std::string& name)
{
    if (id < NO_COMPRESSION || id >= NUM_COMPRESSION_METHODS)
        name = UNKNOWN_COMPRESSION_ID_MSG;
    name = IdToDesc[static_cast<int> (id)].name;
}

/// Returns a codec ID's short description (lowercase).
void
getCompressionDescriptionFromId (Compression id, std::string& desc)
{
    if (id < NO_COMPRESSION || id >= NUM_COMPRESSION_METHODS)
        desc = UNKNOWN_COMPRESSION_ID_MSG;
    desc = IdToDesc[static_cast<int> (id)].name + ": " +
           IdToDesc[static_cast<int> (id)].desc;
}

/// Returns the codec name's ID, NUM_COMPRESSION_METHODS if not found.
void
getCompressionIdFromName (const std::string& name, Compression& id)
{
    std::string lowercaseName (name);
    for (auto& ch: lowercaseName)
        ch = std::tolower (ch);

    auto it = CompressionNameToId.find (lowercaseName);
    id      = it != CompressionNameToId.end ()
                  ? it->second
                  : Compression::NUM_COMPRESSION_METHODS;
}

/// Return true if a compression id exists.
bool
isValidCompression (int id)
{
    return id >= NO_COMPRESSION && id < NUM_COMPRESSION_METHODS;
}

/// Return a string enumerating all compression names, with a custom separator.
void
getCompressionNamesString (const std::string& separator, std::string& str)
{
    int i = 0;
    for (; i < static_cast<int> (NUM_COMPRESSION_METHODS) - 1; i++)
    {
        str += IdToDesc[i].name + separator;
    }
    str += IdToDesc[i].name;
}

/// Return the number of scan lines expected by a given compression method.
int
getCompressionNumScanlines (Compression id)
{
    if (id < NO_COMPRESSION || id >= NUM_COMPRESSION_METHODS) return -1;
    return IdToDesc[static_cast<int> (id)].numScanlines;
}

/// Return true is the compression method exists and doesn't preserve data integrity.
bool
isLossyCompression (Compression id)
{
    return id >= NO_COMPRESSION && id < NUM_COMPRESSION_METHODS &&
           IdToDesc[static_cast<int> (id)].lossy;
}

/// Return true is the compression method exists and supports deep data.
bool
isValidDeepCompression (Compression id)
{
    return id >= NO_COMPRESSION && id < NUM_COMPRESSION_METHODS &&
           IdToDesc[static_cast<int> (id)].deep;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
