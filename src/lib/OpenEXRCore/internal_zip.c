/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_compress.h"
#include "internal_decompress.h"

#include "internal_coding.h"
#include "internal_structs.h"

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#if defined __SSE2__ || (_MSC_VER >= 1300 && (_M_IX86 || _M_X64))
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
reconstruct (uint8_t* buf, uint64_t outSize)
{
    static const uint64_t bytesPerChunk = sizeof (__m128i);
    const uint64_t        vOutSize      = outSize / bytesPerChunk;
    const __m128i         c             = _mm_set1_epi8 (-128);
    const __m128i         shuffleMask   = _mm_set1_epi8 (15);
    __m128i *             vBuf, vPrev;
    uint8_t               prev;

    /*
     * The first element doesn't have its high bit flipped during compression,
     * so it must not be flipped here.  To make the SIMD loop nice and
     * uniform, we pre-flip the bit so that the loop will unflip it again.
     */
    buf[0] += -128;
    vBuf  = (__m128i*) buf;
    vPrev = _mm_setzero_si128 ();

    for (uint64_t i = 0; i < vOutSize; ++i)
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
    for (uint64_t i = vOutSize * bytesPerChunk; i < outSize; ++i)
    {
        uint8_t d = prev + buf[i] - 128;
        buf[i]    = d;
        prev      = d;
    }
}
#else
static void
reconstruct (uint8_t* buf, uint64_t sz)
{
    uint8_t* t    = buf + 1;
    uint8_t* stop = buf + sz;
    while (t < stop)
    {
        int d = (int) (t[-1]) + (int) (t[0]) - 128;
        t[0]  = (uint8_t) d;
        ++t;
    }
}
#endif

/**************************************/

#ifdef IMF_HAVE_SSE2
static void
interleave (uint8_t* out, const uint8_t* source, uint64_t outSize)
{
    static const uint64_t bytesPerChunk = 2 * sizeof (__m128i);
    const uint64_t        vOutSize      = outSize / bytesPerChunk;
    const __m128i*        v1            = (const __m128i*) source;
    const __m128i*        v2   = (const __m128i*) (source + (outSize + 1) / 2);
    __m128i*              vOut = (__m128i*) out;
    const uint8_t *       t1, *t2;
    uint8_t*              sOut;

    for (uint64_t i = 0; i < vOutSize; ++i)
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

    for (uint64_t i = vOutSize * bytesPerChunk; i < outSize; ++i)
        *(sOut++) = (i % 2 == 0) ? *(t1++) : *(t2++);
}

#else

static void
interleave (uint8_t* out, const uint8_t* source, uint64_t outSize)
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

static exr_result_t
undo_zip_impl (
    const void* compressed_data,
    uint64_t    comp_buf_size,
    void*       uncompressed_data,
    uint64_t    uncompressed_size,
    void*       scratch_data,
    uint64_t    scratch_size)
{
    uLongf outSize = (uLongf) uncompressed_size;
    int    rstat;

    if (scratch_size < uncompressed_size) return EXR_ERR_INVALID_ARGUMENT;

    rstat = uncompress (
        (Bytef*) scratch_data,
        &outSize,
        (const Bytef*) compressed_data,
        (uLong) comp_buf_size);
    if (rstat == Z_OK)
    {
        if (outSize == uncompressed_size)
        {
            reconstruct (scratch_data, outSize);
            interleave (uncompressed_data, scratch_data, outSize);
            rstat = EXR_ERR_SUCCESS;
        }
        else
        {
            rstat = EXR_ERR_CORRUPT_CHUNK;
        }
    }
    else
    {
        rstat = EXR_ERR_CORRUPT_CHUNK;
    }

    return (exr_result_t) rstat;
}

/**************************************/

exr_result_t
internal_exr_undo_zip (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    exr_result_t rv;
    rv = internal_decode_alloc_buffer (
        decode,
        EXR_TRANSCODE_BUFFER_SCRATCH1,
        &(decode->scratch_buffer_1),
        &(decode->scratch_alloc_size_1),
        uncompressed_size);
    if (rv != EXR_ERR_SUCCESS) return rv;
    return undo_zip_impl (
        compressed_data,
        comp_buf_size,
        uncompressed_data,
        uncompressed_size,
        decode->scratch_buffer_1,
        decode->scratch_alloc_size_1);
}

/**************************************/

static exr_result_t
apply_zip_impl (exr_encode_pipeline_t* encode)
{
    uint8_t*       t1   = encode->scratch_buffer_1;
    uint8_t*       t2   = t1 + (encode->packed_bytes + 1) / 2;
    const uint8_t* raw  = encode->packed_buffer;
    const uint8_t* stop = raw + encode->packed_bytes;
    int            p, level;
    uLongf         compbufsz = encode->compressed_alloc_size;
    exr_result_t   rv        = EXR_ERR_SUCCESS;

    rv = exr_get_zip_compression_level (
        encode->context, encode->part_index, &level);
    if (rv != EXR_ERR_SUCCESS) return rv;

    /* reorder */
    while (raw < stop)
    {
        *(t1++) = *(raw++);
        if (raw < stop) *(t2++) = *(raw++);
    }

    /* reorder */
    t1 = encode->scratch_buffer_1;
    t2 = t1 + encode->packed_bytes;
    t1++;
    p = (int) t1[-1];
    while (t1 < t2)
    {
        int d = (int) (t1[0]) - p + (128 + 256);
        p     = (int) t1[0];
        t1[0] = (uint8_t) d;
        ++t1;
    }

    if (Z_OK != compress2 (
                    (Bytef*) encode->compressed_buffer,
                    &compbufsz,
                    (const Bytef*) encode->scratch_buffer_1,
                    encode->packed_bytes,
                    level))
    {
        return EXR_ERR_CORRUPT_CHUNK;
    }
    if (compbufsz > encode->packed_bytes)
    {
        memcpy (
            encode->compressed_buffer,
            encode->packed_buffer,
            encode->packed_bytes);
        compbufsz = encode->packed_bytes;
    }
    encode->compressed_bytes = compbufsz;
    return EXR_ERR_SUCCESS;
}

exr_result_t
internal_exr_apply_zip (exr_encode_pipeline_t* encode)
{
    exr_result_t rv;

    rv = internal_encode_alloc_buffer (
        encode,
        EXR_TRANSCODE_BUFFER_SCRATCH1,
        &(encode->scratch_buffer_1),
        &(encode->scratch_alloc_size_1),
        encode->packed_bytes);
    if (rv != EXR_ERR_SUCCESS) return rv;

    return apply_zip_impl (encode);
}
