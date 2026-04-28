/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <stdint.h>
#include <string.h>

/* Optimized planar byte-shuffle (Blosc-style) for EXR ZSTD paths. */

static void
shuffle_encode_4_impl (const uint8_t* in, size_t size, uint8_t* out)
{
    size_t         elements = size / 4;
    uint8_t *const p0       = out;
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
shuffle_encode_2_impl (const uint8_t* in, size_t size, uint8_t* out)
{
    size_t         elements = size / 2;
    uint8_t *const p0       = out;
    uint8_t* const p1       = out + elements;
    for (size_t i = 0; i < elements; ++i)
    {
        p0[i] = in[i * 2 + 0];
        p1[i] = in[i * 2 + 1];
    }
}

static void
shuffle_decode_4_impl (const uint8_t* in, size_t size, uint8_t* out)
{
    size_t               elements = size / 4;
    const uint8_t *const p0       = in;
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
shuffle_decode_2_impl (const uint8_t* in, size_t size, uint8_t* out)
{
    size_t               elements = size / 2;
    const uint8_t *const p0       = in;
    const uint8_t* const p1       = in + elements;
    for (size_t i = 0; i < elements; ++i)
    {
        out[i * 2 + 0] = p0[i];
        out[i * 2 + 1] = p1[i];
    }
}

/*
// AVX2 (reference / optional SIMD path; not compiled)
__attribute__((target("avx2")))
static inline void shuffle_decode_4(const uint8_t* in, size_t size, uint8_t* out) {
    size_t elements = size / 4;
    const uint8_t *p0 = in;
    const uint8_t *p1 = in + elements;
    const uint8_t *p2 = in + elements * 2;
    const uint8_t *p3 = in + elements * 3;
    size_t i = 0;

    if (elements >= 32) {
        for (; i <= elements - 32; i += 32) {
            __m256i v0 = _mm256_loadu_si256((const __m256i*)(p0 + i));
            __m256i v1 = _mm256_loadu_si256((const __m256i*)(p1 + i));
            __m256i v2 = _mm256_loadu_si256((const __m256i*)(p2 + i));
            __m256i v3 = _mm256_loadu_si256((const __m256i*)(p3 + i));

            // Stage 1: Interleave 0-1 and 2-3
            __m256i v01_lo = _mm256_unpacklo_epi8(v0, v1);
            __m256i v01_hi = _mm256_unpackhi_epi8(v0, v1);
            __m256i v23_lo = _mm256_unpacklo_epi8(v2, v3);
            __m256i v23_hi = _mm256_unpackhi_epi8(v2, v3);

            // Stage 2: Interleave the results (unpack 16-bit to 32-bit)
            __m256i res0 = _mm256_unpacklo_epi16(v01_lo, v23_lo);
            __m256i res1 = _mm256_unpackhi_epi16(v01_lo, v23_lo);
            __m256i res2 = _mm256_unpacklo_epi16(v01_hi, v23_hi);
            __m256i res3 = _mm256_unpackhi_epi16(v01_hi, v23_hi);

            // Permute to fix the cross-lane order
            __m256i out0 = _mm256_permute2x128_si256(res0, res1, 0x20); // Elements 0-7
            __m256i out1 = _mm256_permute2x128_si256(res0, res1, 0x31); // Elements 16-23
            __m256i out2 = _mm256_permute2x128_si256(res2, res3, 0x20); // Elements 8-15
            __m256i out3 = _mm256_permute2x128_si256(res2, res3, 0x31); // Elements 24-31

            // CORRECTED STORE ORDER: out0 -> out2 -> out1 -> out3
            _mm256_storeu_si256((__m256i*)(out + i * 4),       out0);
            _mm256_storeu_si256((__m256i*)(out + i * 4 + 32),  out2);
            _mm256_storeu_si256((__m256i*)(out + i * 4 + 64),  out1);
            _mm256_storeu_si256((__m256i*)(out + i * 4 + 96),  out3);
        }
    }

    // Scalar fallback
    for (; i < elements; ++i) {
        out[i * 4 + 0] = p0[i]; out[i * 4 + 1] = p1[i];
        out[i * 4 + 2] = p2[i]; out[i * 4 + 3] = p3[i];
    }
}

__attribute__((target("avx2")))
static inline void shuffle_decode_2(const uint8_t* in, size_t size, uint8_t* out) {
    size_t elements = size / 2;
    const uint8_t *p0 = in;
    const uint8_t *p1 = in + elements;
    size_t i = 0;

    if (elements >= 32) {
        for (; i <= elements - 32; i += 32) {
            __m256i v0 = _mm256_loadu_si256((const __m256i*)(p0 + i));
            __m256i v1 = _mm256_loadu_si256((const __m256i*)(p1 + i));

            // Interleave bytes (results in lane-interleaved data)
            __m256i lo = _mm256_unpacklo_epi8(v0, v1);
            __m256i hi = _mm256_unpackhi_epi8(v0, v1);

            // Fix the AVX2 lane issue to make it linear
            // We want [lo_lane0, hi_lane0, lo_lane1, hi_lane1]
            __m256i out0 = _mm256_permute2x128_si256(lo, hi, 0x20);
            __m256i out1 = _mm256_permute2x128_si256(lo, hi, 0x31);

            _mm256_storeu_si256((__m256i*)(out + i * 2),      out0);
            _mm256_storeu_si256((__m256i*)(out + i * 2 + 32), out1);
        }
    }

    // Scalar fallback
    for (; i < elements; ++i) {
        out[i * 2 + 0] = p0[i];
        out[i * 2 + 1] = p1[i];
    }
}
*/
/*
// SSE (reference / optional SIMD path; not compiled)
static inline void shuffle_decode_2(const uint8_t* in, size_t size, uint8_t* out) {
    size_t elements = size / 2;
    const uint8_t *p0 = in;
    const uint8_t *p1 = in + elements;

    size_t i = 0;

    // Process 16 elements (32 bytes) at a time
    if (elements >= 16) {
        for (; i <= elements - 16; i += 16) {
            // Load 16 bytes from p0 and 16 bytes from p1
            __m128i v0 = _mm_loadu_si128((const __m128i*)(p0 + i));
            __m128i v1 = _mm_loadu_si128((const __m128i*)(p1 + i));

            // Interleave (Unpack) the bytes:
            // res_lo: p0[0], p1[0], p0[1], p1[1] ... p0[7], p1[7]
            // res_hi: p0[8], p1[8], p0[9], p1[9] ... p0[15], p1[15]
            __m128i res_lo = _mm_unpacklo_epi8(v0, v1);
            __m128i res_hi = _mm_unpackhi_epi8(v0, v1);

            // Store the interleaved result back to out
            _mm_storeu_si128((__m128i*)(out + i * 2), res_lo);
            _mm_storeu_si128((__m128i*)(out + i * 2 + 16), res_hi);
        }
    }

    // Clean up remaining elements
    for (; i < elements; ++i) {
        out[i * 2 + 0] = p0[i];
        out[i * 2 + 1] = p1[i];
    }
}
*/

void
exr_zstd_shuffle_encode_4 (const uint8_t* in, size_t size, uint8_t* out)
{
    shuffle_encode_4_impl (in, size, out);
}

void
exr_zstd_shuffle_encode_2 (const uint8_t* in, size_t size, uint8_t* out)
{
    shuffle_encode_2_impl (in, size, out);
}

/** Unshuffle planar layout; \a shuffle_el_bytes must be 4 or 2 (EXR ZSTD inner segment). */
void
exr_zstd_shuffle_decode_bytes (
    uint8_t* dst, const uint8_t* shuf, size_t dSize, uint64_t shuffle_el_bytes)
{
    if (shuffle_el_bytes == 4)
        shuffle_decode_4_impl (shuf, dSize, dst);
    else
        shuffle_decode_2_impl (shuf, dSize, dst);
}
