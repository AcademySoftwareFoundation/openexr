/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <stdint.h>
#include <string.h>

/* Optimized planar byte-shuffle (Blosc-style) for EXR ZSTD paths. */

static void
shuffle_encode_4_scalar (const uint8_t* in, size_t size, uint8_t* out)
{
    size_t         elements = size / 4;
    uint8_t* const p0       = out;
    uint8_t* const p1       = out + elements;
    uint8_t* const p2       = out + elements * 2;
    uint8_t* const p3       = out + elements * 3;
    for (size_t i = 0; i < elements; ++i)
    {
        p0[i] = in[i * 4 + 0];
        p1[i] = in[i * 4 + 1];
        p2[i] = in[i * 4 + 2];
        p3[i] = in[i * 4 + 3];
    }
}

static void
shuffle_encode_2_scalar (const uint8_t* in, size_t size, uint8_t* out)
{
    size_t         elements = size / 2;
    uint8_t* const p0       = out;
    uint8_t* const p1       = out + elements;
    for (size_t i = 0; i < elements; ++i)
    {
        p0[i] = in[i * 2 + 0];
        p1[i] = in[i * 2 + 1];
    }
}

static void
shuffle_decode_4_scalar (const uint8_t* in, size_t size, uint8_t* out)
{
    size_t               elements = size / 4;
    const uint8_t* const p0       = in;
    const uint8_t* const p1       = in + elements;
    const uint8_t* const p2       = in + elements * 2;
    const uint8_t* const p3       = in + elements * 3;
    for (size_t i = 0; i < elements; ++i)
    {
        out[i * 4 + 0] = p0[i];
        out[i * 4 + 1] = p1[i];
        out[i * 4 + 2] = p2[i];
        out[i * 4 + 3] = p3[i];
    }
}

static void
shuffle_decode_2_scalar (const uint8_t* in, size_t size, uint8_t* out)
{
    size_t               elements = size / 2;
    const uint8_t* const p0       = in;
    const uint8_t* const p1       = in + elements;
    for (size_t i = 0; i < elements; ++i)
    {
        out[i * 2 + 0] = p0[i];
        out[i * 2 + 1] = p1[i];
    }
}

/* ========================================================================= */
/* GCC / Linux x86_64 SIMD                         */
/* ========================================================================= */

#if defined(__GNUC__) && defined(__x86_64__)

#    include <immintrin.h>

__attribute__ ((target ("avx2"))) static void
shuffle_decode_4_avx2 (const uint8_t* in, size_t size, uint8_t* out)
{
    size_t         elements = size / 4;
    const uint8_t* p0       = in;
    const uint8_t* p1       = in + elements;
    const uint8_t* p2       = in + elements * 2;
    const uint8_t* p3       = in + elements * 3;
    size_t         i        = 0;

    if (elements >= 32)
    {
        for (; i <= elements - 32; i += 32)
        {
            __m256i v0 = _mm256_loadu_si256 ((const __m256i*) (p0 + i));
            __m256i v1 = _mm256_loadu_si256 ((const __m256i*) (p1 + i));
            __m256i v2 = _mm256_loadu_si256 ((const __m256i*) (p2 + i));
            __m256i v3 = _mm256_loadu_si256 ((const __m256i*) (p3 + i));

            __m256i v01_lo = _mm256_unpacklo_epi8 (v0, v1);
            __m256i v01_hi = _mm256_unpackhi_epi8 (v0, v1);
            __m256i v23_lo = _mm256_unpacklo_epi8 (v2, v3);
            __m256i v23_hi = _mm256_unpackhi_epi8 (v2, v3);

            __m256i res0 = _mm256_unpacklo_epi16 (v01_lo, v23_lo);
            __m256i res1 = _mm256_unpackhi_epi16 (v01_lo, v23_lo);
            __m256i res2 = _mm256_unpacklo_epi16 (v01_hi, v23_hi);
            __m256i res3 = _mm256_unpackhi_epi16 (v01_hi, v23_hi);

            __m256i out0 = _mm256_permute2x128_si256 (res0, res1, 0x20);
            __m256i out1 = _mm256_permute2x128_si256 (res0, res1, 0x31);
            __m256i out2 = _mm256_permute2x128_si256 (res2, res3, 0x20);
            __m256i out3 = _mm256_permute2x128_si256 (res2, res3, 0x31);

            _mm256_storeu_si256 ((__m256i*) (out + i * 4), out0);
            _mm256_storeu_si256 ((__m256i*) (out + i * 4 + 32), out2);
            _mm256_storeu_si256 ((__m256i*) (out + i * 4 + 64), out1);
            _mm256_storeu_si256 ((__m256i*) (out + i * 4 + 96), out3);
        }
    }

    for (; i < elements; ++i)
    {
        out[i * 4 + 0] = p0[i];
        out[i * 4 + 1] = p1[i];
        out[i * 4 + 2] = p2[i];
        out[i * 4 + 3] = p3[i];
    }
}

__attribute__ ((target ("avx2"))) static void
shuffle_decode_2_avx2 (const uint8_t* in, size_t size, uint8_t* out)
{
    size_t         elements = size / 2;
    const uint8_t* p0       = in;
    const uint8_t* p1       = in + elements;
    size_t         i        = 0;

    if (elements >= 32)
    {
        for (; i <= elements - 32; i += 32)
        {
            __m256i v0 = _mm256_loadu_si256 ((const __m256i*) (p0 + i));
            __m256i v1 = _mm256_loadu_si256 ((const __m256i*) (p1 + i));

            __m256i lo = _mm256_unpacklo_epi8 (v0, v1);
            __m256i hi = _mm256_unpackhi_epi8 (v0, v1);

            __m256i out0 = _mm256_permute2x128_si256 (lo, hi, 0x20);
            __m256i out1 = _mm256_permute2x128_si256 (lo, hi, 0x31);

            _mm256_storeu_si256 ((__m256i*) (out + i * 2), out0);
            _mm256_storeu_si256 ((__m256i*) (out + i * 2 + 32), out1);
        }
    }

    for (; i < elements; ++i)
    {
        out[i * 2 + 0] = p0[i];
        out[i * 2 + 1] = p1[i];
    }
}

/*
__attribute__((target("avx512f,avx512bw,avx512dq,avx512vl")))
static void shuffle_decode_4_avx512(const uint8_t* in, size_t size, uint8_t* out) {
    size_t elements = size / 4;
    const uint8_t *p0 = in;
    const uint8_t *p1 = in + elements;
    const uint8_t *p2 = in + elements * 2;
    const uint8_t *p3 = in + elements * 3;
    size_t i = 0;

    if (elements >= 64) {
        for (; i <= elements - 64; i += 64) {
            __m512i b0 = _mm512_loadu_si512((const __m512i*)(p0 + i));
            __m512i b1 = _mm512_loadu_si512((const __m512i*)(p1 + i));
            __m512i b2 = _mm512_loadu_si512((const __m512i*)(p2 + i));
            __m512i b3 = _mm512_loadu_si512((const __m512i*)(p3 + i));

            // Stage 1: Interleave bytes to 16-bit (Lane-local)
            __m512i b01_lo = _mm512_unpacklo_epi8(b0, b1);
            __m512i b01_hi = _mm512_unpackhi_epi8(b0, b1);
            __m512i b23_lo = _mm512_unpacklo_epi8(b2, b3);
            __m512i b23_hi = _mm512_unpackhi_epi8(b2, b3);

            // Stage 2: Interleave 16-bit to 32-bit pixels (Lane-local)
            __m512i r0_l = _mm512_unpacklo_epi16(b01_lo, b23_lo);
            __m512i r0_h = _mm512_unpackhi_epi16(b01_lo, b23_lo);
            __m512i r1_l = _mm512_unpacklo_epi16(b01_hi, b23_hi);
            __m512i r1_h = _mm512_unpackhi_epi16(b01_hi, b23_hi);

            // Stage 3: 4x4 Transpose of the 128-bit lanes to make the output linear
            __m512i t0 = _mm512_shuffle_i32x4(r0_l, r0_h, _MM_SHUFFLE(1, 0, 1, 0));
            __m512i t1 = _mm512_shuffle_i32x4(r1_l, r1_h, _MM_SHUFFLE(1, 0, 1, 0));
            __m512i t2 = _mm512_shuffle_i32x4(r0_l, r0_h, _MM_SHUFFLE(3, 2, 3, 2));
            __m512i t3 = _mm512_shuffle_i32x4(r1_l, r1_h, _MM_SHUFFLE(3, 2, 3, 2));

            __m512i out0 = _mm512_shuffle_i32x4(t0, t1, _MM_SHUFFLE(2, 0, 2, 0));
            __m512i out1 = _mm512_shuffle_i32x4(t0, t1, _MM_SHUFFLE(3, 1, 3, 1));
            __m512i out2 = _mm512_shuffle_i32x4(t2, t3, _MM_SHUFFLE(2, 0, 2, 0));
            __m512i out3 = _mm512_shuffle_i32x4(t2, t3, _MM_SHUFFLE(3, 1, 3, 1));

            // Store perfectly linear 32-bit pixels
            _mm512_storeu_si512((__m512i*)(out + i * 4),       out0);
            _mm512_storeu_si512((__m512i*)(out + i * 4 + 64),  out1);
            _mm512_storeu_si512((__m512i*)(out + i * 4 + 128), out2);
            _mm512_storeu_si512((__m512i*)(out + i * 4 + 192), out3);
        }
    }

    // Scalar fallback for the remainder
    for (; i < elements; ++i) {
        out[i * 4 + 0] = p0[i]; out[i * 4 + 1] = p1[i];
        out[i * 4 + 2] = p2[i]; out[i * 4 + 3] = p3[i];
    }
}

__attribute__((target("avx512f,avx512bw,avx512dq,avx512vl,avx512vbmi")))
static void shuffle_decode_2_avx512(const uint8_t* in, size_t size, uint8_t* out) {
    size_t elements = size / 2;
    const uint8_t *p0 = in;
    const uint8_t *p1 = in + elements;
    size_t i = 0;

    if (elements >= 64) {
        __attribute__((aligned(64))) uint8_t mask0_31_raw[64];
        __attribute__((aligned(64))) uint8_t mask32_63_raw[64];
        for (int j = 0; j < 32; ++j) {
            mask0_31_raw[j*2]     = j;
            mask0_31_raw[j*2 + 1] = j + 64;
            mask32_63_raw[j*2]     = j + 32;
            mask32_63_raw[j*2 + 1] = j + 32 + 64;
        }

        __m512i m0 = _mm512_load_si512((const __m512i*)mask0_31_raw);
        __m512i m1 = _mm512_load_si512((const __m512i*)mask32_63_raw);

        for (; i <= elements - 64; i += 64) {
            __m512i b0 = _mm512_loadu_si512((const __m512i*)(p0 + i));
            __m512i b1 = _mm512_loadu_si512((const __m512i*)(p1 + i));

            // FIXED: Using permuteX2var instead of permi2var
            __m512i out0 = _mm512_permutex2var_epi8(b0, m0, b1);
            __m512i out1 = _mm512_permutex2var_epi8(b0, m1, b1);

            _mm512_storeu_si512((__m512i*)(out + i * 2),      out0);
            _mm512_storeu_si512((__m512i*)(out + i * 2 + 64), out1);
        }
    }

    for (; i < elements; ++i) {
        out[i * 2 + 0] = p0[i];
        out[i * 2 + 1] = p1[i];
    }
}
*/
#endif

/* ========================================================================= */
/* Client-Facing API                                */
/* ========================================================================= */

void
exr_zstd_shuffle_encode_4 (const uint8_t* in, size_t size, uint8_t* out)
{
    shuffle_encode_4_scalar (in, size, out);
}

void
exr_zstd_shuffle_encode_2 (const uint8_t* in, size_t size, uint8_t* out)
{
    shuffle_encode_2_scalar (in, size, out);
}

/** Unshuffle planar layout; \a shuffle_el_bytes must be 4 or 2 (EXR ZSTD inner segment). */
void
exr_zstd_shuffle_decode_bytes (
    uint8_t* dst, const uint8_t* shuf, size_t dSize, uint64_t shuffle_el_bytes)
{
#if defined(__GNUC__) && defined(__x86_64__)
    __builtin_cpu_init ();

    if (shuffle_el_bytes == 4)
    {
        /* AVX512 BW & DQ are required for the unpack+permutex2var logic */
        /*if (__builtin_cpu_supports("avx512bw") && __builtin_cpu_supports("avx512dq")) {
            shuffle_decode_4_avx512(shuf, dSize, dst);
        } else */
        if (__builtin_cpu_supports ("avx2"))
        {
            shuffle_decode_4_avx2 (shuf, dSize, dst);
        }
        else { shuffle_decode_4_scalar (shuf, dSize, dst); }
    }
    else
    {
        /* AVX512 VBMI is required for the vpermi2b logic */
        /*if (__builtin_cpu_supports("avx512vbmi")) {
            shuffle_decode_2_avx512(shuf, dSize, dst);
        } else */
        if (__builtin_cpu_supports ("avx2"))
        {
            shuffle_decode_2_avx2 (shuf, dSize, dst);
        }
        else { shuffle_decode_2_scalar (shuf, dSize, dst); }
    }
#else
    /* Safe fallback for ARM, Windows/MSVC, or non-x86 compilation */
    if (shuffle_el_bytes == 4)
        shuffle_decode_4_scalar (shuf, dSize, dst);
    else
        shuffle_decode_2_scalar (shuf, dSize, dst);
#endif
}