/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#if (defined(_WIN32) || defined(_WIN64))
#    ifdef NOMINMAX
#        undef NOMINMAX
#    endif
#    define NOMINMAX
#endif

#include <openexr.h>

#include "base_units.h"

#include "test_value.h"

#include <cstring>
#include <iostream>

#include <ImfSystemSpecific.h>
#include <ImfNamespace.h>
#include "../../lib/OpenEXRCore/internal_cpuid.h"
#include "../../lib/OpenEXRCore/internal_coding.h"
#include "../../lib/OpenEXRCore/openexr_context.h"
#include "../../lib/OpenEXRCore/openexr_part.h"

void
testBase (const std::string& tempdir)
{
    int         major, minor, patch;
    const char* extra;
    const char* compextra = COMP_EXTRA;

    exr_get_library_version (&major, &minor, &patch, &extra);
    if (major != COMP_MAJ || minor != COMP_MIN || patch != COMP_PATCH ||
        strcmp (extra, compextra))
    {
        std::cerr << "ERROR testing library, wrong library version: '" << major
                  << "." << minor << "." << patch;
        if (extra[0] != '\0') std::cerr << extra;
        std::cerr << "' vs compiled in '" << COMP_MAJ << "." << COMP_MIN << "."
                  << COMP_PATCH;
        if (compextra[0] != '\0') std::cerr << compextra;
        std::cerr << "'" << std::endl;
        EXRCORE_TEST (false);
    }
    std::cout << "Testing OpenEXR library version: '" << major << "." << minor
              << "." << patch;
    if (extra[0] != '\0') std::cout << extra;
    std::cout << "'" << std::endl;

    exr_get_library_version (NULL, &minor, &patch, &extra);
    exr_get_library_version (&major, NULL, &patch, &extra);
    exr_get_library_version (&major, &minor, NULL, &extra);
    exr_get_library_version (&major, &minor, &patch, NULL);

    major = (OPENEXR_VERSION_HEX >> 24) & 0xff;
    minor = (OPENEXR_VERSION_HEX >> 16) & 0xff;
    patch = (OPENEXR_VERSION_HEX >> 8) & 0xff;

    EXRCORE_TEST (major == COMP_MAJ);
    EXRCORE_TEST (minor == COMP_MIN);
    EXRCORE_TEST (patch == COMP_PATCH);

#if OPENEXR_VERSION_HEX > 0
    // confirm the macro compiles in an #if
#endif
}

void
testBaseErrors (const std::string& tempdir)
{
    const char* errmsg;

    // just spot check that we get results and don't get null for out
    // of bounds access
    errmsg = exr_get_default_error_message (EXR_ERR_SUCCESS);
    if (errmsg == NULL) { EXRCORE_TEST (false); }
    errmsg = exr_get_default_error_message (EXR_ERR_OUT_OF_MEMORY);
    if (errmsg == NULL) { EXRCORE_TEST (false); }
    errmsg = exr_get_default_error_message (EXR_ERR_UNKNOWN);
    if (errmsg == NULL) { EXRCORE_TEST (false); }
    errmsg = exr_get_default_error_message (-1);
    if (errmsg == NULL || strcmp (errmsg, "Unknown error code"))
    {
        std::cerr << "errmsg: " << errmsg << std::endl;
        EXRCORE_TEST (false);
    }
    errmsg = exr_get_default_error_message ((int) EXR_ERR_UNKNOWN + 1);
    if (errmsg == NULL) { EXRCORE_TEST (false); }
    errmsg = exr_get_default_error_message (110);
    if (errmsg == NULL || strcmp (errmsg, "Unknown error code"))
    {
        EXRCORE_TEST (false);
    }

    EXRCORE_TEST (
        0 ==
        strcmp (
            exr_get_error_code_as_string (EXR_ERR_SUCCESS), "EXR_ERR_SUCCESS"));
    EXRCORE_TEST (
        0 ==
        strcmp (
            exr_get_error_code_as_string (EXR_ERR_UNKNOWN), "EXR_ERR_UNKNOWN"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string ((int) EXR_ERR_UNKNOWN + 1),
                 "EXR_ERR_UNKNOWN"));
    EXRCORE_TEST (
        0 == strcmp (exr_get_error_code_as_string (-1), "EXR_ERR_UNKNOWN"));
    EXRCORE_TEST (
        0 == strcmp (exr_get_error_code_as_string (-2), "EXR_ERR_UNKNOWN"));
    EXRCORE_TEST (
        0 ==
        strcmp (exr_get_error_code_as_string (INT32_MIN), "EXR_ERR_UNKNOWN"));

    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_MISSING_REQ_ATTR),
                 "EXR_ERR_MISSING_REQ_ATTR"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_OUT_OF_MEMORY),
                 "EXR_ERR_OUT_OF_MEMORY"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_MISSING_CONTEXT_ARG),
                 "EXR_ERR_MISSING_CONTEXT_ARG"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_INVALID_ARGUMENT),
                 "EXR_ERR_INVALID_ARGUMENT"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_ARGUMENT_OUT_OF_RANGE),
                 "EXR_ERR_ARGUMENT_OUT_OF_RANGE"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_FILE_ACCESS),
                 "EXR_ERR_FILE_ACCESS"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_FILE_BAD_HEADER),
                 "EXR_ERR_FILE_BAD_HEADER"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_NOT_OPEN_READ),
                 "EXR_ERR_NOT_OPEN_READ"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_NOT_OPEN_WRITE),
                 "EXR_ERR_NOT_OPEN_WRITE"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_HEADER_NOT_WRITTEN),
                 "EXR_ERR_HEADER_NOT_WRITTEN"));
    EXRCORE_TEST (
        0 ==
        strcmp (
            exr_get_error_code_as_string (EXR_ERR_READ_IO), "EXR_ERR_READ_IO"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_WRITE_IO),
                 "EXR_ERR_WRITE_IO"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_NAME_TOO_LONG),
                 "EXR_ERR_NAME_TOO_LONG"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_MISSING_REQ_ATTR),
                 "EXR_ERR_MISSING_REQ_ATTR"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_INVALID_ATTR),
                 "EXR_ERR_INVALID_ATTR"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_NO_ATTR_BY_NAME),
                 "EXR_ERR_NO_ATTR_BY_NAME"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_BAD_CHUNK_LEADER),
                 "EXR_ERR_BAD_CHUNK_LEADER"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_CORRUPT_CHUNK),
                 "EXR_ERR_CORRUPT_CHUNK"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_INVALID_SAMPLE_DATA),
                 "EXR_ERR_INVALID_SAMPLE_DATA"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_ATTR_TYPE_MISMATCH),
                 "EXR_ERR_ATTR_TYPE_MISMATCH"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_ATTR_SIZE_MISMATCH),
                 "EXR_ERR_ATTR_SIZE_MISMATCH"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_INCOMPLETE_CHUNK_TABLE),
                 "EXR_ERR_INCOMPLETE_CHUNK_TABLE"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_SCAN_TILE_MIXEDAPI),
                 "EXR_ERR_SCAN_TILE_MIXEDAPI"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_TILE_SCAN_MIXEDAPI),
                 "EXR_ERR_TILE_SCAN_MIXEDAPI"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_MODIFY_SIZE_CHANGE),
                 "EXR_ERR_MODIFY_SIZE_CHANGE"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_ALREADY_WROTE_ATTRS),
                 "EXR_ERR_ALREADY_WROTE_ATTRS"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_INCORRECT_PART),
                 "EXR_ERR_INCORRECT_PART"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_INCORRECT_CHUNK),
                 "EXR_ERR_INCORRECT_CHUNK"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_USE_SCAN_DEEP_WRITE),
                 "EXR_ERR_USE_SCAN_DEEP_WRITE"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_USE_TILE_DEEP_WRITE),
                 "EXR_ERR_USE_TILE_DEEP_WRITE"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_USE_SCAN_NONDEEP_WRITE),
                 "EXR_ERR_USE_SCAN_NONDEEP_WRITE"));
    EXRCORE_TEST (
        0 == strcmp (
                 exr_get_error_code_as_string (EXR_ERR_USE_TILE_NONDEEP_WRITE),
                 "EXR_ERR_USE_TILE_NONDEEP_WRITE"));
}

void
testBaseLimits (const std::string& tempdir)
{
    int mxw, mxh;
    exr_set_default_maximum_image_size (42, 42);
    exr_get_default_maximum_image_size (&mxw, &mxh);
    if (mxw != 42 || mxh != 42)
    {
        std::cerr << "Unable to set_default_maximum_image_size: 42, 42 -> "
                  << mxw << ", " << mxh << std::endl;
        EXRCORE_TEST (false);
    }
    exr_set_default_maximum_image_size (-1, -1);
    exr_get_default_maximum_image_size (&mxw, &mxh);
    if (mxw != 42 || mxh != 42)
    {
        std::cerr
            << "Invalid request not ignored to set_default_maximum_image_size: 42, 42 -> "
            << mxw << ", " << mxh << std::endl;
        EXRCORE_TEST (false);
    }

    exr_set_default_maximum_image_size (84, -1);
    exr_get_default_maximum_image_size (&mxw, &mxh);
    if (mxw != 42 || mxh != 42)
    {
        std::cerr
            << "Invalid request not ignored to set_default_maximum_image_size: 42, 42 -> "
            << mxw << ", " << mxh << std::endl;
        EXRCORE_TEST (false);
    }

    exr_set_default_maximum_image_size (-1, 84);
    exr_get_default_maximum_image_size (&mxw, &mxh);
    if (mxw != 42 || mxh != 42)
    {
        std::cerr
            << "Invalid request not ignored to set_default_maximum_image_size: 42, 42 -> "
            << mxw << ", " << mxh << std::endl;
        EXRCORE_TEST (false);
    }

    exr_set_default_maximum_tile_size (128, 128);
    exr_get_default_maximum_tile_size (&mxw, &mxh);
    if (mxw != 128 || mxh != 128)
    {
        std::cerr << "Unable to set_default_maximum_tile_size: 128, 128 -> "
                  << mxw << ", " << mxh << std::endl;
        EXRCORE_TEST (false);
    }
    exr_set_default_maximum_tile_size (-1, -1);
    exr_get_default_maximum_tile_size (&mxw, &mxh);
    if (mxw != 128 || mxh != 128)
    {
        std::cerr
            << "Invalid request not ignored to set_default_maximum_image_size: 128, 128 -> "
            << mxw << ", " << mxh << std::endl;
        EXRCORE_TEST (false);
    }
    exr_set_default_maximum_tile_size (84, -1);
    exr_get_default_maximum_tile_size (&mxw, &mxh);
    if (mxw != 128 || mxh != 128)
    {
        std::cerr
            << "Invalid request not ignored to set_default_maximum_image_size: 128, 128 -> "
            << mxw << ", " << mxh << std::endl;
        EXRCORE_TEST (false);
    }
    exr_set_default_maximum_tile_size (-1, 84);
    exr_get_default_maximum_tile_size (&mxw, &mxh);
    if (mxw != 128 || mxh != 128)
    {
        std::cerr
            << "Invalid request not ignored to set_default_maximum_image_size: 128, 128 -> "
            << mxw << ", " << mxh << std::endl;
        EXRCORE_TEST (false);
    }
    exr_set_default_maximum_image_size (0, 0);
    exr_set_default_maximum_tile_size (0, 0);

    exr_set_default_zip_compression_level (4);
    exr_get_default_zip_compression_level (&mxw);
    EXRCORE_TEST (mxw == 4);

    exr_set_default_zip_compression_level (-1);
    exr_get_default_zip_compression_level (&mxw);
    EXRCORE_TEST (mxw == -1);
    exr_set_default_zip_compression_level (-2);
    exr_get_default_zip_compression_level (&mxw);
    EXRCORE_TEST (mxw == -1);

    exr_set_default_zip_compression_level (15);
    exr_get_default_zip_compression_level (&mxw);
    EXRCORE_TEST (mxw == 9);
    exr_set_default_zip_compression_level (-1);

    float dcq;
    exr_set_default_dwa_compression_quality (23.f);
    exr_get_default_dwa_compression_quality (&dcq);
    EXRCORE_TEST (dcq == 23.f);

    exr_set_default_dwa_compression_quality (-1.f);
    exr_get_default_dwa_compression_quality (&dcq);
    EXRCORE_TEST (dcq == 0.f);

    exr_set_default_dwa_compression_quality (200.f);
    exr_get_default_dwa_compression_quality (&dcq);
    EXRCORE_TEST (dcq == 100.f);
    exr_set_default_dwa_compression_quality (45.f);
}

void
testBaseDebug (const std::string& tempdir)
{
    exr_context_t c = NULL;

    // make sure we don't crash with null file handle (there should be error prints)
    exr_print_context_info (NULL, 0);
    exr_print_context_info (c, 0);
    exr_print_context_info (c, 1);
}

void
testCPUIdent (const std::string& tempdir)
{
    int                          hf16c, havx, hsse2;
    OPENEXR_IMF_NAMESPACE::CpuId id;
    check_for_x86_simd (&hf16c, &havx, &hsse2);

    if (hf16c != (int) id.f16c)
    {
        std::cerr << "CPU Id test f16c mismatch: " << hf16c << " vs "
                  << (int) id.f16c << std::endl;
        EXRCORE_TEST (false);
    }

    if (havx != (int) id.avx)
    {
        std::cerr << "CPU Id test avx mismatch: " << havx << " vs "
                  << (int) id.avx << std::endl;
        EXRCORE_TEST (false);
    }

    if (hsse2 != (int) id.sse2)
    {
        std::cerr << "CPU Id test sse2 mismatch: " << hsse2 << " vs "
                  << (int) id.sse2 << std::endl;
        EXRCORE_TEST (false);
    }

#if defined(__x86_64__) || defined(_M_X64)
    if (has_native_half () != (hf16c && havx))
    {
        std::cerr << "CPU Id test has native half mismatch" << std::endl;
        EXRCORE_TEST (false);
    }
#else
    has_native_half ();
#endif
}

void
testHalf (const std::string& tempdir)
{
    EXRCORE_TEST (half_to_float (0) == 0.f);
    EXRCORE_TEST (float_to_half (0.f) == 0);
    EXRCORE_TEST (float_to_half_int (0.f) == 0);
    EXRCORE_TEST (half_to_float_int (0) == 0);
    EXRCORE_TEST (half_to_uint (0) == 0);
    EXRCORE_TEST (half_to_uint (0x8000) == 0);
    EXRCORE_TEST (float_to_uint (0) == 0);
    EXRCORE_TEST (float_to_uint (-1.f) == 0);
    EXRCORE_TEST (float_to_uint_int (0) == 0);

    EXRCORE_TEST (uint_to_half (0) == 0);
    EXRCORE_TEST (uint_to_half (128344) == 0x7c00);

    EXRCORE_TEST (uint_to_float (0) == 0.f);
    EXRCORE_TEST (uint_to_float_int (0) == 0);
}

////////////////////////////////////////

void testTempContext (const std::string& tempdir)
{
    exr_context_t c = NULL;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    int pc = -1;

    printf ("Testing initial temporary context API\n");

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_start_temporary_context (NULL, tempdir.c_str (), NULL));

    EXRCORE_TEST_RVAL (exr_start_temporary_context (&c, tempdir.c_str (), NULL));
    EXRCORE_TEST_RVAL (exr_get_count (c, &pc));
    EXRCORE_TEST (pc == 1);
    exr_finish (&c);

    EXRCORE_TEST_RVAL (exr_start_temporary_context (
                           &c, tempdir.c_str (), &cinit));
    EXRCORE_TEST_RVAL (exr_initialize_required_attr_simple (
                           c, 0, 1920, 1080, EXR_COMPRESSION_NONE));
    exr_finish (&c);

    printf ("ok.\n");
}

