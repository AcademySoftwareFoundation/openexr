#include <string>
#include <map>
#include <chrono>
#include <numeric>
#include <cmath>

#include "ImfArray.h"
#include "ImfCompression.h"
#include "ImfHeader.h"
#include "ImfRgbaFile.h"
#include "ImfFrameBuffer.h"
#include <ImfNamespace.h>
#include <OpenEXRConfig.h>

#include "cxxopts.hpp"

namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;

static std::map<std::string, Compression> comp_table = {
    {{"NO_COMPRESSION", NO_COMPRESSION},
     {"RLE_COMPRESSION", RLE_COMPRESSION},
     {"ZIPS_COMPRESSION", ZIPS_COMPRESSION},
     {"ZIP_COMPRESSION", ZIP_COMPRESSION},
     {"PIZ_COMPRESSION", PIZ_COMPRESSION},
     {"PXR24_COMPRESSION", PXR24_COMPRESSION},
     {"B44_COMPRESSION", B44_COMPRESSION},
     {"B44A_COMPRESSION", B44A_COMPRESSION},
     {"DWAA_COMPRESSION", DWAA_COMPRESSION},
     {"DWAB_COMPRESSION", DWAA_COMPRESSION},
     {"HTJ2K_COMPRESSION", HTJ2K_COMPRESSION}}
};

template <class T>
double
mean (const T& v)
{
    double r = 0;

    for (auto i = v.begin (); i != v.end (); i++)
    {
        r += *i;
    }

    return r / static_cast<double> (v.size ());
}

template <class T>
double
stddev (const T& v, double mean)
{
    double r = 0;

    for (auto i = v.begin (); i != v.end (); i++)
    {
        r += pow (*i - mean, 2);
    }

    return sqrt (r / static_cast<double> (v.size ()));
}

class OMemStream : public OStream
{
public:
    OMemStream (std::stringstream* ss) : OStream ("<omemfile>"), _buffer (ss)
    {
        this->_buffer->seekp (0);
    }

    virtual ~OMemStream () {}

    virtual void write (const char c[/*n*/], int n)
    {
        this->_buffer->write (c, n);
    }

    virtual uint64_t tellp () { return this->_buffer->tellp (); }

    virtual void seekp (uint64_t pos) { this->_buffer->seekp (pos); }

private:
    std::stringstream* _buffer;
};

class IMemStream : public IStream
{
public:
    IMemStream (std::stringstream* ss) : IStream ("<imemfile>"), _buffer (ss)
    {
        this->_buffer->exceptions (
            std::stringstream::failbit | std::stringstream::eofbit);
        this->_buffer->seekp (std::ios::end);
        this->_size = this->_buffer->tellp ();
        this->_buffer->seekg (0);
    }

    virtual ~IMemStream () {}

    virtual bool read (char c[/*n*/], int n)
    {
        this->_buffer->read (c, n);

        return this->_buffer->tellg () != this->_size;
    }

    virtual uint64_t tellg () { return this->_buffer->tellg (); }

    virtual void seekg (uint64_t pos) { this->_buffer->seekg (pos); }

    virtual void clear () { this->_buffer->clear (); }

private:
    std::stringstream*          _buffer;
    std::stringstream::pos_type _size;
};

int
main (int argc, char* argv[])
{
    cxxopts::Options options (
        "exrperf", "OpenEXR compress/uncompress benchmarks");

    options.add_options () (
        "r,repetitions",
        "Repetition count",
        cxxopts::value<int> ()->default_value ("5")) (
        "t,threads",
        "Number of threads",
        cxxopts::value<int> ()->default_value ("1")) (
        "v,verbose",
        "Output more information",
        cxxopts::value<bool> ()->default_value ("false")) (
        "l",
        "Line by line read",
        cxxopts::value<bool> ()->default_value ("false")) (
        "file", "Input image", cxxopts::value<std::string> ()) (
        "compression", "Compression", cxxopts::value<std::string> ());

    options.parse_positional ({"file", "compression"});

    options.show_positional_help ();

    auto args = options.parse (argc, argv);

    if (args.count ("compression") != 1 || args.count ("file") != 1)
    {
        std::cout << options.help () << std::endl;
        exit (-1);
    }

    Compression c = comp_table[args["compression"].as<std::string> ()];

    auto& src_fn = args["file"].as<std::string> ();

    /* load src image */
    RgbaInputFile src_file (src_fn.c_str ());

    Box2i dw     = src_file.dataWindow ();
    int   width  = dw.max.x - dw.min.x + 1;
    int   height = dw.max.y - dw.min.y + 1;

    Array2D<Rgba> src_pixels (width, height);

    src_file.setFrameBuffer (&src_pixels[-dw.min.x][-dw.min.y], 1, width);
    src_file.readPixels (dw.min.y, dw.max.y);

    Header src_header         = src_file.header ();
    src_header.compression () = c;

    /* thread count */

    setGlobalThreadCount (args["threads"].as<int> ());

    /* mem buffer */

    std::stringstream mem_file;

    /* encode performance */

    std::vector<double> encode_times;

    int encoded_size;

    for (int i = 0; i < args["repetitions"].as<int> (); i++)
    {

        OMemStream o_memfile (&mem_file);

        RgbaOutputFile o_file (o_memfile, src_header, src_file.channels ());
        o_file.setFrameBuffer (&src_pixels[-dw.min.x][-dw.min.y], 1, width);

        auto start = std::chrono::high_resolution_clock::now ();
        o_file.writePixels (height);
        auto dur = std::chrono::high_resolution_clock::now () - start;

        encode_times.push_back (std::chrono::duration<double> (dur).count ());

        if (i == 0) { encoded_size = mem_file.tellp (); }
    }

    /* decode performance */

    std::vector<double> decode_times;

    for (int i = 0; i < args["repetitions"].as<int> (); i++)
    {

        IMemStream i_memfile (&mem_file);

        RgbaInputFile i_file (i_memfile);

        Array2D<Rgba> decoded_pixels (width, height);
        i_file.setFrameBuffer (&decoded_pixels[-dw.min.x][-dw.min.y], 1, width);

        auto start = std::chrono::high_resolution_clock::now ();
        if (args["l"].as<bool> ()) {
            for (size_t j = dw.min.y; j <= dw.max.y; j++)
            {
                i_file.readPixels (j, j);
            }
        } else {
            i_file.readPixels (dw.min.y, dw.max.y);
        }
        auto dur = std::chrono::high_resolution_clock::now () - start;

        decode_times.push_back (std::chrono::duration<double> (dur).count ());

        /* compare pixels */

        for (size_t y = 0; y < height; y++)
        {
            for (size_t x = 0; x < width; x++)
            {
                if (decoded_pixels[x][y].r != src_pixels[x][y].r ||
                    decoded_pixels[x][y].g != src_pixels[x][y].g ||
                    decoded_pixels[x][y].b != src_pixels[x][y].b)
                {
                    std::cerr << "Not lossless at " << x << ", " << y
                              << std::endl;
                    exit (-1);
                }
            }
        }
    }

    double encode_time_mean = mean (encode_times);
    double encode_time_dev  = stddev (encode_times, encode_time_mean);

    double decode_time_mean = mean (decode_times);
    double decode_time_dev  = stddev (decode_times, decode_time_mean);

    if (args["verbose"].as<bool> ())
        std::cout
            << "fn, c, n, threads, encoded size, encode time mean, encode time stddev, decode time mean, decode time stddev"
            << std::endl;

    std::string fn = src_fn.substr (src_fn.find_last_of ("/\\") + 1);

    std::cout << fn << ", " << args["compression"].as<std::string> () << ", "
              << args["repetitions"].as<int> () << ", "
              << args["threads"].as<int> () << ", " << encoded_size << ", "
              << encode_time_mean << ", " << encode_time_dev << ", "
              << decode_time_mean << ", " << decode_time_dev << std::endl;

    return 0;
}