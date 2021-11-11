/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <openexr.h>

#include "base_units.h"

#include "test_value.h"

#include <cstring>
#include <iostream>

void
testBase (const std::string& tempdir)
{
    int         maj, min, patch;
    const char* extra;
    const char* compextra = COMP_EXTRA;

    exr_get_library_version (&maj, &min, &patch, &extra);
    if (maj != COMP_MAJ || min != COMP_MIN || patch != COMP_PATCH ||
        !strcmp (extra, compextra))
    {
        std::cerr << "ERROR testing library, wrong library version: " << maj
                  << "." << min << "." << patch;
        if (extra[0] != '\0') std::cerr << "-" << extra;
        std::cerr << " vs compiled in " << COMP_MAJ << "." << COMP_MIN << "."
                  << COMP_PATCH;
        if (compextra[0] != '\0') std::cerr << "-" << compextra;
        std::cerr << std::endl;
        EXRCORE_TEST (false);
    }
    std::cout << "Testing OpenEXR library version: " << maj << "." << min << "."
              << patch;
    if (extra[0] != '\0') std::cout << "-" << extra;
    std::cout << std::endl;

    exr_get_library_version (NULL, &min, &patch, &extra);
    exr_get_library_version (&maj, NULL, &patch, &extra);
    exr_get_library_version (&maj, &min, NULL, &extra);
    exr_get_library_version (&maj, &min, &patch, NULL);
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

    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_SUCCESS), "EXR_ERR_SUCCESS" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_UNKNOWN), "EXR_ERR_UNKNOWN" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string((int)EXR_ERR_UNKNOWN + 1), "EXR_ERR_UNKNOWN" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(-1), "EXR_ERR_UNKNOWN" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(-2), "EXR_ERR_UNKNOWN" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(INT32_MIN), "EXR_ERR_UNKNOWN" ) );

    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_MISSING_REQ_ATTR), "EXR_ERR_MISSING_REQ_ATTR" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_OUT_OF_MEMORY), "EXR_ERR_OUT_OF_MEMORY" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_MISSING_CONTEXT_ARG), "EXR_ERR_MISSING_CONTEXT_ARG" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_INVALID_ARGUMENT), "EXR_ERR_INVALID_ARGUMENT" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_ARGUMENT_OUT_OF_RANGE), "EXR_ERR_ARGUMENT_OUT_OF_RANGE" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_FILE_ACCESS), "EXR_ERR_FILE_ACCESS" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_FILE_BAD_HEADER), "EXR_ERR_FILE_BAD_HEADER" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_NOT_OPEN_READ), "EXR_ERR_NOT_OPEN_READ" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_NOT_OPEN_WRITE), "EXR_ERR_NOT_OPEN_WRITE" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_HEADER_NOT_WRITTEN), "EXR_ERR_HEADER_NOT_WRITTEN" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_READ_IO), "EXR_ERR_READ_IO" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_WRITE_IO), "EXR_ERR_WRITE_IO" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_NAME_TOO_LONG), "EXR_ERR_NAME_TOO_LONG" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_MISSING_REQ_ATTR), "EXR_ERR_MISSING_REQ_ATTR" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_INVALID_ATTR), "EXR_ERR_INVALID_ATTR" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_NO_ATTR_BY_NAME), "EXR_ERR_NO_ATTR_BY_NAME" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_BAD_CHUNK_LEADER), "EXR_ERR_BAD_CHUNK_LEADER" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_CORRUPT_CHUNK), "EXR_ERR_CORRUPT_CHUNK" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_INVALID_SAMPLE_DATA), "EXR_ERR_INVALID_SAMPLE_DATA" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_ATTR_TYPE_MISMATCH), "EXR_ERR_ATTR_TYPE_MISMATCH" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_ATTR_SIZE_MISMATCH), "EXR_ERR_ATTR_SIZE_MISMATCH" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_SCAN_TILE_MIXEDAPI), "EXR_ERR_SCAN_TILE_MIXEDAPI" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_TILE_SCAN_MIXEDAPI), "EXR_ERR_TILE_SCAN_MIXEDAPI" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_MODIFY_SIZE_CHANGE), "EXR_ERR_MODIFY_SIZE_CHANGE" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_ALREADY_WROTE_ATTRS), "EXR_ERR_ALREADY_WROTE_ATTRS" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_INCORRECT_PART), "EXR_ERR_INCORRECT_PART" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_INCORRECT_CHUNK), "EXR_ERR_INCORRECT_CHUNK" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_USE_SCAN_DEEP_WRITE), "EXR_ERR_USE_SCAN_DEEP_WRITE" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_USE_TILE_DEEP_WRITE), "EXR_ERR_USE_TILE_DEEP_WRITE" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_USE_SCAN_NONDEEP_WRITE), "EXR_ERR_USE_SCAN_NONDEEP_WRITE" ) );
    EXRCORE_TEST( 0 == strcmp( exr_get_error_code_as_string(EXR_ERR_USE_TILE_NONDEEP_WRITE), "EXR_ERR_USE_TILE_NONDEEP_WRITE" ) );
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

