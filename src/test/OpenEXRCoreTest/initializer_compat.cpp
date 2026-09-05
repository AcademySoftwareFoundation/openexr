// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include "initializer_compat.h"

#include "openexr.h"
#include "../../lib/OpenEXRCore/backward_compatibility.h"
#include "test_value.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <string>

void
testInitializerCompat (const std::string& tempdir)
{
    // Library-wide defaults used when a caller's initializer does not include
    // the v4 fields (lossy_htj2k_quality, zstd_level).
    float default_lossy_htj2k_quality = -1.f;
    int   default_zstd_level          = -1;
    exr_get_default_lossy_htj2k_quality (&default_lossy_htj2k_quality);
    exr_get_default_zstd_compression_level (&default_zstd_level);

    // Pin struct layout on 64-bit LP64 only: absolute sizes (104 / 112) depend
    // on 8-byte function pointers. Runtime behavior tests below use sizeof()
    // and work on all platforms.
#if INTPTR_MAX == INT64_MAX
    EXRCORE_TEST (sizeof (struct _exr_context_initializer_v3) == 104);
    EXRCORE_TEST (sizeof (exr_context_initializer_t) == 112);
    // flags must stay at the same offset across v3 and the current struct.
    EXRCORE_TEST (
        offsetof (exr_context_initializer_t, flags) ==
        offsetof (struct _exr_context_initializer_v3, flags));
    // lossy_htj2k_quality replaced the v3 reserved pad at the same offset.
    EXRCORE_TEST (
        offsetof (exr_context_initializer_t, lossy_htj2k_quality) ==
        offsetof (struct _exr_context_initializer_v3, pad));
    EXRCORE_TEST (
        sizeof (struct _exr_context_initializer_v3) <
        sizeof (exr_context_initializer_t));
#endif

    // Simulate an application built against the v3 initializer API: it passes
    // size = sizeof(v3) and never knew about lossy_htj2k_quality or zstd_level.
    // Only poison the reserved pad bytes (not the whole struct — memset(0xFF)
    // on function pointers would make the library call invalid addresses).
    {
        struct _exr_context_initializer_v3 old = {0};
        old.size                               = sizeof (old);
        old.flags                              = EXR_CONTEXT_FLAG_STRICT_HEADER;
        memset (old.pad, 0xFF, sizeof (old.pad));
        float fake_quality = 42.f;
        memcpy (old.pad, &fake_quality, sizeof (fake_quality));

        exr_context_t ctxt = NULL;
        std::string   fn   = tempdir + "v3_init_compat.exr";
        int           partidx;

        EXRCORE_TEST_RVAL (exr_start_write (
            &ctxt,
            fn.c_str (),
            EXR_WRITE_FILE_DIRECTLY,
            (const exr_context_initializer_t*) &old));
        EXRCORE_TEST_RVAL (
            exr_add_part (ctxt, "beauty", EXR_STORAGE_SCANLINE, &partidx));

        float hq = -1.f;
        int   zl = -1;
        EXRCORE_TEST_RVAL (exr_get_lossy_htj2k_quality (ctxt, 0, &hq));
        EXRCORE_TEST_RVAL (exr_get_zstd_compression_level (ctxt, 0, &zl));
        // fill_context_data() must not read pad as lossy_htj2k_quality when
        // size < sizeof(v4); part levels should match library defaults.
        EXRCORE_TEST (hq == default_lossy_htj2k_quality);
        EXRCORE_TEST (zl == default_zstd_level);

        EXRCORE_TEST_RVAL (exr_finish (&ctxt));
    }

    // Positive control: a v4-sized initializer with explicit levels must
    // propagate them to new parts (not the library defaults).
    {
        const float explicit_lossy_quality = 77.f;
        const int   explicit_zstd_level    = 10;

        exr_context_initializer_t cur = EXR_DEFAULT_CONTEXT_INITIALIZER;
        cur.lossy_htj2k_quality       = explicit_lossy_quality;
        cur.zstd_level                = explicit_zstd_level;

        exr_context_t ctxt = NULL;
        std::string   fn   = tempdir + "v4_init_compat.exr";
        int           partidx;

        EXRCORE_TEST_RVAL (exr_start_write (
            &ctxt, fn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cur));
        EXRCORE_TEST_RVAL (
            exr_add_part (ctxt, "beauty", EXR_STORAGE_SCANLINE, &partidx));

        float hq = -1.f;
        int   zl = -1;
        EXRCORE_TEST_RVAL (exr_get_lossy_htj2k_quality (ctxt, 0, &hq));
        EXRCORE_TEST_RVAL (exr_get_zstd_compression_level (ctxt, 0, &zl));
        EXRCORE_TEST (hq == explicit_lossy_quality);
        EXRCORE_TEST (zl == explicit_zstd_level);
        EXRCORE_TEST (hq != default_lossy_htj2k_quality);
        EXRCORE_TEST (zl != default_zstd_level);

        EXRCORE_TEST_RVAL (exr_finish (&ctxt));
    }
}
