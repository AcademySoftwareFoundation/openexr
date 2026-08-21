// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include "color_metadata.h"

#include "test_value.h"
#include "openexr.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <vector>

namespace
{

const int width = 4;

const char* const allIDs[] = {
    "lin_rec709_scene",
    "lin_ap0_scene",
    "lin_ap1_scene",
    "lin_p3d65_scene",
    "lin_rec2020_scene",
    "lin_adobergb_scene"};

const size_t numIDs = sizeof (allIDs) / sizeof (allIDs[0]);

//
// Write a two part file whose parts have the given colorInteropID values,
// passing NULL to leave the attribute off a part.
//
// The C++ writer refuses to produce a file whose parts have conflicting
// colorInteropID values, so use the core API, which does not validate
// shared attributes on write. Every chunk of every part has to be written:
// the core treats a write left incomplete as failed, and discards the file.
//
void
writeTwoPart (const std::string& fn, const char* firstID, const char* secondID)
{
    const char* names[]  = {"first", "second"};
    const char* ids[]    = {firstID, secondID};
    const int   numParts = 2;

    exr_context_t             f     = NULL;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;

    remove (fn.c_str ());

    EXRCORE_TEST_RVAL (
        exr_start_write (&f, fn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));

    for (int p = 0; p < numParts; p++)
    {
        int partidx = -1;
        EXRCORE_TEST_RVAL (
            exr_add_part (f, names[p], EXR_STORAGE_SCANLINE, &partidx));

        // a single uncompressed scanline, so that each part is one chunk
        exr_attr_box2i_t dataw = {{0, 0}, {width - 1, 0}};
        exr_attr_box2i_t dispw = {{0, 0}, {width - 1, 0}};
        exr_attr_v2f_t   swc   = {0.f, 0.f};

        EXRCORE_TEST_RVAL (exr_initialize_required_attr (
            f,
            partidx,
            &dataw,
            &dispw,
            1.f,
            &swc,
            1.f,
            EXR_LINEORDER_INCREASING_Y,
            EXR_COMPRESSION_NONE));

        EXRCORE_TEST_RVAL (exr_add_channel (
            f,
            partidx,
            "R",
            EXR_PIXEL_HALF,
            EXR_PERCEPTUALLY_LOGARITHMIC,
            1,
            1));

        if (ids[p])
            EXRCORE_TEST_RVAL (
                exr_attr_set_string (f, partidx, "colorInteropID", ids[p]));
    }

    EXRCORE_TEST_RVAL (exr_write_header (f));

    std::vector<uint16_t> pixels (width, 0);

    for (int p = 0; p < numParts; p++)
    {
        exr_chunk_info_t      cinfo;
        exr_encode_pipeline_t encoder;

        EXRCORE_TEST_RVAL (exr_write_scanline_chunk_info (f, p, 0, &cinfo));
        EXRCORE_TEST_RVAL (exr_encoding_initialize (f, p, &cinfo, &encoder));

        encoder.channels[0].encode_from_ptr =
            reinterpret_cast<const uint8_t*> (pixels.data ());
        encoder.channels[0].user_pixel_stride =
            static_cast<int32_t> (sizeof (uint16_t));
        encoder.channels[0].user_line_stride =
            static_cast<int32_t> (sizeof (uint16_t) * width);

        EXRCORE_TEST_RVAL (
            exr_encoding_choose_default_routines (f, p, &encoder));
        EXRCORE_TEST_RVAL (exr_encoding_run (f, p, &encoder));
        EXRCORE_TEST_RVAL (exr_encoding_destroy (f, &encoder));
    }

    EXRCORE_TEST_RVAL (exr_finish (&f));
}

//
// Check both parts of a file written by writeTwoPart.
//
void
expectWarnings (
    const std::string& fn, uint32_t firstExpected, uint32_t secondExpected)
{
    exr_context_t             f     = NULL;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    uint32_t                  warnings;

    cinit.flags |= EXR_CONTEXT_FLAG_SILENT_HEADER_PARSE;

    EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));

    warnings = ~0u;
    EXRCORE_TEST_RVAL (exr_check_color_metadata (f, 0, &warnings));
    EXRCORE_TEST (warnings == firstExpected);

    warnings = ~0u;
    EXRCORE_TEST_RVAL (exr_check_color_metadata (f, 1, &warnings));
    EXRCORE_TEST (warnings == secondExpected);

    EXRCORE_TEST_RVAL (exr_finish (&f));

    remove (fn.c_str ());
}

} // namespace

void
testColorInteropChromaticities (const std::string& tempdir)
{
    exr_attr_chromaticities_t chroma;
    const char*               id = NULL;

    //
    // Every supported ID round trips, and the six entries are distinct.
    //

    for (size_t i = 0; i < numIDs; i++)
    {
        EXRCORE_TEST_RVAL (
            exr_color_interop_id_to_chromaticities (allIDs[i], &chroma));

        id = NULL;
        EXRCORE_TEST_RVAL (exr_chromaticities_to_color_interop_id (
            &chroma,
            EXR_COLOR_INTEROP_ID_CHROMATICITIES_TOLERANCE,
            &id));
        EXRCORE_TEST (id != NULL);
        EXRCORE_TEST (0 == strcmp (id, allIDs[i]));

        for (size_t j = 0; j < i; j++)
        {
            exr_attr_chromaticities_t other;
            EXRCORE_TEST_RVAL (
                exr_color_interop_id_to_chromaticities (allIDs[j], &other));
            EXRCORE_TEST (
                0 != memcmp (&other, &chroma, sizeof (chroma)));
        }
    }

    //
    // Spot check the table values, including the negative blue y of
    // lin_ap0_scene, since this is the library's one definition of them.
    //

    EXRCORE_TEST_RVAL (
        exr_color_interop_id_to_chromaticities ("lin_ap0_scene", &chroma));
    EXRCORE_TEST (chroma.red_x == 0.73470f);
    EXRCORE_TEST (chroma.red_y == 0.26530f);
    EXRCORE_TEST (chroma.green_x == 0.00000f);
    EXRCORE_TEST (chroma.green_y == 1.00000f);
    EXRCORE_TEST (chroma.blue_x == 0.00010f);
    EXRCORE_TEST (chroma.blue_y == -0.07700f);
    EXRCORE_TEST (chroma.white_x == 0.32168f);
    EXRCORE_TEST (chroma.white_y == 0.33767f);

    //
    // IDs with no defined mapping, including the two reserved values.
    //

    {
        exr_attr_chromaticities_t untouched = chroma;

        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_NO_ATTR_BY_NAME,
            exr_color_interop_id_to_chromaticities ("unknown", &chroma));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_NO_ATTR_BY_NAME,
            exr_color_interop_id_to_chromaticities ("data", &chroma));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_NO_ATTR_BY_NAME,
            exr_color_interop_id_to_chromaticities ("", &chroma));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_NO_ATTR_BY_NAME,
            exr_color_interop_id_to_chromaticities (
                "ocio:lin_awg4_scene", &chroma));

        // a failed lookup leaves the output alone
        EXRCORE_TEST (0 == memcmp (&untouched, &chroma, sizeof (chroma)));
    }

    //
    // Chromaticities matching no entry.
    //

    {
        exr_attr_chromaticities_t nonsense = {
            0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f};

        id = NULL;
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_NO_ATTR_BY_NAME,
            exr_chromaticities_to_color_interop_id (
                &nonsense,
                EXR_COLOR_INTEROP_ID_CHROMATICITIES_TOLERANCE,
                &id));
        EXRCORE_TEST (id == NULL);
    }

    //
    // Matching is within +/- tolerance in x and y, on every coordinate.
    //

    {
        const float             offset = 0.0009f;
        exr_attr_chromaticities_t nudged;

        EXRCORE_TEST_RVAL (exr_color_interop_id_to_chromaticities (
            "lin_p3d65_scene", &nudged));
        nudged.red_x += offset;
        nudged.green_y -= offset;
        nudged.white_x += offset;

        id = NULL;
        EXRCORE_TEST_RVAL (exr_chromaticities_to_color_interop_id (
            &nudged,
            EXR_COLOR_INTEROP_ID_CHROMATICITIES_TOLERANCE,
            &id));
        EXRCORE_TEST (0 == strcmp (id, "lin_p3d65_scene"));

        // and just outside it does not match at all
        nudged.red_x += 0.01f;
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_NO_ATTR_BY_NAME,
            exr_chromaticities_to_color_interop_id (
                &nudged,
                EXR_COLOR_INTEROP_ID_CHROMATICITIES_TOLERANCE,
                &id));
    }

    //
    // lin_rec709_scene and lin_adobergb_scene differ only in green, so a
    // tolerance loose enough to span that difference matches whichever of
    // the two comes first in the table.
    //

    {
        exr_attr_chromaticities_t adobergb;

        EXRCORE_TEST_RVAL (exr_color_interop_id_to_chromaticities (
            "lin_adobergb_scene", &adobergb));

        id = NULL;
        EXRCORE_TEST_RVAL (
            exr_chromaticities_to_color_interop_id (&adobergb, 0.2f, &id));
        EXRCORE_TEST (0 == strcmp (id, "lin_rec709_scene"));
    }

    //
    // Bad arguments.
    //

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_color_interop_id_to_chromaticities (NULL, &chroma));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_color_interop_id_to_chromaticities ("lin_ap0_scene", NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_chromaticities_to_color_interop_id (NULL, 0.001f, &id));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_chromaticities_to_color_interop_id (&chroma, 0.001f, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_chromaticities_to_color_interop_id (&chroma, -0.001f, &id));
}

void
testColorMetadataValues (const std::string& tempdir)
{
    exr_attr_chromaticities_t ap1;
    exr_attr_chromaticities_t rec709;
    exr_attr_chromaticities_t nonsense = {
        0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f};
    exr_color_metadata_t      values;
    uint32_t                  warnings;

    EXRCORE_TEST_RVAL (
        exr_color_interop_id_to_chromaticities ("lin_ap1_scene", &ap1));
    EXRCORE_TEST_RVAL (
        exr_color_interop_id_to_chromaticities ("lin_rec709_scene", &rec709));

    //
    // With no colorInteropID, the chromaticities are not checked.
    //

    memset (&values, 0, sizeof (values));
    values.chromaticities      = &nonsense;
    values.has_white_luminance = 1;
    values.has_adopted_neutral = 1;
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (warnings == EXR_COLOR_METADATA_OK);

    //
    // An ID that agrees with the chromaticities is clean.
    //

    memset (&values, 0, sizeof (values));
    values.color_interop_id = "lin_ap1_scene";
    values.chromaticities   = &ap1;
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (warnings == EXR_COLOR_METADATA_OK);

    //
    // An ID with no chromaticities mapping is not checked against them.
    //

    memset (&values, 0, sizeof (values));
    values.color_interop_id = "ocio:lin_awg4_scene";
    values.chromaticities   = &rec709;
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (warnings == EXR_COLOR_METADATA_OK);

    //
    // An ID that disagrees with the chromaticities is reported.
    //

    memset (&values, 0, sizeof (values));
    values.color_interop_id = "lin_ap1_scene";
    values.chromaticities   = &rec709;
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (warnings == EXR_COLOR_METADATA_CHROMATICITIES_DIFFER);

    //
    // Chromaticities that match no known color space at all are the same
    // finding: they are not the ones the ID names.
    //

    {
        memset (&values, 0, sizeof (values));
        values.color_interop_id = "lin_ap1_scene";
        values.chromaticities   = &nonsense;
        EXRCORE_TEST_RVAL (
            exr_check_color_metadata_values (&values, &warnings));
        EXRCORE_TEST (
            warnings == EXR_COLOR_METADATA_CHROMATICITIES_DIFFER);
    }

    //
    // An empty ID says nothing; it should be omitted, or "unknown", instead.
    //

    memset (&values, 0, sizeof (values));
    values.color_interop_id = "";
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (warnings == EXR_COLOR_METADATA_EMPTY_INTEROP_ID);

    //
    // A part tagged "data" is deliberately not color managed, so each
    // colorimetry attribute on it is reported, and they combine.
    //

    memset (&values, 0, sizeof (values));
    values.color_interop_id = "data";
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (warnings == EXR_COLOR_METADATA_OK);

    memset (&values, 0, sizeof (values));
    values.color_interop_id    = "data";
    values.chromaticities      = &ap1;
    values.has_white_luminance = 1;
    values.has_adopted_neutral = 1;
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (
        warnings ==
        (EXR_COLOR_METADATA_DATA_HAS_CHROMATICITIES |
         EXR_COLOR_METADATA_DATA_HAS_WHITE_LUMINANCE |
         EXR_COLOR_METADATA_DATA_HAS_ADOPTED_NEUTRAL));

    //
    // A later part may omit colorInteropID, inheriting the first part's, or
    // set it to "data", or repeat it.
    //

    memset (&values, 0, sizeof (values));
    values.compare_to_first_part       = 1;
    values.first_part_color_interop_id = "lin_ap1_scene";
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (warnings == EXR_COLOR_METADATA_OK);

    values.color_interop_id = "data";
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (warnings == EXR_COLOR_METADATA_OK);

    values.color_interop_id = "lin_ap1_scene";
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (warnings == EXR_COLOR_METADATA_OK);

    //
    // But any other value that differs from the first part's leaves the
    // part's color space ambiguous, as does a first part with no ID.
    //

    values.color_interop_id = "lin_ap0_scene";
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (warnings == EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED);

    values.first_part_color_interop_id = NULL;
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (warnings == EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED);

    //
    // The shared attribute rule is only applied when asked for.
    //

    values.compare_to_first_part = 0;
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (warnings == EXR_COLOR_METADATA_OK);

    //
    // Per-part and shared problems are reported together.
    //

    memset (&values, 0, sizeof (values));
    values.color_interop_id            = "lin_ap0_scene";
    values.chromaticities              = &rec709;
    values.compare_to_first_part       = 1;
    values.first_part_color_interop_id = "lin_ap1_scene";
    EXRCORE_TEST_RVAL (exr_check_color_metadata_values (&values, &warnings));
    EXRCORE_TEST (
        warnings ==
        (EXR_COLOR_METADATA_CHROMATICITIES_DIFFER |
         EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED));

    //
    // acesImageContainerFlag asserts ST 2065-4 compliance, which requires the
    // color space be ACES2065-1. A file that says so consistently is clean.
    //

    {
        exr_attr_chromaticities_t ap0;

        EXRCORE_TEST_RVAL (
            exr_color_interop_id_to_chromaticities ("lin_ap0_scene", &ap0));

        memset (&values, 0, sizeof (values));
        values.has_aces_image_container_flag = 1;
        values.aces_image_container_flag     = 1;
        values.color_interop_id              = "lin_ap0_scene";
        values.chromaticities                = &ap0;
        EXRCORE_TEST_RVAL (
            exr_check_color_metadata_values (&values, &warnings));
        EXRCORE_TEST (warnings == EXR_COLOR_METADATA_OK);

        //
        // The colorInteropID may be omitted; the chromaticities may not.
        //

        values.color_interop_id = NULL;
        EXRCORE_TEST_RVAL (
            exr_check_color_metadata_values (&values, &warnings));
        EXRCORE_TEST (warnings == EXR_COLOR_METADATA_OK);

        values.chromaticities = NULL;
        EXRCORE_TEST_RVAL (
            exr_check_color_metadata_values (&values, &warnings));
        EXRCORE_TEST (
            warnings == EXR_COLOR_METADATA_ACES_FLAG_NO_CHROMATICITIES);

        //
        // Chromaticities of some other color space, or of none.
        //

        values.chromaticities = &rec709;
        EXRCORE_TEST_RVAL (
            exr_check_color_metadata_values (&values, &warnings));
        EXRCORE_TEST (
            warnings == EXR_COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0);

        {
            values.chromaticities = &nonsense;
            EXRCORE_TEST_RVAL (
                exr_check_color_metadata_values (&values, &warnings));
            EXRCORE_TEST (
                warnings ==
                EXR_COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0);
        }

        //
        // A present colorInteropID has to agree. Here the ID and the
        // chromaticities agree with each other, so only the ACES rule is
        // broken, twice.
        //

        memset (&values, 0, sizeof (values));
        values.has_aces_image_container_flag = 1;
        values.aces_image_container_flag     = 1;
        values.color_interop_id              = "lin_rec709_scene";
        values.chromaticities                = &rec709;
        EXRCORE_TEST_RVAL (
            exr_check_color_metadata_values (&values, &warnings));
        EXRCORE_TEST (
            warnings ==
            (EXR_COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0 |
             EXR_COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0));

        //
        // 1 is the only defined value of the flag. A caller that saw a
        // present attribute of the wrong type passes a value other than 1,
        // and gets the same report.
        //

        memset (&values, 0, sizeof (values));
        values.has_aces_image_container_flag = 1;
        values.aces_image_container_flag     = 0;
        values.color_interop_id              = "lin_ap0_scene";
        values.chromaticities                = &ap0;
        EXRCORE_TEST_RVAL (
            exr_check_color_metadata_values (&values, &warnings));
        EXRCORE_TEST (warnings == EXR_COLOR_METADATA_ACES_FLAG_NOT_ONE);

        values.aces_image_container_flag = 2;
        EXRCORE_TEST_RVAL (
            exr_check_color_metadata_values (&values, &warnings));
        EXRCORE_TEST (warnings == EXR_COLOR_METADATA_ACES_FLAG_NOT_ONE);

        //
        // Without the flag, none of this is checked: ap0 chromaticities with
        // a rec709 ID is just an ordinary disagreement, and the absence of
        // chromaticities is unremarkable.
        //

        memset (&values, 0, sizeof (values));
        values.color_interop_id = "lin_rec709_scene";
        values.chromaticities   = &ap0;
        EXRCORE_TEST_RVAL (
            exr_check_color_metadata_values (&values, &warnings));
        EXRCORE_TEST (warnings == EXR_COLOR_METADATA_CHROMATICITIES_DIFFER);

        //
        // A part tagged "data" cannot also be an ACES container.
        //

        memset (&values, 0, sizeof (values));
        values.has_aces_image_container_flag = 1;
        values.aces_image_container_flag     = 1;
        values.color_interop_id              = "data";
        values.chromaticities                = &ap0;
        EXRCORE_TEST_RVAL (
            exr_check_color_metadata_values (&values, &warnings));
        EXRCORE_TEST (
            warnings ==
            (EXR_COLOR_METADATA_DATA_HAS_CHROMATICITIES |
             EXR_COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0));
    }

    //
    // Bad arguments.
    //

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_check_color_metadata_values (NULL, &warnings));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_check_color_metadata_values (&values, NULL));

    //
    // Every flag has a distinct description.
    //

    {
        static const exr_color_metadata_warning_t all[] = {
            EXR_COLOR_METADATA_EMPTY_INTEROP_ID,
            EXR_COLOR_METADATA_CHROMATICITIES_DIFFER,
            EXR_COLOR_METADATA_DATA_HAS_CHROMATICITIES,
            EXR_COLOR_METADATA_DATA_HAS_WHITE_LUMINANCE,
            EXR_COLOR_METADATA_DATA_HAS_ADOPTED_NEUTRAL,
            EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED,
            EXR_COLOR_METADATA_ACES_FLAG_NOT_ONE,
            EXR_COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0,
            EXR_COLOR_METADATA_ACES_FLAG_NO_CHROMATICITIES,
            EXR_COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0};
        static const size_t numAll = sizeof (all) / sizeof (all[0]);

        for (size_t i = 0; i < numAll; i++)
        {
            const char* s = exr_get_color_metadata_warning_as_string (all[i]);
            EXRCORE_TEST (s != NULL);
            EXRCORE_TEST (s[0] != '\0');

            for (size_t j = 0; j < i; j++)
                EXRCORE_TEST (
                    0 !=
                    strcmp (
                        s,
                        exr_get_color_metadata_warning_as_string (all[j])));
        }
    }
}

void
testColorMetadataFile (const std::string& tempdir)
{
    //
    // A later part may omit colorInteropID, set it to "data", or repeat the
    // first part's.
    //

    writeTwoPart (tempdir + "core_cif_omitted.exr", "lin_ap1_scene", NULL);
    expectWarnings (
        tempdir + "core_cif_omitted.exr",
        EXR_COLOR_METADATA_OK,
        EXR_COLOR_METADATA_OK);

    writeTwoPart (tempdir + "core_cif_data.exr", "lin_ap1_scene", "data");
    expectWarnings (
        tempdir + "core_cif_data.exr",
        EXR_COLOR_METADATA_OK,
        EXR_COLOR_METADATA_OK);

    writeTwoPart (
        tempdir + "core_cif_same.exr", "lin_ap1_scene", "lin_ap1_scene");
    expectWarnings (
        tempdir + "core_cif_same.exr",
        EXR_COLOR_METADATA_OK,
        EXR_COLOR_METADATA_OK);

    //
    // But any other value that differs from the first part's is reported
    // against the later part, not the first.
    //

    writeTwoPart (
        tempdir + "core_cif_conflict.exr", "lin_ap1_scene", "lin_ap0_scene");
    expectWarnings (
        tempdir + "core_cif_conflict.exr",
        EXR_COLOR_METADATA_OK,
        EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED);

    writeTwoPart (tempdir + "core_cif_nofirst.exr", NULL, "lin_ap1_scene");
    expectWarnings (
        tempdir + "core_cif_nofirst.exr",
        EXR_COLOR_METADATA_OK,
        EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED);

    //
    // An empty ID on a later part is both empty and non-conforming.
    //

    writeTwoPart (tempdir + "core_cif_empty.exr", "lin_ap1_scene", "");
    expectWarnings (
        tempdir + "core_cif_empty.exr",
        EXR_COLOR_METADATA_OK,
        EXR_COLOR_METADATA_EMPTY_INTEROP_ID |
            EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED);

    //
    // Bad arguments against a real context.
    //

    {
        const std::string         fn    = tempdir + "core_cif_args.exr";
        exr_context_t             f     = NULL;
        exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
        uint32_t                  warnings;

        cinit.flags |= EXR_CONTEXT_FLAG_SILENT_HEADER_PARSE;

        writeTwoPart (fn, "lin_ap1_scene", NULL);

        EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));

        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_INVALID_ARGUMENT, exr_check_color_metadata (f, 0, NULL));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_INCORRECT_PART,
            exr_check_color_metadata (f, -1, &warnings));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_INCORRECT_PART,
            exr_check_color_metadata (f, 2, &warnings));

        EXRCORE_TEST_RVAL (exr_finish (&f));

        remove (fn.c_str ());
    }
}
