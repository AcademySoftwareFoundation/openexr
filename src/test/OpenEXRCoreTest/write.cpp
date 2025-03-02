// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#ifdef _WIN32
// windows is very particular about when windows.h is included
#include <windows.h>
#include <fileapi.h>
#include <inttypes.h>
#include <strsafe.h>
#else
#include <unistd.h>
#endif

#include "write.h"

#include "test_value.h"

#include <openexr.h>

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <iomanip>
#include <iostream>
#include <memory>

static void
err_cb (exr_const_context_t f, exr_result_t code, const char* msg)
{
    std::cerr << "err_cb " << exr_get_error_code_as_string (code) << " ("
              << code << "): " << msg << std::endl;
}

void
testWriteBadArgs (const std::string& tempdir)
{
    exr_context_t             f;
    std::string               fn    = tempdir + "invalid.exr";
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_start_write (NULL, fn.c_str (), EXR_WRITE_FILE_DIRECTLY, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_start_write (&f, NULL, EXR_WRITE_FILE_DIRECTLY, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_start_write (&f, NULL, EXR_WRITE_FILE_DIRECTLY, &cinit));

    //    EXRCORE_TEST_RVAL_FAIL (EXR_ERR_FILE_ACCESS, exr_start_write (&f, fn.c_str (), &cinit));
}

void
testWriteBadFiles (const std::string& tempdir)
{}

void
testStartWriteScan (const std::string& tempdir)
{
    exr_context_t outf;
    std::string   outfn = tempdir + "testscan.exr";
    int           partidx;

    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));

    EXRCORE_TEST_RVAL (exr_get_count (outf, &partidx));
    EXRCORE_TEST (partidx == 0);
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_count (NULL, &partidx));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_count (outf, NULL));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_add_part (NULL, "beauty", EXR_STORAGE_SCANLINE, &partidx));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_add_part (outf, "beauty", EXR_STORAGE_LAST_TYPE, &partidx));
    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "beauty", EXR_STORAGE_SCANLINE, &partidx));
    EXRCORE_TEST (partidx == 0);
    EXRCORE_TEST_RVAL (exr_get_count (outf, &partidx));
    EXRCORE_TEST (partidx == 1);
    partidx = 0;

    exr_chunk_info_t cinfo;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NOT_OPEN_READ,
        exr_read_scanline_chunk_info (outf, partidx, 42, &cinfo));

    const char* partname;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_name (NULL, partidx, &partname));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_name (outf, partidx - 1, &partname));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_name (outf, partidx + 1, &partname));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_name (outf, partidx, NULL));
    EXRCORE_TEST_RVAL (exr_get_name (outf, partidx, &partname));
    EXRCORE_TEST (0 == strcmp (partname, "beauty"));

    exr_storage_t storage;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_storage (NULL, partidx, &storage));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_storage (outf, partidx - 1, &storage));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_storage (outf, partidx + 1, &storage));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_storage (outf, partidx, NULL));
    EXRCORE_TEST_RVAL (exr_get_storage (outf, partidx, &storage));
    EXRCORE_TEST (storage == EXR_STORAGE_SCANLINE);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_TILE_SCAN_MIXEDAPI,
        exr_set_tile_descriptor (
            outf, partidx, 32, 32, EXR_TILE_ONE_LEVEL, EXR_TILE_ROUND_DOWN));

    EXRCORE_TEST_RVAL (exr_finish (&outf));
    remove (outfn.c_str ());
}

////////////////////////////////////////

void
testStartWriteDeepScan (const std::string& tempdir)
{
    exr_context_t outf;
    std::string   outfn = tempdir + "testdeepscan.exr";
    int           partidx;

    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));

    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "beauty", EXR_STORAGE_DEEP_SCANLINE, &partidx));
    EXRCORE_TEST (partidx == 0);
    EXRCORE_TEST_RVAL (exr_get_count (outf, &partidx));
    EXRCORE_TEST (partidx == 1);
    partidx = 0;

    exr_storage_t storage;
    EXRCORE_TEST_RVAL (exr_get_storage (outf, partidx, &storage));
    EXRCORE_TEST (storage == EXR_STORAGE_DEEP_SCANLINE);

    uint32_t verflags;
    EXRCORE_TEST_RVAL (
        exr_get_file_version_and_flags (outf, &verflags));
    EXRCORE_TEST (verflags == (2 | 0x00000800));

    EXRCORE_TEST_RVAL (exr_finish (&outf));
    remove (outfn.c_str ());
}

////////////////////////////////////////

void
testStartWriteTile (const std::string& tempdir)
{
    exr_context_t outf;
    std::string   outfn = tempdir + "testattr.exr";
    int           partidx;

    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));

    EXRCORE_TEST_RVAL (exr_get_count (outf, &partidx));
    EXRCORE_TEST (partidx == 0);
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_count (NULL, &partidx));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_count (outf, NULL));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_add_part (NULL, "beauty", EXR_STORAGE_TILED, &partidx));
    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "beauty", EXR_STORAGE_TILED, &partidx));
    EXRCORE_TEST (partidx == 0);
    EXRCORE_TEST_RVAL (exr_get_count (outf, &partidx));
    EXRCORE_TEST (partidx == 1);
    partidx = 0;

    exr_chunk_info_t cinfo;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NOT_OPEN_READ,
        exr_read_scanline_chunk_info (outf, partidx, 42, &cinfo));

    const char* partname;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_name (NULL, partidx, &partname));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_name (outf, partidx - 1, &partname));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_name (outf, partidx + 1, &partname));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_name (outf, partidx, NULL));
    EXRCORE_TEST_RVAL (exr_get_name (outf, partidx, &partname));
    EXRCORE_TEST (0 == strcmp (partname, "beauty"));

    exr_storage_t storage;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_storage (NULL, partidx, &storage));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_storage (outf, partidx - 1, &storage));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_storage (outf, partidx + 1, &storage));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_storage (outf, partidx, NULL));
    EXRCORE_TEST_RVAL (exr_get_storage (outf, partidx, &storage));
    EXRCORE_TEST (storage == EXR_STORAGE_TILED);

    EXRCORE_TEST_RVAL (exr_finish (&outf));
    remove (outfn.c_str ());
}

////////////////////////////////////////

void
testStartWriteDeepTile (const std::string& tempdir)
{
    exr_context_t outf;
    std::string   outfn = tempdir + "testattr.exr";
    int           partidx;

    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));

    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "beauty", EXR_STORAGE_DEEP_TILED, &partidx));
    EXRCORE_TEST (partidx == 0);
    EXRCORE_TEST_RVAL (exr_get_count (outf, &partidx));
    EXRCORE_TEST (partidx == 1);
    partidx = 0;

    exr_storage_t storage;
    EXRCORE_TEST_RVAL (exr_get_storage (outf, partidx, &storage));
    EXRCORE_TEST (storage == EXR_STORAGE_DEEP_TILED);

    uint32_t verflags;
    EXRCORE_TEST_RVAL (
        exr_get_file_version_and_flags (outf, &verflags));
    EXRCORE_TEST (verflags == (2 | 0x00000800));

    EXRCORE_TEST_RVAL (exr_finish (&outf));
    remove (outfn.c_str ());
}

void
testWriteBaseHeader (const std::string& tempdir)
{
    exr_context_t outf;
    std::string   outfn = tempdir + "testattr.exr";
    int           partidx;
    uint32_t      verflags;

    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));

    EXRCORE_TEST_RVAL (exr_get_count (outf, &partidx));
    EXRCORE_TEST (partidx == 0);
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_count (NULL, &partidx));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_count (outf, NULL));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_add_part (NULL, "beauty", EXR_STORAGE_TILED, &partidx));
    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "beauty", EXR_STORAGE_TILED, &partidx));
    EXRCORE_TEST (partidx == 0);
    EXRCORE_TEST_RVAL (exr_get_count (outf, &partidx));
    EXRCORE_TEST (partidx == 1);
    partidx = 0;

    exr_chunk_info_t cinfo;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NOT_OPEN_READ,
        exr_read_scanline_chunk_info (outf, partidx, 42, &cinfo));

    const char* partname;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_name (NULL, partidx, &partname));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_name (outf, partidx - 1, &partname));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_name (outf, partidx + 1, &partname));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_name (outf, partidx, NULL));
    EXRCORE_TEST_RVAL (exr_get_name (outf, partidx, &partname));
    EXRCORE_TEST (0 == strcmp (partname, "beauty"));

    exr_storage_t storage;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_storage (NULL, partidx, &storage));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_storage (outf, partidx - 1, &storage));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_storage (outf, partidx + 1, &storage));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_storage (outf, partidx, NULL));
    EXRCORE_TEST_RVAL (exr_get_storage (outf, partidx, &storage));
    EXRCORE_TEST (storage == EXR_STORAGE_TILED);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_set_longname_support (NULL, 0));
    EXRCORE_TEST_RVAL (exr_set_longname_support (outf, 1));
    EXRCORE_TEST_RVAL (exr_attr_set_int (outf, partidx, "shortname", 42));
    EXRCORE_TEST_RVAL (exr_set_longname_support (outf, 0));
    EXRCORE_TEST_RVAL (exr_set_longname_support (outf, 1));
    EXRCORE_TEST_RVAL (exr_add_channel (
        outf,
        partidx,
        "reallongreallongreallonglongchannelname",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NAME_TOO_LONG, exr_set_longname_support (outf, 0));
    EXRCORE_TEST_RVAL (exr_finish (&outf));
    remove (outfn.c_str ());

    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));
    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "beauty", EXR_STORAGE_TILED, &partidx));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NAME_TOO_LONG,
        exr_attr_set_int (
            outf, partidx, "areallongreallongreallonglongname", 42));

    EXRCORE_TEST_RVAL (exr_set_longname_support (outf, 1));
    EXRCORE_TEST_RVAL (exr_attr_set_int (
        outf, partidx, "areallongreallongreallonglongname", 42));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NAME_TOO_LONG, exr_set_longname_support (outf, 0));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_get_zip_compression_level (NULL, 1, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_zip_compression_level (outf, -1, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_zip_compression_level (outf, 5, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_get_zip_compression_level (outf, 0, NULL));
    int zlev = -2;
    EXRCORE_TEST_RVAL (exr_get_zip_compression_level (outf, 0, &zlev));
    EXRCORE_TEST (zlev == -1);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_set_zip_compression_level (NULL, 0, 5));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_set_zip_compression_level (outf, -1, 5));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_set_zip_compression_level (outf, 5, 5));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_set_zip_compression_level (outf, 0, -2));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_set_zip_compression_level (outf, 0, 42));
    EXRCORE_TEST_RVAL (exr_set_zip_compression_level (outf, 0, 4));
    EXRCORE_TEST_RVAL (exr_get_zip_compression_level (outf, 0, &zlev));
    EXRCORE_TEST (zlev == 4);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_get_dwa_compression_level (NULL, 0, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_dwa_compression_level (outf, -1, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_dwa_compression_level (outf, 5, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_get_dwa_compression_level (outf, 0, NULL));
    float dlev = -3.f;
    EXRCORE_TEST_RVAL (exr_get_dwa_compression_level (outf, 0, &dlev));
    EXRCORE_TEST (dlev == 45.f);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_set_dwa_compression_level (NULL, 0, 5));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_set_dwa_compression_level (outf, -1, 5));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_set_dwa_compression_level (outf, 5, 5));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_set_dwa_compression_level (outf, 0, -2.f));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_set_dwa_compression_level (outf, 0, INFINITY));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_set_dwa_compression_level (outf, 0, NAN));
    EXRCORE_TEST_RVAL (exr_set_dwa_compression_level (outf, 0, 42.f));
    EXRCORE_TEST_RVAL (exr_get_dwa_compression_level (outf, 0, &dlev));
    EXRCORE_TEST (dlev == 42.f);
    EXRCORE_TEST_RVAL (exr_set_dwa_compression_level (outf, 0, 420.f));
    EXRCORE_TEST_RVAL (exr_get_dwa_compression_level (outf, 0, &dlev));
    EXRCORE_TEST (dlev == 420.f);

    EXRCORE_TEST_RVAL (exr_finish (&outf));
    remove (outfn.c_str ());

    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));
    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "beauty", EXR_STORAGE_SCANLINE, &partidx));
    exr_attr_box2i_t dataw = {{-2, -3}, {514, 515}};
    exr_attr_box2i_t dispw = {{0, 0}, {512, 512}};
    exr_attr_v2f_t   swc   = {0.5f, 0.5f};
    EXRCORE_TEST_RVAL (exr_initialize_required_attr (
        outf,
        partidx,
        &dataw,
        &dispw,
        1.f,
        &swc,
        1.f,
        EXR_LINEORDER_INCREASING_Y,
        EXR_COMPRESSION_ZIPS));
    EXRCORE_TEST_RVAL (exr_add_channel (
        outf,
        partidx,
        "R",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));
    EXRCORE_TEST_RVAL (exr_add_channel (
        outf,
        partidx,
        "G",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));
    EXRCORE_TEST_RVAL (exr_add_channel (
        outf,
        partidx,
        "B",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));

    EXRCORE_TEST_RVAL (exr_write_header (outf));
    EXRCORE_TEST_RVAL (
        exr_get_file_version_and_flags (outf, &verflags));
    EXRCORE_TEST (verflags == 2);

    uint64_t cto;
    EXRCORE_TEST_RVAL (
        exr_get_chunk_table_offset (outf, 0, &cto));
    EXRCORE_TEST (cto == 364);

    EXRCORE_TEST_RVAL (exr_finish (&outf));
    remove (outfn.c_str ());

    ////

    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));
    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "beauty", EXR_STORAGE_TILED, &partidx));
    dataw = {{0, 0}, {512, 512}};
    EXRCORE_TEST_RVAL (exr_initialize_required_attr (
        outf,
        partidx,
        &dataw,
        &dispw,
        1.f,
        &swc,
        1.f,
        EXR_LINEORDER_INCREASING_Y,
        EXR_COMPRESSION_ZIPS));
    EXRCORE_TEST_RVAL (exr_add_channel (
        outf,
        partidx,
        "R",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));
    EXRCORE_TEST_RVAL (exr_add_channel (
        outf,
        partidx,
        "G",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));
    EXRCORE_TEST_RVAL (exr_add_channel (
        outf,
        partidx,
        "B",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));

    EXRCORE_TEST_RVAL_FAIL (EXR_ERR_MISSING_REQ_ATTR, exr_write_header (outf));
    EXRCORE_TEST_RVAL (exr_set_tile_descriptor (
        outf, partidx, 32, 32, EXR_TILE_ONE_LEVEL, EXR_TILE_ROUND_DOWN));

    uint32_t              txsize, tysize;
    exr_tile_level_mode_t lmode;
    exr_tile_round_mode_t rmode;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_get_tile_descriptor (
            NULL, partidx, &txsize, &tysize, &lmode, &rmode));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_tile_descriptor (outf, -1, &txsize, &tysize, &lmode, &rmode));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_tile_descriptor (
            outf, partidx + 10, &txsize, &tysize, &lmode, &rmode));
    EXRCORE_TEST_RVAL (exr_get_tile_descriptor (
        outf, partidx, &txsize, &tysize, &lmode, &rmode));
    EXRCORE_TEST (txsize == 32 && tysize == 32);

    EXRCORE_TEST_RVAL (exr_write_header (outf));
    EXRCORE_TEST_RVAL (
        exr_get_file_version_and_flags (outf, &verflags));
    EXRCORE_TEST (verflags == (2 | 0x00000200));

    EXRCORE_TEST_RVAL (exr_finish (&outf));
    remove (outfn.c_str ());

    ////

    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));
    EXRCORE_TEST_RVAL_FAIL (EXR_ERR_FILE_BAD_HEADER, exr_write_header (outf));
    EXRCORE_TEST_RVAL (exr_finish (&outf));
    remove (outfn.c_str ());
}

void
testWriteAttrs (const std::string& tempdir)
{
    exr_context_t outf;
    std::string   outfn = tempdir + "testattr.exr";
    int           partidx;
    const char*   partname;

    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));
    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "tester", EXR_STORAGE_SCANLINE, &partidx));
    EXRCORE_TEST_RVAL (exr_initialize_required_attr_simple (
        outf, partidx, 1, 1, EXR_COMPRESSION_ZIPS));
    EXRCORE_TEST_RVAL (exr_add_channel (
        outf,
        partidx,
        "R",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));
    EXRCORE_TEST_RVAL (exr_add_channel (
        outf,
        partidx,
        "G",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));
    EXRCORE_TEST_RVAL (exr_add_channel (
        outf,
        partidx,
        "B",
        EXR_PIXEL_HALF,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        1,
        1));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_set_name (NULL, partidx, "a"));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_set_name (outf, -1, "a"));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_set_name (outf, 1, "a"));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_set_name (outf, partidx, NULL));
    EXRCORE_TEST_RVAL (exr_set_name (outf, partidx, ""));
    EXRCORE_TEST_RVAL (exr_set_name (outf, partidx, "beauty"));
    EXRCORE_TEST_RVAL (exr_get_name (outf, partidx, &partname));
    EXRCORE_TEST (0 == strcmp (partname, "beauty"));

    const exr_attr_chlist_t* chans;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_channels (NULL, partidx, &chans));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_get_channels (outf, -1, &chans));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_get_channels (outf, 1, &chans));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_channels (outf, partidx, NULL));
    EXRCORE_TEST_RVAL (exr_get_channels (outf, partidx, &chans));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_set_channels (NULL, partidx, chans));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_set_channels (outf, -1, chans));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_set_channels (outf, 1, chans));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_set_channels (outf, partidx, NULL));
    EXRCORE_TEST_RVAL (exr_set_channels (outf, partidx, chans));
    EXRCORE_TEST_RVAL (exr_get_channels (outf, partidx, &chans));

#define TEST_CORNER_CASE_GET(fn, arg)                                          \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_MISSING_CONTEXT_ARG, fn (NULL, partidx, arg));                 \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, fn (outf, -1, arg));                    \
    EXRCORE_TEST_RVAL_FAIL (EXR_ERR_ARGUMENT_OUT_OF_RANGE, fn (outf, 1, arg)); \
    EXRCORE_TEST_RVAL (fn (outf, partidx, arg))
#define TEST_CORNER_CASE_SET(fn, arg)                                          \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_MISSING_CONTEXT_ARG, fn (NULL, partidx, arg));                 \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, fn (outf, -1, arg));                    \
    EXRCORE_TEST_RVAL_FAIL (EXR_ERR_ARGUMENT_OUT_OF_RANGE, fn (outf, 1, arg)); \
    EXRCORE_TEST_RVAL (fn (outf, partidx, arg))

#define TEST_CORNER_CASE_NAME(fnt, arg, alt)                                   \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_MISSING_CONTEXT_ARG,                                           \
        exr_attr_set_##fnt (NULL, partidx, #arg, &arg));                       \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_set_##fnt (outf, -1, #arg, &arg));                            \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_set_##fnt (outf, 1, #arg, &arg));                             \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_INVALID_ARGUMENT,                                              \
        exr_attr_set_##fnt (outf, partidx, #arg, NULL));                       \
    EXRCORE_TEST_RVAL (exr_attr_set_##fnt (outf, partidx, #arg, &arg));        \
    alt altv = 0;                                                              \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ATTR_TYPE_MISMATCH,                                            \
        exr_attr_set_##alt (outf, partidx, #arg, altv));                       \
    memset (&arg, 0, sizeof (arg));                                            \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_MISSING_CONTEXT_ARG,                                           \
        exr_attr_get_##fnt (NULL, partidx, #arg, &arg));                       \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_get_##fnt (outf, -1, #arg, &arg));                            \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_get_##fnt (outf, 1, #arg, &arg));                             \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_INVALID_ARGUMENT,                                              \
        exr_attr_get_##fnt (outf, partidx, NULL, &arg));                       \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_INVALID_ARGUMENT,                                              \
        exr_attr_get_##fnt (outf, partidx, "", &arg));                         \
    EXRCORE_TEST_RVAL (exr_attr_get_##fnt (outf, partidx, #arg, &arg))

#define TEST_CORNER_CASE_NAME_C(fnt, arg, cnt, oarg, alt)                      \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_MISSING_CONTEXT_ARG,                                           \
        exr_attr_set_##fnt (NULL, partidx, #arg, cnt, arg));                   \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_set_##fnt (outf, -1, #arg, cnt, arg));                        \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_set_##fnt (outf, 1, #arg, cnt, arg));                         \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_INVALID_ARGUMENT,                                              \
        exr_attr_set_##fnt (outf, partidx, #arg, -1, arg));                    \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_INVALID_ARGUMENT,                                              \
        exr_attr_set_##fnt (outf, partidx, #arg, cnt, NULL));                  \
    EXRCORE_TEST_RVAL (exr_attr_set_##fnt (outf, partidx, #arg, cnt, arg));    \
    alt altv = 0;                                                              \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ATTR_TYPE_MISMATCH,                                            \
        exr_attr_set_##alt (outf, partidx, #arg, altv));                       \
    memset (&arg, 0, sizeof (arg));                                            \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_MISSING_CONTEXT_ARG,                                           \
        exr_attr_get_##fnt (NULL, partidx, #arg, &cnt, oarg));                 \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_get_##fnt (outf, -1, #arg, &cnt, oarg));                      \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_get_##fnt (outf, 1, #arg, &cnt, oarg));                       \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_INVALID_ARGUMENT,                                              \
        exr_attr_get_##fnt (outf, partidx, NULL, &cnt, oarg));                 \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_INVALID_ARGUMENT,                                              \
        exr_attr_get_##fnt (outf, partidx, "", &cnt, oarg));                   \
    EXRCORE_TEST_RVAL (exr_attr_get_##fnt (outf, partidx, #arg, &cnt, oarg))

#define TEST_CORNER_CASE_NAME_V(fnt, arg, alt)                                 \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_MISSING_CONTEXT_ARG,                                           \
        exr_attr_set_##fnt (NULL, partidx, #arg, arg));                        \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_set_##fnt (outf, -1, #arg, arg));                             \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_set_##fnt (outf, 1, #arg, arg));                              \
    EXRCORE_TEST_RVAL (exr_attr_set_##fnt (outf, partidx, #arg, arg));         \
    alt altv = 0;                                                              \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ATTR_TYPE_MISMATCH,                                            \
        exr_attr_set_##alt (outf, partidx, #arg, altv));                       \
    memset (&arg, 0, sizeof (arg));                                            \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_MISSING_CONTEXT_ARG,                                           \
        exr_attr_get_##fnt (NULL, partidx, #arg, &arg));                       \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_get_##fnt (outf, -1, #arg, &arg));                            \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_get_##fnt (outf, 1, #arg, &arg));                             \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_INVALID_ARGUMENT,                                              \
        exr_attr_get_##fnt (outf, partidx, NULL, &arg));                       \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_INVALID_ARGUMENT,                                              \
        exr_attr_get_##fnt (outf, partidx, "", &arg));                         \
    EXRCORE_TEST_RVAL (exr_attr_get_##fnt (outf, partidx, #arg, &arg))

#define TEST_CORNER_CASE_NAME_NOALT(fnt, arg, oarg)                            \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_MISSING_CONTEXT_ARG,                                           \
        exr_attr_set_##fnt (NULL, partidx, #arg, arg));                        \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_set_##fnt (outf, -1, #arg, arg));                             \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_set_##fnt (outf, 1, #arg, arg));                              \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_INVALID_ARGUMENT,                                              \
        exr_attr_set_##fnt (outf, partidx, #arg, NULL));                       \
    EXRCORE_TEST_RVAL (exr_attr_set_##fnt (outf, partidx, #arg, arg));         \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_MISSING_CONTEXT_ARG,                                           \
        exr_attr_get_##fnt (NULL, partidx, #arg, &oarg));                      \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_get_##fnt (outf, -1, #arg, &oarg));                           \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                         \
        exr_attr_get_##fnt (outf, 1, #arg, &oarg));                            \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_INVALID_ARGUMENT,                                              \
        exr_attr_get_##fnt (outf, partidx, NULL, &oarg));                      \
    EXRCORE_TEST_RVAL_FAIL (                                                   \
        EXR_ERR_INVALID_ARGUMENT,                                              \
        exr_attr_get_##fnt (outf, partidx, "", &oarg));                        \
    EXRCORE_TEST_RVAL (exr_attr_get_##fnt (outf, partidx, #arg, &oarg))

    {
        int32_t version = -1;
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_set_version (outf, partidx, 2));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_set_version (outf, partidx, 0));
        TEST_CORNER_CASE_SET (exr_set_version, 1);
        TEST_CORNER_CASE_GET (exr_get_version, &version);
        EXRCORE_TEST (version == 1);
    }

    {
        exr_compression_t ct;
        TEST_CORNER_CASE_GET (exr_get_compression, &ct);
        EXRCORE_TEST (ct == EXR_COMPRESSION_ZIPS);
        TEST_CORNER_CASE_SET (exr_set_compression, EXR_COMPRESSION_ZIP);
        EXRCORE_TEST_RVAL (exr_get_compression (outf, partidx, &ct));
        EXRCORE_TEST (ct == EXR_COMPRESSION_ZIP);
    }

    {
        TEST_CORNER_CASE_SET (exr_set_chunk_count, 1);
    }

    {
        exr_attr_box2i_t dw;
        TEST_CORNER_CASE_GET (exr_get_data_window, &dw);
        TEST_CORNER_CASE_SET (exr_set_data_window, &dw);
    }

    {
        exr_attr_box2i_t dpw;
        TEST_CORNER_CASE_GET (exr_get_display_window, &dpw);
        TEST_CORNER_CASE_SET (exr_set_display_window, &dpw);
    }

    {
        exr_lineorder_t lo;
        TEST_CORNER_CASE_GET (exr_get_lineorder, &lo);
        EXRCORE_TEST (lo == EXR_LINEORDER_INCREASING_Y);
        TEST_CORNER_CASE_SET (exr_set_lineorder, EXR_LINEORDER_DECREASING_Y);
        EXRCORE_TEST_RVAL (exr_get_lineorder (outf, partidx, &lo));
        EXRCORE_TEST (lo == EXR_LINEORDER_DECREASING_Y);
        EXRCORE_TEST_RVAL (
            exr_set_lineorder (outf, partidx, EXR_LINEORDER_INCREASING_Y));
    }

    {
        float par;
        TEST_CORNER_CASE_GET (exr_get_pixel_aspect_ratio, &par);
        TEST_CORNER_CASE_SET (exr_set_pixel_aspect_ratio, 1.f);
    }

    {
        exr_attr_v2f_t swc;
        TEST_CORNER_CASE_GET (exr_get_screen_window_center, &swc);
        TEST_CORNER_CASE_SET (exr_set_screen_window_center, &swc);
    }

    {
        float sww;
        TEST_CORNER_CASE_GET (exr_get_screen_window_width, &sww);
        TEST_CORNER_CASE_SET (exr_set_screen_window_width, 1.f);
    }

    {
        int intv = 42;
        TEST_CORNER_CASE_NAME_V (int, intv, float);
        EXRCORE_TEST (intv == 42);
    }

    {
        float floatv = 42.f;
        TEST_CORNER_CASE_NAME_V (float, floatv, int);
        EXRCORE_TEST (floatv == 42.f);
    }

    {
        double doublev = 42.0;
        TEST_CORNER_CASE_NAME_V (double, doublev, int);
        EXRCORE_TEST (doublev == 42.0);
    }

    {
        const exr_attr_chlist_t* addlchans = chans;
        TEST_CORNER_CASE_NAME_NOALT (channels, addlchans, addlchans);
    }

    {
        exr_attr_chromaticities_t mychroma = {
            1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
        TEST_CORNER_CASE_NAME (chromaticities, mychroma, int);
        EXRCORE_TEST (mychroma.red_y == 2.f);
        EXRCORE_TEST (mychroma.white_x == 7.f);
    }

    {
        exr_compression_t compt = EXR_COMPRESSION_ZIPS;
        TEST_CORNER_CASE_NAME_V (compression, compt, int);
        EXRCORE_TEST (compt == EXR_COMPRESSION_ZIPS);
    }

    {
        exr_envmap_t envmapt = EXR_ENVMAP_LATLONG;
        TEST_CORNER_CASE_NAME_V (envmap, envmapt, int);
        EXRCORE_TEST (envmapt == EXR_ENVMAP_LATLONG);
    }

    {
        float        myfvec[] = {1.f, 2.f};
        const float* outfvec  = NULL;
        int          fvsz     = 2;
        TEST_CORNER_CASE_NAME_C (float_vector, myfvec, fvsz, &outfvec, int);
        EXRCORE_TEST (fvsz == 2);
        EXRCORE_TEST (outfvec[1] == 2.f);
    }

    {
        exr_attr_keycode_t mykeycodet = {1, 2, 3, 4, 5, 6, 7};
        TEST_CORNER_CASE_NAME (keycode, mykeycodet, int);
        EXRCORE_TEST (mykeycodet.count == 4);
    }

    {
        exr_lineorder_t mylineordt = EXR_LINEORDER_DECREASING_Y;
        TEST_CORNER_CASE_NAME_V (lineorder, mylineordt, int);
        EXRCORE_TEST (mylineordt == EXR_LINEORDER_DECREASING_Y);
    }

    {
        exr_attr_box2i_t tb2i = {{1, 2}, {3, 4}};
        TEST_CORNER_CASE_NAME (box2i, tb2i, int);
        EXRCORE_TEST (tb2i.min.x == 1);
        EXRCORE_TEST (tb2i.min.y == 2);
        EXRCORE_TEST (tb2i.max.x == 3);
        EXRCORE_TEST (tb2i.max.y == 4);
    }

    {
        exr_attr_box2f_t tb2f = {{1.f, 2.f}, {3.f, 4.f}};
        TEST_CORNER_CASE_NAME (box2f, tb2f, int);
        EXRCORE_TEST (tb2f.min.x == 1.f);
        EXRCORE_TEST (tb2f.min.y == 2.f);
        EXRCORE_TEST (tb2f.max.x == 3.f);
        EXRCORE_TEST (tb2f.max.y == 4.f);
    }

    {
        exr_attr_v2i_t tv2i = {1, 2};
        TEST_CORNER_CASE_NAME (v2i, tv2i, int);
        EXRCORE_TEST (tv2i.x == 1);
        EXRCORE_TEST (tv2i.y == 2);
    }

    {
        exr_attr_m33f_t mym33f = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        TEST_CORNER_CASE_NAME (m33f, mym33f, int);
        EXRCORE_TEST (mym33f.m[3] == 4);
    }

    {
        exr_attr_m33d_t mym33d = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        TEST_CORNER_CASE_NAME (m33d, mym33d, int);
        EXRCORE_TEST (mym33d.m[3] == 4);
    }

    {
        exr_attr_m44f_t mym44f = {
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        TEST_CORNER_CASE_NAME (m44f, mym44f, int);
        EXRCORE_TEST (mym44f.m[3] == 4);
    }

    {
        exr_attr_m44d_t mym44d = {
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        TEST_CORNER_CASE_NAME (m44d, mym44d, int);
        EXRCORE_TEST (mym44d.m[3] == 4);
    }

    {
        uint8_t dummy[] = {
            0xDE,
            0xAD,
            0xBE,
            0xEF,
            0xDE,
            0xAD,
            0xBE,
            0xEF,
            0xDE,
            0xAD,
            0xBE,
            0xEF,
            0xDE,
            0xAD,
            0xBE,
            0xEF};
        exr_attr_preview_t mypreview = {2, 2, 0, dummy};
        TEST_CORNER_CASE_NAME (preview, mypreview, int);
    }

    {
        exr_attr_rational_t myrational = {24000, 1001};
        TEST_CORNER_CASE_NAME (rational, myrational, int);
        EXRCORE_TEST (myrational.num == 24000);
        EXRCORE_TEST (myrational.denom == 1001);
    }

    {
        const char* sname = "teststrattr";
        const char* scal  = "testvalue";
        const char* outsv;
        int32_t     outssz;
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_MISSING_CONTEXT_ARG,
            exr_attr_set_string (NULL, partidx, sname, scal));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_ARGUMENT_OUT_OF_RANGE,
            exr_attr_set_string (outf, -1, sname, scal));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_ARGUMENT_OUT_OF_RANGE,
            exr_attr_set_string (outf, 1, sname, scal));
        EXRCORE_TEST_RVAL (exr_attr_set_string (outf, partidx, sname, NULL));
        EXRCORE_TEST_RVAL (exr_attr_set_string (outf, partidx, sname, ""));
        EXRCORE_TEST_RVAL (exr_attr_set_string (outf, partidx, sname, NULL));
        EXRCORE_TEST_RVAL (exr_attr_set_string (outf, partidx, sname, scal));
        int altv = 0;
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_ATTR_TYPE_MISMATCH,
            exr_attr_set_int (outf, partidx, sname, altv));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_MISSING_CONTEXT_ARG,
            exr_attr_get_string (NULL, partidx, sname, &outssz, &outsv));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_ARGUMENT_OUT_OF_RANGE,
            exr_attr_get_string (outf, -1, sname, &outssz, &outsv));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_ARGUMENT_OUT_OF_RANGE,
            exr_attr_get_string (outf, 1, sname, &outssz, &outsv));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_INVALID_ARGUMENT,
            exr_attr_get_string (outf, partidx, NULL, &outssz, &outsv));
        EXRCORE_TEST_RVAL_FAIL (
            EXR_ERR_INVALID_ARGUMENT,
            exr_attr_get_string (outf, partidx, "", &outssz, &outsv));
        EXRCORE_TEST_RVAL (
            exr_attr_get_string (outf, partidx, sname, &outssz, &outsv));
        EXRCORE_TEST (outssz == 9);
        EXRCORE_TEST (0 == strcmp (outsv, scal));
    }

    {
        const char* mysvec[]   = {"foo", "bar"};
        const char* outsvec[3] = {NULL, NULL, NULL};
        int         svsz       = 2;
        TEST_CORNER_CASE_NAME_C (string_vector, mysvec, svsz, outsvec, int);
        EXRCORE_TEST (svsz == 2);
    }

    {
        exr_attr_tiledesc_t mytiled = {
            32,
            32,
            EXR_PACK_TILE_LEVEL_ROUND (
                EXR_TILE_MIPMAP_LEVELS, EXR_TILE_ROUND_DOWN)};
        TEST_CORNER_CASE_NAME (tiledesc, mytiled, int);
    }

    {
        exr_attr_timecode_t mytimecode = {42, 84};
        TEST_CORNER_CASE_NAME (timecode, mytimecode, int);
        EXRCORE_TEST (mytimecode.time_and_flags == 42);
        EXRCORE_TEST (mytimecode.user_data == 84);
    }

    {
        exr_attr_v2f_t tv2f = {1.f, 2.f};
        TEST_CORNER_CASE_NAME (v2f, tv2f, int);
        EXRCORE_TEST (tv2f.x == 1.f);
        EXRCORE_TEST (tv2f.y == 2.f);
    }

    {
        exr_attr_v2d_t tv2d = {1.0, 2.0};
        TEST_CORNER_CASE_NAME (v2d, tv2d, int);
        EXRCORE_TEST (tv2d.x == 1.0);
        EXRCORE_TEST (tv2d.y == 2.0);
    }

    {
        exr_attr_v3i_t tv3i = {1, 2, 3};
        TEST_CORNER_CASE_NAME (v3i, tv3i, int);
        EXRCORE_TEST (tv3i.x == 1);
        EXRCORE_TEST (tv3i.y == 2);
        EXRCORE_TEST (tv3i.z == 3);
    }

    {
        exr_attr_v3f_t tv3f = {1.f, 2.f, 3.f};
        TEST_CORNER_CASE_NAME (v3f, tv3f, int);
        EXRCORE_TEST (tv3f.x == 1.f);
        EXRCORE_TEST (tv3f.y == 2.f);
        EXRCORE_TEST (tv3f.z == 3.f);
    }

    {
        exr_attr_v3d_t tv3d = {1.0, 2.0, 3.0};
        TEST_CORNER_CASE_NAME (v3d, tv3d, int);
        EXRCORE_TEST (tv3d.x == 1.0);
        EXRCORE_TEST (tv3d.y == 2.0);
        EXRCORE_TEST (tv3d.z == 3.0);
    }

    {
        EXRCORE_TEST_RVAL (
            exr_attr_set_user (outf, partidx, "tuser", "mytype", 4, "foo"));
        int32_t     sz;
        const void* ptr;
        const char* type;
        EXRCORE_TEST_RVAL (
            exr_attr_get_user (outf, partidx, "tuser", &type, &sz, &ptr));
        EXRCORE_TEST (0 == strcmp (type, "mytype"));
        EXRCORE_TEST (sz == 4);
        EXRCORE_TEST (0 == strcmp ((const char*) ptr, "foo"));
    }

    int32_t attrcnt = -1;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_get_attribute_count (NULL, partidx, &attrcnt));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_attribute_count (outf, -1, &attrcnt));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_attribute_count (outf, 1, &attrcnt));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_get_attribute_count (outf, partidx, NULL));
    EXRCORE_TEST_RVAL (exr_get_attribute_count (outf, partidx, &attrcnt));
    std::cout << "Testing writing of " << attrcnt << " attributes" << std::endl;

    const exr_attribute_t* attrget;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_get_attribute_by_index (
            NULL, partidx, EXR_ATTR_LIST_FILE_ORDER, 15, &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_attribute_by_index (
            outf, -1, EXR_ATTR_LIST_FILE_ORDER, 15, &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_attribute_by_index (
            outf, 1, EXR_ATTR_LIST_FILE_ORDER, 15, &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_get_attribute_by_index (
            outf, partidx, EXR_ATTR_LIST_FILE_ORDER, 15, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_get_attribute_by_index (
            outf, partidx, (exr_attr_list_access_mode_t) 42, 15, &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_attribute_by_index (
            outf, partidx, EXR_ATTR_LIST_FILE_ORDER, -1, &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_attribute_by_index (
            outf, partidx, EXR_ATTR_LIST_FILE_ORDER, 1234, &attrget));
    EXRCORE_TEST_RVAL (exr_get_attribute_by_index (
        outf, partidx, EXR_ATTR_LIST_FILE_ORDER, 15, &attrget));
    EXRCORE_TEST_RVAL (exr_get_attribute_by_index (
        outf, partidx, EXR_ATTR_LIST_SORTED_ORDER, 15, &attrget));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_get_attribute_by_name (NULL, partidx, "tv3f", &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_attribute_by_name (outf, -1, "tv3f", &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_attribute_by_name (outf, 1, "tv3f", &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_get_attribute_by_name (outf, partidx, "tv3f", NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_get_attribute_by_name (outf, partidx, NULL, &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_get_attribute_by_name (outf, partidx, "", &attrget));
    EXRCORE_TEST_RVAL (
        exr_get_attribute_by_name (outf, partidx, "tv3f", &attrget));

    const exr_attribute_t** attrlistget;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_get_attribute_list (
            NULL, partidx, EXR_ATTR_LIST_FILE_ORDER, &attrcnt, &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_attribute_list (
            outf, -1, EXR_ATTR_LIST_FILE_ORDER, &attrcnt, &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_attribute_list (
            outf, 1, EXR_ATTR_LIST_FILE_ORDER, &attrcnt, &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_get_attribute_list (
            outf, partidx, EXR_ATTR_LIST_FILE_ORDER, NULL, &attrget));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_get_attribute_list (
            outf,
            partidx,
            (exr_attr_list_access_mode_t) 42,
            &attrcnt,
            &attrget));
    attrcnt = 1;
    EXRCORE_TEST_RVAL (exr_get_attribute_list (
        outf, partidx, EXR_ATTR_LIST_FILE_ORDER, &attrcnt, &attrget));
    attrlistget =
        (const exr_attribute_t**) malloc (attrcnt * sizeof (exr_attribute_t*));
    if (attrlistget)
    {
        EXRCORE_TEST_RVAL (exr_get_attribute_list (
            outf, partidx, EXR_ATTR_LIST_FILE_ORDER, &attrcnt, attrlistget));
        EXRCORE_TEST_RVAL (exr_get_attribute_list (
            outf, partidx, EXR_ATTR_LIST_SORTED_ORDER, &attrcnt, attrlistget));
        free (attrlistget);
    }

    EXRCORE_TEST_RVAL (exr_write_header (outf));
    exr_chunk_info_t cinfo;
    EXRCORE_TEST_RVAL (exr_write_scanline_chunk_info (outf, 0, 0, &cinfo));
    exr_encode_pipeline_t encoder;
    EXRCORE_TEST_RVAL (exr_encoding_initialize (outf, 0, &cinfo, &encoder));
    const uint8_t rgb[]                   = {0, 0, 0, 0, 0, 0};
    encoder.channels[0].encode_from_ptr   = rgb + 4;
    encoder.channels[0].user_pixel_stride = 6;
    encoder.channels[0].user_line_stride  = 6;
    encoder.channels[1].encode_from_ptr   = rgb + 2;
    encoder.channels[1].user_pixel_stride = 6;
    encoder.channels[1].user_line_stride  = 6;
    encoder.channels[2].encode_from_ptr   = rgb;
    encoder.channels[2].user_pixel_stride = 6;
    encoder.channels[2].user_line_stride  = 6;

    EXRCORE_TEST_RVAL (
        exr_encoding_choose_default_routines (outf, 0, &encoder));
    EXRCORE_TEST_RVAL (exr_encoding_run (outf, 0, &encoder));
    EXRCORE_TEST_RVAL (exr_encoding_destroy (outf, &encoder));
    EXRCORE_TEST_RVAL (exr_finish (&outf));

    EXRCORE_TEST_RVAL (exr_start_read (&outf, outfn.c_str (), &cinit));
    EXRCORE_TEST_RVAL (exr_finish (&outf));
    remove (outfn.c_str ());
}

void
testUpdateMeta (const std::string& tempdir)
{}

void
testWriteScans (const std::string& tempdir)
{}

void
testWriteTiles (const std::string& tempdir)
{
    exr_context_t             f, outf, testf;
    std::string               outfn = tempdir;
    std::string               fn    = ILM_IMF_TEST_IMAGEDIR;
    int                       partidx;
    int32_t                   partcnt, outpartcnt, levelsx, levelsy;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    fn += "v1.7.test.tiled.exr";
    EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));
    outfn += "v1.7.test.tiled.exr";
    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));
    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "test", EXR_STORAGE_TILED, &partidx));
    EXRCORE_TEST (partidx == 0);

    EXRCORE_TEST_RVAL (exr_copy_unset_attributes (outf, 0, f, 0));

    exr_storage_t ps;
    EXRCORE_TEST_RVAL (exr_get_storage (outf, 0, &ps));
    EXRCORE_TEST (EXR_STORAGE_TILED == ps);

    levelsx = levelsy = -1;
    EXRCORE_TEST_RVAL (exr_get_tile_levels (outf, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 1);
    EXRCORE_TEST (levelsy == 1);

    EXRCORE_TEST_RVAL (exr_get_tile_sizes (outf, 0, 0, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 12);
    EXRCORE_TEST (levelsy == 24);

    EXRCORE_TEST_RVAL_FAIL (EXR_ERR_NOT_OPEN_WRITE, exr_write_header (f));
    EXRCORE_TEST_RVAL (exr_write_header (outf));

    EXRCORE_TEST_RVAL (exr_get_chunk_count (f, 0, &partcnt));
    EXRCORE_TEST_RVAL (exr_get_chunk_count (outf, 0, &outpartcnt));
    EXRCORE_TEST (partcnt == outpartcnt);

    exr_attr_box2i_t dw;
    int              ty, tx;
    void*            cmem     = NULL;
    size_t           cmemsize = 0;

    EXRCORE_TEST_RVAL (exr_get_data_window (outf, 0, &dw));
    ty = 0;
    for (int32_t y = dw.min.y; y <= dw.max.y; y += levelsy)
    {
        tx = 0;
        for (int32_t x = dw.min.x; x <= dw.max.x; x += levelsx)
        {
            exr_chunk_info_t cinfo;
            EXRCORE_TEST_RVAL (
                exr_read_tile_chunk_info (f, 0, tx, ty, 0, 0, &cinfo));
            if (cmemsize < cinfo.packed_size)
            {
                if (cmem) free (cmem);
                cmem = malloc (cinfo.packed_size);
                if (!cmem) throw std::runtime_error ("out of memory");
                cmemsize = cinfo.packed_size;
            }
            EXRCORE_TEST_RVAL (exr_read_chunk (f, 0, &cinfo, cmem));
            EXRCORE_TEST_RVAL (exr_write_tile_chunk (
                outf, 0, tx, ty, 0, 0, cmem, cinfo.packed_size));
            ++tx;
        }
        ++ty;
    }
    if (cmem) free (cmem);
    EXRCORE_TEST_RVAL (exr_finish (&f));
    EXRCORE_TEST_RVAL (exr_finish (&outf));

    EXRCORE_TEST_RVAL (exr_start_read (&testf, outfn.c_str (), &cinit));
    EXRCORE_TEST_RVAL (exr_get_tile_levels (testf, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 1);
    EXRCORE_TEST (levelsy == 1);

    EXRCORE_TEST_RVAL (exr_get_tile_sizes (testf, 0, 0, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 12);
    EXRCORE_TEST (levelsy == 24);
    EXRCORE_TEST_RVAL (exr_finish (&testf));

    remove (outfn.c_str ());
}

void
testWriteMultiPart (const std::string& tempdir)
{
    exr_context_t outf;
    std::string   outfn = tempdir + "testdeepscan.exr";
    int           partidx;

    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));

    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "beauty", EXR_STORAGE_SCANLINE, &partidx));
    EXRCORE_TEST (partidx == 0);
    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "debug", EXR_STORAGE_TILED, &partidx));
    EXRCORE_TEST (partidx == 1);
    EXRCORE_TEST_RVAL (exr_get_count (outf, &partidx));
    EXRCORE_TEST (partidx == 2);

    exr_storage_t storage;
    EXRCORE_TEST_RVAL (exr_get_storage (outf, 0, &storage));
    EXRCORE_TEST (storage == EXR_STORAGE_SCANLINE);
    EXRCORE_TEST_RVAL (exr_get_storage (outf, 1, &storage));
    EXRCORE_TEST (storage == EXR_STORAGE_TILED);

    uint32_t verflags;
    EXRCORE_TEST_RVAL (
        exr_get_file_version_and_flags (outf, &verflags));
    EXRCORE_TEST (verflags == (2 | 0x00001000));

    EXRCORE_TEST_RVAL (exr_finish (&outf));
    remove (outfn.c_str ());
}

void
testStartWriteUTF8 (const std::string& tempdir)
{
    exr_context_t outf;
    // per google translate, image in Japanese
    std::string   outfn = tempdir + "画像.exr";
    int           partidx;

    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;
    cinit.zip_level = 3;
    cinit.flags |= EXR_CONTEXT_FLAG_WRITE_LEGACY_HEADER;

    exr_set_default_zip_compression_level (-1);

    EXRCORE_TEST_RVAL (exr_start_write (
        &outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));
    EXRCORE_TEST_RVAL (
        exr_add_part (outf, "beauty", EXR_STORAGE_SCANLINE, &partidx));
    EXRCORE_TEST (partidx == 0);
    EXRCORE_TEST_RVAL (exr_get_count (outf, &partidx));
    EXRCORE_TEST (partidx == 1);
    partidx = 0;

    int fw = 1;
    int fh = 1;
    exr_attr_box2i_t dataW = { {0, 0}, {0, 0} };

    EXRCORE_TEST_RVAL (
        exr_initialize_required_attr_simple (outf, partidx, fw, fh, EXR_COMPRESSION_NONE));
    EXRCORE_TEST_RVAL (exr_set_data_window (outf, partidx, &dataW));

    EXRCORE_TEST_RVAL (exr_add_channel (
        outf, partidx, "h", EXR_PIXEL_HALF, EXR_PERCEPTUALLY_LOGARITHMIC, 1, 1));
    EXRCORE_TEST_RVAL (exr_write_header (outf));

    exr_chunk_info_t      cinfo;
    exr_encode_pipeline_t encoder;

    EXRCORE_TEST_RVAL (exr_write_scanline_chunk_info (outf, 0, 0, &cinfo));
    EXRCORE_TEST_RVAL (
        exr_encoding_initialize (outf, 0, &cinfo, &encoder));

    uint16_t hval[] = { 0x1234, 0 };
    for (int c = 0; c < encoder.channel_count; ++c)
    {
        encoder.channels[c].encode_from_ptr   = (const uint8_t *)hval;
        encoder.channels[c].user_pixel_stride = 2;
        encoder.channels[c].user_line_stride  = 2;
    }
    EXRCORE_TEST_RVAL (
        exr_encoding_choose_default_routines (outf, 0, &encoder));
    EXRCORE_TEST_RVAL (exr_encoding_run (outf, 0, &encoder));
    EXRCORE_TEST_RVAL (exr_encoding_destroy (outf, &encoder));

    EXRCORE_TEST_RVAL (exr_finish (&outf));
#ifdef _WIN32
    int      wcSize = 0, fnlen = 0;
    wchar_t* wcFn = NULL;

    fnlen  = (int) strlen (outfn.c_str ());
    wcSize = MultiByteToWideChar (CP_UTF8, 0, outfn.c_str (), fnlen, NULL, 0);
    wcFn   = (wchar_t*) malloc (sizeof (wchar_t) * (wcSize + 1));
    if (wcFn)
    {
        MultiByteToWideChar (CP_UTF8, 0, outfn.c_str (), fnlen, wcFn, wcSize);
        wcFn[wcSize] = 0;
    }
    EXRCORE_TEST ( _waccess (wcFn, 0) != -1 );
    _wremove (wcFn);
    free (wcFn);
#else
    EXRCORE_TEST ( access (outfn.c_str (), F_OK) != -1 );
    remove (outfn.c_str ());
#endif
}
