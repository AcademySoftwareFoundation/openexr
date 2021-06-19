/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_compress.h"
#include "internal_decompress.h"

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
                cbuf[outb++] = *(runs++);
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
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
internal_exr_undo_rle (
    exr_decode_pipeline_t* decode,
    const void*            src,
    size_t                 packsz,
    void*                  out,
    size_t                 outsz)
{
    const signed char* in  = (const signed char*) src;
    uint8_t*           dst = (uint8_t*) out;

    (void)decode;
    while (packsz > 0)
    {
        if (*in < 0)
        {
            size_t count = (size_t)(-((int) *in++));
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
            size_t count = (size_t)( *in++ );
            packsz -= 2;
            if (outsz >= (count + 1))
            {
                memset (dst, *(const uint8_t*) in, (count + 1));
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
