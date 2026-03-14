// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include "read.h"

#include "test_value.h"

#include <openexr.h>

#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#include <iomanip>
#include <iostream>
#include <memory>

static void
err_cb (exr_const_context_t f, int code, const char* msg)
{
    std::cerr << "err_cb ERROR " << code << ": " << msg << std::endl;
}

int64_t
dummyreadstream (
    exr_const_context_t f,
    void*,
    void*,
    uint64_t,
    uint64_t,
    exr_stream_error_func_ptr_t errcb)
{
    return -1;
}

static int s_malloc_fail_on = 0;
static void*
failable_malloc (size_t bytes)
{
    if (s_malloc_fail_on == 1) return NULL;
    if (s_malloc_fail_on > 0) --s_malloc_fail_on;
    return malloc (bytes);
}

static void
failable_free (void* p)
{
    if (!p) abort ();
    free (p);
}

static void
set_malloc_fail_on (int count)
{
    s_malloc_fail_on = count;
}

static void
set_malloc_fail_off ()
{
    s_malloc_fail_on = 0;
}

void
testReadBadArgs (const std::string& tempdir)
{
    exr_context_t             f;
    std::string               fn;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    exr_set_default_memory_routines (&failable_malloc, &failable_free);
    fn = tempdir;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_start_read (NULL, fn.c_str (), NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_start_read (&f, NULL, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_start_read (&f, NULL, &cinit));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_FILE_ACCESS, exr_start_read (&f, "", &cinit));
    // windows fails on directory open, where under unix you can open
    // the directory as a file handle but not read from it
#ifdef _WIN32
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_FILE_ACCESS, exr_start_read (&f, fn.c_str (), &cinit));
#else
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_READ_IO, exr_start_read (&f, fn.c_str (), &cinit));
#endif
    fn.append ("invalid.exr");
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_FILE_ACCESS, exr_start_read (&f, fn.c_str (), &cinit));
    EXRCORE_TEST_RVAL_FAIL_MALLOC (
        EXR_ERR_OUT_OF_MEMORY, exr_start_read (&f, fn.c_str (), &cinit));

    exr_set_default_memory_routines (NULL, NULL);
}

void
testReadBadFiles (const std::string& tempdir)
{
    exr_context_t f;
    std::string   fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "invalid.exr";
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_FILE_BAD_HEADER, exr_start_read (&f, fn.c_str (), &cinit));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_FILE_ACCESS,
        exr_test_file_header ("somenonexistentexrfile.exr", &cinit));
}

void
testReadMeta (const std::string& tempdir)
{
    exr_context_t f;
    std::string   fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.1.exr";
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;
    exr_attribute_t* newattr;
    uint32_t verflags;
    uint64_t cto;

    EXRCORE_TEST_RVAL (exr_test_file_header (fn.c_str (), &cinit));
    EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_get_file_version_and_flags (NULL, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_get_file_version_and_flags (f, NULL));
    EXRCORE_TEST_RVAL (
        exr_get_file_version_and_flags (f, &verflags));
    EXRCORE_TEST (verflags == 2);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_get_chunk_table_offset (NULL, 0, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_chunk_table_offset (f, -1, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_chunk_table_offset (f, 2, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_get_chunk_table_offset (f, 0, NULL));
    EXRCORE_TEST_RVAL (
        exr_get_chunk_table_offset (f, 0, &cto));
    EXRCORE_TEST (cto == 331);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NOT_OPEN_WRITE,
        exr_attr_declare_by_type (f, 0, "foo", "box2i", &newattr));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NOT_OPEN_WRITE,
        exr_attr_declare (f, 0, "bar", EXR_ATTR_BOX2I, &newattr));

    int partidx;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NOT_OPEN_WRITE,
        exr_add_part (f, "beauty", EXR_STORAGE_TILED, &partidx));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NOT_OPEN_WRITE, exr_set_longname_support (f, 0));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NOT_OPEN_WRITE, exr_set_longname_support (f, 1));

    void* udata = (void*) 3;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_user_data (NULL, &udata));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_user_data (f, NULL));
    udata = (void*) 3;
    EXRCORE_TEST_RVAL (exr_get_user_data (f, &udata));
    EXRCORE_TEST (udata == NULL);

    int zlev = -2;
    EXRCORE_TEST_RVAL (exr_get_zip_compression_level (f, 0, &zlev));
    EXRCORE_TEST (zlev == -1);
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NOT_OPEN_WRITE, exr_set_zip_compression_level (f, 0, 4));

    float dlev = -3.f;
    EXRCORE_TEST_RVAL (exr_get_dwa_compression_level (f, 0, &dlev));
    EXRCORE_TEST (dlev == 45.f);
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_NOT_OPEN_WRITE, exr_set_dwa_compression_level (f, 0, 42.f));

    exr_finish (&f);
}

void
testOpenScans (const std::string& tempdir)
{
    exr_context_t f;
    std::string   fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.1.exr";

    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));
    exr_finish (&f);

    fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.planar.exr";
    EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));
    exr_finish (&f);

    fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.interleaved.exr";
    EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));
    exr_finish (&f);
}

void
testOpenTiles (const std::string& tempdir)
{
    exr_context_t             f;
    std::string               fn    = ILM_IMF_TEST_IMAGEDIR;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    fn += "tiled.exr";
    EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));
    exr_finish (&f);

    fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.tiled.exr";
    EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));
    exr_finish (&f);
}

void
testOpenMultiPart (const std::string& tempdir)
{}

void
testReadScans (const std::string& tempdir)
{
    exr_context_t             f;
    std::string               fn    = ILM_IMF_TEST_IMAGEDIR;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    fn += "v1.7.test.interleaved.exr";
    EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));

    int32_t ccount;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_chunk_count (NULL, 0, &ccount));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_get_chunk_count (f, -1, &ccount));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_get_chunk_count (f, 11, &ccount));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_chunk_count (f, 0, NULL));
    EXRCORE_TEST_RVAL (exr_get_chunk_count (f, 0, &ccount));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_get_scanlines_per_chunk (NULL, 0, &ccount));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_scanlines_per_chunk (f, -1, &ccount));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_scanlines_per_chunk (f, 11, &ccount));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_scanlines_per_chunk (f, 0, NULL));
    EXRCORE_TEST_RVAL (exr_get_scanlines_per_chunk (f, 0, &ccount));
    EXRCORE_TEST (ccount == 1);

    exr_attr_box2i_t dw;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_data_window (NULL, 0, &dw));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_get_data_window (f, -1, &dw));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_get_data_window (f, 1, &dw));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_data_window (f, 0, NULL));
    EXRCORE_TEST_RVAL (exr_get_data_window (f, 0, &dw));

    exr_chunk_info_t cinfo;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_read_scanline_chunk_info (NULL, 0, 42, &cinfo));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_read_scanline_chunk_info (f, -1, 42, &cinfo));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_read_scanline_chunk_info (f, 1, 42, &cinfo));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_read_scanline_chunk_info (f, 0, 42, NULL));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_TILE_SCAN_MIXEDAPI,
        exr_read_tile_chunk_info (f, 0, 4, 2, 0, 0, &cinfo));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_read_scanline_chunk_info (f, 0, dw.min.y - 1, &cinfo));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_read_scanline_chunk_info (f, 0, dw.max.y + 1, &cinfo));
    EXRCORE_TEST_RVAL (exr_read_scanline_chunk_info (f, 0, dw.min.y, &cinfo));

    uint64_t pchunksz = 0;
    EXRCORE_TEST_RVAL (exr_get_chunk_unpacked_size (f, 0, &pchunksz));
    EXRCORE_TEST (cinfo.type == EXR_STORAGE_SCANLINE);
    EXRCORE_TEST (cinfo.compression == EXR_COMPRESSION_NONE);
    EXRCORE_TEST (cinfo.packed_size == pchunksz);
    EXRCORE_TEST (cinfo.unpacked_size == pchunksz);
    EXRCORE_TEST (cinfo.sample_count_data_offset == 0);
    EXRCORE_TEST (cinfo.sample_count_table_size == 0);

    exr_decode_pipeline_t decoder;
    EXRCORE_TEST_RVAL (exr_decoding_initialize (f, 0, &cinfo, &decoder));

    EXRCORE_TEST (decoder.channel_count == 2);
    EXRCORE_TEST (!strcmp (decoder.channels[0].channel_name, "R"));
    EXRCORE_TEST (decoder.channels[0].bytes_per_element == 2);
    EXRCORE_TEST (decoder.channels[0].data_type == EXR_PIXEL_HALF);
    EXRCORE_TEST (decoder.channels[0].width == 178);
    EXRCORE_TEST (decoder.channels[0].height == 1);
    EXRCORE_TEST (decoder.channels[0].x_samples == 1);
    EXRCORE_TEST (decoder.channels[0].y_samples == 1);
    EXRCORE_TEST (!strcmp (decoder.channels[1].channel_name, "Z"));
    EXRCORE_TEST (decoder.channels[1].bytes_per_element == 4);
    EXRCORE_TEST (decoder.channels[1].data_type == EXR_PIXEL_FLOAT);
    EXRCORE_TEST (decoder.channels[1].width == 178);
    EXRCORE_TEST (decoder.channels[1].height == 1);
    EXRCORE_TEST (decoder.channels[1].x_samples == 1);
    EXRCORE_TEST (decoder.channels[1].y_samples == 1);

    std::unique_ptr<uint8_t[]> rptr{new uint8_t[178 * 2]};
    std::unique_ptr<uint8_t[]> zptr{new uint8_t[178 * 4]};
    memset (rptr.get (), -1, 178 * 2);
    memset (zptr.get (), -1, 178 * 4);
    decoder.channels[0].decode_to_ptr     = rptr.get ();
    decoder.channels[0].user_pixel_stride = 2;
    decoder.channels[0].user_line_stride  = 2 * 178;
    decoder.channels[1].decode_to_ptr     = zptr.get ();
    decoder.channels[1].user_pixel_stride = 4;
    decoder.channels[1].user_line_stride  = 4 * 178;

    EXRCORE_TEST_RVAL (exr_decoding_choose_default_routines (f, 0, &decoder));

    EXRCORE_TEST_RVAL (exr_decoding_run (f, 0, &decoder));

    // it is compression: none
    EXRCORE_TEST (decoder.packed_buffer == NULL);
    // it is compression: none
    EXRCORE_TEST (decoder.unpacked_buffer == NULL);
    /* TODO: add actual comparison against C++ library */
    const uint16_t* curr = reinterpret_cast<const uint16_t*> (rptr.get ());
    const float*    curz = reinterpret_cast<const float*> (zptr.get ());
    EXRCORE_TEST (*curr == 0);
    EXRCORE_TEST (fabsf (*curz - 0.101991f) < 0.000001f);

    EXRCORE_TEST_RVAL (exr_decoding_destroy (f, &decoder));

    EXRCORE_TEST_RVAL (exr_decoding_initialize (f, 0, &cinfo, &decoder));
    rptr.reset (new uint8_t[178 * 4]);
    decoder.channels[0].decode_to_ptr          = rptr.get ();
    decoder.channels[0].user_pixel_stride      = 4;
    decoder.channels[0].user_line_stride       = 4 * 178;
    decoder.channels[0].user_bytes_per_element = 4;
    decoder.channels[0].user_data_type         = EXR_PIXEL_FLOAT;
    decoder.channels[1].decode_to_ptr          = zptr.get ();
    decoder.channels[1].user_pixel_stride      = 4;
    decoder.channels[1].user_line_stride       = 4 * 178;

    EXRCORE_TEST_RVAL (exr_decoding_choose_default_routines (f, 0, &decoder));

    EXRCORE_TEST_RVAL (exr_decoding_run (f, 0, &decoder));

    EXRCORE_TEST_RVAL (exr_decoding_destroy (f, &decoder));

    exr_finish (&f);
}

void
testReadTiles (const std::string& tempdir)
{
    exr_context_t             f;
    std::string               fn    = ILM_IMF_TEST_IMAGEDIR;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    fn += "v1.7.test.tiled.exr";
    EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));

    exr_storage_t ps;
    EXRCORE_TEST_RVAL (exr_get_storage (f, 0, &ps));
    EXRCORE_TEST (EXR_STORAGE_TILED == ps);

    int32_t ccount;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG, exr_get_chunk_count (NULL, 0, &ccount));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_get_chunk_count (f, -1, &ccount));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_get_chunk_count (f, 11, &ccount));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_get_chunk_count (f, 0, NULL));
    EXRCORE_TEST_RVAL (exr_get_chunk_count (f, 0, &ccount));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_SCAN_TILE_MIXEDAPI,
        exr_get_scanlines_per_chunk (f, 0, &ccount));

    int levelsx = -1, levelsy = -1;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_get_tile_levels (NULL, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == -1);
    EXRCORE_TEST (levelsy == -1);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_tile_levels (f, 1, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == -1);
    EXRCORE_TEST (levelsy == -1);

    EXRCORE_TEST_RVAL (exr_get_tile_levels (f, 0, NULL, NULL));

    levelsx = -1;
    EXRCORE_TEST_RVAL (exr_get_tile_levels (f, 0, &levelsx, NULL));
    EXRCORE_TEST (levelsx == 1);

    levelsy = -1;
    EXRCORE_TEST_RVAL (exr_get_tile_levels (f, 0, NULL, &levelsy));
    EXRCORE_TEST (levelsy == 1);

    levelsx = levelsy = -1;
    EXRCORE_TEST_RVAL (exr_get_tile_levels (f, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 1);
    EXRCORE_TEST (levelsy == 1);

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_get_tile_sizes (NULL, 0, 0, 0, NULL, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_tile_sizes (f, 0, -1, 0, &levelsx, &levelsy));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_tile_sizes (f, 0, 0, -1, &levelsx, &levelsy));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_tile_sizes (f, 0, 0, 100, &levelsx, &levelsy));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_get_tile_sizes (f, 0, 100, 0, &levelsx, &levelsy));
    EXRCORE_TEST_RVAL (exr_get_tile_sizes (f, 0, 0, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 12);
    EXRCORE_TEST (levelsy == 24);
    levelsx = -1;
    EXRCORE_TEST_RVAL (exr_get_tile_sizes (f, 0, 0, 0, &levelsx, NULL));
    EXRCORE_TEST (levelsx == 12);
    levelsy = -1;
    EXRCORE_TEST_RVAL (exr_get_tile_sizes (f, 0, 0, 0, NULL, &levelsy));
    EXRCORE_TEST (levelsy == 24);

    exr_chunk_info_t cinfo;
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_SCAN_TILE_MIXEDAPI,
        exr_read_scanline_chunk_info (f, 0, 42, &cinfo));

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_MISSING_CONTEXT_ARG,
        exr_read_tile_chunk_info (NULL, 0, 4, 2, 0, 0, &cinfo));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_read_tile_chunk_info (f, -1, 4, 2, 0, 0, &cinfo));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_ARGUMENT_OUT_OF_RANGE,
        exr_read_tile_chunk_info (f, 1, 4, 2, 0, 0, &cinfo));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_read_tile_chunk_info (f, 0, 4, 2, 0, 0, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_read_tile_chunk_info (f, 0, 4, 2, 0, -1, &cinfo));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_read_tile_chunk_info (f, 0, 4, 2, -1, 0, &cinfo));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_read_tile_chunk_info (f, 0, 4, -2, 0, 0, &cinfo));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT,
        exr_read_tile_chunk_info (f, 0, -4, 2, 0, 0, &cinfo));

    // actually read a tile...
    EXRCORE_TEST_RVAL (exr_read_tile_chunk_info (f, 0, 4, 2, 0, 0, &cinfo));
    uint64_t pchunksz = 0;
    EXRCORE_TEST_RVAL (exr_get_chunk_unpacked_size (f, 0, &pchunksz));
    EXRCORE_TEST (cinfo.type == EXR_STORAGE_TILED);
    EXRCORE_TEST (cinfo.compression == EXR_COMPRESSION_NONE);
    EXRCORE_TEST (cinfo.packed_size == pchunksz);
    EXRCORE_TEST (cinfo.unpacked_size == pchunksz);
    EXRCORE_TEST (cinfo.sample_count_data_offset == 0);
    EXRCORE_TEST (cinfo.sample_count_table_size == 0);

    exr_decode_pipeline_t decoder;
    EXRCORE_TEST_RVAL (exr_decoding_initialize (f, 0, &cinfo, &decoder));

    EXRCORE_TEST (decoder.channel_count == 2);
    EXRCORE_TEST (!strcmp (decoder.channels[0].channel_name, "G"));
    EXRCORE_TEST (decoder.channels[0].bytes_per_element == 2);
    EXRCORE_TEST (decoder.channels[0].width == 12);
    EXRCORE_TEST (decoder.channels[0].height == 24);
    EXRCORE_TEST (decoder.channels[0].x_samples == 1);
    EXRCORE_TEST (decoder.channels[0].y_samples == 1);
    EXRCORE_TEST (!strcmp (decoder.channels[1].channel_name, "Z"));
    EXRCORE_TEST (decoder.channels[1].bytes_per_element == 4);
    EXRCORE_TEST (decoder.channels[1].width == 12);
    EXRCORE_TEST (decoder.channels[1].height == 24);
    EXRCORE_TEST (decoder.channels[1].x_samples == 1);
    EXRCORE_TEST (decoder.channels[1].y_samples == 1);

    std::unique_ptr<uint8_t[]> gptr{new uint8_t[24 * 12 * 2]};
    std::unique_ptr<uint8_t[]> zptr{new uint8_t[24 * 12 * 4]};
    memset (gptr.get (), 0, 24 * 12 * 2);
    memset (zptr.get (), 0, 24 * 12 * 4);
    decoder.channels[0].decode_to_ptr          = gptr.get ();
    decoder.channels[0].user_pixel_stride      = 2;
    decoder.channels[0].user_line_stride       = 2 * 12;
    decoder.channels[0].user_bytes_per_element = 2;
    decoder.channels[1].decode_to_ptr          = zptr.get ();
    decoder.channels[1].user_pixel_stride      = 4;
    decoder.channels[1].user_line_stride       = 4 * 12;
    decoder.channels[1].user_bytes_per_element = 4;

    EXRCORE_TEST_RVAL (exr_decoding_choose_default_routines (f, 0, &decoder));

    EXRCORE_TEST_RVAL (exr_decoding_run (f, 0, &decoder));

    // it is compression: none
    EXRCORE_TEST (decoder.packed_buffer == NULL);
    // it is compression: none
    EXRCORE_TEST (decoder.unpacked_buffer == NULL);
    /* TODO: add actual comparison against C++ library */
    const uint16_t* curg = reinterpret_cast<const uint16_t*> (gptr.get ());
    const float*    curz = reinterpret_cast<const float*> (zptr.get ());
    EXRCORE_TEST (*curg == 0x33d5);
    EXRCORE_TEST (fabsf (*curz - 0.244778f) < 0.000001f);
    //for ( int y = 0; y < 24; ++y )
    //{
    //    for ( int x = 0; x < 12; ++x )
    //        std::cout << ' ' << std::hex << std::setw( 4 ) << std::setfill( '0' ) << *curg++ << std::dec << " (" << *curz++ << " )";
    //    std::cout << std::endl;
    //}

    EXRCORE_TEST_RVAL (exr_decoding_destroy (f, &decoder));
    exr_finish (&f);
}

void
testReadMultiPart (const std::string& tempdir)
{}

void
testReadUnpack (const std::string& tempdir)
{
    exr_context_t             f;
    std::string               fn    = ILM_IMF_TEST_IMAGEDIR;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    fn += "v1.7.test.tiled.exr";
    EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));

    exr_chunk_info_t cinfo;
    EXRCORE_TEST_RVAL (exr_read_tile_chunk_info (f, 0, 4, 2, 0, 0, &cinfo));

    {
        exr_decode_pipeline_t decoder;
        EXRCORE_TEST_RVAL (exr_decoding_initialize (f, 0, &cinfo, &decoder));

        std::unique_ptr<float[]>    gptr{new float[24 * 12]};
        std::unique_ptr<uint16_t[]> zptr{new uint16_t[24 * 12]};
        memset (gptr.get (), 0, 24 * 12 * 4);
        memset (zptr.get (), 0, 24 * 12 * 2);
        decoder.channels[0].decode_to_ptr          = (uint8_t*) gptr.get ();
        decoder.channels[0].user_pixel_stride      = 4;
        decoder.channels[0].user_line_stride       = 4 * 12;
        decoder.channels[0].user_bytes_per_element = 4;
        decoder.channels[0].user_data_type         = EXR_PIXEL_FLOAT;
        decoder.channels[1].decode_to_ptr          = (uint8_t*) zptr.get ();
        decoder.channels[1].user_pixel_stride      = 2;
        decoder.channels[1].user_line_stride       = 2 * 12;
        decoder.channels[1].user_bytes_per_element = 2;
        decoder.channels[1].user_data_type         = EXR_PIXEL_HALF;

        EXRCORE_TEST_RVAL (
            exr_decoding_choose_default_routines (f, 0, &decoder));

        EXRCORE_TEST_RVAL (exr_decoding_run (f, 0, &decoder));

        /* TODO: add actual comparison against C++ library */

        EXRCORE_TEST_RVAL (exr_decoding_destroy (f, &decoder));
    }

    exr_finish (&f);
}
