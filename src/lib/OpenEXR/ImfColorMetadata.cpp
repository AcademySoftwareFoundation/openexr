//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	Color space metadata consistency checking
//
//-----------------------------------------------------------------------------

#include "ImfColorMetadata.h"

#include "ImfStandardAttributes.h"

#include "openexr_color_metadata.h"

#include <stdint.h>
#include <string.h>

//
// The rules live in OpenEXRCore so that the C API, the C++ API and exrinfo
// all apply the same ones; the functions here adapt a Header to that call.
//

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

namespace
{

//
// The two enumerations are declared separately so that neither public
// header has to include the other's, which makes it possible for them to
// drift. Use a compile-time assert to validate consistency.
//

#define IMF_ASSERT_SAME_WARNING(imfName, coreName)                             \
    static_assert (                                                            \
        static_cast<int> (imfName) == static_cast<int> (coreName),             \
        #imfName " and " #coreName " have diverged")

IMF_ASSERT_SAME_WARNING (COLOR_METADATA_OK, EXR_COLOR_METADATA_OK);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_EMPTY_INTEROP_ID, EXR_COLOR_METADATA_EMPTY_INTEROP_ID);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_CHROMATICITIES_DIFFER,
    EXR_COLOR_METADATA_CHROMATICITIES_DIFFER);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_DATA_HAS_CHROMATICITIES,
    EXR_COLOR_METADATA_DATA_HAS_CHROMATICITIES);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_DATA_HAS_WHITE_LUMINANCE,
    EXR_COLOR_METADATA_DATA_HAS_WHITE_LUMINANCE);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_DATA_HAS_ADOPTED_NEUTRAL,
    EXR_COLOR_METADATA_DATA_HAS_ADOPTED_NEUTRAL);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_INTEROP_ID_NOT_SHARED,
    EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_ACES_FLAG_NOT_ONE, EXR_COLOR_METADATA_ACES_FLAG_NOT_ONE);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0,
    EXR_COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_ACES_FLAG_NO_CHROMATICITIES,
    EXR_COLOR_METADATA_ACES_FLAG_NO_CHROMATICITIES);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0,
    EXR_COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0);

#undef IMF_ASSERT_SAME_WARNING

//
// Describe a header to the Core check. The struct only holds a pointer to
// the chromaticities, so the caller supplies the storage they live in; the
// strings point into the header's attributes.
//

void
fillColorMetadata (
    const Header&              header,
    exr_color_metadata_t&      values,
    exr_attr_chromaticities_t& chromaBuffer)
{
    memset (&values, 0, sizeof (values));

    if (hasColorInteropID (header))
        values.color_interop_id = colorInteropID (header).c_str ();

    if (hasChromaticities (header))
    {
        const Chromaticities& c = chromaticities (header);

        chromaBuffer.red_x   = c.red.x;
        chromaBuffer.red_y   = c.red.y;
        chromaBuffer.green_x = c.green.x;
        chromaBuffer.green_y = c.green.y;
        chromaBuffer.blue_x  = c.blue.x;
        chromaBuffer.blue_y  = c.blue.y;
        chromaBuffer.white_x = c.white.x;
        chromaBuffer.white_y = c.white.y;

        values.chromaticities = &chromaBuffer;
    }

    values.has_white_luminance = hasWhiteLuminance (header) ? 1 : 0;
    values.has_adopted_neutral = hasAdoptedNeutral (header) ? 1 : 0;

    if (hasAcesImageContainerFlag (header))
    {
        values.has_aces_image_container_flag = 1;
        values.aces_image_container_flag = acesImageContainerFlag (header);
    }
    else if (header.find ("acesImageContainerFlag") != header.end ())
    {
        //
        // Present, but not an int: hasAcesImageContainerFlag() only matches
        // the expected type. A wrong type is reported the same way as a
        // wrong value, so any non-conforming value will do here.
        //

        values.has_aces_image_container_flag = 1;
        values.aces_image_container_flag     = 0;
    }
}

} // namespace

unsigned int
checkColorMetadata (const Header& header)
{
    exr_color_metadata_t      values;
    exr_attr_chromaticities_t chromaBuffer;
    uint32_t                  warnings = EXR_COLOR_METADATA_OK;

    fillColorMetadata (header, values, chromaBuffer);

    exr_check_color_metadata_values (&values, &warnings);

    return warnings;
}

unsigned int
checkColorMetadata (const Header& header, const Header& firstPart)
{
    exr_color_metadata_t      values;
    exr_attr_chromaticities_t chromaBuffer;
    uint32_t                  warnings = EXR_COLOR_METADATA_OK;

    fillColorMetadata (header, values, chromaBuffer);

    values.compare_to_first_part = 1;

    if (hasColorInteropID (firstPart))
        values.first_part_color_interop_id =
            colorInteropID (firstPart).c_str ();

    exr_check_color_metadata_values (&values, &warnings);

    return warnings;
}

const char*
colorMetadataWarningToString (ColorMetadataWarning warning)
{
    return exr_get_color_metadata_warning_as_string (
        static_cast<exr_color_metadata_warning_t> (warning));
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
