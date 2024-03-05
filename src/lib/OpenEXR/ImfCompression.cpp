//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "ImfNamespace.h"
#include "ImfCompression.h"
#include <map>

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
isValidCompression (Compression id)
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
