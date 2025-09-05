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

    HTJ2K256_COMPRESSION = 10,   // High-Throughput JPEG2000 (HTJ2K), 256 scanlines

    HTJ2K32_COMPRESSION = 11,    // High-Throughput JPEG2000 (HTJ2K), 32 scanlines

    NUM_COMPRESSION_METHODS // number of different compression methods
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
