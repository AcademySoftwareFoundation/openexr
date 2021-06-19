// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include "write.h"

#include "test_value.h"

#include <openexr.h>

#include <memory.h>
#include <string.h>

#include <iomanip>
#include <iostream>
#include <vector>

#include <ImathRandom.h>
#include <half.h>

using namespace IMATH_NAMESPACE;

struct pixels
{
    int _w, _h;
    int _stride_x;

    std::vector<uint32_t> i;
    std::vector<float>    f;
    std::vector<uint16_t> h;
    std::vector<uint16_t> rgba[4];

    pixels (int iw, int ih, int s) : _w (iw), _h (ih), _stride_x (s)
    {
        i.resize (_stride_x * _h, 0);
        f.resize (_stride_x * _h, 0.f);
        h.resize (_stride_x * _h, 0);
        for (int c = 0; c < 4; ++c)
            rgba[c].resize (_stride_x * _h, 0);
    }

    void fillZero ()
    {
        for (int y = 0; y < _h; ++y)
        {
            for (int x = 0; x < _w; ++x)
            {
                size_t idx = y * _stride_x + x;
                i[idx]     = 0;
                f[idx]     = 0.f;
                h[idx]     = 0;
                for (int c = 0; c < 4; ++c)
                    rgba[c][idx] = 0;
            }
        }
    }

    void fillDead ()
    {
        for (int y = 0; y < _h; ++y)
        {
            for (int x = 0; x < _w; ++x)
            {
                size_t idx = y * _stride_x + x;
                i[idx]     = 0xDEADBEEF;
                union
                {
                    uint32_t iv;
                    float    fv;
                } tmp;
                tmp.iv = 0xDEADBEEF;
                f[idx] = tmp.fv;
                h[idx] = 0xDEAD;
                for (int c = 0; c < 4; ++c)
                    rgba[c][idx] = 0xDEAD;
            }
        }
    }

    void fillPattern1 ()
    {
        for (int y = 0; y < _h; ++y)
        {
            for (int x = 0; x < _w; ++x)
            {
                size_t idx = y * _stride_x + x;
                i[idx]     = (x + y) & 1;
                f[idx]     = float (i[idx]);
                h[idx]     = half (f[idx]).bits ();
                for (int c = 0; c < 4; ++c)
                    rgba[c][idx] = h[idx];
            }
        }
    }

    void fillPattern2 ()
    {
        for (int y = 0; y < _h; ++y)
        {
            for (int x = 0; x < _w; ++x)
            {
                size_t idx = y * _stride_x + x;
                i[idx]     = x % 100 + 100 * (y % 100);
                f[idx] =
                    half (sin (double (y)) + sin (double (x) * 0.5)).bits ();
                h[idx] =
                    half (sin (double (x)) + sin (double (y) * 0.5)).bits ();
                for (int c = 0; c < 4; ++c)
                    rgba[c][idx] =
                        half (sin (double (x + c)) + sin (double (y) * 0.5))
                            .bits ();
            }
        }
    }

    void fillRandom ()
    {
        Rand48 rand;

        for (int y = 0; y < _h; ++y)
        {
            for (int x = 0; x < _w; ++x)
            {
                size_t idx = y * _stride_x + x;
                i[idx]     = rand.nexti ();
                h[idx]     = (uint16_t) (rand.nexti ());
                for (int c = 0; c < 4; ++c)
                    rgba[c][idx] = (uint16_t) (rand.nexti ());

                union
                {
                    int   i;
                    float f;
                } u;
                u.i = rand.nexti ();

                f[idx] = u.f;
            }
        }
    }

    void compareExact (const pixels& o) const
    {
        for (int y = 0; y < _h; ++y)
        {
            for (int x = 0; x < _w; ++x)
            {
                size_t idx = y * _stride_x + x;
                EXRCORE_TEST (o.i[idx] == i[idx]);
                EXRCORE_TEST (o.h[idx] == h[idx]);
                union
                {
                    uint32_t iv;
                    float    fv;
                } a, b;
                a.fv = o.f[idx];
                b.fv = f[idx];
                EXRCORE_TEST (a.iv == b.iv);
                EXRCORE_TEST (o.rgba[0][idx] == rgba[0][idx]);
                EXRCORE_TEST (o.rgba[1][idx] == rgba[1][idx]);
                EXRCORE_TEST (o.rgba[2][idx] == rgba[2][idx]);
                EXRCORE_TEST (o.rgba[3][idx] == rgba[3][idx]);
            }
        }
    }

    void compareClose (const pixels& o, exr_compression_t comp) const
    {
        for (int y = 0; y < _h; ++y)
        {
            for (int x = 0; x < _w; ++x)
            {
                size_t idx = y * _stride_x + x;
                EXRCORE_TEST (o.i[idx] == i[idx]);
                union
                {
                    uint32_t iv;
                    float    fv;
                } a, b;
                a.fv = o.f[idx];
                b.fv = f[idx];
                if (comp == EXR_COMPRESSION_PXR24)
                {

                    EXRCORE_TEST ((o.h[idx] & 0xfffc) == (h[idx] & 0xfffc));
                    int32_t delta =
                        (int32_t) (a.iv >> 8) - (int32_t) (b.iv >> 8);
                    EXRCORE_TEST (delta < 2 && delta > -2);
                    EXRCORE_TEST (
                        (o.rgba[0][idx] & 0xfffc) == (rgba[0][idx] & 0xfffc));
                    EXRCORE_TEST (
                        (o.rgba[1][idx] & 0xfffc) == (rgba[1][idx] & 0xfffc));
                    EXRCORE_TEST (
                        (o.rgba[2][idx] & 0xfffc) == (rgba[2][idx] & 0xfffc));
                    EXRCORE_TEST (
                        (o.rgba[3][idx] & 0xfffc) == (rgba[3][idx] & 0xfffc));
                }
                else
                {
                    // dwa, b44 don't change floats
                    EXRCORE_TEST (a.iv == b.iv);
                }
            }
        }
    }
};
static const int IMG_WIDTH    = 1371;
static const int IMG_HEIGHT   = 159;
static const int IMG_STRIDE_X = 1376;
static const int IMG_DATA_X   = 17;
static const int IMG_DATA_Y   = 29;

////////////////////////////////////////

static void
fill_pointers (
    pixels&                          p,
    const exr_coding_channel_info_t& curchan,
    int32_t                          idxoffset,
    void**                           ptr,
    int32_t*                         pixelstride,
    int32_t*                         linestride)
{
    if (!strcmp (curchan.channel_name, "A"))
    {
        *ptr         = p.rgba[3].data () + idxoffset;
        *pixelstride = 2;
        *linestride  = 2 * p._stride_x;
    }
    else if (!strcmp (curchan.channel_name, "B"))
    {
        *ptr         = p.rgba[2].data () + idxoffset;
        *pixelstride = 2;
        *linestride  = 2 * p._stride_x;
    }
    else if (!strcmp (curchan.channel_name, "G"))
    {
        *ptr         = p.rgba[1].data () + idxoffset;
        *pixelstride = 2;
        *linestride  = 2 * p._stride_x;
    }
    else if (!strcmp (curchan.channel_name, "R"))
    {
        *ptr         = p.rgba[0].data () + idxoffset;
        *pixelstride = 2;
        *linestride  = 2 * p._stride_x;
    }
    else if (!strcmp (curchan.channel_name, "H"))
    {
        *ptr         = p.h.data () + idxoffset;
        *pixelstride = 2;
        *linestride  = 2 * p._stride_x;
    }
    else if (!strcmp (curchan.channel_name, "F"))
    {
        *ptr         = p.f.data () + idxoffset;
        *pixelstride = 4;
        *linestride  = 4 * p._stride_x;
    }
    else if (!strcmp (curchan.channel_name, "I"))
    {
        *ptr         = p.i.data () + idxoffset;
        *pixelstride = 4;
        *linestride  = 4 * p._stride_x;
    }
    else
    {
        std::cerr << "Unknown channel name '" << curchan.channel_name << "'"
                  << std::endl;
        EXRCORE_TEST (false);
        *ptr         = nullptr;
        *pixelstride = -1;
        *linestride  = -1;
    }
}

////////////////////////////////////////

static void
doEncodeScan (exr_context_t f, pixels& p, int xs, int ys)
{
    int32_t                scansperchunk = 0;
    int                    y, starty, endy;
    exr_chunk_block_info_t cinfo;
    exr_encode_pipeline_t  encoder;
    bool                   first = true;

    EXRCORE_TEST_RVAL (exr_get_scanlines_per_chunk (f, 0, &scansperchunk));
    starty = IMG_DATA_Y * ys;
    endy   = starty + IMG_HEIGHT * ys;
    for (y = starty; y < endy; y += scansperchunk)
    {
        EXRCORE_TEST_RVAL (exr_write_scanline_block_info (f, 0, y, &cinfo));
        if (first)
        {
            EXRCORE_TEST_RVAL (
                exr_encoding_initialize (f, 0, &cinfo, &encoder));
        }
        else
        {
            EXRCORE_TEST_RVAL (exr_encoding_update (f, 0, &cinfo, &encoder));
        }

        int32_t idxoffset = ((y - starty) / ys) * p._stride_x;

        for (int c = 0; c < encoder.channel_count; ++c)
        {
            const exr_coding_channel_info_t& curchan = encoder.channels[c];
            void*                            ptr;
            int32_t                          pixelstride;
            int32_t                          linestride;

            fill_pointers (
                p, curchan, idxoffset, &ptr, &pixelstride, &linestride);

            // make sure the setup has initialized our bytes for us
            EXRCORE_TEST (curchan.user_bytes_per_element == pixelstride);

            encoder.channels[c].encode_from_ptr   = (const uint8_t*) ptr;
            encoder.channels[c].user_pixel_stride = pixelstride;
            encoder.channels[c].user_line_stride  = linestride;
        }

        if (first)
        {
            EXRCORE_TEST_RVAL (
                exr_encoding_choose_default_routines (f, 0, &encoder));
        }
        EXRCORE_TEST_RVAL (exr_encoding_run (f, 0, &encoder));

        first = false;
    }
    EXRCORE_TEST_RVAL (exr_encoding_destroy (f, &encoder));
}

////////////////////////////////////////

static void
doEncodeTile (exr_context_t f, pixels& p, int xs, int ys)
{
    int32_t                tlevx, tlevy;
    int32_t                tilew, tileh;
    int32_t                tilex, tiley;
    int                    y, endy;
    int                    x, endx;
    exr_chunk_block_info_t cinfo;
    exr_encode_pipeline_t  encoder;
    bool                   first = true;

    EXRCORE_TEST (xs == 1 && ys == 1);
    EXRCORE_TEST_RVAL (exr_get_tile_levels (f, 0, &tlevx, &tlevy));
    EXRCORE_TEST (tlevx == 1 && tlevy == 1);

    EXRCORE_TEST_RVAL (exr_get_tile_sizes (f, 0, 0, 0, &tilew, &tileh));
    EXRCORE_TEST (tilew == 32 && tileh == 32);

    y     = IMG_DATA_Y * ys;
    endy  = y + IMG_HEIGHT * ys;
    tiley = 0;
    for (; y < endy; y += tileh, ++tiley)
    {
        tilex = 0;

        x    = IMG_DATA_X * xs;
        endx = x + IMG_WIDTH * xs;
        for (; x < endx; x += tilew, ++tilex)
        {
            EXRCORE_TEST_RVAL (
                exr_write_tile_block_info (f, 0, tilex, tiley, 0, 0, &cinfo));
            if (first)
            {
                EXRCORE_TEST_RVAL (
                    exr_encoding_initialize (f, 0, &cinfo, &encoder));
            }
            else
            {
                EXRCORE_TEST_RVAL (
                    exr_encoding_update (f, 0, &cinfo, &encoder));
            }

            int32_t idxoffset = tiley * tileh * p._stride_x + tilex * tilew;

            for (int c = 0; c < encoder.channel_count; ++c)
            {
                const exr_coding_channel_info_t& curchan = encoder.channels[c];
                void*                            ptr;
                int32_t                          pixelstride;
                int32_t                          linestride;

                fill_pointers (
                    p, curchan, idxoffset, &ptr, &pixelstride, &linestride);

                // make sure the setup has initialized our bytes for us
                EXRCORE_TEST (curchan.user_bytes_per_element == pixelstride);

                encoder.channels[c].encode_from_ptr   = (const uint8_t*) ptr;
                encoder.channels[c].user_pixel_stride = pixelstride;
                encoder.channels[c].user_line_stride  = linestride;
            }

            if (first)
            {
                EXRCORE_TEST_RVAL (
                    exr_encoding_choose_default_routines (f, 0, &encoder));
            }
            EXRCORE_TEST_RVAL (exr_encoding_run (f, 0, &encoder));
            first = false;
        }
    }
    EXRCORE_TEST_RVAL (exr_encoding_destroy (f, &encoder));
}

////////////////////////////////////////

static void
doDecodeScan (exr_context_t f, pixels& p, int xs, int ys)
{
    exr_chunk_block_info_t cinfo;
    exr_decode_pipeline_t  decoder;
    int32_t                scansperchunk;
    exr_attr_box2i_t       dw;
    bool                   first = true;

    EXRCORE_TEST_RVAL (exr_get_data_window (f, 0, &dw));
    EXRCORE_TEST (dw.x_min == IMG_DATA_X * xs);
    EXRCORE_TEST (dw.x_max == IMG_DATA_X * xs + IMG_WIDTH * xs - 1);
    EXRCORE_TEST (dw.y_min == IMG_DATA_Y * ys);
    EXRCORE_TEST (dw.y_max == IMG_DATA_Y * ys + IMG_HEIGHT * ys - 1);

    EXRCORE_TEST_RVAL (exr_get_scanlines_per_chunk (f, 0, &scansperchunk));

    exr_storage_t stortype;
    EXRCORE_TEST_RVAL (exr_get_storage (f, 0, &stortype));
    EXRCORE_TEST (stortype == EXR_STORAGE_SCANLINE);

    for (int y = dw.y_min; y <= dw.y_max; y += scansperchunk)
    {
        EXRCORE_TEST_RVAL (exr_read_scanline_block_info (f, 0, y, &cinfo));
        if (first)
        {
            EXRCORE_TEST_RVAL (
                exr_decoding_initialize (f, 0, &cinfo, &decoder));
        }
        else
        {
            EXRCORE_TEST_RVAL (exr_decoding_update (f, 0, &cinfo, &decoder));
        }
        int32_t idxoffset = (y - dw.y_min) * p._stride_x;

        for (int c = 0; c < decoder.channel_count; ++c)
        {
            const exr_coding_channel_info_t& curchan = decoder.channels[c];
            void*                            ptr;
            int32_t                          pixelstride;
            int32_t                          linestride;

            fill_pointers (
                p, curchan, idxoffset, &ptr, &pixelstride, &linestride);

            // make sure the setup has initialized our bytes for us
            EXRCORE_TEST (curchan.user_bytes_per_element == pixelstride);

            decoder.channels[c].decode_to_ptr     = (uint8_t*) ptr;
            decoder.channels[c].user_pixel_stride = pixelstride;
            decoder.channels[c].user_line_stride  = linestride;
        }

        if (first)
        {
            EXRCORE_TEST_RVAL (
                exr_decoding_choose_default_routines (f, 0, &decoder));
        }
        EXRCORE_TEST_RVAL (exr_decoding_run (f, 0, &decoder));

        first = false;
    }
    EXRCORE_TEST_RVAL (exr_decoding_destroy (f, &decoder));
}

////////////////////////////////////////

static void
doDecodeTile (exr_context_t f, pixels& p, int xs, int ys)
{
    int32_t                tlevx, tlevy;
    int32_t                tilew, tileh;
    int32_t                tilex, tiley;
    int                    y, endy;
    int                    x, endx;
    exr_chunk_block_info_t cinfo;
    exr_decode_pipeline_t  decoder;
    bool                   first = true;

    EXRCORE_TEST (xs == 1 && ys == 1);
    EXRCORE_TEST_RVAL (exr_get_tile_levels (f, 0, &tlevx, &tlevy));
    EXRCORE_TEST (tlevx == 1 && tlevy == 1);

    EXRCORE_TEST_RVAL (exr_get_tile_sizes (f, 0, 0, 0, &tilew, &tileh));
    EXRCORE_TEST (tilew == 32 && tileh == 32);

    y     = IMG_DATA_Y * ys;
    endy  = y + IMG_HEIGHT * ys;
    tiley = 0;
    for (; y < endy; y += tileh, ++tiley)
    {
        tilex = 0;

        x    = IMG_DATA_X * xs;
        endx = x + IMG_WIDTH * xs;
        for (; x < endx; x += tilew, ++tilex)
        {
            EXRCORE_TEST_RVAL (
                exr_read_tile_block_info (f, 0, tilex, tiley, 0, 0, &cinfo));
            if (first)
            {
                EXRCORE_TEST_RVAL (
                    exr_decoding_initialize (f, 0, &cinfo, &decoder));
            }
            else
            {
                EXRCORE_TEST_RVAL (
                    exr_decoding_update (f, 0, &cinfo, &decoder));
            }

            int32_t idxoffset = tiley * tileh * p._stride_x + tilex * tilew;

            for (int c = 0; c < decoder.channel_count; ++c)
            {
                const exr_coding_channel_info_t& curchan = decoder.channels[c];
                void*                            ptr;
                int32_t                          pixelstride;
                int32_t                          linestride;

                fill_pointers (
                    p, curchan, idxoffset, &ptr, &pixelstride, &linestride);

                // make sure the setup has initialized our bytes for us
                EXRCORE_TEST (curchan.user_bytes_per_element == pixelstride);

                decoder.channels[c].decode_to_ptr     = (uint8_t*) ptr;
                decoder.channels[c].user_pixel_stride = pixelstride;
                decoder.channels[c].user_line_stride  = linestride;
            }

            if (first)
            {
                EXRCORE_TEST_RVAL (
                    exr_decoding_choose_default_routines (f, 0, &decoder));
            }
            EXRCORE_TEST_RVAL (exr_decoding_run (f, 0, &decoder));
            first = false;
        }
    }
    EXRCORE_TEST_RVAL (exr_decoding_destroy (f, &decoder));
}

////////////////////////////////////////

static void
doWriteRead (
    pixels&            p,
    const std::string& filename,
    bool               tiled,
    int                xs,
    int                ys,
    exr_compression_t  comp,
    const char*        pattern)
{
    exr_context_t             f;
    int                       partidx;
    int                       fw    = p._w * xs;
    int                       fh    = p._h * ys;
    int                       dwx   = IMG_DATA_X * xs;
    int                       dwy   = IMG_DATA_Y * ys;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    exr_attr_box2i_t          dataW;

    dataW.x_min = dwx;
    dataW.y_min = dwy;
    dataW.x_max = dwx + fw - 1;
    dataW.y_max = dwy + fh - 1;

    std::cout << "  " << pattern << " tiled: " << (tiled ? "yes" : "no")
              << " sampling " << xs << ", " << ys << " comp " << (int) comp
              << std::endl;

    EXRCORE_TEST_RVAL (exr_start_write (
        &f, filename.c_str (), EXR_WRITE_FILE_DIRECTLY, &cinit));
    if (tiled)
    {
        EXRCORE_TEST_RVAL (
            exr_add_part (f, "tiled", EXR_STORAGE_TILED, &partidx));
    }
    else
    {
        EXRCORE_TEST_RVAL (
            exr_add_part (f, "scan", EXR_STORAGE_SCANLINE, &partidx));
    }

    EXRCORE_TEST_RVAL (
        exr_initialize_required_attr_simple (f, partidx, fw, fh, comp));
    EXRCORE_TEST_RVAL (exr_set_data_window (f, partidx, &dataW));

    if (tiled)
    {
        EXRCORE_TEST_RVAL (exr_set_tile_descriptor (
            f, partidx, 32, 32, EXR_TILE_ONE_LEVEL, EXR_TILE_ROUND_UP));
    }

    EXRCORE_TEST_RVAL (
        exr_add_channel (f, partidx, "I", EXR_PIXEL_UINT, 1, xs, ys));

    static const char* channels[] = { "R", "G", "B", "A", "H" };

    for (int c = 0; c < 5; ++c)
    {
        EXRCORE_TEST_RVAL (exr_add_channel (
            f, partidx, channels[c], EXR_PIXEL_HALF, 1, xs, ys));
    }
    EXRCORE_TEST_RVAL (
        exr_add_channel (f, partidx, "F", EXR_PIXEL_FLOAT, 1, xs, ys));

    EXRCORE_TEST_RVAL (exr_write_header (f));
    if (tiled)
        doEncodeTile (f, p, xs, ys);
    else
        doEncodeScan (f, p, xs, ys);
    EXRCORE_TEST_RVAL (exr_finish (&f));

    pixels restore = p;

    restore.fillDead ();

    EXRCORE_TEST_RVAL (exr_start_read (&f, filename.c_str (), &cinit));
    if (tiled)
        doDecodeTile (f, restore, xs, ys);
    else
        doDecodeScan (f, restore, xs, ys);

    EXRCORE_TEST_RVAL (exr_finish (&f));
    remove (filename.c_str ());

    switch (comp)
    {
        case EXR_COMPRESSION_NONE:
        case EXR_COMPRESSION_RLE:
        case EXR_COMPRESSION_ZIP:
        case EXR_COMPRESSION_ZIPS: restore.compareExact (p); break;
        case EXR_COMPRESSION_PIZ:
        case EXR_COMPRESSION_PXR24:
        case EXR_COMPRESSION_B44:
        case EXR_COMPRESSION_B44A:
        case EXR_COMPRESSION_DWAA:
        case EXR_COMPRESSION_DWAB: restore.compareClose (p, comp); break;
        case EXR_COMPRESSION_LAST_TYPE:
        default:
            std::cerr << "Unknown compression type " << (int) (comp)
                      << std::endl;
            EXRCORE_TEST (false);
            break;
    }
}

////////////////////////////////////////

static void
testWriteRead (
    pixels&            p,
    const std::string& tempdir,
    exr_compression_t  comp,
    const char*        pattern)
{
    std::string filename    = tempdir + "imf_test_comp.exr";
    const int   maxsampling = 1; //2;
    for (int xs = 1; xs <= maxsampling; ++xs)
    {
        for (int ys = 1; ys <= maxsampling; ++ys)
        {
            doWriteRead (p, filename, false, xs, ys, comp, pattern);
            if (xs == 1 && ys == 1)
                doWriteRead (p, filename, true, xs, ys, comp, pattern);
        }
    }
}

////////////////////////////////////////

static void
testComp (const std::string& tempdir, exr_compression_t comp)
{
    pixels p{ IMG_WIDTH, IMG_HEIGHT, IMG_STRIDE_X };

    p.fillZero ();
    testWriteRead (p, tempdir, comp, "zeroes");
    p.fillPattern1 ();
    testWriteRead (p, tempdir, comp, "pattern1");
    p.fillPattern2 ();
    testWriteRead (p, tempdir, comp, "pattern2");
    p.fillRandom ();
    testWriteRead (p, tempdir, comp, "random");
}

////////////////////////////////////////

void
testNoCompression (const std::string& tempdir)
{
    testComp (tempdir, EXR_COMPRESSION_NONE);
}

void
testRLECompression (const std::string& tempdir)
{
    testComp (tempdir, EXR_COMPRESSION_RLE);
}

void
testZIPCompression (const std::string& tempdir)
{
    testComp (tempdir, EXR_COMPRESSION_ZIP);
}

void
testZIPSCompression (const std::string& tempdir)
{
    testComp (tempdir, EXR_COMPRESSION_ZIPS);
}

void
testPIZCompression (const std::string& tempdir)
{}

void
testPXR24Compression (const std::string& tempdir)
{
    testComp (tempdir, EXR_COMPRESSION_PXR24);
}

void
testB44Compression (const std::string& tempdir)
{}

void
testB44ACompression (const std::string& tempdir)
{}

void
testDWAACompression (const std::string& tempdir)
{}

void
testDWABCompression (const std::string& tempdir)
{}

void
testDeepNoCompression (const std::string& tempdir)
{}

void
testDeepZIPCompression (const std::string& tempdir)
{}

void
testDeepZIPSCompression (const std::string& tempdir)
{}
