// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include "read.h"

#include "test_value.h"

#include "openexr.h"

#include "ImfArray.h"
#include "ImfChannelList.h"
#include "ImfDeepFrameBuffer.h"
#include "ImfDeepScanLineInputFile.h"
#include "ImfDeepScanLineOutputFile.h"
#include "ImfDeepTiledInputFile.h"
#include "ImfDeepTiledOutputFile.h"
#include "ImfPartType.h"
#include <random>
#include <vector>

namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace IMF;
using namespace IMATH_NAMESPACE;

namespace
{

static void
err_cb (exr_const_context_t f, int code, const char* msg)
{
    std::cerr << "err_cb ERROR " << code << ": " << msg << std::endl;
}

const int width  = 273;
const int height = 169;
const int minX   = 10;
const int minY   = 11;
const Box2i
    dataWindow (V2i (minX, minY), V2i (minX + width - 1, minY + height - 1));
const Box2i
    displayWindow (V2i (0, 0), V2i (minX + width * 2, minY + height * 2));

std::vector<int>               channelTypes;
Array2D<unsigned int>          sampleCountScans;
Array2D<Array2D<unsigned int>> sampleCountTiles;
Header                         header;

void
generateRandomScanFile (
    const std::string& filename, int channelCount, Compression compression)
{
    std::cout << "  generating deep scanline file '" << filename
              << "' compression " << compression << std::endl;
    header = Header (
        displayWindow,
        dataWindow,
        1,
        IMATH_NAMESPACE::V2f (0, 0),
        1,
        INCREASING_Y,
        compression);

    std::default_random_engine generator;
    auto                       generate_random_int = [&] (int range) -> int {
        std::uniform_int_distribution<int> distribution (0, range - 1);
        return distribution (generator);
    };
    //
    // Add channels.
    //

    channelTypes.clear ();

    for (int i = 0; i < channelCount; i++)
    {
        int               type = generate_random_int (3);
        std::stringstream ss;
        ss << i;
        std::string str = ss.str ();
        if (type == 0) header.channels ().insert (str, Channel (IMF::UINT));
        if (type == 1) header.channels ().insert (str, Channel (IMF::HALF));
        if (type == 2) header.channels ().insert (str, Channel (IMF::FLOAT));
        channelTypes.push_back (type);
    }

    header.setType (DEEPSCANLINE);

    Array<Array2D<void*>> data (channelCount);
    for (int i = 0; i < channelCount; i++)
        data[i].resizeErase (height, width);

    sampleCountScans.resizeErase (height, width);

    remove (filename.c_str ());
    DeepScanLineOutputFile file (filename.c_str (), header, 8);

    DeepFrameBuffer frameBuffer;

    frameBuffer.insertSampleCountSlice (Slice (
        IMF::UINT, // type // 7
        (char*) (&sampleCountScans[0][0] - dataWindow.min.x -
                 dataWindow.min.y * width), // base
        sizeof (unsigned int) * 1,          // xStride
        sizeof (unsigned int) * width));    // yStride

    for (int i = 0; i < channelCount; i++)
    {
        PixelType type = NUM_PIXELTYPES;
        if (channelTypes[i] == 0) type = IMF::UINT;
        if (channelTypes[i] == 1) type = IMF::HALF;
        if (channelTypes[i] == 2) type = IMF::FLOAT;

        std::stringstream ss;
        ss << i;
        std::string str = ss.str ();

        int sampleSize = 0;
        if (channelTypes[i] == 0) sampleSize = sizeof (unsigned int);
        if (channelTypes[i] == 1) sampleSize = sizeof (half);
        if (channelTypes[i] == 2) sampleSize = sizeof (float);

        int pointerSize = sizeof (char*);

        frameBuffer.insert (
            str, // name // 6
            DeepSlice (
                type, // type // 7
                (char*) (&data[i][0][0] - dataWindow.min.x -
                         dataWindow.min.y * width), // base // 8
                pointerSize * 1,                    // xStride// 9
                pointerSize * width,                // yStride// 10
                sampleSize));                       // sampleStride
    }

    file.setFrameBuffer (frameBuffer);

    for (int i = 0; i < height; i++)
    {
        //
        // Fill in data at the last minute.
        //

        for (int j = 0; j < width; j++)
        {
            sampleCountScans[i][j] = generate_random_int (10) + 1;
            for (int k = 0; k < channelCount; k++)
            {
                if (channelTypes[k] == 0)
                    data[k][i][j] = new unsigned int[sampleCountScans[i][j]];
                if (channelTypes[k] == 1)
                    data[k][i][j] = new half[sampleCountScans[i][j]];
                if (channelTypes[k] == 2)
                    data[k][i][j] = new float[sampleCountScans[i][j]];
                for (unsigned int l = 0; l < sampleCountScans[i][j]; l++)
                {
                    if (channelTypes[k] == 0)
                        ((unsigned int*) data[k][i][j])[l] =
                            (i * width + j) % 2049;
                    if (channelTypes[k] == 1)
                        ((half*) data[k][i][j])[l] = (i * width + j) % 2049;
                    if (channelTypes[k] == 2)
                        ((float*) data[k][i][j])[l] = (i * width + j) % 2049;
                }
            }
        }
    }

    file.writePixels (height);

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            for (int k = 0; k < channelCount; k++)
            {
                if (channelTypes[k] == 0)
                    delete[] (unsigned int*) data[k][i][j];
                if (channelTypes[k] == 1) delete[] (half*) data[k][i][j];
                if (channelTypes[k] == 2) delete[] (float*) data[k][i][j];
            }

    std::cout << "  --> done" << std::endl;
}

void
generateRandomTileFile (
    const std::string& filename, int channelCount, Compression compression)
{
    std::cout << "  generating deep tiled file '" << filename
              << "' compression " << compression << std::endl;
    header = Header (
        displayWindow,
        dataWindow,
        1,
        IMATH_NAMESPACE::V2f (0, 0),
        1,
        INCREASING_Y,
        compression);

    constexpr bool             relativeCoords = false;
    std::default_random_engine generator;
    auto                       generate_random_int = [&] (int range) -> int {
        std::uniform_int_distribution<int> distribution (0, range - 1);
        return distribution (generator);
    };
    //
    // Add channels.
    //

    channelTypes.clear ();

    for (int i = 0; i < channelCount; i++)
    {
        int               type = generate_random_int (3);
        std::stringstream ss;
        ss << i;
        std::string str = ss.str ();
        if (type == 0) header.channels ().insert (str, Channel (IMF::UINT));
        if (type == 1) header.channels ().insert (str, Channel (IMF::HALF));
        if (type == 2) header.channels ().insert (str, Channel (IMF::FLOAT));
        channelTypes.push_back (type);
    }

    header.setType (DEEPTILE);
    header.setTileDescription (TileDescription (
        generate_random_int (width) + 1,
        generate_random_int (height) + 1,
        RIPMAP_LEVELS));

    //
    // Set up the output file
    //
    remove (filename.c_str ());
    DeepTiledOutputFile file (filename.c_str (), header, 8);

    DeepFrameBuffer frameBuffer;

    Array<Array2D<void*>> data (channelCount);
    for (int i = 0; i < channelCount; i++)
        data[i].resizeErase (height, width);

    Array2D<unsigned int> sampleCount;
    sampleCount.resizeErase (height, width);

    std::cout << "   tileSizeX " << file.tileXSize () << "   tileSizeY "
              << file.tileYSize () << std::endl;

    sampleCountTiles.resizeErase (file.numYLevels (), file.numXLevels ());
    for (int i = 0; i < sampleCountTiles.height (); i++)
        for (int j = 0; j < sampleCountTiles.width (); j++)
            sampleCountTiles[i][j].resizeErase (height, width);

    int memOffset;
    if (relativeCoords)
        memOffset = 0;
    else
        memOffset = dataWindow.min.x + dataWindow.min.y * width;

    frameBuffer.insertSampleCountSlice (Slice (
        IMF::UINT,
        (char*) (&sampleCount[0][0] - memOffset),
        sizeof (unsigned int) * 1,
        sizeof (unsigned int) * width,
        1,
        1,
        0,
        relativeCoords,
        relativeCoords));

    for (int i = 0; i < channelCount; i++)
    {
        PixelType type = NUM_PIXELTYPES;
        if (channelTypes[i] == 0) type = IMF::UINT;
        if (channelTypes[i] == 1) type = IMF::HALF;
        if (channelTypes[i] == 2) type = IMF::FLOAT;

        std::stringstream ss;
        ss << i;
        std::string str = ss.str ();

        int sampleSize = 0;
        if (channelTypes[i] == 0) sampleSize = sizeof (unsigned int);
        if (channelTypes[i] == 1) sampleSize = sizeof (half);
        if (channelTypes[i] == 2) sampleSize = sizeof (float);

        int pointerSize = sizeof (char*);

        frameBuffer.insert (
            str,
            DeepSlice (
                type,
                (char*) (&data[i][0][0] - memOffset),
                pointerSize * 1,
                pointerSize * width,
                sampleSize,
                1,
                1,
                0,
                relativeCoords,
                relativeCoords));
    }

    file.setFrameBuffer (frameBuffer);

    for (int ly = 0; ly < file.numYLevels (); ly++)
        for (int lx = 0; lx < file.numXLevels (); lx++)
        {
            Box2i dataWindowL = file.dataWindowForLevel (lx, ly);

            for (int j = 0; j < file.numYTiles (ly); j++)
            {
                for (int i = 0; i < file.numXTiles (lx); i++)
                {
                    Box2i box = file.dataWindowForTile (i, j, lx, ly);
                    for (int y = box.min.y; y <= box.max.y; y++)
                        for (int x = box.min.x; x <= box.max.x; x++)
                        {
                            int dwy = y - dataWindowL.min.y;
                            int dwx = x - dataWindowL.min.x;
                            sampleCount[dwy][dwx] =
                                generate_random_int (10) + 1;
                            sampleCountTiles[ly][lx][dwy][dwx] =
                                sampleCount[dwy][dwx];
                            for (int k = 0; k < channelCount; k++)
                            {
                                if (channelTypes[k] == 0)
                                    data[k][dwy][dwx] =
                                        new unsigned int[sampleCount[dwy][dwx]];
                                if (channelTypes[k] == 1)
                                    data[k][dwy][dwx] =
                                        new half[sampleCount[dwy][dwx]];
                                if (channelTypes[k] == 2)
                                    data[k][dwy][dwx] =
                                        new float[sampleCount[dwy][dwx]];
                                for (unsigned int l = 0;
                                     l < sampleCount[dwy][dwx];
                                     l++)
                                {
                                    if (channelTypes[k] == 0)
                                        ((unsigned int*) data[k][dwy][dwx])[l] =
                                            (dwy * width + dwx) % 2049;
                                    if (channelTypes[k] == 1)
                                        ((half*) data[k][dwy][dwx])[l] =
                                            (dwy * width + dwx) % 2049;
                                    if (channelTypes[k] == 2)
                                        ((float*) data[k][dwy][dwx])[l] =
                                            (dwy * width + dwx) % 2049;
                                }
                            }
                        }
                }
            }

            file.writeTiles (
                0, file.numXTiles (lx) - 1, 0, file.numYTiles (ly) - 1, lx, ly);

            for (int i = 0; i < file.levelHeight (ly); i++)
                for (int j = 0; j < file.levelWidth (lx); j++)
                    for (int k = 0; k < channelCount; k++)
                    {
                        if (channelTypes[k] == 0)
                            delete[] (unsigned int*) data[k][i][j];
                        if (channelTypes[k] == 1)
                            delete[] (half*) data[k][i][j];
                        if (channelTypes[k] == 2)
                            delete[] (float*) data[k][i][j];
                    }
        }
    std::cout << "   --> done" << std::endl;
}

} // namespace

void
testOpenDeep (const std::string& tempdir)
{
    std::string fn = tempdir;

    exr_context_t             f;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    fn += "randomtempdeep.exr";

    int         chancounts[] = {1, 3, 10, 0};
    Compression comps[] = {NO_COMPRESSION, RLE_COMPRESSION, ZIPS_COMPRESSION};
    for (int c = 0; chancounts[c] > 0; ++c)
    {
        for (int cp = 0; cp < 3; ++cp)
        {
            generateRandomScanFile (fn, chancounts[c], comps[cp]);
            try
            {
                DeepScanLineInputFile file (fn.c_str (), 8);
            }
            catch (std::exception& e)
            {
                std::cerr << "ERROR: c++ unable to open generated deep scane"
                          << e.what () << std::endl;
            }

            EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));
            exr_finish (&f);
            generateRandomTileFile (fn, chancounts[c], comps[cp]);
            EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));
            exr_finish (&f);
        }
    }
    remove (fn.c_str ());
}

void
testReadDeep (const std::string& tempdir)
{
    std::string fn = tempdir;

    exr_context_t             f;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    fn += "randomtempdeep.exr";

    int         chancounts[] = {1, 3, 10, 0};
    Compression comps[] = {NO_COMPRESSION, RLE_COMPRESSION, ZIPS_COMPRESSION};
    std::vector<uint8_t> packed;
    std::vector<uint8_t> sampdata;
    exr_chunk_info_t     cinfo;

    for (int c = 0; chancounts[c] > 0; ++c)
    {
        for (int cp = 0; cp < 3; ++cp)
        {
            generateRandomScanFile (fn, chancounts[c], comps[cp]);
            try
            {
                DeepScanLineInputFile file (fn.c_str (), 8);
            }
            catch (std::exception& e)
            {
                std::cerr << "ERROR: c++ unable to open generated deep scane"
                          << e.what () << std::endl;
            }

            EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));

            EXRCORE_TEST_RVAL (
                exr_read_scanline_chunk_info (f, 0, minY + height / 2, &cinfo));
            packed.resize (cinfo.packed_size);
            sampdata.resize (cinfo.sample_count_table_size);
            EXRCORE_TEST_RVAL (
                exr_read_deep_chunk (f, 0, &cinfo, &packed[0], &sampdata[0]));
            if (comps[cp] == NO_COMPRESSION)
            {
                // The table is returned as stored in the file, i.e.
                // little-endian, so decode it explicitly rather than
                // reinterpreting it as host-order integers.
                size_t   N    = sampdata.size () / sizeof (uint32_t);
                auto     le32 = [&] (size_t i) -> uint32_t {
                    const uint8_t* b = sampdata.data () + i * 4;
                    return uint32_t (b[0]) | (uint32_t (b[1]) << 8) |
                           (uint32_t (b[2]) << 16) | (uint32_t (b[3]) << 24);
                };
                EXRCORE_TEST (N == width);
                size_t bps = 0;
                for (auto& c: channelTypes)
                {
                    if (c == 0) bps += sizeof (uint32_t);
                    if (c == 1) bps += sizeof (uint16_t);
                    if (c == 2) bps += sizeof (float);
                }
                EXRCORE_TEST (packed.size () == le32 (N - 1) * bps);
            }

            EXRCORE_TEST_RVAL (
                exr_read_scanline_chunk_info (f, 0, minY + height / 4, &cinfo));
            packed.resize (cinfo.packed_size);
            sampdata.resize (cinfo.sample_count_table_size);
            // we support reading the two bits separately
            EXRCORE_TEST_RVAL (
                exr_read_deep_chunk (f, 0, &cinfo, &packed[0], NULL));
            EXRCORE_TEST_RVAL (
                exr_read_deep_chunk (f, 0, &cinfo, NULL, &sampdata[0]));

            exr_finish (&f);

            generateRandomTileFile (fn, chancounts[c], comps[cp]);
            EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));

            EXRCORE_TEST_RVAL (
                exr_read_tile_chunk_info (f, 0, 0, 1, 0, 0, &cinfo));
            packed.resize (cinfo.packed_size);
            sampdata.resize (cinfo.sample_count_table_size);
            EXRCORE_TEST_RVAL (
                exr_read_deep_chunk (f, 0, &cinfo, &packed[0], &sampdata[0]));

            exr_finish (&f);
        }
    }
    remove (fn.c_str ());
}

void
testWriteDeep (const std::string& tempdir)
{}

//
// Writes a deep tiled ZSTD fixture with its sample count table replaced by
// counts, decodes the tile with the given extra decode flags, and returns the
// result of exr_decoding_run.
//
static exr_result_t
decodeDeepZstdTile (
    const std::string& tempdir, const int32_t counts[3], uint16_t decode_flags)
{
    //
    // A single 1x3 deep tile (channels A half, B float), ZSTD compressed. The
    // 12-byte sample count table (100, 10, -10) is stored uncompressed and
    // directly precedes the 52-byte ZSTD payload at the end of the file. The
    // payload unpacks to 200 half + 400 float bytes, so any non-negative counts
    // summing to 100 describe a valid chunk.
    //
    // Because the table is stored uncompressed, default decode flags make the
    // decoder use it in place (sample_count_table == packed_sample_count_table),
    // while EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL makes it decode to a separate
    // copy.
    //
    static const uint8_t deepZstdTileData[] = {
        0x76, 0x2f, 0x31, 0x01, 0x02, 0x08, 0x00, 0x00, 0x63, 0x68, 0x61, 0x6e,
        0x6e, 0x65, 0x6c, 0x73, 0x00, 0x63, 0x68, 0x6c, 0x69, 0x73, 0x74, 0x00,
        0x25, 0x00, 0x00, 0x00, 0x41, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x42, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x63, 0x68, 0x75, 0x6e, 0x6b, 0x43, 0x6f,
        0x75, 0x6e, 0x74, 0x00, 0x69, 0x6e, 0x74, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x63, 0x6f, 0x6d, 0x70, 0x72, 0x65, 0x73, 0x73,
        0x69, 0x6f, 0x6e, 0x00, 0x63, 0x6f, 0x6d, 0x70, 0x72, 0x65, 0x73, 0x73,
        0x69, 0x6f, 0x6e, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0d, 0x64, 0x61, 0x74,
        0x61, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x00, 0x62, 0x6f, 0x78, 0x32,
        0x69, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x64, 0x69,
        0x73, 0x70, 0x6c, 0x61, 0x79, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x00,
        0x62, 0x6f, 0x78, 0x32, 0x69, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
        0x00, 0x00, 0x6c, 0x69, 0x6e, 0x65, 0x4f, 0x72, 0x64, 0x65, 0x72, 0x00,
        0x6c, 0x69, 0x6e, 0x65, 0x4f, 0x72, 0x64, 0x65, 0x72, 0x00, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x70, 0x69, 0x78, 0x65, 0x6c, 0x41, 0x73, 0x70, 0x65,
        0x63, 0x74, 0x52, 0x61, 0x74, 0x69, 0x6f, 0x00, 0x66, 0x6c, 0x6f, 0x61,
        0x74, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x73, 0x63,
        0x72, 0x65, 0x65, 0x6e, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x43, 0x65,
        0x6e, 0x74, 0x65, 0x72, 0x00, 0x76, 0x32, 0x66, 0x00, 0x08, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x63, 0x72,
        0x65, 0x65, 0x6e, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x57, 0x69, 0x64,
        0x74, 0x68, 0x00, 0x66, 0x6c, 0x6f, 0x61, 0x74, 0x00, 0x04, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x80, 0x3f, 0x74, 0x69, 0x6c, 0x65, 0x73, 0x00, 0x74,
        0x69, 0x6c, 0x65, 0x64, 0x65, 0x73, 0x63, 0x00, 0x09, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70,
        0x65, 0x00, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x00, 0x08, 0x00, 0x00,
        0x00, 0x64, 0x65, 0x65, 0x70, 0x74, 0x69, 0x6c, 0x65, 0x76, 0x65, 0x72,
        0x73, 0x69, 0x6f, 0x6e, 0x00, 0x69, 0x6e, 0x74, 0x00, 0x04, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x8e, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x02,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x0a, 0x00,
        0x00, 0x00, 0xf6, 0xff, 0xff, 0xff, 0x7a, 0x73, 0x74, 0x64, 0x2d, 0x65,
        0x78, 0x72, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0xb5, 0x2f, 0xfd, 0x00, 0x58,
        0x9d, 0x00, 0x00, 0x38, 0xc8, 0x00, 0x41, 0x90, 0x01, 0x00, 0x42, 0x04,
        0x10, 0x00, 0x8c, 0x4a, 0x08, 0x43, 0xa4, 0x77, 0xc0, 0x02};

    std::vector<uint8_t> data (
        deepZstdTileData, deepZstdTileData + sizeof (deepZstdTileData));
    uint8_t* tbl = data.data () + data.size () - 64;
    EXRCORE_TEST (tbl[0] == 0x64 && tbl[8] == 0xf6);
    for (int i = 0; i < 3; ++i)
    {
        tbl[i * 4 + 0] = (uint8_t) (counts[i] & 0xff);
        tbl[i * 4 + 1] = (uint8_t) ((counts[i] >> 8) & 0xff);
        tbl[i * 4 + 2] = (uint8_t) ((counts[i] >> 16) & 0xff);
        tbl[i * 4 + 3] = (uint8_t) ((counts[i] >> 24) & 0xff);
    }

    std::string fn = tempdir + "deepzstdsamplecounts.exr";
    FILE*       fp = fopen (fn.c_str (), "wb");
    EXRCORE_TEST (fp != NULL);
    EXRCORE_TEST (fwrite (data.data (), 1, data.size (), fp) == data.size ());
    fclose (fp);

    exr_context_t             f;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));

    exr_chunk_info_t      cinfo;
    exr_decode_pipeline_t decoder = EXR_DECODE_PIPELINE_INITIALIZER;
    EXRCORE_TEST_RVAL (exr_read_tile_chunk_info (f, 0, 0, 0, 0, 0, &cinfo));
    EXRCORE_TEST_RVAL (exr_decoding_initialize (f, 0, &cinfo, &decoder));
    decoder.decode_flags |= decode_flags;
    EXRCORE_TEST_RVAL (exr_decoding_choose_default_routines (f, 0, &decoder));
    exr_result_t rv = exr_decoding_run (f, 0, &decoder);
    exr_decoding_destroy (f, &decoder);
    exr_finish (&f);

    remove (fn.c_str ());
    return rv;
}

//
// Negative sample counts must be rejected before the pixel data is unsorted,
// otherwise the inverse sort writes past the unpacked buffer.
//
void
testDeepZstdNegativeSampleCounts (const std::string& tempdir)
{
    const int32_t counts[3] = {100, 10, -10};

    EXRCORE_TEST (
        decodeDeepZstdTile (tempdir, counts, 0) ==
        EXR_ERR_INVALID_SAMPLE_DATA);
    EXRCORE_TEST (
        decodeDeepZstdTile (
            tempdir, counts, EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL) ==
        EXR_ERR_INVALID_SAMPLE_DATA);
}

//
// A valid chunk whose sample count table is stored uncompressed must decode.
//
void
testDeepZstdValidSampleCounts (const std::string& tempdir)
{
    const int32_t counts[3] = {50, 25, 25};

    EXRCORE_TEST (
        decodeDeepZstdTile (tempdir, counts, 0) == EXR_ERR_SUCCESS);
    EXRCORE_TEST (
        decodeDeepZstdTile (
            tempdir, counts, EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL) ==
        EXR_ERR_SUCCESS);
}
