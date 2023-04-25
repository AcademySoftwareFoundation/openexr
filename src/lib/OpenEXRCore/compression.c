/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_compression.h"
#include "openexr_base.h"

#include <libdeflate.h>

/* value Aras found to be better trade off of speed vs size */
#define EXR_DEFAULT_ZLIB_COMPRESS_LEVEL 4

/**************************************/

size_t exr_compress_max_buffer_size (size_t in_bytes)
{
    size_t r, extra;

    r = libdeflate_zlib_compress_bound (NULL, in_bytes);
    /*
     * lib deflate has a message about needing a 9 byte boundary
     * but is unclear if it actually adds that or not
     * (see the comment on libdeflate_deflate_compress)
     */
    if (r > (SIZE_MAX - 9))
        return (size_t)(SIZE_MAX);
    r += 9;

    /*
     * old library had uiAdd( uiAdd( in, ceil(in * 0.01) ), 100 )
     */
    extra = (in_bytes * (size_t)130);
    if (extra < in_bytes)
        return (size_t)(SIZE_MAX);
    extra /= (size_t)128;

    if (extra > (SIZE_MAX - 100))
        return (size_t)(SIZE_MAX);

    if (extra > r)
        r = extra;
    return r;
}

/**************************************/

exr_result_t exr_compress_buffer (
    int level,
    const void *in,
    size_t in_bytes,
    void *out,
    size_t out_bytes_avail,
    size_t *actual_out )
{
    struct libdeflate_compressor *comp;

    if (level < 0)
    {
        exr_get_default_zip_compression_level (&level);
        /* truly unset anywhere */
        if (level < 0)
            level = EXR_DEFAULT_ZLIB_COMPRESS_LEVEL;
    }

    comp = libdeflate_alloc_compressor (level);
    if (comp)
    {
        size_t outsz;
        outsz = libdeflate_zlib_compress (
            comp,
            in,
            in_bytes,
            out,
            out_bytes_avail);

        libdeflate_free_compressor (comp);

        if (outsz != 0)
        {
            if (actual_out)
                *actual_out = outsz;
            return EXR_ERR_SUCCESS;
        }
        return EXR_ERR_OUT_OF_MEMORY;
    }
    return EXR_ERR_OUT_OF_MEMORY;
}

/**************************************/

exr_result_t exr_uncompress_buffer (
    const void *in,
    size_t in_bytes,
    void *out,
    size_t out_bytes_avail,
    size_t *actual_out )
{
    struct libdeflate_decompressor *decomp;
    enum libdeflate_result res;
    size_t actual_in_bytes;

    decomp = libdeflate_alloc_decompressor ();
    if (decomp)
    {
        res = libdeflate_zlib_decompress_ex (
            decomp,
            in,
            in_bytes,
            out,
            out_bytes_avail,
            &actual_in_bytes,
            actual_out);

        libdeflate_free_decompressor (decomp);

        if (res == LIBDEFLATE_SUCCESS)
        {
            if (in_bytes == actual_in_bytes)
                return EXR_ERR_SUCCESS;
            /* it's an error to not consume the full buffer, right? */
        }
        return EXR_ERR_CORRUPT_CHUNK;
    }
    return EXR_ERR_OUT_OF_MEMORY;
}

