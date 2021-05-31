/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_unpack.h"
#include "internal_xdr.h"

#include "openexr_attr.h"

#include <ImathConfig.h>
/* only recently has imath supported half in C (C++ only before),
 * allow an older version to still work, and if that is available, we
 * will favor the implementation there as it will be the latest
 * up-to-date optimizations */
#if (IMATH_VERSION_MAJOR > 3) ||                                               \
    (IMATH_VERSION_MAJOR == 3 && IMATH_VERSION_MINOR >= 1)
#    define IMATH_HALF_SAFE_FOR_C
/* avoid the library dependency */
#    define IMATH_HALF_NO_TABLES_AT_ALL
#    include <half.h>
#endif

#if defined(__has_include)
#    if __has_include(<x86intrin.h>)
#        include <x86intrin.h>
#    elif __has_include(<intrin.h>)
#        include <intrin.h>
#    endif
#endif
#include <math.h>

#include <string.h>

#if defined(__x86_64__) || defined(_M_X64)
#    ifndef _WIN32
#        include <cpuid.h>
#    endif
#endif

/**************************************/

static inline float
half_to_float (uint16_t hv)
{
#ifdef IMATH_HALF_SAFE_FOR_C
    return imath_half_to_float (hv);
#else
    /* replicate the code here from imath 3.1 since we are on an older
     * version which doesn't have a half that is safe for C code. Same
     * author, so free to do so. */
#    if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#        define OUR_LIKELY(x) (__builtin_expect ((x), 1))
#        define OUR_UNLIKELY(x) (__builtin_expect ((x), 0))
#    else
#        define OUR_LIKELY(x) (x)
#        define OUR_UNLIKELY(x) (x)
#    endif
    union
    {
        uint32_t i;
        float    f;
    } v;
    uint32_t hexpmant = ((uint32_t) (hv) << 17) >> 4;
    v.i               = ((uint32_t) (hv >> 15)) << 31;
    if (OUR_LIKELY ((hexpmant >= 0x00800000)))
    {
        v.i |= hexpmant;
        if (OUR_LIKELY ((hexpmant < 0x0f800000)))
            v.i += 0x38000000;
        else
            v.i |= 0x7f800000;
    }
    else if (hexpmant != 0)
    {
        uint32_t lc;
#    if defined(_MSC_VER)
        lc = __lzcnt (hexpmant);
#    elif defined(__GNUC__) || defined(__clang__)
        lc = (uint32_t) __builtin_clz (hexpmant);
#    else
        lc = 0;
        while (0 == ((hexpmant << lc) & 0x80000000))
            ++lc;
#    endif
        lc -= 8;
        v.i |= 0x38800000;
        v.i |= (hexpmant << lc);
        v.i -= (lc << 23);
    }
    return v.f;
#endif
}

static inline uint16_t
float_to_half (uint32_t fint)
{
#ifdef IMATH_HALF_SAFE_FOR_C
    union
    {
        uint32_t i;
        float    f;
    } v;
    v.i = fint;
    return imath_float_to_half (v.f);
#else
    uint16_t ret;
    uint32_t e, m, ui, r, shift;

    ui  = (fint & ~0x80000000);
    ret = ((fint >> 16) & 0x8000);

    if (ui >= 0x38800000)
    {
        if (OUR_UNLIKELY (ui >= 0x7f800000))
        {
            ret |= 0x7c00;
            if (ui == 0x7f800000) return ret;
            m = (ui & 0x7fffff) >> 13;
            return (uint16_t) (ret | m | (m == 0));
        }

        if (OUR_UNLIKELY (ui > 0x477fefff)) return ret | 0x7c00;

        ui -= 0x38000000;
        ui = ((ui + 0x00000fff + ((ui >> 13) & 1)) >> 13);
        return (uint16_t) (ret | ui);
    }

    // zero or flush to 0
    if (ui < 0x33000001) return ret;

    // produce a denormalized half
    e     = (ui >> 23);
    shift = 0x7e - e;
    m     = 0x800000 | (ui & 0x7fffff);
    r     = m << (32 - shift);
    ret |= (m >> shift);
    if (r > 0x80000000 || (r == 0x80000000 && (ret & 0x1) != 0)) ++ret;
    return ret;
#endif
}

/**************************************/

static inline uint32_t
half_to_uint (uint16_t hv)
{
    /* replicating logic from imfmisc if negative or nan, return 0, if
     * inf, return uint32 max otherwise convert to float and cast to
     * uint */
    if (hv & 0x8000) return 0;
    if ((hv & 0x7c00) == 0x7c00)
    {
        if ((hv & 0x3ff) != 0) return 0;
        return UINT32_MAX;
    }
    return (uint32_t) (half_to_float (hv));
}

static inline uint32_t
float_to_uint (uint32_t fint)
{
    union
    {
        uint32_t i;
        float    f;
    } v;
    v.i = fint;
    if (v.f < 0.f || isnan (v.f)) return 0;
    if (isinf (v.f) || v.f > (float) (UINT32_MAX)) return UINT32_MAX;
    return (uint32_t) (v.f);
}

static inline uint16_t
uint_to_half (uint32_t ui)
{
    if (ui > 65504) return 0x7c00;

    /* this may look upside down, but re-using code */
    union
    {
        uint32_t i;
        float    f;
    } v;
    v.f = (float) ui;
    return float_to_half (v.i);
}

/**************************************/

#ifndef __F16C__
static inline void
half_to_float4 (float* out, const uint16_t* src)
{
    out[0] = half_to_float (src[0]);
    out[1] = half_to_float (src[1]);
    out[2] = half_to_float (src[2]);
    out[3] = half_to_float (src[3]);
}

static inline void
half_to_float8 (float* out, const uint16_t* src)
{
    half_to_float4 (out, src);
    half_to_float4 (out + 4, src + 4);
}
#endif

#if (defined(__x86_64__) || defined(_M_X64)) && (defined(__F16C__) || defined(__GNUC__) || defined(__clang__))

#if defined(__F16C__)
static inline void
half_to_float_buffer (float* out, const uint16_t* in, int w)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__ ((target ("f16c"))) static void
half_to_float_buffer_f16c (float* out, const uint16_t* in, int w)
#endif
{
    while (w >= 8)
    {
        _mm256_storeu_ps (out, _mm256_cvtph_ps (_mm_loadu_si128 ((const __m128i *)in)));
        out += 8;
        in += 8;
        w -= 8;
    }
    switch (w)
    {
        case 7:
            _mm_storeu_ps (out, _mm_cvtph_ps (_mm_loadu_si64 (in)));
            out[4] = half_to_float (in[4]);
            out[5] = half_to_float (in[5]);
            out[6] = half_to_float (in[6]);
            break;
        case 6:
            _mm_storeu_ps (out, _mm_cvtph_ps (_mm_loadu_si64 (in)));
            out[4] = half_to_float (in[4]);
            out[5] = half_to_float (in[5]);
            break;
        case 5:
            _mm_storeu_ps (out, _mm_cvtph_ps (_mm_loadu_si64 (in)));
            out[4] = half_to_float (in[4]);
            break;
        case 4: _mm_storeu_ps (out, _mm_cvtph_ps (_mm_loadu_si64 (in))); break;
        case 3:
            out[0] = half_to_float (in[0]);
            out[1] = half_to_float (in[1]);
            out[2] = half_to_float (in[2]);
            break;
        case 2:
            out[0] = half_to_float (in[0]);
            out[1] = half_to_float (in[1]);
            break;
        case 1: out[0] = half_to_float (in[0]); break;
    }
}

#ifndef __F16C__
static void
half_to_float_buffer_impl (float* out, const uint16_t* in, int w)
{
    while (w >= 8)
    {
        half_to_float8 (out, in);
        out += 8;
        in += 8;
        w -= 8;
    }
    switch (w)
    {
        case 7:
            half_to_float4 (out, in);
            out[4] = half_to_float (in[4]);
            out[5] = half_to_float (in[5]);
            out[6] = half_to_float (in[6]);
            break;
        case 6:
            half_to_float4 (out, in);
            out[4] = half_to_float (in[4]);
            out[5] = half_to_float (in[5]);
            break;
        case 5:
            half_to_float4 (out, in);
            out[4] = half_to_float (in[4]);
            break;
        case 4: half_to_float4 (out, in); break;
        case 3:
            out[0] = half_to_float (in[0]);
            out[1] = half_to_float (in[1]);
            out[2] = half_to_float (in[2]);
            break;
        case 2:
            out[0] = half_to_float (in[0]);
            out[1] = half_to_float (in[1]);
            break;
        case 1: out[0] = half_to_float (in[0]); break;
    }
}

static void (*half_to_float_buffer)(float *, const uint16_t *, int) = &half_to_float_buffer_impl;

static void choose_half_to_float_impl()
{
#    ifdef _WIN32
    int regs[4];

    __cpuid (regs, 0);
    if (regs[0] >= 1)
    {
        __cpuidex (regs, 1, 0);
    }
#    else
    unsigned int regs[4];
    __get_cpuid (0, &regs[0], &regs[1], &regs[2], &regs[3]);
    if (regs[0] >= 1)
    {
        __get_cpuid (1, &regs[0], &regs[1], &regs[2], &regs[3]);
    }
#    endif
    /* F16C is indicated by bit 29 */
    if (regs[0] & (1 << 29)) 
        half_to_float_buffer = &half_to_float_buffer_f16c;
}
#else
/* when we explicitly compile against f16, force it in */
static void choose_half_to_float_impl()
{
}

#endif /* F16C */

#else

static inline void
half_to_float_buffer (float* out, const uint16_t* in, int w)
{
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
    for (int x = 0; x < w; ++x)
        out[x] = half_to_float (one_to_native16 (in[x]));
#else
    while (w >= 8)
    {
        half_to_float8 (out, in);
        out += 8;
        in += 8;
        w -= 8;
    }
    switch (w)
    {
        case 7:
            half_to_float4 (out, in);
            out[4] = half_to_float (in[4]);
            out[5] = half_to_float (in[5]);
            out[6] = half_to_float (in[6]);
            break;
        case 6:
            half_to_float4 (out, in);
            out[4] = half_to_float (in[4]);
            out[5] = half_to_float (in[5]);
            break;
        case 5:
            half_to_float4 (out, in);
            out[4] = half_to_float (in[4]);
            break;
        case 4: half_to_float4 (out, in); break;
        case 3:
            out[0] = half_to_float (in[0]);
            out[1] = half_to_float (in[1]);
            out[2] = half_to_float (in[2]);
            break;
        case 2:
            out[0] = half_to_float (in[0]);
            out[1] = half_to_float (in[1]);
            break;
        case 1: out[0] = half_to_float (in[0]); break;
    }
#endif
}

static void choose_half_to_float_impl()
{
}

#endif

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

        in0 = (const uint16_t*) srcbuffer;
        in1 = in0 + w;
        in2 = in1 + w;

        srcbuffer += w * 6; // 3 * sizeof(uint16_t), avoid type conversion
        for (int x = 0; x < w; ++x)
        {
            out[0] = one_to_native16 (in0[x]);
            out[1] = one_to_native16 (in1[x]);
            out[2] = one_to_native16 (in2[x]);
            out += 3;
        }
        out0 += linc0;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
unpack_half_to_float_3chan_interleave (exr_decode_pipeline_t* decode)
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
        float* out = (float*) out0;

        in0 = (const uint16_t*) srcbuffer;
        in1 = in0 + w;
        in2 = in1 + w;

        srcbuffer += w * 6; // 3 * sizeof(uint16_t), avoid type conversion
        for (int x = 0; x < w; ++x)
        {
            out[0] = half_to_float (one_to_native16 (in0[x]));
            out[1] = half_to_float (one_to_native16 (in1[x]));
            out[2] = half_to_float (one_to_native16 (in2[x]));
            out += 3;
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
        srcbuffer += w * 6; // 3 * sizeof(uint16_t), avoid type conversion
                            /* specialise to memcpy if we can */
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out0) + x) = one_to_native16 (in0[x]);
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out1) + x) = one_to_native16 (in1[x]);
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out2) + x) = one_to_native16 (in2[x]);
#else
        memcpy (out0, in0, (size_t) (w) * sizeof (uint16_t));
        memcpy (out1, in1, (size_t) (w) * sizeof (uint16_t));
        memcpy (out2, in2, (size_t) (w) * sizeof (uint16_t));
#endif
        out0 += linc0;
        out1 += linc1;
        out2 += linc2;
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
unpack_half_to_float_3chan_planar (exr_decode_pipeline_t* decode)
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
        srcbuffer += w * 6; // 3 * sizeof(uint16_t), avoid type conversion
                            /* specialise to memcpy if we can */
        half_to_float_buffer ((float*) out0, in0, w);
        half_to_float_buffer ((float*) out1, in1, w);
        half_to_float_buffer ((float*) out2, in2, w);

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
        srcbuffer += w * 6; // 3 * sizeof(uint16_t), avoid type conversion
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out0 + x * inc0)) = one_to_native16 (in0[x]);
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out1 + x * inc1)) = one_to_native16 (in1[x]);
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out2 + x * inc2)) = one_to_native16 (in2[x]);
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

        srcbuffer += w * 8; // 4 * sizeof(uint16_t), avoid type conversion
        for (int x = 0; x < w; ++x)
        {
            combined.a = one_to_native16 (in0[x]);
            combined.b = one_to_native16 (in1[x]);
            combined.g = one_to_native16 (in2[x]);
            combined.r = one_to_native16 (in3[x]);
            outall[x]  = combined.allc;
        }
        out0 += linc0;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
unpack_half_to_float_4chan_interleave (exr_decode_pipeline_t* decode)
{
    /* we know we're unpacking all the channels and there is no subsampling */
    const uint8_t*  srcbuffer = decode->unpacked_buffer;
    const uint16_t *in0, *in1, *in2, *in3;
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
        float* out = (float*) out0;
        in0        = (const uint16_t*) srcbuffer;
        in1        = in0 + w;
        in2        = in1 + w;
        in3        = in2 + w;

        srcbuffer += w * 8; // 4 * sizeof(uint16_t), avoid type conversion
        for (int x = 0; x < w; ++x)
        {
            out[0] = half_to_float (one_to_native16 (in0[x]));
            out[1] = half_to_float (one_to_native16 (in1[x]));
            out[2] = half_to_float (one_to_native16 (in2[x]));
            out[3] = half_to_float (one_to_native16 (in3[x]));
            out += 4;
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
        srcbuffer += w * 8; // 4 * sizeof(uint16_t), avoid type conversion
                            /* specialize to memcpy if we can */
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out0) + x) = one_to_native16 (in0[x]);
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out1) + x) = one_to_native16 (in1[x]);
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out2) + x) = one_to_native16 (in2[x]);
        for (int x = 0; x < w; ++x)
            *(((uint16_t*) out3) + x) = one_to_native16 (in3[x]);
#else
        memcpy (out0, in0, (size_t) (w) * sizeof (uint16_t));
        memcpy (out1, in1, (size_t) (w) * sizeof (uint16_t));
        memcpy (out2, in2, (size_t) (w) * sizeof (uint16_t));
        memcpy (out3, in3, (size_t) (w) * sizeof (uint16_t));
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
unpack_half_to_float_4chan_planar (exr_decode_pipeline_t* decode)
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
        srcbuffer += w * 8; // 4 * sizeof(uint16_t), avoid type conversion

        half_to_float_buffer ((float*) out0, in0, w);
        half_to_float_buffer ((float*) out1, in1, w);
        half_to_float_buffer ((float*) out2, in2, w);
        half_to_float_buffer ((float*) out3, in3, w);

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
        in0 = (const uint16_t*) srcbuffer;
        in1 = in0 + w;
        in2 = in1 + w;
        in3 = in2 + w;
        srcbuffer += w * 8; // 4 * sizeof(uint16_t), avoid type conversion
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out0 + x * inc0)) = one_to_native16 (in0[x]);
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out1 + x * inc1)) = one_to_native16 (in1[x]);
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out2 + x * inc2)) = one_to_native16 (in2[x]);
        for (int x = 0; x < w; ++x)
            *((uint16_t*) (out3 + x * inc3)) = one_to_native16 (in3[x]);
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
            /* specialize to memcpy if we can */
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
            if (pixincrement == 2)
            {
                uint16_t*       tmp = (uint16_t*) cdata;
                const uint16_t* src = (const uint16_t*) srcbuffer;
                uint16_t*       end = tmp + w;

                while (tmp < end)
                    *tmp++ = one_to_native16 (*src++);
            }
            else
            {
                const uint16_t* src = (const uint16_t*) srcbuffer;
                for (int x = 0; x < w; ++x)
                {
                    *((uint16_t*) cdata) = one_to_native16 (*src++);
                    cdata += pixincrement;
                }
            }
#else
            if (pixincrement == 2)
            {
                memcpy (cdata, srcbuffer, (size_t) (w) *2);
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
            /* specialize to memcpy if we can */
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
                memcpy (cdata, srcbuffer, (size_t) (w) *4);
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

/**************************************/

static exr_result_t
generic_unpack (exr_decode_pipeline_t* decode)
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
                switch (decc->data_type)
                {
                    case EXR_PIXEL_HALF:
                        switch (decc->output_data_type)
                        {
                            case EXR_PIXEL_HALF: {
                                const uint16_t* src =
                                    (const uint16_t*) srcbuffer;
                                for (int x = 0; x < w; ++x)
                                {
                                    *((uint16_t*) cdata) =
                                        one_to_native16 (*src++);
                                    cdata += pixincrement;
                                }
                                break;
                            }
                            case EXR_PIXEL_FLOAT: {
                                const uint16_t* src =
                                    (const uint16_t*) srcbuffer;
                                for (int x = 0; x < w; ++x)
                                {
                                    uint16_t cval = one_to_native16 (*src++);
                                    *((float*) cdata) = half_to_float (cval);
                                    cdata += pixincrement;
                                }
                                break;
                            }
                            case EXR_PIXEL_UINT: {
                                const uint16_t* src =
                                    (const uint16_t*) srcbuffer;
                                for (int x = 0; x < w; ++x)
                                {
                                    uint16_t cval = one_to_native16 (*src++);
                                    *((uint32_t*) cdata) = half_to_uint (cval);
                                    cdata += pixincrement;
                                }
                                break;
                            }
                            default: return EXR_ERR_INVALID_ARGUMENT;
                        }
                        break;
                    case EXR_PIXEL_FLOAT:
                        switch (decc->output_data_type)
                        {
                            case EXR_PIXEL_HALF: {
                                const uint32_t* src =
                                    (const uint32_t*) srcbuffer;
                                for (int x = 0; x < w; ++x)
                                {
                                    uint32_t fint = one_to_native32 (*src++);
                                    *((uint16_t*) cdata) = float_to_half (fint);
                                    cdata += pixincrement;
                                }
                                break;
                            }
                            case EXR_PIXEL_FLOAT: {
                                const uint32_t* src =
                                    (const uint32_t*) srcbuffer;
                                for (int x = 0; x < w; ++x)
                                {
                                    *((uint32_t*) cdata) =
                                        one_to_native32 (*src++);
                                    cdata += pixincrement;
                                }
                                break;
                            }
                            case EXR_PIXEL_UINT: {
                                const uint32_t* src =
                                    (const uint32_t*) srcbuffer;
                                for (int x = 0; x < w; ++x)
                                {
                                    uint32_t fint = one_to_native32 (*src++);
                                    *((uint32_t*) cdata) = float_to_uint (fint);
                                    cdata += pixincrement;
                                }
                                break;
                            }
                            default: return EXR_ERR_INVALID_ARGUMENT;
                        }
                        break;
                    case EXR_PIXEL_UINT:
                        switch (decc->output_data_type)
                        {
                            case EXR_PIXEL_HALF: {
                                const uint32_t* src =
                                    (const uint32_t*) srcbuffer;
                                for (int x = 0; x < w; ++x)
                                {
                                    uint32_t uv = one_to_native32 (*src++);
                                    *((uint16_t*) cdata) = uint_to_half (uv);
                                    cdata += pixincrement;
                                }
                                break;
                            }
                            case EXR_PIXEL_FLOAT: {
                                const uint32_t* src =
                                    (const uint32_t*) srcbuffer;
                                for (int x = 0; x < w; ++x)
                                {
                                    uint32_t uv = one_to_native32 (*src++);
                                    *((float*) cdata) = (float) (uv);
                                    cdata += pixincrement;
                                }
                                break;
                            }
                            case EXR_PIXEL_UINT: {
                                const uint32_t* src =
                                    (const uint32_t*) srcbuffer;
                                for (int x = 0; x < w; ++x)
                                {
                                    *((uint32_t*) cdata) =
                                        one_to_native32 (*src++);
                                    cdata += pixincrement;
                                }
                                break;
                            }
                            default: return EXR_ERR_INVALID_ARGUMENT;
                        }
                        break;
                    default: return EXR_ERR_INVALID_ARGUMENT;
                }
            }
            srcbuffer += w * bpc;
        }
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

internal_exr_unpack_fn
internal_exr_match_decode (
    exr_decode_pipeline_t* decode,
    int                    isdeep,
    int                    chanstofill,
    int                    chanstounpack,
    int                    sametype,
    int                    sameouttype,
    int                    samebpc,
    int                    sameoutbpc,
    int                    hassampling,
    int                    hastypechange,
    int                    sameoutinc,
    int                    simpinterleave,
    int                    simplineoff)
{
    static int init_cpu_check = 1;
    if ( init_cpu_check )
    {
        choose_half_to_float_impl();
        init_cpu_check = 0;
    }
    
    /* TODO */
    if (isdeep) return NULL;

    if (hastypechange > 0)
    {
        /* other optimizations would not be difficult, but this will
         * be the common one (where on encode / pack we want to do the
         * opposite) */
        if (sametype == (int) EXR_PIXEL_HALF &&
            sameouttype == (int) EXR_PIXEL_FLOAT)
        {
            if (simpinterleave > 0)
            {
                if (decode->channel_count == 4)
                    return &unpack_half_to_float_4chan_interleave;
                if (decode->channel_count == 3)
                    return &unpack_half_to_float_3chan_interleave;
            }

            if (sameoutinc == 4)
            {
                if (decode->channel_count == 4)
                    return &unpack_half_to_float_4chan_planar;
                if (decode->channel_count == 3)
                    return &unpack_half_to_float_3chan_planar;
            }
        }

        return &generic_unpack;
    }

    if (hassampling || chanstofill != decode->channel_count || samebpc <= 0 ||
        sameoutbpc <= 0)
        return &generic_unpack;

    (void) chanstounpack;
    (void) simplineoff;

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
