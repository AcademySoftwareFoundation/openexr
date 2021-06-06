/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_compress.h"

#include <zlib.h>
#include <string.h>

exr_result_t
internal_exr_apply_rle (exr_encode_pipeline_t* encode)
{
    int8_t*       cbuf = encode->compressed_buffer;
    const int8_t* in   = encode->packed_buffer;
    const int8_t* end  = in + encode->packed_bytes;
    const int8_t* runs = in;
    const int8_t* rune = runs + 1;
    size_t        outb = 0;

    if (encode->packed_bytes == 0) return EXR_ERR_OUT_OF_MEMORY;
    if (!encode->scratch_buffer_1) return EXR_ERR_OUT_OF_MEMORY;
    if (!encode->packed_buffer) return EXR_ERR_OUT_OF_MEMORY;
    if (!encode->compressed_buffer) return EXR_ERR_OUT_OF_MEMORY;
#define MIN_RUN_LENGTH 3
#define MAX_RUN_LENGTH 127
    while (runs < end)
    {
        while (rune < end && *runs == *rune &&
               (rune - runs - 1) < MAX_RUN_LENGTH)
            ++rune;
        if ((rune - runs) >= MIN_RUN_LENGTH)
        {
            cbuf[outb++] = (int8_t) ((rune - runs) - 1);
            cbuf[outb++] = *runs;

            runs = rune;
        }
        else
        {
            /* uncompressable */
            while (rune < end &&
                   ((rune + 1 >= end || *rune != *(rune + 1)) ||
                    (rune + 2 >= end || *(rune + 1) != *(rune + 2))) &&
                   (rune - runs) < MAX_RUN_LENGTH)
                ++rune;
            cbuf[outb++] = (int8_t)(runs - rune);
            while (runs < rune)
                cbuf[outb++] = *(rune++);
        }
        if (outb > encode->compressed_alloc_size)
            break;
    }

    if (outb > encode->packed_bytes)
    {
        memcpy (
            encode->compressed_buffer,
            encode->packed_buffer,
            encode->packed_bytes);
        outb = encode->packed_bytes;
    }
    encode->compressed_bytes = outb;
    return EXR_ERR_CHUNK_NOT_READY;
}

exr_result_t
internal_exr_apply_zip (exr_encode_pipeline_t* encode)
{
    uint8_t*       t1   = encode->scratch_buffer_1;
    uint8_t*       t2   = t1 + (encode->packed_bytes + 1) / 2;
    const uint8_t* raw  = encode->packed_buffer;
    const uint8_t* stop = raw + encode->packed_bytes;
    int            p;
    uLongf         compbufsz = encode->compressed_alloc_size;

    if (encode->packed_bytes == 0) return EXR_ERR_OUT_OF_MEMORY;
    if (!encode->scratch_buffer_1) return EXR_ERR_OUT_OF_MEMORY;
    if (!encode->packed_buffer) return EXR_ERR_OUT_OF_MEMORY;
    if (!encode->compressed_buffer) return EXR_ERR_OUT_OF_MEMORY;

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

    if (Z_OK != compress (
                    (Bytef*) encode->compressed_buffer,
                    &compbufsz,
                    (const Bytef*) encode->scratch_buffer_1,
                    encode->packed_bytes))
    {
        return EXR_ERR_BAD_CHUNK_DATA;
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
