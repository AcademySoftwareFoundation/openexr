//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "testCheckFileWarnings.h"

#include "openexr.h"

#include "ImfArray.h"
#include "ImfChannelList.h"
#include "ImfCheckFile.h"
#include "ImfChromaticities.h"
#include "ImfFrameBuffer.h"
#include "ImfHeader.h"
#include "ImfOutputFile.h"
#include "ImfPartType.h"
#include "ImfStandardAttributes.h"

#include <assert.h>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace IMF;
using namespace std;

namespace
{

const int width  = 4;
const int height = 4;

//
// write a single part file with the given header
//
void
writeSinglePart (const std::string& fn, const Header& header)
{
    Header h (header);
    h.dataWindow ()    = IMATH_NAMESPACE::Box2i (
        IMATH_NAMESPACE::V2i (0, 0),
        IMATH_NAMESPACE::V2i (width - 1, height - 1));
    h.displayWindow () = h.dataWindow ();
    h.channels ().insert ("R", Channel (IMF::HALF));

    vector<half> pixels (width * height, half (0.f));

    FrameBuffer fb;
    fb.insert (
        "R",
        Slice (
            IMF::HALF,
            reinterpret_cast<char*> (pixels.data ()),
            sizeof (half),
            sizeof (half) * width));

    remove (fn.c_str ());

    OutputFile out (fn.c_str (), h);
    out.setFrameBuffer (fb);
    out.writePixels (height);
}

//
// Write a two part file whose second part has the given colorInteropID.
//
// The C++ writer refuses to produce a file whose parts have conflicting
// colorInteropID values, so use the core API, which does not validate
// shared attributes on write. Every chunk of every part has to be written:
// the core treats a write left incomplete as failed, and discards the file.
//
void
writeTwoPart (
    const std::string& fn, const char* firstID, const char* secondID)
{
    const char* names[] = {"first", "second"};
    const char* ids[]   = {firstID, secondID};
    const int   numParts = 2;

    exr_context_t             f     = nullptr;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;

    remove (fn.c_str ());

    assert (
        exr_start_write (&f, fn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit) ==
        EXR_ERR_SUCCESS);

    for (int p = 0; p < numParts; p++)
    {
        int partidx = -1;
        assert (
            exr_add_part (f, names[p], EXR_STORAGE_SCANLINE, &partidx) ==
            EXR_ERR_SUCCESS);

        // a single uncompressed scanline, so that each part is one chunk
        exr_attr_box2i_t dataw = {{0, 0}, {width - 1, 0}};
        exr_attr_box2i_t dispw = {{0, 0}, {width - 1, 0}};
        exr_attr_v2f_t   swc   = {0.f, 0.f};

        assert (
            exr_initialize_required_attr (
                f,
                partidx,
                &dataw,
                &dispw,
                1.f,
                &swc,
                1.f,
                EXR_LINEORDER_INCREASING_Y,
                EXR_COMPRESSION_NONE) == EXR_ERR_SUCCESS);

        assert (
            exr_add_channel (
                f,
                partidx,
                "R",
                EXR_PIXEL_HALF,
                EXR_PERCEPTUALLY_LOGARITHMIC,
                1,
                1) == EXR_ERR_SUCCESS);

        if (ids[p])
            assert (
                exr_attr_set_string (f, partidx, "colorInteropID", ids[p]) ==
                EXR_ERR_SUCCESS);
    }

    assert (exr_write_header (f) == EXR_ERR_SUCCESS);

    vector<uint16_t> pixels (width, 0);

    for (int p = 0; p < numParts; p++)
    {
        exr_chunk_info_t      cinfo;
        exr_encode_pipeline_t encoder;

        assert (
            exr_write_scanline_chunk_info (f, p, 0, &cinfo) ==
            EXR_ERR_SUCCESS);
        assert (
            exr_encoding_initialize (f, p, &cinfo, &encoder) ==
            EXR_ERR_SUCCESS);

        encoder.channels[0].encode_from_ptr =
            reinterpret_cast<const uint8_t*> (pixels.data ());
        encoder.channels[0].user_pixel_stride =
            static_cast<int32_t> (sizeof (uint16_t));
        encoder.channels[0].user_line_stride =
            static_cast<int32_t> (sizeof (uint16_t) * width);

        assert (
            exr_encoding_choose_default_routines (f, p, &encoder) ==
            EXR_ERR_SUCCESS);
        assert (exr_encoding_run (f, p, &encoder) == EXR_ERR_SUCCESS);
        assert (exr_encoding_destroy (f, &encoder) == EXR_ERR_SUCCESS);
    }

    assert (exr_finish (&f) == EXR_ERR_SUCCESS);
}

//
// Check a file, asserting that it reads correctly and that the number of
// warnings and the text of the first one are as expected.
//
void
expectWarnings (
    const std::string& fn, size_t count, const char* firstContains = nullptr)
{
    vector<string> warnings;

    bool hadError = checkOpenEXRFile (fn.c_str (), warnings);

    if (hadError)
    {
        cerr << "ERROR -- " << fn << " should read correctly" << endl;
        assert (false);
    }

    if (warnings.size () != count)
    {
        cerr << "ERROR -- " << fn << " : expected " << count
             << " warning(s), got " << warnings.size () << endl;
        for (size_t i = 0; i < warnings.size (); i++)
            cerr << "  " << warnings[i] << endl;
        assert (false);
    }

    if (firstContains)
    {
        if (warnings[0].find (firstContains) == string::npos)
        {
            cerr << "ERROR -- " << fn << " : expected a warning containing '"
                 << firstContains << "', got '" << warnings[0] << "'" << endl;
            assert (false);
        }
    }

    remove (fn.c_str ());
}

} // namespace

void
testCheckFileWarnings (const std::string& tempDir)
{
    cout << "Testing color metadata warnings from checkOpenEXRFile" << endl;

    Chromaticities ap1;
    Chromaticities rec709;
    assert (colorInteropIDToChromaticities ("lin_ap1_scene", ap1));
    assert (colorInteropIDToChromaticities ("lin_rec709_scene", rec709));

    //
    // A colorInteropID that agrees with the chromaticities is clean.
    //

    {
        Header h;
        addColorInteropID (h, "lin_ap1_scene");
        addChromaticities (h, ap1);
        std::string fn = tempDir + "checkwarn_clean.exr";
        writeSinglePart (fn, h);
        expectWarnings (fn, 0);
    }

    //
    // An ID with no chromaticities mapping is not checked against them.
    //

    {
        Header h;
        addColorInteropID (h, "ocio:lin_awg4_scene");
        addChromaticities (h, rec709);
        std::string fn = tempDir + "checkwarn_no_match.exr";
        writeSinglePart (fn, h);
        expectWarnings (fn, 0);
    }

    //
    // An ID that disagrees with the chromaticities is a warning.
    //

    {
        Header h;
        addColorInteropID (h, "lin_ap1_scene");
        addChromaticities (h, rec709);
        std::string fn = tempDir + "checkwarn_mismatch.exr";
        writeSinglePart (fn, h);
        expectWarnings (fn, 1, "chromaticities are those of 'lin_rec709_scene'");
    }

    //
    // A part tagged "data" should not carry colorimetry attributes.
    //

    {
        Header h;
        addColorInteropID (h, "data");
        addChromaticities (h, ap1);
        std::string fn = tempDir + "checkwarn_data.exr";
        writeSinglePart (fn, h);
        expectWarnings (fn, 1, "'data' but the part has a chromaticities");
    }

    //
    // An empty ID warns.
    //

    {
        Header h;
        addColorInteropID (h, "");
        std::string fn = tempDir + "checkwarn_empty.exr";
        writeSinglePart (fn, h);
        expectWarnings (fn, 1, "colorInteropID is empty");
    }

    //
    // A later part may omit colorInteropID, or set it to "data".
    //

    {
        std::string fn = tempDir + "checkwarn_mp_omitted.exr";
        writeTwoPart (fn, "lin_ap1_scene", nullptr);
        expectWarnings (fn, 0);
    }

    {
        std::string fn = tempDir + "checkwarn_mp_data.exr";
        writeTwoPart (fn, "lin_ap1_scene", "data");
        expectWarnings (fn, 0);
    }

    {
        std::string fn = tempDir + "checkwarn_mp_same.exr";
        writeTwoPart (fn, "lin_ap1_scene", "lin_ap1_scene");
        expectWarnings (fn, 0);
    }

    //
    // But any other value that differs from the first part's is a warning,
    // but such a file still reads.
    //

    {
        std::string fn = tempDir + "checkwarn_mp_conflict.exr";
        writeTwoPart (fn, "lin_ap1_scene", "lin_ap0_scene");
        expectWarnings (fn, 1, "part 0's is 'lin_ap1_scene'");
    }

    {
        std::string fn = tempDir + "checkwarn_mp_nofirst.exr";
        writeTwoPart (fn, nullptr, "lin_ap1_scene");
        expectWarnings (fn, 1, "but part 0 has none");
    }

    cout << " ... ok\n" << endl;
}
