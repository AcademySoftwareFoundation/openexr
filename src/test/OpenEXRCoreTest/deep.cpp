// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include "read.h"

#include "test_value.h"

#include <openexr.h>

#include <ImfArray.h>
#include <ImfChannelList.h>
#include <ImfDeepFrameBuffer.h>
#include <ImfDeepScanLineInputFile.h>
#include <ImfDeepScanLineOutputFile.h>
#include <ImfDeepTiledInputFile.h>
#include <ImfDeepTiledOutputFile.h>
#include <ImfPartType.h>
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
        (char*) (&sampleCountScans[0][0] - dataWindow.min.x - dataWindow.min.y * width), // base
        sizeof (unsigned int) * 1,       // xStride
        sizeof (unsigned int) * width)); // yStride

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
                (char*) (&data[i][0][0] - dataWindow.min.x - dataWindow.min.y * width), // base // 8
                pointerSize * 1,     // xStride// 9
                pointerSize * width, // yStride// 10
                sampleSize));        // sampleStride
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
                if (channelTypes[k] == 0) delete[](unsigned int*) data[k][i][j];
                if (channelTypes[k] == 1) delete[](half*) data[k][i][j];
                if (channelTypes[k] == 2) delete[](float*) data[k][i][j];
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
                            delete[](unsigned int*) data[k][i][j];
                        if (channelTypes[k] == 1) delete[](half*) data[k][i][j];
                        if (channelTypes[k] == 2)
                            delete[](float*) data[k][i][j];
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

    int         chancounts[] = { 1, 3, 10, 0 };
    Compression comps[] = { NO_COMPRESSION, RLE_COMPRESSION, ZIPS_COMPRESSION };
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

    int         chancounts[] = { 1, 3, 10, 0 };
    Compression comps[] = { NO_COMPRESSION, RLE_COMPRESSION, ZIPS_COMPRESSION };
    std::vector<uint8_t> packed;
    std::vector<uint8_t> sampdata;
    exr_chunk_info_t cinfo;

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

            EXRCORE_TEST_RVAL (exr_read_scanline_chunk_info (f, 0, minY + height / 2, &cinfo));
            packed.resize(cinfo.packed_size);
            sampdata.resize(cinfo.sample_count_table_size);
            EXRCORE_TEST_RVAL (exr_read_deep_chunk (f, 0, &cinfo, &packed[0], &sampdata[0]));
            if (comps[cp] == NO_COMPRESSION)
            {
                const uint32_t *sampcount = reinterpret_cast<const uint32_t *>( sampdata.data() );
                size_t N = sampdata.size() / sizeof(uint32_t);
                EXRCORE_TEST(N == width);
                size_t bps = 0;
                for (auto &c: channelTypes)
                {
                    if (c == 0) bps += sizeof(uint32_t);
                    if (c == 1) bps += sizeof(uint16_t);
                    if (c == 2) bps += sizeof(float);
                }
                EXRCORE_TEST(packed.size() == (sampcount[N-1]) * bps);
            }

            EXRCORE_TEST_RVAL (exr_read_scanline_chunk_info (f, 0, minY + height / 4, &cinfo));
            packed.resize(cinfo.packed_size);
            sampdata.resize(cinfo.sample_count_table_size);
            // we support reading the two bits separately
            EXRCORE_TEST_RVAL (exr_read_deep_chunk (f, 0, &cinfo, &packed[0], NULL));
            EXRCORE_TEST_RVAL (exr_read_deep_chunk (f, 0, &cinfo, NULL, &sampdata[0]));

            exr_finish (&f);

            generateRandomTileFile (fn, chancounts[c], comps[cp]);
            EXRCORE_TEST_RVAL (exr_start_read (&f, fn.c_str (), &cinit));

            EXRCORE_TEST_RVAL (exr_read_tile_chunk_info (f, 0, 0, 1, 0, 0, &cinfo));
            packed.resize(cinfo.packed_size);
            sampdata.resize(cinfo.sample_count_table_size);
            EXRCORE_TEST_RVAL (exr_read_deep_chunk (f, 0, &cinfo, &packed[0], &sampdata[0]));

            exr_finish (&f);
        }
    }
    remove (fn.c_str ());
}

void
testWriteDeep (const std::string& tempdir)
{}
