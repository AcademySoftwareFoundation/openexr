// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

// Windows specific addition to prevent the indirect import of the redefined min/max macros
#if defined _WIN32 || defined _WIN64
#    ifdef NOMINMAX
#        undef NOMINMAX
#    endif
#    define NOMINMAX
#endif

#include "write.h"

#include "test_value.h"

#include <openexr.h>

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>
#include <cmath>

#include <ImathRandom.h>
#include <ImfArray.h>
#include <ImfChannelList.h>
#include <ImfCompressor.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfHuf.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfTiledOutputFile.h>
#include <half.h>

#include "internal_ht_common.cpp"

#ifdef __linux
#    include <sys/types.h>
#    include <sys/stat.h>
#    include <fcntl.h>
#    include <unistd.h>

static int
compare_files (const char* fn, const char* fn2)
{
    struct stat sb1, sb2;
    int fd1 = -1, fd2 = -1;
    int ret = 0;

    fd1 = open (fn, O_RDONLY);
    fd2 = open (fn2, O_RDONLY);
    if (fd1 >= 0 && fd2 >= 0)
    {
        if (0 == fstat (fd1, &sb1) && 0 == fstat (fd2, &sb2))
        {
            if (sb1.st_size != sb2.st_size)
            {
                std::cerr << "File sizes do not match: '" << fn << "' "
                          << sb1.st_size << " '" << fn2 << "' " << sb2.st_size
                          << std::endl;
                close (fd1);
                close (fd2);
                return 1;
            }

            uint8_t buf1[512], buf2[512];
            size_t  toRead   = sb1.st_size;
            size_t  chunkReq = sizeof (buf1);
            size_t  offset   = 0;
            while (toRead > 0)
            {
                ssize_t nr1 = read (fd1, buf1, chunkReq);
                ssize_t nr2 = read (fd2, buf2, chunkReq);
                if (nr1 < 0 || nr2 < 0)
                {
                    std::cerr << "Unable to read from files " << nr1 << ", "
                              << nr2 << std::endl;
                    ret = -1;
                    break;
                }
                if (nr1 != nr2)
                {
                    std::cerr << "Mismatch in read amounts " << nr1 << ", "
                              << nr2 << std::endl;
                    ret = -1;
                    break;
                }
                if (nr1 > 0)
                {
                    if (memcmp (buf1, buf2, nr1) != 0)
                    {
                        for (ssize_t b = 0; b < nr1; ++b)
                        {
                            if (buf1[b] != buf2[b])
                            {
                                std::cerr << "Files '" << fn << "' and '" << fn2
                                          << "' differ in chunk starting at "
                                          << offset + b << std::endl;
                                break;
                            }
                        }
                        ret = -1;
                        break;
                    }
                }
                offset += nr1;
                toRead -= nr1;
            }
        }
        close (fd1);
        close (fd2);
        return ret;
    }
    else
    {
        std::cerr << "Unable to open '" << fn << "' and '" << fn2 << "'"
                  << std::endl;
    }
    if (fd1 >= 0) close (fd1);
    if (fd2 >= 0) close (fd2);
    return -1;
}
#endif /* linux */

#if defined(OPENEXR_ENABLE_API_VISIBILITY)
#    include "../../lib/OpenEXRCore/internal_huf.c"

void*
internal_exr_alloc (size_t bytes)
{
    return malloc (bytes);
}
void
internal_exr_free (void* p)
{
    if (p) free (p);
}

#else
#    include "../../lib/OpenEXRCore/internal_huf.h"
#endif

using namespace IMATH_NAMESPACE;
namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace IMF;

#if (IMATH_VERSION_MAJOR < 3) ||                                               \
    (IMATH_VERSION_MAJOR == 3 && IMATH_VERSION_MINOR < 1)
inline float
imath_half_to_float (uint16_t a)
{
    half tmp;
    tmp.setBits (a);
    return static_cast<float> (tmp);
}
#endif

////////////////////////////////////////

inline int
shiftAndRound (int x, int shift)
{
    x <<= 1;
    int a = (1 << shift) - 1;
    shift += 1;
    int b = (x >> shift) & 1;
    return (x + a + b) >> shift;
}

inline bool
withinDWAErrorBounds (const uint16_t a, const uint16_t b)
{
    float a1 = imath_half_to_float (a);
    if (!std::isnan (a1))
    {
        float a2          = imath_half_to_float (b);
        float denominator = std::max (1.f, std::max (fabsf (a2), fabsf (a1)));
        if (fabs (a1 / denominator - a2 / denominator) >= 0.1)
        {
            std::cerr << "DWA"
                      << " B bits " << std::hex << b << std::dec << " (" << a2
                      << ") too different from A1 bits " << std::hex << a
                      << std::dec << " (" << a1 << ")"
                      << " delta " << fabs (a1 / denominator - a2 / denominator)
                      << std::endl;
            return false;
        }
    }
    return true;
}

inline bool
withinB44ErrorBounds (const uint16_t A[4][4], const uint16_t B[4][4])
{
    //
    // Assuming that a 4x4 pixel block, B, was generated by
    // compressing and uncompressing another pixel block, A,
    // using OpenEXR's B44 compression method, check whether
    // the differences between A and B are what we would
    // expect from the compressor.
    //

    //
    // The block may not have been compressed at all if it
    // was part of a very small tile.
    //

    bool equal = true;

    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            if (A[i][j] != B[i][j]) equal = false;

    if (equal) return true;

    //
    // The block was compressed.
    //
    // Perform a "light" version of the B44 compression on A
    // (see the pack() function in ImfB44Compressor.cpp).
    //

    uint16_t t[16];

    for (int i = 0; i < 16; ++i)
    {
        uint16_t Abits = A[i / 4][i % 4];

        if ((Abits & 0x7c00) == 0x7c00)
            t[i] = 0x8000;
        else if (Abits & 0x8000)
            t[i] = ~Abits;
        else
            t[i] = Abits | 0x8000;
    }

    uint16_t tMax = 0;

    for (int i = 0; i < 16; ++i)
        if (tMax < t[i]) tMax = t[i];

    int shift = -1;
    int d[16];
    int r[15];
    int rMin;
    int rMax;

    do
    {
        shift += 1;

        for (int i = 0; i < 16; ++i)
            d[i] = shiftAndRound (tMax - t[i], shift);

        const int bias = 0x20;

        r[0] = d[0] - d[4] + bias;
        r[1] = d[4] - d[8] + bias;
        r[2] = d[8] - d[12] + bias;

        r[3] = d[0] - d[1] + bias;
        r[4] = d[4] - d[5] + bias;
        r[5] = d[8] - d[9] + bias;
        r[6] = d[12] - d[13] + bias;

        r[7]  = d[1] - d[2] + bias;
        r[8]  = d[5] - d[6] + bias;
        r[9]  = d[9] - d[10] + bias;
        r[10] = d[13] - d[14] + bias;

        r[11] = d[2] - d[3] + bias;
        r[12] = d[6] - d[7] + bias;
        r[13] = d[10] - d[11] + bias;
        r[14] = d[14] - d[15] + bias;

        rMin = r[0];
        rMax = r[0];

        for (int i = 1; i < 15; ++i)
        {
            if (rMin > r[i]) rMin = r[i];

            if (rMax < r[i]) rMax = r[i];
        }
    } while (rMin < 0 || rMax > 0x3f);

    t[0] = tMax - (d[0] << shift);

    //
    // Now perform a "light" version of the decompression method.
    // (see the unpack() function in ImfB44Compressor.cpp).
    //

    uint16_t  A1[16];
    const int bias = 0x20 << shift;

    A1[0]  = t[0];
    A1[4]  = A1[0] + (r[0] << shift) - bias;
    A1[8]  = A1[4] + (r[1] << shift) - bias;
    A1[12] = A1[8] + (r[2] << shift) - bias;

    A1[1]  = A1[0] + (r[3] << shift) - bias;
    A1[5]  = A1[4] + (r[4] << shift) - bias;
    A1[9]  = A1[8] + (r[5] << shift) - bias;
    A1[13] = A1[12] + (r[6] << shift) - bias;

    A1[2]  = A1[1] + (r[7] << shift) - bias;
    A1[6]  = A1[5] + (r[8] << shift) - bias;
    A1[10] = A1[9] + (r[9] << shift) - bias;
    A1[14] = A1[13] + (r[10] << shift) - bias;

    A1[3]  = A1[2] + (r[11] << shift) - bias;
    A1[7]  = A1[6] + (r[12] << shift) - bias;
    A1[11] = A1[10] + (r[13] << shift) - bias;
    A1[15] = A1[14] + (r[14] << shift) - bias;

    //
    // Compare the result with B, allowing for an difference
    // of a couple of units in the last place.
    //

    for (int i = 0; i < 16; ++i)
    {
        uint16_t A1bits = A1[i];
        uint16_t Bbits  = B[i / 4][i % 4];
        uint16_t Abits  = A[i / 4][i % 4];

        if (Bbits & 0x8000)
            Bbits = ~Bbits;
        else
            Bbits = Bbits | 0x8000;

        if (Bbits > A1bits + 5 || Bbits < A1bits - 5)
        {
            std::cerr << "B44 idx " << i << ": B bits " << std::hex << Bbits
                      << std::dec << " (" << imath_half_to_float (Bbits)
                      << ") too different from A1 bits " << std::hex << A1bits
                      << std::dec << " (" << imath_half_to_float (A1bits) << ")"
                      << " orig " << std::hex << Abits << std::dec << " ("
                      << imath_half_to_float (Abits) << ")" << std::endl;
            return false;
        }
    }

    return true;
}

////////////////////////////////////////

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

    static inline void compareExact (
        uint16_t    a,
        uint16_t    b,
        int         x,
        int         y,
        const char* taga,
        const char* tagb,
        const char* chan)
    {
        if (a != b)
        {
            std::cout << chan << " half at " << std::dec << x << ", " << y
                      << " not equal: " << taga << " 0x" << std::hex << a
                      << std::dec << " (" << imath_half_to_float (a) << ") vs "
                      << tagb << " 0x" << std::hex << b << std::hex << " ("
                      << imath_half_to_float (b) << ")" << std::dec
                      << std::endl;
        }
        EXRCORE_TEST (a == b);
    }
    static inline void compareExact (
        uint32_t    a,
        uint32_t    b,
        int         x,
        int         y,
        const char* taga,
        const char* tagb,
        const char* chan)
    {
        if (a != b)
        {
            std::cout << chan << " uint at " << x << ", " << y
                      << " not equal: " << taga << " 0x" << std::hex << a
                      << std::dec << " vs " << tagb << " 0x" << std::hex << b
                      << std::dec << std::endl;
        }
        EXRCORE_TEST (a == b);
    }
    static inline void compareExact (
        float       af,
        float       bf,
        int         x,
        int         y,
        const char* taga,
        const char* tagb,
        const char* chan)
    {
        union
        {
            uint32_t iv;
            float    fv;
        } a, b;
        a.fv = af;
        b.fv = bf;
        if (a.iv != b.iv)
        {
            std::cout << chan << " float at " << x << ", " << y
                      << " not equal: " << taga << " 0x" << std::hex << a.iv
                      << std::dec << " (" << af << ") vs " << tagb << " "
                      << std::hex << b.iv << std::dec << " (" << bf << ")"
                      << std::endl;
        }
        EXRCORE_TEST (a.iv == b.iv);
    }
    void
    compareExact (const pixels& o, const char* otag, const char* selftag) const
    {
        for (int y = 0; y < _h; ++y)
        {
            for (int x = 0; x < _w; ++x)
            {
                size_t idx = y * _stride_x + x;
                compareExact (o.i[idx], i[idx], x, y, otag, selftag, "I");
                compareExact (o.h[idx], h[idx], x, y, otag, selftag, "H");
                compareExact (o.f[idx], f[idx], x, y, otag, selftag, "F");
                compareExact (
                    o.rgba[0][idx], rgba[0][idx], x, y, otag, selftag, "R");
                compareExact (
                    o.rgba[1][idx], rgba[1][idx], x, y, otag, selftag, "G");
                compareExact (
                    o.rgba[2][idx], rgba[2][idx], x, y, otag, selftag, "B");
                compareExact (
                    o.rgba[3][idx], rgba[3][idx], x, y, otag, selftag, "A");
            }
        }
    }

    void compareClose (
        const pixels&     o,
        exr_compression_t comp,
        const char*       otag,
        const char*       selftag) const
    {
        for (int y = 0; y < _h; ++y)
        {
            for (int x = 0; x < _w; ++x)
            {
                size_t idx = y * _stride_x + x;

                compareExact (o.i[idx], i[idx], x, y, otag, selftag, "I");

                if (comp == EXR_COMPRESSION_PXR24)
                {
                    union
                    {
                        uint32_t iv;
                        float    fv;
                    } a, b;
                    a.fv = o.f[idx];
                    b.fv = f[idx];
                    int32_t delta =
                        (int32_t) (a.iv >> 8) - (int32_t) (b.iv >> 8);
                    EXRCORE_TEST_LOCATION (delta < 2 && delta > -2, x, y);
                }
                else
                {
                    // b44 & dwa don't change floats
                    compareExact (o.f[idx], f[idx], x, y, otag, selftag, "F");
                }
            }
        }

        if (comp == EXR_COMPRESSION_B44 || comp == EXR_COMPRESSION_B44A)
        {
            uint16_t orig[4][4], unc[4][4];
            for (int y = 0; y < _h; y += 4)
            {
                for (int x = 0; x < _w; x += 4)
                {
                    for (int y1 = 0; y1 < 4; ++y1)
                    {
                        for (int x1 = 0; x1 < 4; ++x1)
                        {
                            int y2       = std::min (y + y1, _h - 1);
                            int x2       = std::min (x + x1, _w - 1);
                            orig[y1][x1] = o.h[y2 * _stride_x + x2];
                            unc[y1][x1]  = h[y2 * _stride_x + x2];
                        }
                    }
                    EXRCORE_TEST_LOCATION (
                        withinB44ErrorBounds (orig, unc), x, y)
                    for (int c = 0; c < 4; ++c)
                    {

                        for (int y1 = 0; y1 < 4; ++y1)
                        {
                            for (int x1 = 0; x1 < 4; ++x1)
                            {
                                int y2       = std::min (y + y1, _h - 1);
                                int x2       = std::min (x + x1, _w - 1);
                                orig[y1][x1] = o.rgba[c][y2 * _stride_x + x2];
                                unc[y1][x1]  = rgba[c][y2 * _stride_x + x2];
                            }
                        }
                        EXRCORE_TEST_LOCATION (
                            withinB44ErrorBounds (orig, unc), x, y)
                    }
                }
            }
        }
        else if (comp == EXR_COMPRESSION_DWAA || comp == EXR_COMPRESSION_DWAB)
        {
            for (int y = 0; y < _h; ++y)
            {
                for (int x = 0; x < _w; ++x)
                {
                    size_t idx = y * _stride_x + x;
                    compareExact (o.h[idx], h[idx], x, y, otag, selftag, "H");

                    for (int c = 0; c < 4; ++c)
                    {
                        EXRCORE_TEST_LOCATION (
                            withinDWAErrorBounds (o.rgba[c][idx], rgba[c][idx]),
                            x,
                            y)
                    }
                }
            }
        }
        else if (comp == EXR_COMPRESSION_PXR24)
        {
            for (int y = 0; y < _h; ++y)
            {
                for (int x = 0; x < _w; ++x)
                {
                    size_t idx = y * _stride_x + x;
                    compareExact (o.h[idx], h[idx], x, y, otag, selftag, "H");
                    compareExact (
                        o.rgba[0][idx], rgba[0][idx], x, y, otag, selftag, "R");
                    compareExact (
                        o.rgba[1][idx], rgba[1][idx], x, y, otag, selftag, "G");
                    compareExact (
                        o.rgba[2][idx], rgba[2][idx], x, y, otag, selftag, "B");
                    compareExact (
                        o.rgba[3][idx], rgba[3][idx], x, y, otag, selftag, "A");
                }
            }
        }
        else
        {
            for (int y = 0; y < _h; ++y)
            {
                for (int x = 0; x < _w; ++x)
                {
                    size_t idx = y * _stride_x + x;
                    EXRCORE_TEST (o.h[idx] == h[idx]);
                    EXRCORE_TEST (o.rgba[0][idx] == rgba[0][idx]);
                    EXRCORE_TEST (o.rgba[1][idx] == rgba[1][idx]);
                    EXRCORE_TEST (o.rgba[2][idx] == rgba[2][idx]);
                    EXRCORE_TEST (o.rgba[3][idx] == rgba[3][idx]);
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
    int32_t               scansperchunk = 0;
    int                   y, starty, endy;
    exr_chunk_info_t      cinfo;
    exr_encode_pipeline_t encoder;
    bool                  first = true;

    EXRCORE_TEST_RVAL (exr_get_scanlines_per_chunk (f, 0, &scansperchunk));
    starty = IMG_DATA_Y * ys;
    endy   = starty + IMG_HEIGHT * ys;
    for (y = starty; y < endy; y += scansperchunk)
    {
        EXRCORE_TEST_RVAL (exr_write_scanline_chunk_info (f, 0, y, &cinfo));
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

            if (curchan.height == 0)
            {
                encoder.channels[c].encode_from_ptr   = NULL;
                encoder.channels[c].user_pixel_stride = 0;
                encoder.channels[c].user_line_stride  = 0;
                continue;
            }

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
    int32_t               tlevx, tlevy;
    int32_t               tilew, tileh;
    int32_t               tilex, tiley;
    int                   y, endy;
    int                   x, endx;
    exr_chunk_info_t      cinfo;
    exr_encode_pipeline_t encoder;
    bool                  first = true;

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
                exr_write_tile_chunk_info (f, 0, tilex, tiley, 0, 0, &cinfo));
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
    exr_chunk_info_t      cinfo;
    exr_decode_pipeline_t decoder;
    int32_t               scansperchunk;
    exr_attr_box2i_t      dw;
    bool                  first = true;

    EXRCORE_TEST_RVAL (exr_get_data_window (f, 0, &dw));
    EXRCORE_TEST (dw.min.x == IMG_DATA_X * xs);
    EXRCORE_TEST (dw.max.x == IMG_DATA_X * xs + IMG_WIDTH * xs - 1);
    EXRCORE_TEST (dw.min.y == IMG_DATA_Y * ys);
    EXRCORE_TEST (dw.max.y == IMG_DATA_Y * ys + IMG_HEIGHT * ys - 1);

    EXRCORE_TEST_RVAL (exr_get_scanlines_per_chunk (f, 0, &scansperchunk));

    exr_storage_t stortype;
    EXRCORE_TEST_RVAL (exr_get_storage (f, 0, &stortype));
    EXRCORE_TEST (stortype == EXR_STORAGE_SCANLINE);

    //const uint8_t* tmp;
    for (int y = dw.min.y; y <= dw.max.y; y += scansperchunk)
    {
        EXRCORE_TEST_RVAL (exr_read_scanline_chunk_info (f, 0, y, &cinfo));
        if (first)
        {
            EXRCORE_TEST_RVAL (
                exr_decoding_initialize (f, 0, &cinfo, &decoder));
        }
        else
        {
            EXRCORE_TEST_RVAL (exr_decoding_update (f, 0, &cinfo, &decoder));
        }
        int32_t idxoffset = ((y - dw.min.y) / ys) * p._stride_x;

        for (int c = 0; c < decoder.channel_count; ++c)
        {
            const exr_coding_channel_info_t& curchan = decoder.channels[c];
            void*                            ptr;
            int32_t                          pixelstride;
            int32_t                          linestride;

            if (curchan.height == 0)
            {
                decoder.channels[c].decode_to_ptr     = NULL;
                decoder.channels[c].user_pixel_stride = 0;
                decoder.channels[c].user_line_stride  = 0;
                continue;
            }

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
    int32_t               tlevx, tlevy;
    int32_t               tilew, tileh;
    int32_t               tilex, tiley;
    int                   y, endy;
    int                   x, endx;
    exr_chunk_info_t      cinfo;
    exr_decode_pipeline_t decoder;
    bool                  first = true;

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
                exr_read_tile_chunk_info (f, 0, tilex, tiley, 0, 0, &cinfo));
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

static const char* channels[] = {"R", "G", "B", "A", "H"};

static void
saveCPP (
    pixels&            p,
    const std::string& cppfilename,
    bool               tiled,
    int                xs,
    int                ys,
    exr_compression_t  comp,
    int                fw,
    int                fh,
    int                dwx,
    int                dwy)
{
    Header hdr (
        (Box2i (V2i (0, 0), V2i (fw - 1, fh - 1))),
        (Box2i (V2i (dwx, dwy), V2i (dwx + fw - 1, dwy + fh - 1))));

    hdr.compression ()         = (IMF::Compression) ((int) comp);
    hdr.zipCompressionLevel () = 4;
    EXRCORE_TEST (((const Header&) hdr).zipCompressionLevel () == 4);

    hdr.channels ().insert ("I", Channel (IMF::UINT, xs, ys));
    for (int c = 0; c < 5; ++c)
        hdr.channels ().insert (channels[c], Channel (IMF::HALF, xs, ys));
    hdr.channels ().insert ("F", Channel (IMF::FLOAT, xs, ys));
    {
        FrameBuffer fb;
        V2i         origin{dwx, dwy};
        fb.insert (
            "I",
            Slice::Make (
                IMF::UINT,
                p.i.data (),
                origin,
                fw,
                fh,
                0,
                4 * p._stride_x,
                xs,
                ys,
                0.0,
                false,
                false));
        fb.insert (
            "H",
            Slice::Make (
                IMF::HALF,
                p.h.data (),
                origin,
                fw,
                fh,
                0,
                2 * p._stride_x,
                xs,
                ys,
                0.0,
                false,
                false));
        fb.insert (
            "F",
            Slice::Make (
                IMF::FLOAT,
                p.f.data (),
                origin,
                fw,
                fh,
                0,
                4 * p._stride_x,
                xs,
                ys,
                0.0,
                false,
                false));

        for (int c = 0; c < 4; c++)
        {
            fb.insert (
                channels[c],
                Slice::Make (
                    IMF::HALF,
                    p.rgba[c].data (),
                    origin,
                    fw,
                    fh,
                    0,
                    2 * p._stride_x,
                    xs,
                    ys,
                    0.0,
                    false,
                    false));
        }

        if (tiled)
        {
            TileDescription td;
            hdr.setTileDescription (td);
            TiledOutputFile out (cppfilename.c_str (), hdr);
            out.setFrameBuffer (fb);
            out.writeTiles (
                0,
                static_cast<int> (
                    ceil (
                        static_cast<float> (fw) /
                        static_cast<float> (td.xSize)) -
                    1),
                0,
                static_cast<int> (
                    ceil (
                        static_cast<float> (fh) /
                        static_cast<float> (td.ySize)) -
                    1));
        }
        else
        {
            OutputFile out (cppfilename.c_str (), hdr);
            out.setFrameBuffer (fb);
            out.writePixels (fh);
        }
    }
}

////////////////////////////////////////

static void
loadCPP (
    pixels&            p,
    const std::string& filename,
    bool               tiled,
    int                fw,
    int                fh,
    int                dwx,
    int                dwy)
{
    InputFile in (filename.c_str ());
    {
        FrameBuffer fb;
        V2i         origin{dwx, dwy};
        fb.insert (
            "I",
            Slice::Make (
                IMF::UINT,
                p.i.data (),
                origin,
                fw,
                fh,
                0,
                4 * p._stride_x,
                in.header ().channels ()["I"].xSampling,
                in.header ().channels ()["I"].ySampling,
                0.0,
                false,
                false));
        fb.insert (
            "H",
            Slice::Make (
                IMF::HALF,
                p.h.data (),
                origin,
                fw,
                fh,
                0,
                2 * p._stride_x,
                in.header ().channels ()["H"].xSampling,
                in.header ().channels ()["H"].ySampling,
                0.0,
                false,
                false));

        for (int c = 0; c < 4; c++)
        {
            fb.insert (
                channels[c],
                Slice::Make (
                    IMF::HALF,
                    p.rgba[c].data (),
                    origin,
                    fw,
                    fh,
                    0,
                    2 * p._stride_x,
                    in.header ().channels ()[channels[c]].xSampling,
                    in.header ().channels ()[channels[c]].ySampling,
                    0.0,
                    false,
                    false));
        }
        fb.insert (
            "F",
            Slice::Make (
                IMF::FLOAT,
                p.f.data (),
                origin,
                fw,
                fh,
                0,
                4 * p._stride_x,
                in.header ().channels ()["F"].xSampling,
                in.header ().channels ()["F"].ySampling,
                0.0,
                false,
                false));

        in.setFrameBuffer (fb);
        in.readPixels (dwy, dwy + fh - 1);
    }
}

////////////////////////////////////////

static void
doWriteRead (
    pixels&            p,
    const std::string& filename,
    const std::string& cppfilename,
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

    exr_set_default_zip_compression_level (-1);
    cinit.zip_level = 3;
    cinit.flags |= EXR_CONTEXT_FLAG_WRITE_LEGACY_HEADER;

    dataW.min.x = dwx;
    dataW.min.y = dwy;
    dataW.max.x = dwx + fw - 1;
    dataW.max.y = dwy + fh - 1;

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

    int zlev;
    EXRCORE_TEST_RVAL (exr_get_zip_compression_level (f, partidx, &zlev));
    EXRCORE_TEST (zlev == 3);
    EXRCORE_TEST_RVAL (exr_set_zip_compression_level (f, partidx, 4));
    if (tiled)
    {
        EXRCORE_TEST_RVAL (exr_set_tile_descriptor (
            f, partidx, 32, 32, EXR_TILE_ONE_LEVEL, EXR_TILE_ROUND_DOWN));
    }

    EXRCORE_TEST_RVAL (exr_add_channel (
        f, partidx, "I", EXR_PIXEL_UINT, EXR_PERCEPTUALLY_LOGARITHMIC, xs, ys));

    for (int c = 0; c < 5; ++c)
    {
        EXRCORE_TEST_RVAL (exr_add_channel (
            f,
            partidx,
            channels[c],
            EXR_PIXEL_HALF,
            EXR_PERCEPTUALLY_LOGARITHMIC,
            xs,
            ys));
    }
    EXRCORE_TEST_RVAL (exr_add_channel (
        f,
        partidx,
        "F",
        EXR_PIXEL_FLOAT,
        EXR_PERCEPTUALLY_LOGARITHMIC,
        xs,
        ys));

    EXRCORE_TEST_RVAL (exr_write_header (f));
    if (tiled)
        doEncodeTile (f, p, xs, ys);
    else
        doEncodeScan (f, p, xs, ys);
    EXRCORE_TEST_RVAL (exr_finish (&f));

    try
    {
        saveCPP (p, cppfilename, tiled, xs, ys, comp, fw, fh, dwx, dwy);
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR saving " << cppfilename << ": " << e.what ()
                  << std::endl;
        EXRCORE_TEST_FAIL (saveCPP);
    }

#ifdef __linux
    if (0 != compare_files (filename.c_str (), cppfilename.c_str ()))
    {
        EXRCORE_TEST_FAIL (compare_files);
    }
#endif
    pixels restore    = p;
    pixels cpprestore = p;
    pixels cpploadc   = p;
    pixels cpploadcpp = p;

    restore.fillDead ();
    cpprestore.fillDead ();
    cpploadc.fillDead ();
    cpploadcpp.fillDead ();

    EXRCORE_TEST_RVAL (exr_start_read (&f, filename.c_str (), &cinit));
    if (tiled)
        doDecodeTile (f, restore, xs, ys);
    else
        doDecodeScan (f, restore, xs, ys);
    EXRCORE_TEST_RVAL (exr_finish (&f));

    EXRCORE_TEST_RVAL (exr_start_read (&f, cppfilename.c_str (), &cinit));
    if (tiled)
        doDecodeTile (f, cpprestore, xs, ys);
    else
        doDecodeScan (f, cpprestore, xs, ys);
    EXRCORE_TEST_RVAL (exr_finish (&f));

    try
    {
        loadCPP (cpploadcpp, cppfilename, tiled, fw, fh, dwx, dwy);
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR loading " << cppfilename << ": " << e.what ()
                  << std::endl;
        EXRCORE_TEST_FAIL (loadCPP);
    }

    try
    {
        loadCPP (cpploadc, filename, tiled, fw, fh, dwx, dwy);
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR loading " << filename << ": " << e.what ()
                  << std::endl;
        EXRCORE_TEST_FAIL (loadCPP);
    }

    {
        restore.compareExact (cpprestore, "C loaded C++", "C loaded C");
        cpploadcpp.compareExact (cpploadc, "C++ loaded C", "C++ loaded C++");
        restore.compareExact (cpploadc, "C++ loaded C", "C loaded C");
        restore.compareExact (cpploadcpp, "C++ loaded C++", "C loaded C");
    }

    switch (comp)
    {
        case EXR_COMPRESSION_NONE:
        case EXR_COMPRESSION_RLE:
        case EXR_COMPRESSION_ZIP:
        case EXR_COMPRESSION_ZIPS:
            restore.compareExact (p, "orig", "C loaded C");
            break;
        case EXR_COMPRESSION_PIZ:
        case EXR_COMPRESSION_PXR24:
        case EXR_COMPRESSION_B44:
        case EXR_COMPRESSION_B44A:
        case EXR_COMPRESSION_DWAA:
        case EXR_COMPRESSION_DWAB:
            restore.compareClose (p, comp, "orig", "C loaded C");
            break;
        case EXR_COMPRESSION_LAST_TYPE:
        default:
            std::cerr << "Unknown compression type " << (int) (comp)
                      << std::endl;
            EXRCORE_TEST (false);
            break;
    }
    remove (filename.c_str ());
    remove (cppfilename.c_str ());
}

////////////////////////////////////////

static void
testWriteRead (
    pixels&            p,
    const std::string& tempdir,
    exr_compression_t  comp,
    const char*        pattern)
{
    std::string filename =
        tempdir + pattern + std::string ("_imf_test_comp.exr");
    std::string cppfilename =
        tempdir + pattern + std::string ("_imf_test_comp_cpp.exr");
    const int maxsampling = 2;
    for (int xs = 1; xs <= maxsampling; ++xs)
    {
        for (int ys = 1; ys <= maxsampling; ++ys)
        {
            doWriteRead (
                p, filename, cppfilename, false, xs, ys, comp, pattern);
            if (xs == 1 && ys == 1)
                doWriteRead (
                    p, filename, cppfilename, true, xs, ys, comp, pattern);
        }
    }
}

////////////////////////////////////////

static void
testComp (const std::string& tempdir, exr_compression_t comp)
{
    pixels p{IMG_WIDTH, IMG_HEIGHT, IMG_STRIDE_X};

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
testHUF (const std::string& tempdir)
{
    uint64_t esize = internal_exr_huf_compress_spare_bytes ();
    uint64_t dsize = internal_exr_huf_decompress_spare_bytes ();
    // decsize 1 << 16 + 1
    // decsize 1 << 14
    EXRCORE_TEST (esize == 65537 * (8 + 8 + sizeof (uint64_t*) + 4));
    const uint64_t hufdecsize =
        (sizeof (uint32_t*) + sizeof (int32_t) + sizeof (uint32_t));
    // sizeof(FastHufDecoder) is bother to manually compute, just assume it's ok
    // if it's returning at least enough for the slow path
    EXRCORE_TEST (dsize >= (65537 * sizeof (uint64_t) + 16383 * hufdecsize));

    std::vector<uint8_t> hspare;

    hspare.resize (std::max (esize, dsize));
    pixels p{IMG_WIDTH, 1, IMG_STRIDE_X};

    p.fillZero ();
    std::vector<uint8_t> encoded;
    encoded.resize (IMG_WIDTH * 2 * 3 + 65536);
    uint64_t ebytes;
    EXRCORE_TEST_RVAL (internal_huf_compress (
        &ebytes,
        encoded.data (),
        encoded.size (),
        p.h.data (),
        IMG_WIDTH,
        hspare.data (),
        esize));
    std::vector<uint8_t> cppencoded;
    cppencoded.resize (IMG_WIDTH * 2 * 3 + 65536);

    uint64_t cppebytes =
        hufCompress (p.h.data (), IMG_WIDTH, (char*) (&cppencoded[0]));
    EXRCORE_TEST (ebytes == cppebytes);
    for (size_t i = 0; i < ebytes; ++i)
    {
        if (encoded[i] != cppencoded[i])
        {
            std::cerr << "Error: byte " << i << " differs between new (0x"
                      << std::hex << (int) encoded[i] << std::dec
                      << ") and old (0x" << std::hex << (int) cppencoded[i]
                      << std::dec << ")" << std::endl;
            EXRCORE_TEST (encoded[i] == cppencoded[i]);
        }
    }

    pixels decode = p;
    EXRCORE_TEST_RVAL (internal_huf_decompress (
        NULL,
        encoded.data (),
        ebytes,
        decode.h.data (),
        IMG_WIDTH,
        hspare.data (),
        dsize));
    for (size_t i = 0; i < IMG_WIDTH; ++i)
    {
        EXRCORE_TEST (decode.h[i] == p.h[i]);
    }

    p.fillPattern1 ();
    EXRCORE_TEST_RVAL (internal_huf_compress (
        &ebytes,
        encoded.data (),
        encoded.size (),
        p.h.data (),
        IMG_WIDTH,
        hspare.data (),
        esize));
    cppebytes = hufCompress (p.h.data (), IMG_WIDTH, (char*) (&cppencoded[0]));
    EXRCORE_TEST (ebytes == cppebytes);
    for (size_t i = 0; i < ebytes; ++i)
    {
        if (encoded[i] != cppencoded[i])
        {
            std::cerr << "Error: byte " << i << " differs between new (0x"
                      << std::hex << (int) encoded[i] << std::dec
                      << ") and old (0x" << std::hex << (int) cppencoded[i]
                      << std::dec << ")" << std::endl;
            EXRCORE_TEST (encoded[i] == cppencoded[i]);
        }
    }
    EXRCORE_TEST_RVAL (internal_huf_decompress (
        NULL,
        encoded.data (),
        ebytes,
        decode.h.data (),
        IMG_WIDTH,
        hspare.data (),
        dsize));
    for (size_t i = 0; i < IMG_WIDTH; ++i)
    {
        EXRCORE_TEST (decode.h[i] == p.h[i]);
    }

    p.fillRandom ();
    EXRCORE_TEST_RVAL (internal_huf_compress (
        &ebytes,
        encoded.data (),
        encoded.size (),
        p.h.data (),
        IMG_WIDTH,
        hspare.data (),
        esize));
    cppebytes = hufCompress (p.h.data (), IMG_WIDTH, (char*) (&cppencoded[0]));
    EXRCORE_TEST (ebytes == cppebytes);
    for (size_t i = 0; i < ebytes; ++i)
    {
        if (encoded[i] != cppencoded[i])
        {
            std::cerr << "Error: byte " << i << " differs between new (0x"
                      << std::hex << (int) encoded[i] << std::dec
                      << ") and old (0x" << std::hex << (int) cppencoded[i]
                      << std::dec << ")" << std::endl;
            EXRCORE_TEST (encoded[i] == cppencoded[i]);
        }
    }
    EXRCORE_TEST_RVAL (internal_huf_decompress (
        NULL,
        encoded.data (),
        ebytes,
        decode.h.data (),
        IMG_WIDTH,
        hspare.data (),
        dsize));
    for (size_t i = 0; i < IMG_WIDTH; ++i)
    {
        EXRCORE_TEST (decode.h[i] == p.h[i]);
    }
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
{
    testComp (tempdir, EXR_COMPRESSION_PIZ);
}

void
testPXR24Compression (const std::string& tempdir)
{
    testComp (tempdir, EXR_COMPRESSION_PXR24);
}

void
testB44Compression (const std::string& tempdir)
{
    testComp (tempdir, EXR_COMPRESSION_B44);
}

void
testB44ACompression (const std::string& tempdir)
{
    testComp (tempdir, EXR_COMPRESSION_B44A);
}

void
testDWAACompression (const std::string& tempdir)
{
    testComp (tempdir, EXR_COMPRESSION_DWAA);
}

void
testDWABCompression (const std::string& tempdir)
{
    testComp (tempdir, EXR_COMPRESSION_DWAB);
}

struct ht_channel_map_tests {
    exr_coding_channel_info_t   channels[6];
    int                         channel_count;
    bool                        pass;
    int                         rgb_index[3];
};

void
testHTChannelMap (const std::string& tempdir)
{
    std::vector<CodestreamChannelInfo> cs_to_file_ch;
    ht_channel_map_tests               tests[] = {
        {{{"R"}, {"G"}, {"B"}}, 3, true, {0, 1, 2}},
        {{{"r"}, {"G"}, {"b"}}, 3, true, {0, 1, 2}},
        {{{"B"}, {"G"}, {"R"}}, 3, true, {2, 1, 0}},
        {{{"red"}, {"green"}, {"blue"}}, 3, true, {0, 1, 2}},
        {{{"Red"}, {"Green"}, {"Blue"}, {"alpha"}}, 4, true, {0, 1, 2}},
        {{{"hello.R"}, {"hello.G"}, {"hello.B"}}, 3, true, {0, 1, 2}},
        {{{"hello.R"}, {"bye.R"}, {"hello.G"}, {"bye.R"}, {"hello.B"}, {"bye.B"}}, 6, true, {0, 2, 4}},
        {{{"red"}, {"green"}, {"blue"}}, 3, true, {0, 1, 2}},
        /* the following are expected to fail */
        {{{"redqueen"}, {"greens"}, {"blueberry"}}, 3, false, {0, 1, 2}},
        {{{"hello.R"}, {"bye.G"}, {"hello.B"}}, 3, false, {0, 2, 4}},
    };
    int test_count = sizeof(tests) / sizeof(ht_channel_map_tests);

    for (size_t i = 0; i < test_count; i++)
    {
        EXRCORE_TEST (
            make_channel_map (
                tests[i].channel_count, tests[i].channels, cs_to_file_ch) ==
            tests[i].pass);
        if (tests[i].pass)
        {
            for (size_t j = 0; j < 3; j++)
            {
                EXRCORE_TEST (
                    tests[i].rgb_index[j] == cs_to_file_ch[j].file_index);
            }
        }
    }

    exr_coding_channel_info_t channels_1[] = {
        { "R" }, { "G" }, { "B" }
    };

    exr_coding_channel_info_t channels_2[] = {
        { "R" }, { "G" }, { "1.B" }
    };

    EXRCORE_TEST (make_channel_map (3, channels_1, cs_to_file_ch));
    EXRCORE_TEST (! make_channel_map (3, channels_2, cs_to_file_ch));
}

void
testDeepNoCompression (const std::string& tempdir)
{}

void
testDeepZIPCompression (const std::string& tempdir)
{}

void
testDeepZIPSCompression (const std::string& tempdir)
{}
