/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_read.h"

#include "internal_structs.h"
#include "internal_xdr.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#if defined __SSE2__ || (_MSC_VER >= 1300 && !_M_CEE_PURE)
#    define IMF_HAVE_SSE2 1
#    include <emmintrin.h>
#    include <mmintrin.h>
#endif
#if defined __SSE4_1__
#    define IMF_HAVE_SSE4_1 1
#    include <smmintrin.h>
#endif

/**************************************/

#ifdef IMF_HAVE_SSE4_1
static void
reconstruct (uint8_t* buf, size_t outSize)
{
    static const size_t bytesPerChunk = sizeof (__m128i);
    const size_t        vOutSize      = outSize / bytesPerChunk;
    const __m128i       c             = _mm_set1_epi8 (-128);
    const __m128i       shuffleMask   = _mm_set1_epi8 (15);
    __m128i *           vBuf, vPrev;
    uint8_t             prev;

    /*
     * The first element doesn't have its high bit flipped during compression,
     * so it must not be flipped here.  To make the SIMD loop nice and
     * uniform, we pre-flip the bit so that the loop will unflip it again.
     */
    buf[0] += -128;
    vBuf  = (__m128i*) buf;
    vPrev = _mm_setzero_si128 ();

    for (size_t i = 0; i < vOutSize; ++i)
    {
        __m128i d = _mm_add_epi8 (_mm_loadu_si128 (vBuf), c);

        /* Compute the prefix sum of elements. */
        d = _mm_add_epi8 (d, _mm_slli_si128 (d, 1));
        d = _mm_add_epi8 (d, _mm_slli_si128 (d, 2));
        d = _mm_add_epi8 (d, _mm_slli_si128 (d, 4));
        d = _mm_add_epi8 (d, _mm_slli_si128 (d, 8));
        d = _mm_add_epi8 (d, vPrev);

        _mm_storeu_si128 (vBuf++, d);

        // Broadcast the high byte in our result to all lanes of the prev
        // value for the next iteration.
        vPrev = _mm_shuffle_epi8 (d, shuffleMask);
    }

    prev = _mm_extract_epi8 (vPrev, 15);
    for (size_t i = vOutSize * bytesPerChunk; i < outSize; ++i)
    {
        uint8_t d = prev + buf[i] - 128;
        buf[i]    = d;
        prev      = d;
    }
}
#else
static void
reconstruct (uint8_t* buf, size_t sz)
{
    uint8_t* t    = buf + 1;
    uint8_t* stop = buf + sz;
    while (t < stop)
    {
        int d = (int) (t[-1]) + (int) (t[0]) - 128;
        t[0]  = d;
        ++t;
    }
}
#endif

/**************************************/

#ifdef IMF_HAVE_SSE2
static void
interleave (uint8_t* out, const uint8_t* source, size_t outSize)
{
    static const size_t bytesPerChunk = 2 * sizeof (__m128i);
    const size_t        vOutSize      = outSize / bytesPerChunk;
    const __m128i*      v1            = (const __m128i*) source;
    const __m128i*      v2   = (const __m128i*) (source + (outSize + 1) / 2);
    __m128i*            vOut = (__m128i*) out;
    const uint8_t *     t1, *t2;
    uint8_t*            sOut;

    for (size_t i = 0; i < vOutSize; ++i)
    {
        __m128i a  = _mm_loadu_si128 (v1++);
        __m128i b  = _mm_loadu_si128 (v2++);
        __m128i lo = _mm_unpacklo_epi8 (a, b);
        __m128i hi = _mm_unpackhi_epi8 (a, b);

        _mm_storeu_si128 (vOut++, lo);
        _mm_storeu_si128 (vOut++, hi);
    }

    t1   = (const uint8_t*) v1;
    t2   = (const uint8_t*) v2;
    sOut = (uint8_t*) vOut;

    for (size_t i = vOutSize * bytesPerChunk; i < outSize; ++i)
        *(sOut++) = (i % 2 == 0) ? *(t1++) : *(t2++);
}

#else

static void
interleave (uint8_t* out, const uint8_t* source, size_t outSize)
{
    const char* t1   = source;
    const char* t2   = source + (outSize + 1) / 2;
    char*       s    = out;
    char* const stop = s + outSize;

    while (true)
    {
        if (s < stop)
            *(s++) = *(t1++);
        else
            break;

        if (s < stop)
            *(s++) = *(t2++);
        else
            break;
    }
}

#endif

/**************************************/

static int
undozip (
    const struct _internal_exr_context* ctxt,
    const void*                         src,
    size_t                              packsz,
    void*                               out,
    size_t                              outsz)
{
    uLongf outSize = outsz;
    int    rstat;
    /* TODO: find a way to not do this? */
    uint8_t* tmpbuf = ctxt->alloc_fn (outsz);
    if (tmpbuf == NULL) return EXR_ERR_OUT_OF_MEMORY;

    rstat = uncompress ((Bytef*) tmpbuf, &outSize, (const Bytef*) src, packsz);
    if (rstat == Z_OK)
    {
        reconstruct (tmpbuf, outSize);
        interleave (out, tmpbuf, outSize);
        rstat = EXR_ERR_SUCCESS;
    }
    else
    {
        rstat = EXR_ERR_BAD_CHUNK_DATA;
    }
    ctxt->free_fn (tmpbuf);

    return rstat;
}

/**************************************/

static int
undorle (
    const struct _internal_exr_context* ctxt,
    const void*                         src,
    size_t                              packsz,
    void*                               out,
    size_t                              outsz)
{
    const signed char* in  = (const signed char*) src;
    uint8_t*           dst = (uint8_t*) out;

    while (packsz > 0)
    {
        if (*in < 0)
        {
            int count = -((int) *in++);
            if (packsz >= (count + 1))
            {
                packsz -= (count + 1);
                if (outsz >= count)
                {
                    memcpy (dst, in, count);
                    in += count;
                    dst += count;
                }
                else
                {
                    return EXR_ERR_BAD_CHUNK_DATA;
                }
            }
            else
            {
                return EXR_ERR_BAD_CHUNK_DATA;
            }
        }
        else if (packsz >= 2)
        {
            int count = *in++;
            packsz -= 2;
            if (outsz >= (count + 1))
            {
                memset (dst, *(uint8_t*) in, count + 1);
                dst += count + 1;
                outsz -= (count + 1);
            }
            else
            {
                return EXR_ERR_BAD_CHUNK_DATA;
            }
            ++in;
        }
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_decompress_data (
    const exr_context_t     ctxt,
    const exr_compression_t ctype,
    const void*             compressed_data,
    size_t                  comp_buf_size,
    void*                   uncompressed_data,
    size_t                  uncompressed_size)
{
    exr_result_t rv;
    EXR_PROMOTE_CONST_CONTEXT_OR_ERROR (ctxt);

    switch (ctype)
    {
        case EXR_COMPRESSION_NONE:
            if (compressed_data == uncompressed_data) return EXR_ERR_SUCCESS;
            if (comp_buf_size != uncompressed_size)
                return pctxt->report_error (
                    ctxt,
                    EXR_ERR_INVALID_ARGUMENT,
                    "no compresssion set but buffers do not match");
            if (uncompressed_size > 0)
                memcpy (uncompressed_data, compressed_data, uncompressed_size);
            break;
        case EXR_COMPRESSION_RLE:
            if (compressed_data == uncompressed_data) return EXR_ERR_SUCCESS;
            if (comp_buf_size > 0)
            {
                rv = undorle (
                    pctxt,
                    compressed_data,
                    comp_buf_size,
                    uncompressed_data,
                    uncompressed_size);
                if (rv != EXR_ERR_SUCCESS)
                    return pctxt->report_error (
                        ctxt, rv, "Unable to RLE decompress");
            }
            break;
        case EXR_COMPRESSION_ZIP:
        case EXR_COMPRESSION_ZIPS:
            if (compressed_data == uncompressed_data) return EXR_ERR_SUCCESS;
            if (comp_buf_size > 0)
            {
                rv = undozip (
                    pctxt,
                    compressed_data,
                    comp_buf_size,
                    uncompressed_data,
                    uncompressed_size);
                if (rv != EXR_ERR_SUCCESS)
                    return pctxt->report_error (
                        ctxt, rv, "Unable to decompress using zlib");
            }
            break;
        case EXR_COMPRESSION_PIZ:
        case EXR_COMPRESSION_PXR24:
        case EXR_COMPRESSION_B44:
        case EXR_COMPRESSION_B44A:
        case EXR_COMPRESSION_DWAA:
        case EXR_COMPRESSION_DWAB:
        default:
            return pctxt->print_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Compression technique 0x%02X not yet implemented",
                ctype);
    }

    return EXR_ERR_SUCCESS;
}
