/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_color_metadata.h"

#include "openexr_part.h"

#include <math.h>
#include <string.h>

/**************************************/

/*
 * The chromaticities of the linear, scene-referred color interop IDs, per
 * the Color Interop Forum recommendation for OpenEXR files. This is the
 * library's single definition of these primaries; the C++
 * colorInteropIDToChromaticities() and acesChromaticities() both source
 * them from here.
 *
 * The order of the entries is significant: it is the order in which
 * exr_chromaticities_to_color_interop_id() reports a match, which only
 * matters at a tolerance loose enough for two entries to match at once.
 * (That is, looser than than the default tolerance.)
 *
 * Please note that these are simply the most common linear colorInteropIDs,
 * this is not an exhaustive list. More IDs may be added over time. More
 * info about the Color Interop ID may be found in this document:
 * https://github.com/AcademySoftwareFoundation/ColorInterop/blob/main/
 * Recommendations/03_ColorInteropID/ColorInteropID.md
 */

typedef struct
{
    const char*               id;
    exr_attr_chromaticities_t chromaticities;
} exr_priv_color_interop_id_chromaticities_t;

#define D65_WHITE 0.3127f, 0.3290f
#define ACES_WHITE 0.32168f, 0.33767f

/* clang-format off */

static const exr_priv_color_interop_id_chromaticities_t
    the_color_interop_id_chromaticities[] = {
        /*                          red                green               blue              white */
        {"lin_rec709_scene",   { 0.6400f,  0.3300f,  0.3000f,  0.6000f,  0.1500f,   0.0600f, D65_WHITE}},
        {"lin_ap0_scene",      { 0.7347f,  0.2653f,  0.0000f,  1.0000f,  0.0001f,  -0.0770f, ACES_WHITE}},
        {"lin_ap1_scene",      { 0.7130f,  0.2930f,  0.1650f,  0.8300f,  0.1280f,   0.0440f, ACES_WHITE}},
        {"lin_p3d65_scene",    { 0.6800f,  0.3200f,  0.2650f,  0.6900f,  0.1500f,   0.0600f, D65_WHITE}},
        {"lin_rec2020_scene",  { 0.7080f,  0.2920f,  0.1700f,  0.7970f,  0.1310f,   0.0460f, D65_WHITE}},
        {"lin_adobergb_scene", { 0.6400f,  0.3300f,  0.2100f,  0.7100f,  0.1500f,   0.0600f, D65_WHITE}}};

/* clang-format on */

#undef D65_WHITE
#undef ACES_WHITE

static const size_t the_num_color_interop_id_chromaticities =
    sizeof (the_color_interop_id_chromaticities) /
    sizeof (the_color_interop_id_chromaticities[0]);

/**************************************/

exr_result_t
exr_color_interop_id_to_chromaticities (
    const char* color_interop_id, exr_attr_chromaticities_t* chromaticities)
{
    if (!color_interop_id || !chromaticities) return EXR_ERR_INVALID_ARGUMENT;

    for (size_t i = 0; i < the_num_color_interop_id_chromaticities; ++i)
    {
        if (0 ==
            strcmp (color_interop_id, the_color_interop_id_chromaticities[i].id))
        {
            *chromaticities =
                the_color_interop_id_chromaticities[i].chromaticities;
            return EXR_ERR_SUCCESS;
        }
    }

    return EXR_ERR_NO_ATTR_BY_NAME;
}

/**************************************/

static int
within_tolerance (float a, float b, float tolerance)
{
    return fabsf (a - b) <= tolerance;
}

exr_result_t
exr_chromaticities_to_color_interop_id (
    const exr_attr_chromaticities_t* chromaticities,
    float                            tolerance,
    const char**                     color_interop_id)
{
    if (!chromaticities || !color_interop_id || !(tolerance >= 0.f))
        return EXR_ERR_INVALID_ARGUMENT;

    for (size_t i = 0; i < the_num_color_interop_id_chromaticities; ++i)
    {
        const exr_attr_chromaticities_t* e =
            &(the_color_interop_id_chromaticities[i].chromaticities);

        if (within_tolerance (e->red_x, chromaticities->red_x, tolerance) &&
            within_tolerance (e->red_y, chromaticities->red_y, tolerance) &&
            within_tolerance (e->green_x, chromaticities->green_x, tolerance) &&
            within_tolerance (e->green_y, chromaticities->green_y, tolerance) &&
            within_tolerance (e->blue_x, chromaticities->blue_x, tolerance) &&
            within_tolerance (e->blue_y, chromaticities->blue_y, tolerance) &&
            within_tolerance (e->white_x, chromaticities->white_x, tolerance) &&
            within_tolerance (e->white_y, chromaticities->white_y, tolerance))
        {
            *color_interop_id = the_color_interop_id_chromaticities[i].id;
            return EXR_ERR_SUCCESS;
        }
    }

    return EXR_ERR_NO_ATTR_BY_NAME;
}

/**************************************/

exr_result_t
exr_check_color_metadata_values (
    const exr_color_metadata_t* values, uint32_t* warnings)
{
    const char* id;
    uint32_t    found = EXR_COLOR_METADATA_OK;

    if (!values || !warnings) return EXR_ERR_INVALID_ARGUMENT;

    *warnings = EXR_COLOR_METADATA_OK;

    /*
     * colorInteropID related checks.
     */
    id = values->color_interop_id;
    if (id)
    {
        /*
         * An empty ID says nothing; the attribute should be omitted, or set
         * to "unknown", instead.
         */
        if (id[0] == '\0') found |= EXR_COLOR_METADATA_EMPTY_INTEROP_ID;

        /*
         * If the ID names a color space with defined chromaticities, and the
         * part also carries chromaticities, the two should agree.
         */
        if (values->chromaticities)
        {
            /* Only needed to test whether the ID has defined chromaticities. */
            exr_attr_chromaticities_t from_id;

            if (EXR_ERR_SUCCESS ==
                exr_color_interop_id_to_chromaticities (id, &from_id))
            {
                const char* from_chromaticities = NULL;

                /*
                 * Matching no entry at all is the same finding as matching a
                 * different one: either way these are not the chromaticities
                 * the ID denotes.
                 */
                if (EXR_ERR_SUCCESS !=
                        exr_chromaticities_to_color_interop_id (
                            values->chromaticities,
                            EXR_COLOR_INTEROP_ID_CHROMATICITIES_TOLERANCE,
                            &from_chromaticities) ||
                    0 != strcmp (from_chromaticities, id))
                {
                    found |= EXR_COLOR_METADATA_CHROMATICITIES_DIFFER;
                }
            }
        }

        if (0 == strcmp (id, "data"))
        {
            /*
             * A part tagged "data" is deliberately not color managed, so
             * colorimetry attributes on it are contradictory.
             */
            if (values->chromaticities)
                found |= EXR_COLOR_METADATA_DATA_HAS_CHROMATICITIES;
            if (values->has_white_luminance)
                found |= EXR_COLOR_METADATA_DATA_HAS_WHITE_LUMINANCE;
            if (values->has_adopted_neutral)
                found |= EXR_COLOR_METADATA_DATA_HAS_ADOPTED_NEUTRAL;
            /* The acesImageContainerFlag case is caught below. */
        }
        else if (values->compare_to_first_part)
        {
            /*
             * A part after the first may omit the attribute, inheriting the
             * first part's value, or set it to "data", but any other value
             * that differs from the first part's leaves its color space
             * ambiguous.
             *
             * There is deliberately no equivalent check for chromaticities
             * differing between parts. That one is already enforced by
             * internal_exr_validate_shared_attrs, and unlike this rule it is
             * not conditional on strict_header, so such a file cannot be
             * opened at all. A warning here would be unreachable: this
             * function only ever sees metadata from a context that opened
             * successfully, or that a caller is about to write.
             *
             * The difference in severity is the point. Omitting the ID, or
             * setting it to "data", is legitimate, so a non-conforming value
             * only makes a part's color space ambiguous in a file that still
             * reads. Mismatched chromaticities make the file unreadable,
             * which is an error rather than a warning.
             */
            if (!values->first_part_color_interop_id ||
                0 != strcmp (values->first_part_color_interop_id, id))
            {
                found |= EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED;
            }
        }
    }

    /*
     * acesImageContainerFlag asserts compliance with SMPTE ST 2065-4,
     * the image data must be ACES2065-1, whose colorInteropID is lin_ap0_scene.
     * So the flag constrains the rest of the color metadata.
     *
     * ST 2065-4 requires the chromaticities attribute, which is why it is
     * expected here even though the Color Interop Forum recommendation
     * otherwise discourages setting it alongside a colorInteropID.
     */
    if (values->has_aces_image_container_flag)
    {
        /*
         * 1 is the only defined value. The attribute is a flag, and it is
         * the value that asserts compliance, not the mere presence.
         */
        if (values->aces_image_container_flag != 1)
            found |= EXR_COLOR_METADATA_ACES_FLAG_NOT_ONE;

        /*
         * The ID may be omitted, but if present it must agree.
         */
        if (id && 0 != strcmp (id, "lin_ap0_scene"))
            found |= EXR_COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0;

        if (!values->chromaticities)
        {
            found |= EXR_COLOR_METADATA_ACES_FLAG_NO_CHROMATICITIES;
        }
        else
        {
            const char* from_chromaticities = NULL;

            if (EXR_ERR_SUCCESS != exr_chromaticities_to_color_interop_id (
                                       values->chromaticities,
                                       EXR_COLOR_INTEROP_ID_CHROMATICITIES_TOLERANCE,
                                       &from_chromaticities) ||
                0 != strcmp (from_chromaticities, "lin_ap0_scene"))
            {
                found |= EXR_COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0;
            }
        }
    }

    *warnings = found;
    return EXR_ERR_SUCCESS;
}

/**************************************/

/*
 * Return a part's colorInteropID, or NULL if it has none or the attribute
 * is not a string.
 */
static const char*
get_color_interop_id (exr_const_context_t ctxt, int part_index)
{
    const exr_attribute_t* attr = NULL;

    if (EXR_ERR_SUCCESS !=
        exr_get_attribute_by_name (ctxt, part_index, "colorInteropID", &attr))
        return NULL;

    if (attr->type != EXR_ATTR_STRING) return NULL;

    return attr->string->str;
}

static int
has_attribute (exr_const_context_t ctxt, int part_index, const char* name)
{
    const exr_attribute_t* attr = NULL;

    return EXR_ERR_SUCCESS ==
           exr_get_attribute_by_name (ctxt, part_index, name, &attr);
}

exr_result_t
exr_check_color_metadata (
    exr_const_context_t ctxt, int part_index, uint32_t* warnings)
{
    exr_result_t              rv;
    int                       num_parts = 0;
    exr_color_metadata_t      values;
    exr_attr_chromaticities_t chromaticities;
    const exr_attribute_t*    attr = NULL;

    if (!warnings) return EXR_ERR_INVALID_ARGUMENT;

    *warnings = EXR_COLOR_METADATA_OK;

    rv = exr_get_count (ctxt, &num_parts);
    if (rv != EXR_ERR_SUCCESS) return rv;

    if (part_index < 0 || part_index >= num_parts) return EXR_ERR_INCORRECT_PART;

    memset (&values, 0, sizeof (values));

    values.color_interop_id = get_color_interop_id (ctxt, part_index);

    if (EXR_ERR_SUCCESS ==
        exr_attr_get_chromaticities (
            ctxt, part_index, "chromaticities", &chromaticities))
        values.chromaticities = &chromaticities;

    values.has_white_luminance =
        has_attribute (ctxt, part_index, "whiteLuminance");
    values.has_adopted_neutral =
        has_attribute (ctxt, part_index, "adoptedNeutral");

    if (EXR_ERR_SUCCESS == exr_get_attribute_by_name (
                               ctxt, part_index, "acesImageContainerFlag", &attr))
    {
        values.has_aces_image_container_flag = 1;

        /*
         * A present attribute of the wrong type is reported the same way as
         * a wrong value, so any non-conforming value will do here.
         */
        values.aces_image_container_flag =
            (attr->type == EXR_ATTR_INT) ? attr->i : 0;
    }

    if (part_index > 0)
    {
        values.compare_to_first_part       = 1;
        values.first_part_color_interop_id = get_color_interop_id (ctxt, 0);
    }

    return exr_check_color_metadata_values (&values, warnings);
}

/**************************************/

const char*
exr_get_color_metadata_warning_as_string (exr_color_metadata_warning_t warning)
{
    switch (warning)
    {
        case EXR_COLOR_METADATA_OK: return "EXR_COLOR_METADATA_OK";
        case EXR_COLOR_METADATA_EMPTY_INTEROP_ID:
            return "colorInteropID is empty";
        case EXR_COLOR_METADATA_CHROMATICITIES_DIFFER:
            return "the chromaticities are not those denoted by the colorInteropID";
        case EXR_COLOR_METADATA_DATA_HAS_CHROMATICITIES:
            return "colorInteropID is 'data' but the part has a chromaticities attribute";
        case EXR_COLOR_METADATA_DATA_HAS_WHITE_LUMINANCE:
            return "colorInteropID is 'data' but the part has a whiteLuminance attribute";
        case EXR_COLOR_METADATA_DATA_HAS_ADOPTED_NEUTRAL:
            return "colorInteropID is 'data' but the part has an adoptedNeutral attribute";
        case EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED:
            return "colorInteropID does not match the first part's";
        case EXR_COLOR_METADATA_ACES_FLAG_NOT_ONE:
            return "acesImageContainerFlag is present but is not an int of value 1";
        case EXR_COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0:
            return "acesImageContainerFlag is present but colorInteropID is not 'lin_ap0_scene'";
        case EXR_COLOR_METADATA_ACES_FLAG_NO_CHROMATICITIES:
            return "acesImageContainerFlag is present but the part has no chromaticities attribute";
        case EXR_COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0:
            return "acesImageContainerFlag is present but the chromaticities are not those of 'lin_ap0_scene'";
    }

    return "Unknown color metadata warning";
}
