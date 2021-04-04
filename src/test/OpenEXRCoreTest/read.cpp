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
err_cb (exr_context_t f, int code, const char* msg)
{
    std::cerr << "err_cb ERROR " << code << ": " << msg << std::endl;
}

int64_t
dummyreadstream (
    exr_context_t f,
    void*,
    void*,
    uint64_t,
    uint64_t,
    exr_stream_error_func_ptr_t errcb)
{
    return -1;
}

void
testReadBadArgs (const std::string& tempdir)
{
    exr_context_t             f;
    std::string               fn    = tempdir + "invalid.exr";
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL_FAIL (EXR_ERR_INVALID_ARGUMENT, exr_start_read (NULL, fn.c_str (), NULL));
    EXRCORE_TEST_RVAL_FAIL (EXR_ERR_INVALID_ARGUMENT, exr_start_read (&f, NULL, NULL));
    EXRCORE_TEST_RVAL_FAIL (EXR_ERR_INVALID_ARGUMENT, exr_start_read (&f, NULL, &cinit));

    EXRCORE_TEST_RVAL_FAIL (EXR_ERR_FILE_ACCESS, exr_start_read (&f, fn.c_str (), &cinit));
}

void
testReadBadFiles (const std::string& tempdir)
{
    exr_context_t f;
    std::string   fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "invalid.exr";
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL_FAIL(EXR_ERR_FILE_BAD_HEADER,exr_start_read (&f, fn.c_str (), &cinit));
}

void testReadMeta( const std::string &tempdir )
{
    exr_context_t f;
    std::string   fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.1.exr";
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;
    exr_attribute_t *newattr;
    const exr_attribute_t *attr;

    EXRCORE_TEST_RVAL(exr_start_read (&f, fn.c_str (), &cinit));

    EXRCORE_TEST_RVAL_FAIL(EXR_ERR_NOT_OPEN_WRITE,exr_part_attr_declare_by_type (f, 0, "foo", "box2i", &newattr));
    EXRCORE_TEST_RVAL_FAIL(EXR_ERR_NOT_OPEN_WRITE,exr_part_attr_declare (f, 0, "bar", EXR_ATTR_BOX2I, &newattr));

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

    EXRCORE_TEST_RVAL(exr_start_read (&f, fn.c_str (), &cinit));
    exr_finish (&f);

    fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.planar.exr";
    EXRCORE_TEST_RVAL(exr_start_read (&f, fn.c_str (), &cinit));
    exr_finish (&f);

    fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.interleaved.exr";
    EXRCORE_TEST_RVAL(exr_start_read (&f, fn.c_str (), &cinit));
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
    EXRCORE_TEST_RVAL(exr_start_read (&f, fn.c_str (), &cinit));
    exr_finish (&f);

    fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.tiled.exr";
    EXRCORE_TEST_RVAL(exr_start_read (&f, fn.c_str (), &cinit));
    exr_finish (&f);
}

void
testOpenMultiPart (const std::string& tempdir)
{}

void
testOpenDeep (const std::string& tempdir)
{}

void
testReadScans (const std::string& tempdir)
{}

void
testReadTiles (const std::string& tempdir)
{
    exr_context_t             f;
    std::string               fn    = ILM_IMF_TEST_IMAGEDIR;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    fn += "v1.7.test.tiled.exr";
    EXRCORE_TEST_RVAL(exr_start_read (&f, fn.c_str (), &cinit));

    exr_storage_t ps;
    EXRCORE_TEST_RVAL(exr_part_get_storage (f, 0, &ps));
    EXRCORE_TEST (EXR_STORAGE_TILED == ps);

    int levelsx = -1, levelsy = -1;
    EXRCORE_TEST_RVAL_FAIL(EXR_ERR_MISSING_CONTEXT_ARG, exr_part_get_tile_levels (NULL, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == -1);
    EXRCORE_TEST (levelsy == -1);

    EXRCORE_TEST_RVAL_FAIL(EXR_ERR_ARGUMENT_OUT_OF_RANGE, exr_part_get_tile_levels (f, 1, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == -1);
    EXRCORE_TEST (levelsy == -1);

    EXRCORE_TEST_RVAL(exr_part_get_tile_levels (f, 0, NULL, NULL));

    levelsx = -1;
    EXRCORE_TEST_RVAL(exr_part_get_tile_levels (f, 0, &levelsx, NULL));
    EXRCORE_TEST (levelsx == 1);

    levelsy = -1;
    EXRCORE_TEST_RVAL(exr_part_get_tile_levels (f, 0, NULL, &levelsy));
    EXRCORE_TEST (levelsy == 1);

    levelsx = levelsy = -1;
    EXRCORE_TEST_RVAL(exr_part_get_tile_levels (f, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 1);
    EXRCORE_TEST (levelsy == 1);

    EXRCORE_TEST_RVAL(exr_part_get_tile_sizes (f, 0, 0, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 12);
    EXRCORE_TEST (levelsy == 24);

    exr_chunk_block_info_t cinfo;
    EXRCORE_TEST_RVAL_FAIL(EXR_ERR_SCAN_TILE_MIXEDAPI, exr_part_read_scanline_block_info (f, 0, 42, &cinfo));

    // actually read a tile...
    EXRCORE_TEST_RVAL(exr_part_read_tile_block_info (f, 0, 4, 2, 0, 0, &cinfo));
    uint64_t pchunksz = 0;
    EXRCORE_TEST_RVAL(exr_part_get_chunk_unpacked_size (f, 0, &pchunksz));
    EXRCORE_TEST (cinfo.type == EXR_STORAGE_TILED);
    EXRCORE_TEST (cinfo.compression == EXR_COMPRESSION_NONE);
    EXRCORE_TEST (cinfo.packed_size == pchunksz);
    EXRCORE_TEST (cinfo.unpacked_size == pchunksz);
    EXRCORE_TEST (cinfo.sample_count_data_offset == 0);
    EXRCORE_TEST (cinfo.sample_count_table_size == 0);

    exr_decode_pipeline_t decoder;
    EXRCORE_TEST_RVAL(exr_initialize_decoding (f, 0, &cinfo, &decoder));

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

    std::unique_ptr<uint8_t[]> gptr{ new uint8_t[24 * 12 * 2] };
    std::unique_ptr<uint8_t[]> zptr{ new uint8_t[24 * 12 * 4] };
    memset (gptr.get (), 0, 24 * 12 * 2);
    memset (zptr.get (), 0, 24 * 12 * 4);
    decoder.channels[0].decode_to_ptr            = gptr.get ();
    decoder.channels[0].output_pixel_stride = 2;
    decoder.channels[0].output_line_stride  = 2 * 12;
    decoder.channels[0].output_bytes_per_element = 2;
    decoder.channels[1].decode_to_ptr            = zptr.get ();
    decoder.channels[1].output_pixel_stride = 4;
    decoder.channels[1].output_line_stride  = 4 * 12;
    decoder.channels[1].output_bytes_per_element = 4;

    EXRCORE_TEST_RVAL(exr_decoding_choose_default_routines (f, 0, &decoder));

    EXRCORE_TEST_RVAL(exr_decoding_run (f, 0, &decoder));

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

    EXRCORE_TEST_RVAL(exr_destroy_decoding (f, &decoder));
    exr_finish (&f);

#if 0
    /* TODO: Need to get more test material */
    EXRCORE_TEST_RVAL(exr_start_read (
        &f,
        "/home/kimball/Development/OSS/OpenEXR/kdt3rd/testmips.exr",
        &cinit));

    EXRCORE_TEST_RVAL(exr_part_get_storage (f, 0, &ps));
    EXRCORE_TEST (EXR_STORAGE_TILED == ps);

    levelsx = -1;
    levelsy = -1;
    EXRCORE_TEST_RVAL(exr_part_get_tile_levels (f, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 11);
    EXRCORE_TEST (levelsy == 11);

    EXRCORE_TEST_RVAL(exr_part_get_tile_sizes (f, 0, 0, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 32);
    EXRCORE_TEST (levelsy == 32);

    EXRCORE_TEST_RVAL(exr_part_get_tile_sizes (f, 0, 10, 10, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 1);
    EXRCORE_TEST (levelsy == 1);

    EXRCORE_TEST_RVAL(exr_decode_chunk_init_tile (f, 0, &chunk, 4, 2, 0, 0, 1));
    EXRCORE_TEST_RVAL(exr_part_get_chunk_unpacked_size (f, 0, &pchunksz));
    EXRCORE_TEST (chunk.unpacked.size == pchunksz);
    EXRCORE_TEST (chunk.channel_count == 3);
    EXRCORE_TEST_RVAL(exr_destroy_decode_chunk_info (f, &chunk));

    EXRCORE_TEST_RVAL(exr_decode_chunk_init_tile (f, 0, &chunk, 0, 0, 10, 10, 1));
    EXRCORE_TEST (chunk.unpacked.size == 1 * 1 * 2 * 3);
    EXRCORE_TEST (chunk.channel_count == 3);
    EXRCORE_TEST (chunk.width == 1);
    EXRCORE_TEST (chunk.height == 1);
    EXRCORE_TEST (chunk.channels[0].width == 1);
    EXRCORE_TEST (chunk.channels[0].height == 1);
    EXRCORE_TEST (chunk.channels[1].width == 1);
    EXRCORE_TEST (chunk.channels[1].height == 1);
    EXRCORE_TEST (chunk.channels[2].width == 1);
    EXRCORE_TEST (chunk.channels[2].height == 1);

    EXRCORE_TEST_RVAL(exr_destroy_decode_chunk_info (f, &chunk));
    EXRCORE_TEST_RVAL(exr_finish (&f));
#endif
}

void
testReadMultiPart (const std::string& tempdir)
{}

void
testReadDeep (const std::string& tempdir)
{}
