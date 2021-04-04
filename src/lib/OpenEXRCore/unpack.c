/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_unpack.h"

#include <string.h>

/**************************************/

static exr_result_t
unpack_16bit_3chan_interleave (exr_decode_pipeline_t* decode)
{
    /* we know we're unpacking all the channels and there is no subsampling */
    const uint8_t*  srcbuffer = decode->unpacked_buffer;
    const uint16_t *in0, *in1, *in2;
    uint8_t*        out0;
    int             w, h;
    int             linc0;

    w     = decode->channels[0].width;
    h     = decode->chunk_block.height;
    linc0 = decode->channels[0].output_line_stride;

    out0 = decode->channels[0].decode_to_ptr;

    /* interleaving case, we can do this! */
    for (int y = 0; y < h; ++y)
    {
        uint16_t* out = (uint16_t*) out0;
        in0           = (const uint16_t*) srcbuffer;
        in1           = in0 + w;
        in2           = in1 + w;

        srcbuffer += w * 3 * sizeof (uint16_t);
        for (int x = 0; x < w; ++x)
        {
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
            out[x * 3]     = le16toh (in1[x]);
            out[x * 3 + 1] = le16toh (in2[x]);
            out[x * 3 + 2] = le16toh (in3[x]);
#else
            out[x * 3]     = in0[x];
            out[x * 3 + 1] = in1[x];
            out[x * 3 + 2] = in2[x];
#endif
        }
        out0 += linc0;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
unpack_16bit_3chan_planar (exr_decode_pipeline_t* decode)
{
    /* we know we're unpacking all the channels and there is no subsampling */
    const uint8_t*  srcbuffer = decode->unpacked_buffer;
    const uint16_t *in0, *in1, *in2;
    uint8_t *       out0, *out1, *out2;
    int             w, h;
    int             inc0, inc1, inc2;
    int             linc0, linc1, linc2;

    w     = decode->channels[0].width;
    h     = decode->chunk_block.height;
    inc0  = decode->channels[0].output_pixel_stride;
    inc1  = decode->channels[1].output_pixel_stride;
    inc2  = decode->channels[2].output_pixel_stride;
    linc0 = decode->channels[0].output_line_stride;
    linc1 = decode->channels[1].output_line_stride;
    linc2 = decode->channels[2].output_line_stride;

    out0 = decode->channels[0].decode_to_ptr;
    out1 = decode->channels[1].decode_to_ptr;
    out2 = decode->channels[2].decode_to_ptr;

    // planar output
    for (int y = 0; y < h; ++y)
    {
        in0 = (const uint16_t*) srcbuffer;
        in1 = in0 + w;
        in2 = in1 + 1;
        srcbuffer += w * 3 * sizeof (uint16_t);
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out0) + x) = le16toh (in0[x]);
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out1) + x) = le16toh (in1[x]);
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out2) + x) = le16toh (in2[x]);
#else
        memcpy (out0, in0, w * sizeof (uint16_t));
        memcpy (out1, in1, w * sizeof (uint16_t));
        memcpy (out2, in2, w * sizeof (uint16_t));
#endif
        out0 += linc0;
        out1 += linc1;
        out2 += linc2;
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
unpack_16bit_3chan (exr_decode_pipeline_t* decode)
{
    /* we know we're unpacking all the channels and there is no subsampling */
    const uint8_t*  srcbuffer = decode->unpacked_buffer;
    const uint16_t *in0, *in1, *in2;
    uint8_t *       out0, *out1, *out2;
    int             w, h;
    int             inc0, inc1, inc2;
    int             linc0, linc1, linc2;

    w     = decode->channels[0].width;
    h     = decode->chunk_block.height;
    inc0  = decode->channels[0].output_pixel_stride;
    inc1  = decode->channels[1].output_pixel_stride;
    inc2  = decode->channels[2].output_pixel_stride;
    linc0 = decode->channels[0].output_line_stride;
    linc1 = decode->channels[1].output_line_stride;
    linc2 = decode->channels[2].output_line_stride;

    out0 = decode->channels[0].decode_to_ptr;
    out1 = decode->channels[1].decode_to_ptr;
    out2 = decode->channels[2].decode_to_ptr;

    for (int y = 0; y < h; ++y)
    {
        in0 = (const uint16_t*) srcbuffer;
        in1 = in0 + w;
        in2 = in1 + w;
        srcbuffer += w * 6;
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out0 + x * inc0)) = le16toh (in0[x]);
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out1 + x * inc1)) = le16toh (in1[x]);
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out2 + x * inc2)) = le16toh (in2[x]);
#else
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out0 + x * inc0)) = in0[x];
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out1 + x * inc1)) = in1[x];
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out2 + x * inc2)) = in2[x];
#endif
        out0 += linc0;
        out1 += linc1;
        out2 += linc2;
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
unpack_16bit_4chan_interleave (exr_decode_pipeline_t* decode)
{
    /* we know we're unpacking all the channels and there is no subsampling */
    const uint8_t*  srcbuffer = decode->unpacked_buffer;
    const uint16_t *in0, *in1, *in2, *in3;
    uint8_t*        out0;
    int             w, h;
    int             linc0;
    /* TODO: can do this with sse and do 2 outpixels at once */
    union
    {
        struct
        {
            uint16_t a;
            uint16_t b;
            uint16_t g;
            uint16_t r;
        };
        uint64_t allc;
    } combined;

    w     = decode->channels[0].width;
    h     = decode->chunk_block.height;
    linc0 = decode->channels[0].output_line_stride;

    out0 = decode->channels[0].decode_to_ptr;

    /* interleaving case, we can do this! */
    for (int y = 0; y < h; ++y)
    {
        uint64_t* outall = (uint64_t*) out0;
        in0              = (const uint16_t*) srcbuffer;
        in1              = in0 + w;
        in2              = in1 + w;
        in3              = in2 + w;

        srcbuffer += w * 4 * sizeof (uint16_t);
        for (int x = 0; x < w; ++x)
        {
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
            combined.a = le16toh (in0[x]);
            combined.b = le16toh (in1[x]);
            combined.g = le16toh (in2[x]);
            combined.r = le16toh (in3[x]);
#else
            combined.a = in0[x];
            combined.b = in1[x];
            combined.g = in2[x];
            combined.r = in3[x];
#endif
            outall[x] = combined.allc;
        }
        out0 += linc0;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
unpack_16bit_4chan_planar (exr_decode_pipeline_t* decode)
{
    /* we know we're unpacking all the channels and there is no subsampling */
    const uint8_t*  srcbuffer = decode->unpacked_buffer;
    const uint16_t *in0, *in1, *in2, *in3;
    uint8_t *       out0, *out1, *out2, *out3;
    int             w, h;
    int             linc0, linc1, linc2, linc3;

    w     = decode->channels[0].width;
    h     = decode->chunk_block.height;
    linc0 = decode->channels[0].output_line_stride;
    linc1 = decode->channels[1].output_line_stride;
    linc2 = decode->channels[2].output_line_stride;
    linc3 = decode->channels[3].output_line_stride;

    out0 = decode->channels[0].decode_to_ptr;
    out1 = decode->channels[1].decode_to_ptr;
    out2 = decode->channels[2].decode_to_ptr;
    out3 = decode->channels[3].decode_to_ptr;

    // planar output
    for (int y = 0; y < h; ++y)
    {
        in0 = (const uint16_t*) srcbuffer;
        in1 = in0 + w;
        in2 = in1 + w;
        in3 = in2 + w;
        srcbuffer += w * 4 * sizeof (uint16_t);
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out0) + x) = le16toh (in0[x]);
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out1) + x) = le16toh (in1[x]);
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out2) + x) = le16toh (in2[x]);
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out3) + x) = le16toh (in3[x]);
#else
        memcpy (out0, in0, w * sizeof (uint16_t));
        memcpy (out1, in1, w * sizeof (uint16_t));
        memcpy (out2, in2, w * sizeof (uint16_t));
        memcpy (out3, in3, w * sizeof (uint16_t));
#endif
        out0 += linc0;
        out1 += linc1;
        out2 += linc2;
        out3 += linc3;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
unpack_16bit_4chan (exr_decode_pipeline_t* decode)
{
    /* we know we're unpacking all the channels and there is no subsampling */
    const uint8_t*  srcbuffer = decode->unpacked_buffer;
    const uint16_t *in0, *in1, *in2, *in3;
    uint8_t *       out0, *out1, *out2, *out3;
    int             w, h;
    int             inc0, inc1, inc2, inc3;
    int             linc0, linc1, linc2, linc3;

    w     = decode->channels[0].width;
    h     = decode->chunk_block.height;
    inc0  = decode->channels[0].output_pixel_stride;
    inc1  = decode->channels[1].output_pixel_stride;
    inc2  = decode->channels[2].output_pixel_stride;
    inc3  = decode->channels[3].output_pixel_stride;
    linc0 = decode->channels[0].output_line_stride;
    linc1 = decode->channels[1].output_line_stride;
    linc2 = decode->channels[2].output_line_stride;
    linc3 = decode->channels[3].output_line_stride;

    out0 = decode->channels[0].decode_to_ptr;
    out1 = decode->channels[1].decode_to_ptr;
    out2 = decode->channels[2].decode_to_ptr;
    out3 = decode->channels[3].decode_to_ptr;

    for (int y = 0; y < h; ++y)
    {
        in0 = (uint16_t*) srcbuffer;
        in1 = in0 + w;
        in2 = in1 + w;
        in3 = in2 + w;
        srcbuffer += w * 4 * sizeof(uint16_t);
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out0 + x * inc0)) = le16toh (in0[x]);
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out1 + x * inc1)) = le16toh (in1[x]);
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out2 + x * inc2)) = le16toh (in2[x]);
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out3 + x * inc3)) = le16toh (in3[x]);
#else
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out0 + x * inc0)) = in0[x];
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out1 + x * inc1)) = in1[x];
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out2 + x * inc2)) = in2[x];
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out3 + x * inc3)) = in3[x];
#endif
        out0 += linc0;
        out1 += linc1;
        out2 += linc2;
        out3 += linc3;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
unpack_16bit (exr_decode_pipeline_t* decode)
{
    /* we know we're unpacking all the channels and there is no subsampling */
    const uint8_t* srcbuffer = decode->unpacked_buffer;
    uint8_t*       cdata;
    int            w, h, pixincrement;

    h = decode->chunk_block.height;
    for (int y = 0; y < h; ++y)
    {
        for (int c = 0; c < decode->channel_count; ++c)
        {
            exr_coding_channel_info_t* decc = (decode->channels + c);

            cdata        = decc->decode_to_ptr;
            w            = decc->width;
            pixincrement = decc->output_pixel_stride;
            cdata += (uint64_t) y * (uint64_t) decc->output_line_stride;
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
            if (pixincrement == 2)
            {
                uint16_t*       tmp = (uint16_t*) cdata;
                const uint16_t* src = (const uint16_t*) srcbuffer;
                uint16_t*       end = tmp + w;

                while (tmp < end)
                    *tmp++ = le16toh (*src++);
            }
            else
            {
                const uint16_t* src = (const uint16_t*) srcbuffer;
                for (int x = 0; x < w; ++x)
                {
                    *((uint16_t*) cdata) = le16toh (*src++);
                    cdata += pixincrement;
                }
            }
#else
            if (pixincrement == 2)
            {
                memcpy (cdata, srcbuffer, w * pixincrement);
            }
            else
            {
                const uint16_t* src = (const uint16_t*) srcbuffer;
                for (int x = 0; x < w; ++x)
                {
                    *((uint16_t*) cdata) = *src++;
                    cdata += pixincrement;
                }
            }
#endif
            srcbuffer += w * 2;
        }
    }
    return EXR_ERR_SUCCESS;
}

//static exr_result_t unpack_32bit_3chan (exr_decode_pipeline_t* decode);
//static exr_result_t unpack_32bit_4chan (exr_decode_pipeline_t* decode);

static exr_result_t
unpack_32bit (exr_decode_pipeline_t* decode)
{
    /* we know we're unpacking all the channels and there is no subsampling */
    const uint8_t* srcbuffer = decode->unpacked_buffer;
    uint8_t*       cdata;
    int64_t        w, h, pixincrement;
    int            chans = decode->channel_count;

    h = (int64_t) decode->chunk_block.height;

    for (int64_t y = 0; y < h; ++y)
    {
        for (int c = 0; c < chans; ++c)
        {
            exr_coding_channel_info_t* decc = (decode->channels + c);

            cdata        = decc->decode_to_ptr;
            w            = decc->width;
            pixincrement = decc->output_pixel_stride;
            cdata += y * (int64_t) decc->output_line_stride;
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
            if (pixincrement == 4)
            {
                uint32_t*       tmp = (uint32_t*) cdata;
                const uint32_t* src = (const uint32_t*) srcbuffer;
                uint32_t*       end = tmp + w;

                while (tmp < end)
                    *tmp++ = le32toh (*src++);
            }
            else
            {
                const uint32_t* src = (const uint32_t*) srcbuffer;
                for (int64_t x = 0; x < w; ++x)
                {
                    *((uint32_t*) cdata) = le32toh (*src++);
                    cdata += pixincrement;
                }
            }
#else
            if (pixincrement == 4)
            {
                memcpy (cdata, srcbuffer, w * pixincrement);
            }
            else
            {
                const uint32_t* src = (const uint32_t*) srcbuffer;
                for (int64_t x = 0; x < w; ++x)
                {
                    *((uint32_t*) cdata) = *src++;
                    cdata += pixincrement;
                }
            }
#endif
            srcbuffer += w * 4;
        }
    }
    return EXR_ERR_SUCCESS;
}

static exr_result_t
generic_unpack_subsampled (exr_decode_pipeline_t* decode)
{
    const uint8_t* srcbuffer = decode->unpacked_buffer;
    uint8_t*       cdata;
    int            w, bpc;

    for (int y = 0; y < decode->chunk_block.height; ++y)
    {
        int cury = y + decode->chunk_block.start_y;
        for (int c = 0; c < decode->channel_count; ++c)
        {
            exr_coding_channel_info_t* decc = (decode->channels + c);

            cdata = decc->decode_to_ptr;
            w     = decc->width;
            bpc   = decc->bytes_per_element;
            /*obpc   = decc->output_bytes_per_element;*/

            if (decc->y_samples > 1)
            {
                if ((cury % decc->y_samples) != 0) continue;
                if (cdata)
                    cdata +=
                        ((uint64_t) (y / decc->y_samples) *
                         (uint64_t) decc->output_line_stride);
            }
            else if (cdata)
            {
                cdata += (uint64_t) y * (uint64_t) decc->output_line_stride;
            }

            if (cdata)
            {
                int pixincrement = decc->output_pixel_stride;
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
                if (bpc == 2)
                {
                    if (pixincrement == 2)
                    {
                        uint16_t*       tmp = (uint16_t*) cdata;
                        const uint16_t* src = (const uint16_t*) srcbuffer;
                        uint16_t*       end = tmp + w;

                        while (tmp < end)
                            *tmp++ = le16toh (*src++);
                    }
                    else
                    {
                        const uint16_t* src = (const uint16_t*) srcbuffer;
                        for (int x = 0; x < w; ++x)
                        {
                            *((uint16_t*) cdata) = le16toh (*src++);
                            cdata += pixincrement;
                        }
                    }
                }
                else
                {
                    if (pixincrement == 4)
                    {
                        uint32_t*       tmp = (uint32_t*) cdata;
                        const uint32_t* src = (const uint32_t*) srcbuffer;
                        uint32_t*       end = tmp + w;

                        while (tmp < end)
                            *tmp++ = le32toh (*src++);
                    }
                    else
                    {
                        const uint32_t* src = (const uint32_t*) srcbuffer;
                        for (int x = 0; x < w; ++x)
                        {
                            *((uint32_t*) cdata) = le32toh (*src++);
                            cdata += pixincrement;
                        }
                    }
                }
#else
                if (bpc == pixincrement)
                {
                    memcpy (cdata, srcbuffer, w * pixincrement);
                }
                else if (bpc == 2)
                {
                    const uint16_t* src = (const uint16_t*) srcbuffer;
                    for (int x = 0; x < w; ++x)
                    {
                        *((uint16_t*) cdata) = *src++;
                        cdata += pixincrement;
                    }
                }
                else if (bpc == 4)
                {
                    const uint32_t* src = (const uint32_t*) srcbuffer;
                    for (int x = 0; x < w; ++x)
                    {
                        *((uint32_t*) cdata) = *src++;
                        cdata += pixincrement;
                    }
                }
#endif /* byte order check */
            }
            srcbuffer += w * bpc;
        }
    }
    return EXR_ERR_SUCCESS;
}

internal_exr_unpack_fn
internal_exr_match_decode (
    exr_decode_pipeline_t* decode,
    int                    isdeep,
    int                    chanstofill,
    int                    chanstounpack,
    int                    samebpc,
    int                    sameoutbpc,
    int                    hassampling,
    int                    hastypechange,
    int                    sameoutinc,
    int                    simpinterleave,
    int                    simplineoff)
{
    /* TODO */
    if (isdeep || hastypechange) return NULL;

    if (hassampling || chanstofill != decode->channel_count || samebpc <= 0 ||
        sameoutbpc <= 0)
        return &generic_unpack_subsampled;

    /*
    if (hastypechange > 0)
    {
        if (samebpc == 2 && sameoutbpc == 4)
        {
        }
        else if (samebpc == 4 && sameoutbpc == 2)
        {
        }
    }
    */

    if (samebpc == 2)
    {
        if (simpinterleave > 0)
        {
            if (decode->channel_count == 4)
                return &unpack_16bit_4chan_interleave;
            if (decode->channel_count == 3)
                return &unpack_16bit_3chan_interleave;
        }

        if (sameoutinc == 2)
        {
            if (decode->channel_count == 4) return &unpack_16bit_4chan_planar;
            if (decode->channel_count == 3) return &unpack_16bit_3chan_planar;
        }

        if (decode->channel_count == 4) return &unpack_16bit_4chan;
        if (decode->channel_count == 3) return &unpack_16bit_3chan;

        return &unpack_16bit;
    }

    if (samebpc == 4)
    {
        //if (decode->channel_count == 4) return &unpack_32bit_4chan;
        //if (decode->channel_count == 3) return &unpack_32bit_3chan;
        return &unpack_32bit;
    }

    return NULL;
}
