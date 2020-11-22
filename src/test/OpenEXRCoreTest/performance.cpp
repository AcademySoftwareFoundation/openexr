// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include <errno.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <time.h>

#include <vector>
#include <string>
#include <chrono>
#include <set>

#include <openexr.h>
#include <ImfMultiPartInputFile.h>
#include <ImfInputPart.h>
#include <ImfCompressor.h>
#include <ImfChannelList.h>
#include <ImfThreading.h>
#include <IlmThreadPool.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace ILMTHREAD_NAMESPACE;

static void error_handler_new( EXR_TYPE(FILE) *file, int code, const char *msg )
{
    std::cerr << "Core EXR ERROR:";
    if ( file )
        std::cerr << " '" << EXR_FUN(get_file_name)( file ) << "'";
    std::cerr << " (" << code << "): " << msg << std::endl;
}

class CoreReadTask : public Task
{
public:
    CoreReadTask( TaskGroup *g, EXR_TYPE(FILE) *f, int y, uint8_t *ptr )
        : Task( g ), _f( f ), _y( y ), _ptr( ptr )
    {}
    void execute() override
    {
        EXR_TYPE(decode_chunk_info) chunk = {0};
        int rv = EXR_FUN(decode_chunk_init_scanline)( _f, 0, &chunk, _y, 1 );
        if ( rv == EXR_DEF(ERR_SUCCESS) )
        {
            uint8_t *curchanptr = _ptr;
            int bytesperpixel = 0;
            for ( int c = 0; c < chunk.channel_count; ++c )
                bytesperpixel += chunk.channels[c].bytes_per_pel;
            for ( int c = 0; c < chunk.channel_count; ++c )
            {
                EXR_TYPE(channel_decode_info) &outc = chunk.channels[c];
                outc.data_ptr = curchanptr;
                outc.output_pixel_stride = bytesperpixel;
                outc.output_line_stride = outc.width * bytesperpixel;
                curchanptr += chunk.channels[c].bytes_per_pel;
            }
            rv = EXR_FUN(read_chunk)( _f, &chunk );
        }
        EXR_FUN(destroy_decode_chunk_info)( &chunk );
    }
private:
    EXR_TYPE(FILE) *_f;
    int _y;
    uint8_t *_ptr;
};

#define THREADS 16

static uint64_t read_pixels_raw( EXR_TYPE(FILE) *f )
{
    EXR_TYPE(attr_box2i) dw = EXR_FUN(get_data_window)( f, 0 );
    int64_t w = (int64_t)dw.x_max - (int64_t)dw.x_min + (int64_t)1;
    int64_t h = (int64_t)dw.x_max - (int64_t)dw.x_min + (int64_t)1;
    uint64_t ret = 0;

    if ( w <= 0 )
        return ret;
    if ( h <= 0 )
        return ret;

    auto stortype = EXR_FUN(get_part_storage)( f, 0 );

    if ( stortype == EXR_DEF(STORAGE_TILED) ||
         stortype == EXR_DEF(STORAGE_DEEP_TILED) )
        throw std::logic_error( "Tiled performance read test NYI" );

    if ( EXR_FUN(get_line_order)( f, 0 ) != EXR_DEF(LINEORDER_DECREASING_Y) )
    {
        std::vector<uint8_t> rawBuf;
        size_t sizePerChunk = EXR_FUN(get_chunk_unpacked_size)( f, 0 );
        int32_t ccount = EXR_FUN(get_chunk_count)( f, 0 );
        int32_t linesread = EXR_FUN(get_scanlines_per_chunk)( f, 0 );
        if ( ccount <= 0 || sizePerChunk == 0 || sizePerChunk == size_t(-1) )
        {
            EXR_FUN(print_info)( f, 1 );
            std::cerr << "sizePerChunk: " << sizePerChunk << std::endl;
            std::cerr << "chunk_count: " << ccount << std::endl;
            std::cerr << "linesperchunk: " << linesread << std::endl;
            throw std::logic_error( "invalid chunk information" );
        }

        rawBuf.resize( size_t(ccount) * sizePerChunk );
        uint8_t *imgptr = rawBuf.data();

#if THREADS > 0
        TaskGroup taskgroup;
        ThreadPool &tp = ThreadPool::globalThreadPool();

        for ( int y = dw.y_min; y <= dw.y_max; )
        {
            tp.addTask( new CoreReadTask( &taskgroup, f, y, imgptr ) );
            imgptr += sizePerChunk;
            y += linesread;
            ret += linesread * w;
        }
#else
        EXR_TYPE(decode_chunk_info) chunk = {0};
        for ( int y = dw.y_min; y <= dw.y_max; )
        {
            int rv = EXR_FUN(decode_chunk_init_scanline)( f, 0, &chunk, y, 1 );
            if ( rv == EXR_DEF(ERR_SUCCESS) )
            {
                uint8_t *curchanptr = imgptr;
                int bytesperpixel = 0;
                for ( int c = 0; c < chunk.channel_count; ++c )
                    bytesperpixel += chunk.channels[c].bytes_per_pel;
                for ( int c = 0; c < chunk.channel_count; ++c )
                {
                    EXR_TYPE(channel_decode_info) &outc = chunk.channels[c];
                    outc.data_ptr = curchanptr;
                    outc.output_pixel_stride = bytesperpixel;
                    outc.output_line_stride = outc.width * bytesperpixel;
                    curchanptr += chunk.channels[c].bytes_per_pel;
                }
                rv = EXR_FUN(read_chunk)( f, &chunk );
                EXR_FUN(destroy_decode_chunk_info)( &chunk );
            }
            imgptr += sizePerChunk;
            y += linesread;
            ret += linesread * w;
        }
#endif
#if 0
        std::vector<uint8_t> compBuf, rawBuf;
        const EXR_TYPE(attr_chlist) *channels = EXR_FUN(get_channels)( f, 0 );
        size_t sizePerChunk = EXR_FUN(get_chunk_unpacked_size)( f, 0 );
        int32_t ccount = EXR_FUN(get_chunk_count)( f, 0 );
        if ( ccount < 0 || sizePerChunk == size_t(-1) )
            throw std::logic_error( "invalid chunk information" );

        rawBuf.resize( size_t(ccount) * sizePerChunk );
        uint8_t *imgptr = rawBuf.data();

        // random or increasing
        for ( int y = dw.y_min; y <= dw.y_max; )
        {
            EXR_TYPE(chunk_info) chunk = {0};
            if ( EXR_FUN(compute_chunk_for_scanline)( f, 0, y, &chunk ) )
                throw std::runtime_error( "Unable to compute chunk for scanline" );
            compBuf.resize( chunk.packed_size );
            if ( chunk.unpacked_size > sizePerChunk )
                throw std::logic_error( "invalid chunk size" );
            rawBuf.resize( chunk.unpacked_size );

            linesread = chunk.scan_last_y - chunk.scan_start_y + 1;

            if ( EXR_FUN(read_chunk)( f, &chunk, compBuf.data(), compBuf.size(),
                                      imgptr, chunk.unpacked_size ) )
                throw std::runtime_error( "Unable to compute chunk for scanline" );

            imgptr += chunk.unpacked_size;
            y += linesread;
            ret += linesread * w;
        }
#endif
    }
    else
        throw std::runtime_error( "decreasing y not yet finished" );

    return ret;
}

static void readCore( const std::string &fn,
                      uint64_t &headerTimeAccum,
                      uint64_t &imgDataTimeAccum,
                      uint64_t &closeTimeAccum,
                      uint64_t &pixCount )
{
    EXR_TYPE(FILE) *f = NULL;

    auto hstart = std::chrono::steady_clock::now();
    if ( EXR_DEF(ERR_SUCCESS) == EXR_FUN(start_read)( &f, fn.c_str(), &error_handler_new ) )
    {
        auto hend = std::chrono::steady_clock::now();
        pixCount += read_pixels_raw( f );
        auto imgtime = std::chrono::steady_clock::now();
        EXR_FUN(close)( &f );
        auto closetime = std::chrono::steady_clock::now();
        headerTimeAccum += std::chrono::duration_cast<std::chrono::nanoseconds>( hend - hstart ).count();
        imgDataTimeAccum += std::chrono::duration_cast<std::chrono::nanoseconds>( imgtime - hend ).count();
        closeTimeAccum += std::chrono::duration_cast<std::chrono::nanoseconds>( closetime - imgtime ).count();
    }
}

static uint64_t read_pixels_raw( MultiPartInputFile *f )
{
    auto &head = f->header(0);
    auto &dw = head.dataWindow();
    int64_t w = dw.max.x - dw.min.x + 1;
    int64_t h = dw.max.y - dw.min.y + 1;
    int linesread;
    uint64_t ret = 0;

    if ( w <= 0 )
        return ret;

    if ( head.hasTileDescription() )
        throw std::logic_error( "Tiled performance read test NYI" );

    if ( head.lineOrder() != DECREASING_Y )
    {
        std::vector<char> rawBuf;
        const char *outPtr;
        InputPart part { *f, 0 };
        const ChannelList &chans = head.channels();
        int layercount = 0;
        int bpp = 0;

        for ( auto b = chans.begin(), e = chans.end(); b != e; ++b )
        {
            ++layercount;
            if ( b.channel().type == HALF )
                bpp += 2;
            else
                bpp += 4;
        }

        int scanlinebytes = w * bpp;
        rawBuf.resize( h * scanlinebytes );
        char *data = rawBuf.data();
        char *buf = (char*)data - dw.min.x * bpp - dw.min.y * scanlinebytes;

        Imf::FrameBuffer frameBuffer;
        int chanoffset = 0;
        for ( auto c = chans.begin(), e = chans.end(); c != e; ++c )
        {
            frameBuffer.insert(c.name(),
                               Imf::Slice(c.channel().type,
                                          buf + chanoffset,
                                          bpp, scanlinebytes) );
            if ( c.channel().type == HALF )
                chanoffset += 2;
            else
                chanoffset += 4;
        }

        part.setFrameBuffer( frameBuffer );
        part.readPixels( dw.min.y, dw.max.y );
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
        throw std::runtime_error( "decreasing y not yet finished" );
    return ret;
}

static void readImf( const std::string &fn,
                     uint64_t &headerTimeAccum,
                     uint64_t &imgDataTimeAccum,
                     uint64_t &closeTimeAccum,
                     uint64_t &pixCount )
{
    try
    {
        auto hstart = std::chrono::steady_clock::now();
        MultiPartInputFile *infile = new MultiPartInputFile( fn.c_str() );
        auto hend = std::chrono::steady_clock::now();
        pixCount += read_pixels_raw( infile );
        auto imgtime = std::chrono::steady_clock::now();
        delete infile;
        auto closetime = std::chrono::steady_clock::now();
        headerTimeAccum += std::chrono::duration_cast<std::chrono::nanoseconds>( hend - hstart ).count();
        imgDataTimeAccum += std::chrono::duration_cast<std::chrono::nanoseconds>( imgtime - hend ).count();
        closeTimeAccum += std::chrono::duration_cast<std::chrono::nanoseconds>( closetime - imgtime ).count();
    }
    catch ( std::exception &e )
    {
        std::cerr << "MultiPartInputFile: " << e.what();
    }
}

static int usageAndExit( const char *argv0, int ec )
{
    std::cerr << "Usage: " << argv0 << "[--imf|--core] <file1> [<file2>...]" << std::endl;
    return ec;
}

int main(int argc, char *argv[])
{
    std::vector<std::string> files;
    bool coreOnly = false, imfOnly = false;
    for ( int a = 1; a < argc; ++a )
    {
        if (!strcmp(argv[a], "-h") ||
            !strcmp(argv[a], "--help") ||
            !strcmp(argv[a], "-?"))
        {
            return usageAndExit( argv[0], 0 );
        }
        else if ( !strcmp(argv[a], "--imf"))
        {
            imfOnly = true;
            if ( coreOnly )
            {
                std::cerr << "--imf and --core are mutually exclusive" << std::endl;
                return usageAndExit( argv[0], 1 );
            }
        }
        else if ( !strcmp(argv[a], "--core"))
        {
            coreOnly = true;
            if ( imfOnly )
            {
                std::cerr << "--imf and --core are mutually exclusive" << std::endl;
                return usageAndExit( argv[0], 1 );
            }
        }
        else
            files.push_back( argv[a] );
    }

    if ( files.empty() )
        return usageAndExit( argv[0], 1 );

    setGlobalThreadCount( THREADS );
    bool odd = false;
    uint64_t headerNanosN = 0, dataNanosN = 0, closeNanosN = 0, pixCountN = 0, fileCount = 0;
    uint64_t headerNanosO = 0, dataNanosO = 0, closeNanosO = 0, pixCountO = 0;
    constexpr int count = 4;
    for ( int c = 0; c < count; ++c )
    {
        for ( auto &f: files )
        {
            try
            {
                if ( odd )
                {
                    if ( ! imfOnly )
                        readCore( f, headerNanosN, dataNanosN, closeNanosN, pixCountN );
                    if ( ! coreOnly )
                        readImf( f, headerNanosO, dataNanosO, closeNanosO, pixCountO );
                }
                else
                {
                    if ( ! coreOnly )
                        readImf( f, headerNanosO, dataNanosO, closeNanosO, pixCountO );
                    if ( ! imfOnly )
                        readCore( f, headerNanosN, dataNanosN, closeNanosN, pixCountN );
                }
            }
            catch ( std::exception &e )
            {
                std::cerr << "ERROR: " << e.what() << std::endl;
                return 1;
            }
            odd = !odd;
            ++fileCount;
        }
    }

    if ( pixCountN != pixCountO )
        std::cerr << "ERROR: different pixel counts recorded: core " << pixCountN << " ilmimf " << pixCountO << std::endl;

    std::cout << "Stats for reading: " << files.size() << " files " << count << " times\n\n";

    uint64_t totN = headerNanosN + dataNanosN + closeNanosN;
    uint64_t totO = headerNanosO + dataNanosO + closeNanosO;
    std::cout << " Timers  " << std::setw(15) << std::left << std::setfill(' ') << "Core" << " IlmImf\n"
              << " Header: " << std::setw(15) << std::left << std::setfill(' ') << headerNanosN  << " "
              << std::setw(15) << std::left << std::setfill(' ') << headerNanosO << " ns\n"
              << "   Data: " << std::setw(15) << std::left << std::setfill(' ') << dataNanosN  << " "
              << std::setw(15) << std::left << std::setfill(' ') << dataNanosO << " ns\n"
              << "  Close: " << std::setw(15) << std::left << std::setfill(' ') << closeNanosN  << " "
              << std::setw(15) << std::left << std::setfill(' ') << closeNanosO << " ns\n"
              << "  Total: " << std::setw(15) << std::left << std::setfill(' ') << totN  << " "
              << std::setw(15) << std::left << std::setfill(' ') << totO << " ns\n";

    double aveHN = double(headerNanosN) / double(fileCount);
    double aveDN = double(dataNanosN) / double(fileCount);
    double aveCN = double(closeNanosN) / double(fileCount);
    double aveTN = double(totN) / double(fileCount);
    double aveHO = double(headerNanosO) / double(fileCount);
    double aveDO = double(dataNanosO) / double(fileCount);
    double aveCO = double(closeNanosO) / double(fileCount);
    double aveTO = double(totO) / double(fileCount);

    std::cout << "\n"
              << " Ave     " << std::setw(15) << std::left << std::setfill(' ') << "Core" << " IlmImf\n"
              << " Header: " << std::setw(15) << std::left << std::setfill(' ') << aveHN  << " "
              << std::setw(15) << std::left << std::setfill(' ') << aveHO << " ns\n"
              << "   Data: " << std::setw(15) << std::left << std::setfill(' ') << aveDN  << " "
              << std::setw(15) << std::left << std::setfill(' ') << aveDO << " ns\n"
              << "  Close: " << std::setw(15) << std::left << std::setfill(' ') << aveCN  << " "
              << std::setw(15) << std::left << std::setfill(' ') << aveCO << " ns\n"
              << "  Total: " << std::setw(15) << std::left << std::setfill(' ') << aveTN  << " "
              << std::setw(15) << std::left << std::setfill(' ') << aveTO << " ns\n";

    if ( imfOnly || coreOnly )
        return 0;

    double ratioH = double(headerNanosO) / double(headerNanosN);
    double ratioD = double(dataNanosO) / double(dataNanosN);
    double ratioC = double(closeNanosO) / double(closeNanosN);
    double ratioT = double(totO) / double(totN);

    std::cout << "\n"
              << " Ratios\n"
              << " Header: " << ratioH << "\n"
              << "   Data: " << ratioD << "\n"
              << "  Close: " << ratioC << "\n"
              << "  Total: " << ratioT << std::endl;

    return 0;
}


