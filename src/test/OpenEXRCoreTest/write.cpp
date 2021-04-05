// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include "write.h"

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

void
testWriteBadArgs (const std::string& tempdir)
{
    exr_context_t             f;
    std::string               fn    = tempdir + "invalid.exr";
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_start_write (NULL, fn.c_str (), EXR_WRITE_FILE_DIRECTLY, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_start_write (&f, NULL, EXR_WRITE_FILE_DIRECTLY, NULL));
    EXRCORE_TEST_RVAL_FAIL (
        EXR_ERR_INVALID_ARGUMENT, exr_start_write (&f, NULL, EXR_WRITE_FILE_DIRECTLY, &cinit));

    //    EXRCORE_TEST_RVAL_FAIL (EXR_ERR_FILE_ACCESS, exr_start_write (&f, fn.c_str (), &cinit));
}

void
testWriteBadFiles (const std::string& tempdir)
{}

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
    EXRCORE_TEST_RVAL (exr_start_write (&outf, outfn.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));
    EXRCORE_TEST_RVAL (
        exr_part_add (outf, "test", EXR_STORAGE_TILED, &partidx));
    EXRCORE_TEST (partidx == 0);

    EXRCORE_TEST_RVAL (exr_part_copy_unset_attributes (outf, 0, f, 0));

    exr_storage_t ps;
    EXRCORE_TEST_RVAL (exr_part_get_storage (outf, 0, &ps));
    EXRCORE_TEST (EXR_STORAGE_TILED == ps);

    levelsx = levelsy = -1;
    EXRCORE_TEST_RVAL (exr_part_get_tile_levels (outf, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 1);
    EXRCORE_TEST (levelsy == 1);

    EXRCORE_TEST_RVAL (
        exr_part_get_tile_sizes (outf, 0, 0, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 12);
    EXRCORE_TEST (levelsy == 24);

    EXRCORE_TEST_RVAL_FAIL (EXR_ERR_NOT_OPEN_WRITE, exr_write_header (f));
    EXRCORE_TEST_RVAL (exr_write_header (outf));

    EXRCORE_TEST_RVAL (exr_part_get_chunk_count (f, 0, &partcnt));
    EXRCORE_TEST_RVAL (exr_part_get_chunk_count (outf, 0, &outpartcnt));
    EXRCORE_TEST (partcnt == outpartcnt);

    exr_attr_box2i_t dw;
    int              curchunk = 0;
    int              ty, tx;
    void*            cmem     = NULL;
    size_t           cmemsize = 0;

    EXRCORE_TEST_RVAL (exr_part_get_data_window (outf, 0, &dw));
    ty = 0;
    for (int32_t y = dw.y_min; y <= dw.y_max; y += levelsy)
    {
        tx = 0;
        for (int32_t x = dw.x_min; x <= dw.x_max; x += levelsx)
        {
            exr_chunk_block_info_t cinfo;
            EXRCORE_TEST_RVAL (
                exr_part_read_tile_block_info (f, 0, tx, ty, 0, 0, &cinfo));
            if (cmemsize < cinfo.packed_size)
            {
                if (cmem) free (cmem);
                cmem = malloc (cinfo.packed_size);
                if (!cmem) throw std::runtime_error ("out of memory");
                cmemsize = cinfo.packed_size;
            }
            EXRCORE_TEST_RVAL (exr_part_read_chunk (f, 0, &cinfo, cmem));
            EXRCORE_TEST_RVAL (exr_part_write_tile_chunk (outf, 0, tx, ty, 0, 0, cmem, cinfo.packed_size));
            ++tx;
        }
        ++ty;
    }
    EXRCORE_TEST_RVAL (exr_finish (&outf));

    EXRCORE_TEST_RVAL (exr_start_read (&testf, outfn.c_str (), &cinit));
    EXRCORE_TEST_RVAL (exr_part_get_tile_levels (testf, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 1);
    EXRCORE_TEST (levelsy == 1);

    EXRCORE_TEST_RVAL (
        exr_part_get_tile_sizes (testf, 0, 0, 0, &levelsx, &levelsy));
    EXRCORE_TEST (levelsx == 12);
    EXRCORE_TEST (levelsy == 24);
    EXRCORE_TEST_RVAL (exr_finish (&testf));

    remove( outfn.c_str() );
}

void
testWriteMultiPart (const std::string& tempdir)
{}

void
testWriteDeep (const std::string& tempdir)
{}
