//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_COLOR_METADATA_H
#define INCLUDED_IMF_COLOR_METADATA_H

//-----------------------------------------------------------------------------
//
//	Checking color space metadata for consistency
//
//	checkColorMetadata() reports combinations of colorInteropID,
//	chromaticities, whiteLuminance, adoptedNeutral and
//	acesImageContainerFlag that leave a part's color space ambiguous or
//	self-contradictory.
//
//	None of these make a file malformed or unsafe to read, which is why
//	this is separate from the file validation in ImfCheckFile.h: they are
//	warnings about the meaning of the metadata, not errors.
//
//	A header with neither colorInteropID nor acesImageContainerFlag makes
//	no claim about its color space, and so never produces a warning.
//
//	The rules are shared with the C API; see exr_check_color_metadata()
//	in OpenEXRCore/openexr_color_metadata.h.
//
//	New in OpenEXR v3.5
//
//-----------------------------------------------------------------------------

#include "ImfExport.h"
#include "ImfHeader.h"
#include "ImfNamespace.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

//
// The individual problems that checkColorMetadata() can report. These are
// flags, returned OR'd together.
//

enum ColorMetadataWarning
{
    //
    // No inconsistency was found.
    //

    COLOR_METADATA_OK = 0,

    //
    // colorInteropID is present but empty; it should be omitted, or set to
    // "unknown", instead.
    //

    COLOR_METADATA_EMPTY_INTEROP_ID = 1 << 0,

    //
    // colorInteropID denotes a color space with defined chromaticities, the
    // header also has a chromaticities attribute, and they are different.
    //

    COLOR_METADATA_CHROMATICITIES_DIFFER = 1 << 1,

    //
    // colorInteropID is "data", marking the part as deliberately not color
    // managed, yet the header carries color-related attributes.
    //

    COLOR_METADATA_DATA_HAS_CHROMATICITIES  = 1 << 2,
    COLOR_METADATA_DATA_HAS_WHITE_LUMINANCE = 1 << 3,
    COLOR_METADATA_DATA_HAS_ADOPTED_NEUTRAL = 1 << 4,

    //
    // This part of a multipart file has a colorInteropID that is neither
    // "data" nor equal to the first part's, so it does not conform to the
    // shared attribute rules. Only the two-header overload of
    // checkColorMetadata() reports this.
    //

    COLOR_METADATA_INTEROP_ID_NOT_SHARED = 1 << 5,

    //
    // acesImageContainerFlag asserts compliance with SMPTE ST 2065-4, which
    // requires the color space of the image data be ACES2065-1, whose primaries
    // are those of lin_ap0_scene. So the flag constrains the rest of the
    // color metadata.
    //
    // The only defined value of the flag is 1; it is the value that asserts
    // compliance, not the mere presence of the attribute. An attribute of a
    // type other than int is reported the same way as a wrong value.
    //
    // The colorInteropID may be omitted, but if present it has to agree.
    // The chromaticities are required by ST 2065-4, which is why they are
    // expected here even though the Color Interop Forum recommendation
    // otherwise discourages setting them alongside a colorInteropID.
    //

    COLOR_METADATA_ACES_FLAG_NOT_ONE                = 1 << 6,
    COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0     = 1 << 7,
    COLOR_METADATA_ACES_FLAG_NO_CHROMATICITIES      = 1 << 8,
    COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0 = 1 << 9
};

//
// Check one header's color space metadata. Use this for a single-part file,
// or for the first part of a multipart file.
//
// Returns zero or more ColorMetadataWarning flags OR'd together, or
// COLOR_METADATA_OK if nothing was found. Does not throw.
//

IMF_EXPORT unsigned int checkColorMetadata (const Header& header);

//
// Check the header of a part after the first in a multipart file, passing
// the first part's header as firstPart. This additionally checks the shared
// attribute rules, and so may report COLOR_METADATA_INTEROP_ID_NOT_SHARED.
//

IMF_EXPORT unsigned int
checkColorMetadata (const Header& header, const Header& firstPart);

//
// A static, human-readable description of a single warning flag. Describes
// one flag, so callers reporting a set of warnings should test each flag in
// turn rather than passing the combined value.
//

IMF_EXPORT const char*
colorMetadataWarningToString (ColorMetadataWarning warning);

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
