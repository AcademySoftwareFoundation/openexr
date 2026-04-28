#include <stdint.h>
#include <string.h>

/* ========================================================================= */
/* SCALAR IMPLEMENTATIONS                                                    */
/* ========================================================================= */

static void delta_encode_row_u16_scalar(uint8_t* p, uint64_t n) {
    if (n <= 1) return;
    for (uint64_t k = n - 1; k > 0; --k) {
        uint16_t a, b;
        memcpy(&a, p + k * 2, 2);
        memcpy(&b, p + (k - 1) * 2, 2);
        uint16_t d = (uint16_t)((unsigned)a - (unsigned)b);
        memcpy(p + k * 2, &d, 2);
    }
}

static void delta_decode_row_u16_scalar(uint8_t* p, uint64_t n) {
    for (uint64_t k = 1; k < n; ++k) {
        uint16_t a, b;
        memcpy(&a, p + k * 2, 2);
        memcpy(&b, p + (k - 1) * 2, 2);
        uint16_t s = (uint16_t)((unsigned)a + (unsigned)b);
        memcpy(p + k * 2, &s, 2);
    }
}

static void delta_encode_row_u32_scalar(uint8_t* p, uint64_t n) {
    if (n <= 1) return;
    for (uint64_t k = n - 1; k > 0; --k) {
        uint32_t a, b;
        memcpy(&a, p + k * 4, 4);
        memcpy(&b, p + (k - 1) * 4, 4);
        uint32_t d = a - b;
        memcpy(p + k * 4, &d, 4);
    }
}

static void delta_decode_row_u32_scalar(uint8_t* p, uint64_t n) {
    for (uint64_t k = 1; k < n; ++k) {
        uint32_t a, b;
        memcpy(&a, p + k * 4, 4);
        memcpy(&b, p + (k - 1) * 4, 4);
        uint32_t s = a + b;
        memcpy(p + k * 4, &s, 4);
    }
}

/* ========================================================================= */
/* AVX2 IMPLEMENTATIONS                                                      */
/* ========================================================================= */

#if defined(__GNUC__) && defined(__x86_64__)
#include <immintrin.h>

/*
__attribute__((target("avx2")))
static void delta_encode_row_u16_avx2(uint8_t* p, uint64_t n) {
    if (n <= 1) return;
    uint64_t i = n;

    for (; i >= 16 + 1; i -= 16) {
        __m256i curr = _mm256_loadu_si256((const __m256i*)(p + (i - 16) * 2));
        __m256i prev = _mm256_loadu_si256((const __m256i*)(p + (i - 17) * 2));
        __m256i delta = _mm256_sub_epi16(curr, prev);
        _mm256_storeu_si256((__m256i*)(p + (i - 16) * 2), delta);
    }

    for (uint64_t k = i - 1; k > 0; --k) {
        uint16_t a, b;
        memcpy(&a, p + k * 2, 2);
        memcpy(&b, p + (k - 1) * 2, 2);
        uint16_t d = (uint16_t)((unsigned)a - (unsigned)b);
        memcpy(p + k * 2, &d, 2);
    }
}*/

__attribute__((target("avx2")))
static void delta_decode_row_u16_avx2(uint8_t* p, uint64_t n) {
    if (n <= 1) return;
    uint64_t i = 1;
    uint16_t prev_sum;
    memcpy(&prev_sum, p, 2);

    __m256i bcast_mask = _mm256_set_epi8(
        15, 14, 15, 14, 15, 14, 15, 14, 15, 14, 15, 14, 15, 14, 15, 14,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    );

    for (; i + 16 <= n; i += 16) {
        __m256i x = _mm256_loadu_si256((const __m256i*)(p + i * 2));
        x = _mm256_add_epi16(x, _mm256_slli_si256(x, 2));
        x = _mm256_add_epi16(x, _mm256_slli_si256(x, 4));
        x = _mm256_add_epi16(x, _mm256_slli_si256(x, 8));

        __m256i lane_shift = _mm256_permute2x128_si256(x, x, 0x08);
        lane_shift = _mm256_shuffle_epi8(lane_shift, bcast_mask);
        x = _mm256_add_epi16(x, lane_shift);

        x = _mm256_add_epi16(x, _mm256_set1_epi16(prev_sum));
        _mm256_storeu_si256((__m256i*)(p + i * 2), x);

        __m128i hi_lane = _mm256_extracti128_si256(x, 1);
        prev_sum = (uint16_t)_mm_extract_epi16(hi_lane, 7);
    }

    for (uint64_t k = i; k < n; ++k) {
        uint16_t a, b;
        memcpy(&a, p + k * 2, 2);
        memcpy(&b, p + (k - 1) * 2, 2);
        uint16_t s = (uint16_t)((unsigned)a + (unsigned)b);
        memcpy(p + k * 2, &s, 2);
    }
}

/*
__attribute__((target("avx2")))
static void delta_encode_row_u32_avx2(uint8_t* p, uint64_t n) {
    if (n <= 1) return;
    uint64_t i = n;

    for (; i >= 8 + 1; i -= 8) {
        __m256i curr = _mm256_loadu_si256((const __m256i*)(p + (i - 8) * 4));
        __m256i prev = _mm256_loadu_si256((const __m256i*)(p + (i - 9) * 4));
        __m256i delta = _mm256_sub_epi32(curr, prev);
        _mm256_storeu_si256((__m256i*)(p + (i - 8) * 4), delta);
    }

    for (uint64_t k = i - 1; k > 0; --k) {
        uint32_t a, b;
        memcpy(&a, p + k * 4, 4);
        memcpy(&b, p + (k - 1) * 4, 4);
        uint32_t d = a - b;
        memcpy(p + k * 4, &d, 4);
    }
}*/

__attribute__((target("avx2")))
static void delta_decode_row_u32_avx2(uint8_t* p, uint64_t n) {
    if (n <= 1) return;
    uint64_t i = 1;
    uint32_t prev_sum;
    memcpy(&prev_sum, p, 4);

    for (; i + 8 <= n; i += 8) {
        __m256i x = _mm256_loadu_si256((const __m256i*)(p + i * 4));
        x = _mm256_add_epi32(x, _mm256_slli_si256(x, 4));
        x = _mm256_add_epi32(x, _mm256_slli_si256(x, 8));

        __m256i lane_shift = _mm256_permute2x128_si256(x, x, 0x08);
        lane_shift = _mm256_shuffle_epi32(lane_shift, _MM_SHUFFLE(3, 3, 3, 3));
        x = _mm256_add_epi32(x, lane_shift);

        x = _mm256_add_epi32(x, _mm256_set1_epi32(prev_sum));
        _mm256_storeu_si256((__m256i*)(p + i * 4), x);

        __m128i hi_lane = _mm256_extracti128_si256(x, 1);
        prev_sum = (uint32_t)_mm_extract_epi32(hi_lane, 3);
    }

    for (uint64_t k = i; k < n; ++k) {
        uint32_t a, b;
        memcpy(&a, p + k * 4, 4);
        memcpy(&b, p + (k - 1) * 4, 4);
        uint32_t s = a + b;
        memcpy(p + k * 4, &s, 4);
    }
}

#endif

/* ========================================================================= */
/* CLIENT-FACING DISPATCH API                                                */
/* ========================================================================= */

void delta_encode_row_u16(uint8_t* p, uint64_t n) {
/*#if defined(__GNUC__) && defined(__x86_64__)
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx2")) {
        delta_encode_row_u16_avx2(p, n);
        return;
    }
#endif*/
    delta_encode_row_u16_scalar(p, n);
}

void delta_decode_row_u16(uint8_t* p, uint64_t n) {
#if defined(__GNUC__) && defined(__x86_64__)
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx2")) {
        delta_decode_row_u16_avx2(p, n);
        return;
    }
#endif
    delta_decode_row_u16_scalar(p, n);
}

void delta_encode_row_u32(uint8_t* p, uint64_t n) {
/*#if defined(__GNUC__) && defined(__x86_64__)
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx2")) {
        delta_encode_row_u32_avx2(p, n);
        return;
    }
#endif*/
    delta_encode_row_u32_scalar(p, n);
}

void delta_decode_row_u32(uint8_t* p, uint64_t n) {
#if defined(__GNUC__) && defined(__x86_64__)
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx2")) {
        delta_decode_row_u32_avx2(p, n);
        return;
    }
#endif
    delta_decode_row_u32_scalar(p, n);
}