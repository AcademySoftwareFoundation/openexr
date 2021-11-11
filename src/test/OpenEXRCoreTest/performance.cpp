// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include <errno.h>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <chrono>
#include <set>
#include <string>
#include <vector>

#include <IlmThreadPool.h>
#include <ImfChannelList.h>
#include <ImfCompressor.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfInputPart.h>
#include <ImfMultiPartInputFile.h>
#include <ImfThreading.h>
#include <openexr.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace ILMTHREAD_NAMESPACE;

static void
error_handler_new (exr_const_context_t file, int code, const char* msg)
{
    const char* fn;
    exr_get_file_name (file, &fn);
    std::cerr << "Core EXR ERROR:";
    if (file) std::cerr << " '" << fn << "'";
    std::cerr << " (" << code << "): " << msg << std::endl;
}

class CoreReadTask : public Task
{
public:
    CoreReadTask (TaskGroup* g, exr_context_t f, int y, uint8_t* ptr)
        : Task (g), _f (f), _y (y), _ptr (ptr)
    {}
    void execute () override
    {
        exr_chunk_info_t      cinfo = { 0 };
        exr_decode_pipeline_t chunk;
        exr_result_t rv = exr_read_scanline_chunk_info (_f, 0, _y, &cinfo);
        if (rv == EXR_ERR_SUCCESS)
            rv = exr_decoding_initialize (_f, 0, &cinfo, &chunk);
        if (rv == EXR_ERR_SUCCESS)
        {
            uint8_t* curchanptr    = _ptr;
            int      bytesperpixel = 0;
            for (int c = 0; c < chunk.channel_count; ++c)
                bytesperpixel += chunk.channels[c].bytes_per_element;
            for (int c = 0; c < chunk.channel_count; ++c)
            {
                exr_coding_channel_info_t& outc = chunk.channels[c];
                outc.decode_to_ptr              = curchanptr;
                outc.user_pixel_stride          = bytesperpixel;
                outc.user_line_stride           = outc.width * bytesperpixel;
                outc.user_bytes_per_element =
                    chunk.channels[c].bytes_per_element;
                curchanptr += chunk.channels[c].bytes_per_element;
            }

            rv = exr_decoding_choose_default_routines (_f, 0, &chunk);
        }
        if (rv == EXR_ERR_SUCCESS) rv = exr_decoding_run (_f, 0, &chunk);
        exr_decoding_destroy (_f, &chunk);
    }

private:
    exr_context_t _f;
    int           _y;
    uint8_t*      _ptr;
};

//#define THREADS 0
#define THREADS 16

static uint64_t
read_pixels_raw (exr_context_t f)
{
    uint64_t ret = 0;

    exr_attr_box2i_t dw;
    if (EXR_ERR_SUCCESS != exr_get_data_window (f, 0, &dw))
        throw std::logic_error ("Unable to query data window from part");
    int64_t w = (int64_t) dw.max.x - (int64_t) dw.min.x + (int64_t) 1;
    int64_t h = (int64_t) dw.max.x - (int64_t) dw.min.x + (int64_t) 1;

    if (w <= 0) return ret;
    if (h <= 0) return ret;

    exr_storage_t stortype;
    if (EXR_ERR_SUCCESS != exr_get_storage (f, 0, &stortype))
        throw std::logic_error ("Unable to query storage type from part");

    if (stortype == EXR_STORAGE_TILED || stortype == EXR_STORAGE_DEEP_TILED)
        throw std::logic_error ("Tiled performance read test NYI");

    exr_lineorder_t lo;
    if (EXR_ERR_SUCCESS != exr_get_lineorder (f, 0, &lo))
        throw std::logic_error ("Unable to query line order from part");
    if (lo != EXR_LINEORDER_DECREASING_Y)
    {
        std::vector<uint8_t> rawBuf;
        uint64_t             sizePerChunk = 0;
        int32_t              ccount = 0, linesread = 0;
        if (EXR_ERR_SUCCESS !=
            exr_get_chunk_unpacked_size (f, 0, &sizePerChunk))
            throw std::logic_error (
                "Unable to get chunk unpacked size for part 0");
        if (EXR_ERR_SUCCESS != exr_get_chunk_count (f, 0, &ccount))
            throw std::logic_error ("Unable to get chunk count for part 0");
        if (EXR_ERR_SUCCESS != exr_get_scanlines_per_chunk (f, 0, &linesread))
            throw std::logic_error (
                "Unable to get scanlines per chunk for part 0");
        if (ccount <= 0 || sizePerChunk == 0 || sizePerChunk == size_t (-1))
        {
            exr_print_context_info (f, 1);
            std::cerr << "sizePerChunk: " << sizePerChunk << std::endl;
            std::cerr << "chunk_count: " << ccount << std::endl;
            std::cerr << "linesperchunk: " << linesread << std::endl;
            throw std::logic_error ("invalid chunk information");
        }

        rawBuf.resize (size_t (ccount) * sizePerChunk);
        uint8_t* imgptr = rawBuf.data ();

#if THREADS > 0
        TaskGroup   taskgroup;
        ThreadPool& tp = ThreadPool::globalThreadPool ();

        for (int y = dw.min.y; y <= dw.max.y;)
        {
            tp.addTask (new CoreReadTask (&taskgroup, f, y, imgptr));
            imgptr += sizePerChunk;
            y += linesread;
            ret += linesread * w;
        }
#else
        exr_chunk_info_t      cinfo = { 0 };
        exr_decode_pipeline_t chunk = { 0 };
        for (int y = dw.min.y; y <= dw.max.y;)
        {
            exr_result_t rv = exr_read_scanline_chunk_info (f, 0, y, &cinfo);
            if (rv != EXR_ERR_SUCCESS)
                throw std::runtime_error ("unable to init scanline block info");

            if (y == dw.min.y)
                rv = exr_initialize_decoding (f, 0, &cinfo, &chunk);
            else
                rv = exr_decoding_update (f, 0, &cinfo, &chunk);

            if (rv != EXR_ERR_SUCCESS)
                throw std::runtime_error ("unable to init decoding pipeline");

            uint8_t* curchanptr    = imgptr;
            int      bytesperpixel = 0;
            for (int c = 0; c < chunk.channel_count; ++c)
                bytesperpixel += chunk.channels[c].bytes_per_element;
            for (int c = 0; c < chunk.channel_count; ++c)
            {
                exr_coding_channel_info_t& outc = chunk.channels[c];
                outc.decode_to_ptr              = curchanptr;
                outc.user_pixel_stride          = bytesperpixel;
                outc.user_line_stride           = outc.width * bytesperpixel;
                outc.user_bytes_per_element =
                    chunk.channels[c].bytes_per_element;
                curchanptr += chunk.channels[c].bytes_per_element;
            }

            rv = exr_decoding_choose_default_routines (f, 0, &chunk);
            if (rv != EXR_ERR_SUCCESS)
                throw std::runtime_error ("unable to choose default routines");
            rv = exr_decoding_run (f, 0, &chunk);
            if (rv != EXR_ERR_SUCCESS)
                throw std::runtime_error ("unable to run decoding pipeline");
            imgptr += sizePerChunk;
            y += linesread;
            ret += linesread * w;
        }
        exr_destroy_decoding (f, &chunk);
#endif
#if 0
        std::vector<uint8_t> compBuf, rawBuf;
        const exr_attr_chlist_t *channels = exr_get_channels( f, 0 );
        size_t sizePerChunk = exr_get_chunk_unpacked_size( f, 0 );
        int32_t ccount = exr_get_chunk_count( f, 0 );
        if ( ccount < 0 || sizePerChunk == size_t(-1) )
            throw std::logic_error( "invalid chunk information" );

        rawBuf.resize( size_t(ccount) * sizePerChunk );
        uint8_t *imgptr = rawBuf.data();

        // random or increasing
        for ( int y = dw.min.y; y <= dw.max.y; )
        {
            exr_chunk_info_t chunk = {0};
            if ( exr_compute_chunk_for_scanline( f, 0, y, &chunk ) )
                throw std::runtime_error( "Unable to compute chunk for scanline" );
            compBuf.resize( chunk.packed_size );
            if ( chunk.unpacked_size > sizePerChunk )
                throw std::logic_error( "invalid chunk size" );
            rawBuf.resize( chunk.unpacked_size );

            linesread = chunk.scan_last_y - chunk.scan_start_y + 1;

            if ( exr_read_chunk( f, &chunk, compBuf.data(), compBuf.size(),
                                      imgptr, chunk.unpacked_size ) )
                throw std::runtime_error( "Unable to compute chunk for scanline" );

            imgptr += chunk.unpacked_size;
            y += linesread;
            ret += linesread * w;
        }
#endif
    }
    else
        throw std::runtime_error ("decreasing y not yet finished");

    return ret;
}

static void
readCore (
    const std::string& fn,
    uint64_t&          headerTimeAccum,
    uint64_t&          imgDataTimeAccum,
    uint64_t&          closeTimeAccum,
    uint64_t&          pixCount)
{
    try
    {
        exr_context_t             c;
        exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;

        cinit.error_handler_fn = &error_handler_new;

        auto hstart = std::chrono::steady_clock::now ();
        if (EXR_ERR_SUCCESS == exr_start_read (&c, fn.c_str (), &cinit))
        {
            auto hend = std::chrono::steady_clock::now ();
            pixCount += read_pixels_raw (c);
            auto imgtime = std::chrono::steady_clock::now ();
            exr_finish (&c);
            auto closetime = std::chrono::steady_clock::now ();
            headerTimeAccum +=
                std::chrono::duration_cast<std::chrono::nanoseconds> (
                    hend - hstart)
                    .count ();
            imgDataTimeAccum +=
                std::chrono::duration_cast<std::chrono::nanoseconds> (
                    imgtime - hend)
                    .count ();
            closeTimeAccum +=
                std::chrono::duration_cast<std::chrono::nanoseconds> (
                    closetime - imgtime)
                    .count ();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "readCore: " << e.what () << std::endl;
        throw;
    }
}

static uint64_t
read_pixels_raw (MultiPartInputFile* f)
{
    auto&    head = f->header (0);
    auto&    dw   = head.dataWindow ();
    int64_t  w    = dw.max.x - dw.min.x + 1;
    int64_t  h    = dw.max.y - dw.min.y + 1;
    int      linesread;
    uint64_t ret = 0;

    if (w <= 0) return ret;

    if (head.hasTileDescription ())
        throw std::logic_error ("Tiled performance read test NYI");

    if (head.lineOrder () != DECREASING_Y)
    {
        std::vector<char>  rawBuf;
        const char*        outPtr;
        InputPart          part{ *f, 0 };
        const ChannelList& chans      = head.channels ();
        int                layercount = 0;
        int                bpp        = 0;

        for (auto b = chans.begin (), e = chans.end (); b != e; ++b)
        {
            ++layercount;
            if (b.channel ().type == HALF)
                bpp += 2;
            else
                bpp += 4;
        }

        int scanlinebytes = w * bpp;
        rawBuf.resize (h * scanlinebytes);
        char* data = rawBuf.data ();
        char* buf  = (char*) data - dw.min.x * bpp - dw.min.y * scanlinebytes;

        FrameBuffer frameBuffer;
        int              chanoffset = 0;
        for (auto c = chans.begin (), e = chans.end (); c != e; ++c)
        {
            frameBuffer.insert (
                c.name (),
                Slice (
                    c.channel ().type, buf + chanoffset, bpp, scanlinebytes));
            if (c.channel ().type == HALF)
                chanoffset += 2;
            else
                chanoffset += 4;
        }

        part.setFrameBuffer (frameBuffer);
        part.readPixels (dw.min.y, dw.max.y);
#if 0
        Compressor *comp = nullptr;
        try
        {
            if ( layercount == 0 )
                throw std::runtime_error( "channels" );
            comp = newCompressor( head.compression(), w * bpp, head );
            
            int linesread = comp->numScanLines();
            int bufSize = linesread * w * 4 * layercount;
            if ( bufSize == 0 )
                throw std::runtime_error( "bufsize" );
            rawBuf.resize( bufSize );

            // random or increasing
            for ( int y = dw.min.y; y <= dw.max.y; )
            {
                int pixDS = bufSize;
                part.rawPixelDataToBuffer( y, rawBuf.data(), pixDS );
                comp->uncompress( rawBuf.data(), pixDS, y, outPtr );
                y += linesread;
                ret += linesread * w;
            }
        }
        catch ( ... )
        {
            delete comp;
            throw;
        }
#endif
    }
    else
        throw std::runtime_error ("decreasing y not yet finished");
    return ret;
}

static void
readImf (
    const std::string& fn,
    uint64_t&          headerTimeAccum,
    uint64_t&          imgDataTimeAccum,
    uint64_t&          closeTimeAccum,
    uint64_t&          pixCount)
{
    try
    {
        auto                hstart = std::chrono::steady_clock::now ();
        MultiPartInputFile* infile = new MultiPartInputFile (fn.c_str ());
        auto                hend   = std::chrono::steady_clock::now ();
        pixCount += read_pixels_raw (infile);
        auto imgtime = std::chrono::steady_clock::now ();
        delete infile;
        auto closetime = std::chrono::steady_clock::now ();
        headerTimeAccum +=
            std::chrono::duration_cast<std::chrono::nanoseconds> (hend - hstart)
                .count ();
        imgDataTimeAccum +=
            std::chrono::duration_cast<std::chrono::nanoseconds> (
                imgtime - hend)
                .count ();
        closeTimeAccum += std::chrono::duration_cast<std::chrono::nanoseconds> (
                              closetime - imgtime)
                              .count ();
    }
    catch (std::exception& e)
    {
        std::cerr << "MultiPartInputFile: " << e.what ();
    }
}

static int
usageAndExit (const char* argv0, int ec)
{
    std::cerr << "Usage: " << argv0 << "[--imf|--core] <file1> [<file2>...]"
              << std::endl;
    return ec;
}

int
main (int argc, char* argv[])
{
    std::vector<std::string> files;
    bool                     coreOnly = false, imfOnly = false;
    for (int a = 1; a < argc; ++a)
    {
        if (!strcmp (argv[a], "-h") || !strcmp (argv[a], "--help") ||
            !strcmp (argv[a], "-?"))
        {
            return usageAndExit (argv[0], 0);
        }
        else if (!strcmp (argv[a], "--imf"))
        {
            imfOnly = true;
            if (coreOnly)
            {
                std::cerr << "--imf and --core are mutually exclusive"
                          << std::endl;
                return usageAndExit (argv[0], 1);
            }
        }
        else if (!strcmp (argv[a], "--core"))
        {
            coreOnly = true;
            if (imfOnly)
            {
                std::cerr << "--imf and --core are mutually exclusive"
                          << std::endl;
                return usageAndExit (argv[0], 1);
            }
        }
        else
            files.push_back (argv[a]);
    }

    if (files.empty ()) return usageAndExit (argv[0], 1);

    setGlobalThreadCount (THREADS);
    bool     odd          = false;
    uint64_t headerNanosN = 0, dataNanosN = 0, closeNanosN = 0, pixCountN = 0,
             fileCount    = 0;
    uint64_t headerNanosO = 0, dataNanosO = 0, closeNanosO = 0, pixCountO = 0;
    constexpr int count = 20;
    for (int c = 0; c < count; ++c)
    {
        for (auto& f: files)
        {
            try
            {
                if (odd)
                {
                    if (!imfOnly)
                        readCore (
                            f,
                            headerNanosN,
                            dataNanosN,
                            closeNanosN,
                            pixCountN);
                    if (!coreOnly)
                        readImf (
                            f,
                            headerNanosO,
                            dataNanosO,
                            closeNanosO,
                            pixCountO);
                }
                else
                {
                    if (!coreOnly)
                        readImf (
                            f,
                            headerNanosO,
                            dataNanosO,
                            closeNanosO,
                            pixCountO);
                    if (!imfOnly)
                        readCore (
                            f,
                            headerNanosN,
                            dataNanosN,
                            closeNanosN,
                            pixCountN);
                }
            }
            catch (std::exception& e)
            {
                std::cerr << "ERROR: " << e.what () << std::endl;
                return 1;
            }
            odd = !odd;
            ++fileCount;
        }
    }

    if (pixCountN != pixCountO)
        std::cerr << "ERROR: different pixel counts recorded: core "
                  << pixCountN << " ilmimf " << pixCountO << std::endl;

    std::cout << "Stats for reading: " << files.size () << " files " << count
              << " times\n\n";

    uint64_t totN = headerNanosN + dataNanosN + closeNanosN;
    uint64_t totO = headerNanosO + dataNanosO + closeNanosO;
    std::cout << " Timers  " << std::setw (15) << std::left
              << std::setfill (' ') << "Core"
              << " IlmImf\n"
              << " Header: " << std::setw (15) << std::left
              << std::setfill (' ') << headerNanosN << " " << std::setw (15)
              << std::left << std::setfill (' ') << headerNanosO << " ns\n"
              << "   Data: " << std::setw (15) << std::left
              << std::setfill (' ') << dataNanosN << " " << std::setw (15)
              << std::left << std::setfill (' ') << dataNanosO << " ns\n"
              << "  Close: " << std::setw (15) << std::left
              << std::setfill (' ') << closeNanosN << " " << std::setw (15)
              << std::left << std::setfill (' ') << closeNanosO << " ns\n"
              << "  Total: " << std::setw (15) << std::left
              << std::setfill (' ') << totN << " " << std::setw (15)
              << std::left << std::setfill (' ') << totO << " ns\n";

    double aveHN = double (headerNanosN) / double (fileCount);
    double aveDN = double (dataNanosN) / double (fileCount);
    double aveCN = double (closeNanosN) / double (fileCount);
    double aveTN = double (totN) / double (fileCount);
    double aveHO = double (headerNanosO) / double (fileCount);
    double aveDO = double (dataNanosO) / double (fileCount);
    double aveCO = double (closeNanosO) / double (fileCount);
    double aveTO = double (totO) / double (fileCount);

    std::cout << "\n"
              << " Ave     " << std::setw (15) << std::left
              << std::setfill (' ') << "Core"
              << " IlmImf\n"
              << " Header: " << std::setw (15) << std::left
              << std::setfill (' ') << aveHN << " " << std::setw (15)
              << std::left << std::setfill (' ') << aveHO << " ns\n"
              << "   Data: " << std::setw (15) << std::left
              << std::setfill (' ') << aveDN << " " << std::setw (15)
              << std::left << std::setfill (' ') << aveDO << " ns\n"
              << "  Close: " << std::setw (15) << std::left
              << std::setfill (' ') << aveCN << " " << std::setw (15)
              << std::left << std::setfill (' ') << aveCO << " ns\n"
              << "  Total: " << std::setw (15) << std::left
              << std::setfill (' ') << aveTN << " " << std::setw (15)
              << std::left << std::setfill (' ') << aveTO << " ns\n";

    if (imfOnly || coreOnly) return 0;

    double ratioH = double (headerNanosO) / double (headerNanosN);
    double ratioD = double (dataNanosO) / double (dataNanosN);
    double ratioC = double (closeNanosO) / double (closeNanosN);
    double ratioT = double (totO) / double (totN);

    std::cout << "\n"
              << " Ratios\n"
              << " Header: " << ratioH << "\n"
              << "   Data: " << ratioD << "\n"
              << "  Close: " << ratioC << "\n"
              << "  Total: " << ratioT << std::endl;

    return 0;
}
