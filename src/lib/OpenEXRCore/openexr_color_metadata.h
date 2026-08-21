/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_COLOR_METADATA_H
#define OPENEXR_CORE_COLOR_METADATA_H

#include "openexr_context.h"

#include "openexr_attr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @file */

/**
 * @defgroup Color metadata
 * @brief Functions relating the colorInteropID standard attribute to
 * chromaticities, and checking a part's color space metadata for internal
 * consistency.
 *
 * New in OpenEXR v3.5.
 * @{
 */

/**************************************/

/** @brief Tolerance recommended when matching a chromaticities attribute
 * against the chromaticities of a color interop ID.
 *
 * This is the value the Color Interop Forum recommendation suggests.
 */
#define EXR_COLOR_INTEROP_ID_CHROMATICITIES_TOLERANCE 0.001f

/** @brief Return the chromaticities denoted by a color interop ID.
 *
 * The Color Interop Forum recommendation for OpenEXR files defines a
 * mapping between chromaticities and the subset of color interop IDs that
 * denote linear, scene-referred RGB color spaces. Only these six IDs have
 * a defined mapping:
 *
 *     lin_rec709_scene     lin_ap0_scene       lin_ap1_scene
 *     lin_p3d65_scene      lin_rec2020_scene   lin_adobergb_scene
 *
 * See <https://github.com/AcademySoftwareFoundation/ColorInterop/blob/main/
 * Recommendations/04_OpenEXRFiles/OpenEXRFiles.md>.
 *
 * Note that the recommendation is that the chromaticities attribute not be
 * set when setting the colorInteropID, other than for ST 2065-4 compliance.
 * This function is intended for feeding legacy consumers that understand
 * only chromaticities.
 *
 * @return EXR_ERR_SUCCESS if @p color_interop_id has a defined mapping, in
 * which case @p chromaticities is filled in; EXR_ERR_NO_ATTR_BY_NAME for
 * any other ID, including "unknown" and "data", in which case
 * @p chromaticities is left unmodified; EXR_ERR_INVALID_ARGUMENT if either
 * argument is NULL.
 */
EXR_EXPORT exr_result_t exr_color_interop_id_to_chromaticities (
    const char* color_interop_id, exr_attr_chromaticities_t* chromaticities);

/** @brief Return the color interop ID denoting a set of chromaticities.
 *
 * The reverse of exr_color_interop_id_to_chromaticities(); the same six IDs
 * are the only possible results.
 *
 * A set of chromaticities matches an entry when all four coordinate pairs
 * are within +/- @p tolerance in both x and y. Pass
 * EXR_COLOR_INTEROP_ID_CHROMATICITIES_TOLERANCE unless you have reason not to.
 *
 * @return EXR_ERR_SUCCESS if the chromaticities match an entry, in which
 * case @p color_interop_id receives a pointer to a static string that must
 * not be freed; EXR_ERR_NO_ATTR_BY_NAME if they match no entry, in which
 * case @p color_interop_id is left unmodified; EXR_ERR_INVALID_ARGUMENT if
 * either pointer argument is NULL or @p tolerance is negative.
 */
EXR_EXPORT exr_result_t exr_chromaticities_to_color_interop_id (
    const exr_attr_chromaticities_t* chromaticities,
    float                            tolerance,
    const char**                     color_interop_id);

/** @brief Inconsistencies that exr_check_color_metadata() can report.
 *
 * None of these make a file malformed or unsafe to read. They are semantic
 * problems that leave a part's color space ambiguous, so they are reported
 * as warnings rather than as errors.
 *
 * The values are flags, and are returned OR'd together in a uint32_t.
 */
typedef enum exr_color_metadata_warning
{
    /** No inconsistency was found. */
    EXR_COLOR_METADATA_OK = 0,
    /** colorInteropID is present but empty; it should be omitted, or set
     * to "unknown", instead. */
    EXR_COLOR_METADATA_EMPTY_INTEROP_ID = 1 << 0,
    /** colorInteropID denotes a color space with defined chromaticities, the
     * part also has a chromaticities attribute, and they are different. */
    EXR_COLOR_METADATA_CHROMATICITIES_DIFFER = 1 << 1,
    /** colorInteropID is "data", marking the part as deliberately not color
     * managed, yet the part has a chromaticities attribute. */
    EXR_COLOR_METADATA_DATA_HAS_CHROMATICITIES = 1 << 2,
    /** colorInteropID is "data", yet the part has a whiteLuminance
     * attribute. */
    EXR_COLOR_METADATA_DATA_HAS_WHITE_LUMINANCE = 1 << 3,
    /** colorInteropID is "data", yet the part has an adoptedNeutral
     * attribute. */
    EXR_COLOR_METADATA_DATA_HAS_ADOPTED_NEUTRAL = 1 << 4,
    /** This part of a multipart file has a colorInteropID that is neither
     * "data" nor equal to the first part's, so it does not conform to the
     * shared attribute rules. */
    EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED = 1 << 5,
    /** acesImageContainerFlag is present but is not an int of value 1. It
     * is the only defined value; the attribute is a flag, and its presence
     * alone is not the assertion. */
    EXR_COLOR_METADATA_ACES_FLAG_NOT_ONE = 1 << 6,
    /** acesImageContainerFlag is present, so the part must be ACES2065-1,
     * but its colorInteropID is present and is not "lin_ap0_scene". */
    EXR_COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0 = 1 << 7,
    /** acesImageContainerFlag is present, but the part has no
     * chromaticities attribute. ST 2065-4 requires it. */
    EXR_COLOR_METADATA_ACES_FLAG_NO_CHROMATICITIES = 1 << 8,
    /** acesImageContainerFlag is present and the part has chromaticities,
     * but they are not those of "lin_ap0_scene". */
    EXR_COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0 = 1 << 9
} exr_color_metadata_warning_t;

/** @brief The color space metadata of one part, as input to
 * exr_check_color_metadata_values().
 *
 * All strings are NULL-terminated and are only read for the duration of the
 * call.
 */
typedef struct
{
    /** The part's colorInteropID, or NULL if it has none. */
    const char* color_interop_id;
    /** The part's chromaticities, or NULL if it has none. */
    const exr_attr_chromaticities_t* chromaticities;
    /** Non-zero if the part has a whiteLuminance attribute. */
    int has_white_luminance;
    /** Non-zero if the part has an adoptedNeutral attribute. */
    int has_adopted_neutral;
    /** Non-zero if the part has an acesImageContainerFlag attribute, of
     * any type. */
    int has_aces_image_container_flag;
    /** The value of the part's acesImageContainerFlag. Only read when
     * @p has_aces_image_container_flag is non-zero. Pass any value other
     * than 1 when the attribute is present but is not an int, since a
     * wrong type and a wrong value are reported the same way. */
    int32_t aces_image_container_flag;
    /** The first part's colorInteropID, or NULL if the first part has none.
     * Only read when @p compare_to_first_part is non-zero. */
    const char* first_part_color_interop_id;
    /** Non-zero to check the shared attribute rules against
     * @p first_part_color_interop_id. Leave this zero when checking the
     * first part of a file, or a single-part file, since there is then
     * nothing to compare against. */
    int compare_to_first_part;
} exr_color_metadata_t;

/** @brief Check a part's color space metadata for internal inconsistency.
 *
 * This is the rule implementation behind exr_check_color_metadata(). Prefer
 * that function when you have a context to check; use this one to check
 * metadata you are about to write, before any context exists.
 *
 * @param values The metadata to check.
 * @param warnings Receives zero or more exr_color_metadata_warning_t flags
 * OR'd together, or EXR_COLOR_METADATA_OK if nothing was found.
 *
 * @return EXR_ERR_SUCCESS whether or not warnings were found;
 * EXR_ERR_INVALID_ARGUMENT if either argument is NULL.
 */
EXR_EXPORT exr_result_t exr_check_color_metadata_values (
    const exr_color_metadata_t* values, uint32_t* warnings);

/** @brief Check one part of a file for inconsistent color space metadata.
 *
 * Reads only header attributes; no pixel data is decoded, and nothing is
 * allocated, so this is safe to call on untrusted files.
 *
 * When @p part_index is greater than zero, the part's colorInteropID is
 * additionally checked against the first part's, and
 * EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED may be reported. Note that a
 * context opened with EXR_CONTEXT_FLAG_STRICT_HEADER fails to parse such a
 * file outright, so that flag will report the same problem as an error
 * instead.
 *
 * @param ctxt The context to check.
 * @param part_index Which part of @p ctxt to check.
 * @param warnings Receives zero or more exr_color_metadata_warning_t flags
 * OR'd together, or EXR_COLOR_METADATA_OK if nothing was found.
 *
 * @return EXR_ERR_SUCCESS whether or not warnings were found;
 * EXR_ERR_INVALID_ARGUMENT if @p warnings is NULL;
 * EXR_ERR_INCORRECT_PART if @p part_index is out of range.
 */
EXR_EXPORT exr_result_t exr_check_color_metadata (
    exr_const_context_t ctxt, int part_index, uint32_t* warnings);

/** @brief Return a static string describing one color metadata warning.
 *
 * The string should not be freed (it is compiled into the binary). It
 * describes a single flag, so callers reporting a set of warnings should
 * test each flag in turn rather than passing the combined value.
 *
 * @return A description of @p warning, or a placeholder for
 * EXR_COLOR_METADATA_OK and for values that are not a single known flag.
 */
EXR_EXPORT const char* exr_get_color_metadata_warning_as_string (
    exr_color_metadata_warning_t warning);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_COLOR_METADATA_H */
