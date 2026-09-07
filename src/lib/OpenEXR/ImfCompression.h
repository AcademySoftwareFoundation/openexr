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

#include "openexr_attr.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

// All available compression methods. This is a type that is not
// easily replaced by the core C library as it is a c++ type and so
// not easily aliased as it would need to be in the global namespace
// (or have different names). Instead, we replicate it, but keep the
// numbers in sync.
enum IMF_EXPORT_ENUM Compression
{
    NO_COMPRESSION = EXR_COMPRESSION_NONE,
    RLE_COMPRESSION = EXR_COMPRESSION_RLE,
    ZIPS_COMPRESSION = EXR_COMPRESSION_ZIPS,
    ZIP_COMPRESSION = EXR_COMPRESSION_ZIP,
    PIZ_COMPRESSION = EXR_COMPRESSION_PIZ,
    PXR24_COMPRESSION = EXR_COMPRESSION_PXR24,
    B44_COMPRESSION = EXR_COMPRESSION_B44,
    B44A_COMPRESSION = EXR_COMPRESSION_B44A,
    DWAA_COMPRESSION = EXR_COMPRESSION_DWAA,
    DWAB_COMPRESSION = EXR_COMPRESSION_DWAB,
    HTJ2K256_COMPRESSION = EXR_COMPRESSION_HTJ2K256,
    HTJ2K32_COMPRESSION = EXR_COMPRESSION_HTJ2K32,
    LJ2K_COMPRESSION = EXR_COMPRESSION_LJ2K,
    ZSTD_COMPRESSION = EXR_COMPRESSION_ZSTD,
    NUM_COMPRESSION_METHODS = EXR_COMPRESSION_LAST_TYPE
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

/// Controls the default quality level for the lossy HTJ2K compression
IMF_EXPORT void setDefaultLossyHTJ2KQuality (float quality);

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
