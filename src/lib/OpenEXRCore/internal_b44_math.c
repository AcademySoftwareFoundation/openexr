/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <stdint.h>
#include <half.h>

/**************************************/

#if defined(__SSE2__) || (defined(_MSC_VER) && defined(_M_X64))
#    define IMF_HAVE_SSE2 1
#    include <immintrin.h>
#endif

// Only use AVX2 code path when F16C is also present.
// This is true for all current CPUs, but gcc/clang still need separate
// command line flags to enable both, or using a meta-flag like
// -march=x86-64-v3. On MSVC there's no __F16C__; it is always available
// with /arch:AVX2.
#if defined(__AVX2__) && (defined(__F16C__) || defined(_MSC_VER))
#    define IMF_HAVE_AVX2 1
#    include <immintrin.h>
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#    define IMF_HAVE_NEON_AARCH64 1
#    include <arm_neon.h>
#endif

/**************************************/

// log() and exp() implementations based on math used in Highway SIMD library
// https://github.com/google/highway/blob/1.3.0/hwy/contrib/math/math-inl.h

static inline float
muladd (float a, float b, float c)
{
    return a * b + c;
}

static inline float
mulsub (float a, float b, float c)
{
    return a * b - c;
}

typedef union FP32
{
    int32_t  i;
    uint32_t u;
    float    f;
} FP32;

static const float kOneOverLog2  = 1.4426950f;
static const float kExpLn2Part0f = -0.69314575f;
static const float kExpLn2Part1f = -1.42860676e-6f;
static const float kExpPoly0     = 0.5f;
static const float kExpPoly1     = 0.16666666f;
static const float kExpPoly2     = 0.041666485f;
static const float kExpPoly3     = 0.0083333607f;
static const float kExpPoly4     = 0.0013930435f;

static inline float
pow2_int (int32_t x)
{
    FP32 r;
    r.i = (x + 0x7F) << 23;
    return r.f;
}

// Basic idea is that exp2(x) = exp2(integer + fraction) = exp2(integer) * exp2(fraction);
// the integer part is done by putting it into float exponent bits; and the fractional
// part is done with a polynomial. There's some adjustment since we need exp(), not exp2().
static inline float
exp_scalar (float x)
{
    // reduce
    float   xlg2 = x * kOneOverLog2;
    int32_t q    = (int) (xlg2 + (x < 0.0f ? -0.5f : +0.5f));
    float   qf   = (float) q;
    x            = muladd (qf, kExpLn2Part0f, x);
    x            = muladd (qf, kExpLn2Part1f, x);

    // polynomial approximation
    float x2 = x * x;
    float x4 = x2 * x2;
    float f1 = muladd (
        muladd (
            x4,
            kExpPoly4,
            muladd (
                x2,
                muladd (kExpPoly3, x, kExpPoly2),
                muladd (kExpPoly1, x, kExpPoly0))),
        x2,
        x);

    // reconstruct
    float   f2 = f1 + 1.0f;
    int32_t y  = q >> 1;
    float   f3 = (f2 * pow2_int (y)) * pow2_int (q - y);

    return f3;
}

static const float   kLogLn2Hi   = 0.69313812256f;
static const float   kLogLn2Lo   = 9.0580006145e-6f;
static const float   kLogPoly0   = 0.66666663f;
static const float   kLogPoly1   = 0.40000972f;
static const float   kLogPoly2   = 0.28498787f;
static const float   kLogPoly3   = 0.24279079f;
static const int32_t kLogMagic   = 0x3F3504F3;
static const int32_t kLogExpMask = 0x3F800000;
static const int32_t kLogManMask = 0x007FFFFF;

static inline float
log_scalar (float x)
{
    FP32 bits;

    // reduce x into [sqrt(2)/2, sqrt(2)]
    bits.f = x;
    bits.i += kLogExpMask - kLogMagic;
    float exp = (float) ((int) (bits.u >> 23) - 0x7f);
    bits.i    = (bits.i & kLogManMask) + kLogMagic;
    x         = bits.f;

    // approximate
    float ym1 = x - 1.0f;
    float z   = ym1 / (x + 1.0f);
    float z2  = z * z;
    float z4  = z2 * z2;
    float zp  = muladd (
        muladd (kLogPoly2, z4, kLogPoly0),
        z2,
        muladd (kLogPoly3, z4, kLogPoly1) * z4);

    // reconstruct
    return mulsub (exp, kLogLn2Hi, mulsub (z, ym1 - zp, exp * kLogLn2Lo) - ym1);
}

/**************************************/

// 4x SIMD SSE2 log and exp implementations, with the same math as above

#ifdef IMF_HAVE_SSE2

static inline __m128
muladd_sse (__m128 a, __m128 b, __m128 c)
{
    return _mm_add_ps (_mm_mul_ps (a, b), c);
}

static inline __m128
mulsub_sse (__m128 a, __m128 b, __m128 c)
{
    return _mm_sub_ps (_mm_mul_ps (a, b), c);
}

static inline __m128
pow2_int_sse (__m128i x)
{
    __m128i r = _mm_add_epi32 (x, _mm_set1_epi32 (0x7F));
    r         = _mm_slli_epi32 (r, 23);
    return _mm_castsi128_ps (r);
}

static inline __m128
exp_sse (__m128 x)
{
    const __m128 kOneOverLog2_sse  = _mm_set1_ps (kOneOverLog2);
    const __m128 kExpLn2Part0f_sse = _mm_set1_ps (kExpLn2Part0f);
    const __m128 kExpLn2Part1f_sse = _mm_set1_ps (kExpLn2Part1f);
    const __m128 kExpPoly0_sse     = _mm_set1_ps (kExpPoly0);
    const __m128 kExpPoly1_sse     = _mm_set1_ps (kExpPoly1);
    const __m128 kExpPoly2_sse     = _mm_set1_ps (kExpPoly2);
    const __m128 kExpPoly3_sse     = _mm_set1_ps (kExpPoly3);
    const __m128 kExpPoly4_sse     = _mm_set1_ps (kExpPoly4);

    // reduce
    __m128  xlg2 = _mm_mul_ps (x, kOneOverLog2_sse);
    __m128i q    = _mm_cvtps_epi32 (xlg2);
    __m128  qf   = _mm_cvtepi32_ps (q);
    x            = muladd_sse (qf, kExpLn2Part0f_sse, x);
    x            = muladd_sse (qf, kExpLn2Part1f_sse, x);

    // polynomial approximation
    __m128 x2 = _mm_mul_ps (x, x);
    __m128 x4 = _mm_mul_ps (x2, x2);
    __m128 f1 = muladd_sse (
        muladd_sse (
            x4,
            kExpPoly4_sse,
            muladd_sse (
                x2,
                muladd_sse (kExpPoly3_sse, x, kExpPoly2_sse),
                muladd_sse (kExpPoly1_sse, x, kExpPoly0_sse))),
        x2,
        x);

    // reconstruct
    __m128       f2  = _mm_add_ps (f1, _mm_set1_ps (1.0f));
    __m128i      y   = _mm_srai_epi32 (q, 1);
    const __m128 py  = pow2_int_sse (y);
    const __m128 pey = pow2_int_sse (_mm_sub_epi32 (q, y));
    __m128       f3  = _mm_mul_ps (_mm_mul_ps (f2, py), pey);
    return f3;
}

static inline __m128
log_sse (__m128 x)
{
    const __m128  kLogLn2Hi_sse = _mm_set1_ps (kLogLn2Hi);
    const __m128  kLogLn2Lo_sse = _mm_set1_ps (kLogLn2Lo);
    const __m128  kLogPoly0_sse = _mm_set1_ps (kLogPoly0);
    const __m128  kLogPoly1_sse = _mm_set1_ps (kLogPoly1);
    const __m128  kLogPoly2_sse = _mm_set1_ps (kLogPoly2);
    const __m128  kLogPoly3_sse = _mm_set1_ps (kLogPoly3);
    const __m128i kLogMagic_sse = _mm_set1_epi32 (kLogMagic);
    const __m128i kLogExpMagicMask_sse =
        _mm_set1_epi32 (kLogExpMask - kLogMagic);
    const __m128i kLogManMask_sse = _mm_set1_epi32 (kLogManMask);

    // reduce x into [sqrt(2)/2, sqrt(2)]
    __m128i bits = _mm_castps_si128 (x);
    bits         = _mm_add_epi32 (bits, kLogExpMagicMask_sse);
    __m128i expu = _mm_srli_epi32 (bits, 23);
    __m128  exp = _mm_cvtepi32_ps (_mm_sub_epi32 (expu, _mm_set1_epi32 (0x7f)));
    bits = _mm_add_epi32 (_mm_and_si128 (bits, kLogManMask_sse), kLogMagic_sse);
    x    = _mm_castsi128_ps (bits);

    // approximate
    const __m128 kOne = _mm_set1_ps (1.0f);
    __m128       ym1  = _mm_sub_ps (x, kOne);
    __m128       z    = _mm_div_ps (ym1, _mm_add_ps (x, kOne));
    __m128       z2   = _mm_mul_ps (z, z);
    __m128       z4   = _mm_mul_ps (z2, z2);
    __m128       zp   = muladd_sse (
        muladd_sse (kLogPoly2_sse, z4, kLogPoly0_sse),
        z2,
        _mm_mul_ps (muladd_sse (kLogPoly3_sse, z4, kLogPoly1_sse), z4));

    // reconstruct
    return mulsub_sse (
        exp,
        kLogLn2Hi_sse,
        _mm_sub_ps (
            mulsub_sse (
                z, _mm_sub_ps (ym1, zp), _mm_mul_ps (exp, kLogLn2Lo_sse)),
            ym1));
}

// SSE2 float<->half conversion based on https://gist.github.com/rygorous/4d9e9e88cab13c703773dc767a23575f
// with some simplifications w.r.t. handling of nan/inf since these do not happen in our case

static inline __m128i
float_to_half_sse (const __m128 f)
{
    const __m128 mask_sign = _mm_set1_ps (-0.0f);
    // smallest FP32 that yields a normalized FP16
    const __m128i c_min_normal = _mm_set1_epi32 ((127 - 14) << 23);
    const __m128i c_subnorm_magic =
        _mm_set1_epi32 (((127 - 15) + (23 - 10) + 1) << 23);
    // adjust exponent and add mantissa rounding
    const __m128i c_normal_bias = _mm_set1_epi32 (0xfff - ((127 - 15) << 23));

    __m128 justsign = _mm_and_ps (f, mask_sign);
    __m128 absf     = _mm_andnot_ps (mask_sign, f); // f & ~mask_sign
    // the cast is "free" (extra bypass latency, but no throughput hit)
    __m128i absf_int = _mm_castps_si128 (absf);

    // subnormal?
    __m128i b_issub = _mm_cmpgt_epi32 (c_min_normal, absf_int);

    // "result is subnormal" path
    __m128 subnorm1 = _mm_add_ps (
        absf,
        _mm_castsi128_ps (
            c_subnorm_magic)); // magic value to round output mantissa
    __m128i subnorm2 = _mm_sub_epi32 (
        _mm_castps_si128 (subnorm1), c_subnorm_magic); // subtract out bias

    // "result is normal" path
    __m128i mantoddbit = _mm_slli_epi32 (
        absf_int, 31 - 13); // shift bit 13 (mantissa LSB) to sign
    __m128i mantodd =
        _mm_srai_epi32 (mantoddbit, 31); // -1 if FP16 mantissa odd, else 0

    __m128i round1 = _mm_add_epi32 (absf_int, c_normal_bias);
    // if mantissa LSB odd, bias towards rounding up (RTNE)
    __m128i round2 = _mm_sub_epi32 (round1, mantodd);
    __m128i normal = _mm_srli_epi32 (round2, 13); // rounded result

    // combine the two non-specials
    __m128i nonspecial = _mm_or_si128 (
        _mm_and_si128 (subnorm2, b_issub), _mm_andnot_si128 (b_issub, normal));

    __m128i sign_shift = _mm_srai_epi32 (_mm_castps_si128 (justsign), 16);
    __m128i result     = _mm_or_si128 (nonspecial, sign_shift);

    return result;
}

static inline __m128
half_to_float_sse (const __m128i h)
{
    const __m128i mask_nosign = _mm_set1_epi32 (0x7fff);
    const __m128  magic_mult =
        _mm_castsi128_ps (_mm_set1_epi32 ((254 - 15) << 23));

    __m128i expmant  = _mm_and_si128 (mask_nosign, h);
    __m128i justsign = _mm_xor_si128 (h, expmant);
    __m128i shifted  = _mm_slli_epi32 (expmant, 13);
    __m128  scaled   = _mm_mul_ps (_mm_castsi128_ps (shifted), magic_mult);
    __m128i sign     = _mm_slli_epi32 (justsign, 16);
    __m128  result   = _mm_or_ps (scaled, _mm_castsi128_ps (sign));
    return result;
}

static inline __m128i
from_linear_sse (__m128i x)
{
    // infinity/nan?
    __m128i expmask    = _mm_set1_epi32 (0x7c00);
    __m128i is_inf_nan = _mm_cmpeq_epi32 (_mm_and_si128 (x, expmask), expmask);
    // <= -832.0: exp out of range
    __m128i is_out_of_range = _mm_cmpgt_epi32 (x, _mm_set1_epi32 (0xe27f));
    __m128i ret_zero        = _mm_or_si128 (is_inf_nan, is_out_of_range);

    // >= 8 * log (HALF_MAX)
    __m128i ret_half_max = _mm_and_si128 (
        _mm_cmpgt_epi32 (x, _mm_set1_epi32 (0x558b)),
        _mm_cmpgt_epi32 (_mm_set1_epi32 (0x8000), x));

    // actual calculation
    __m128 f    = half_to_float_sse (x);
    f           = exp_sse (_mm_mul_ps (f, _mm_set1_ps (0.125f)));
    __m128i res = float_to_half_sse (f);

    // select 0x7BFF where ret_half_max
    res = _mm_or_si128 (
        _mm_and_si128 (ret_half_max, _mm_set1_epi32 (0x7BFF)),
        _mm_andnot_si128 (ret_half_max, res));
    // select 0 where ret_zero
    res = _mm_andnot_si128 (ret_zero, res);
    return res;
}

static __m128i
to_linear_sse (__m128i x)
{
    // infinity/nan?
    const __m128i expmask = _mm_set1_epi32 (0x7c00);
    __m128i is_inf_nan = _mm_cmpeq_epi32 (_mm_and_si128 (x, expmask), expmask);
    // negative?
    const __m128i lowmask  = _mm_set1_epi32 (0x7fff);
    __m128i       is_neg   = _mm_cmpgt_epi32 (x, lowmask);
    __m128i       ret_zero = _mm_or_si128 (is_inf_nan, is_neg);
    // +/-0: return negative infinity
    __m128i ret_neg_inf =
        _mm_cmpeq_epi32 (_mm_and_si128 (x, lowmask), _mm_setzero_si128 ());

    // actual calculation
    __m128 f    = half_to_float_sse (x);
    f           = _mm_mul_ps (_mm_set1_ps (8.0f), log_sse (f));
    __m128i res = float_to_half_sse (f);

    // select 0 where ret_zero
    res = _mm_andnot_si128 (ret_zero, res);
    // select 0xfc00 where ret_neg_inf
    res = _mm_or_si128 (
        _mm_and_si128 (ret_neg_inf, _mm_set1_epi32 (0xfc00)),
        _mm_andnot_si128 (ret_neg_inf, res));
    return res;
}

// pack low 16 bits of each 32-bit lane into low 64 bits
static inline __m128i
pack_low16 (__m128i v)
{
    __m128i t = _mm_slli_epi32 (v, 16);
    t         = _mm_srai_epi32 (t, 16);
    return _mm_packs_epi32 (t, _mm_setzero_si128 ());
}

#endif // IMF_HAVE_SSE2

/**************************************/

// 8x SIMD AVX2+F16C log and exp implementations, with the same math as above

#ifdef IMF_HAVE_AVX2

static inline __m256
muladd_avx (__m256 a, __m256 b, __m256 c)
{
    return _mm256_add_ps (_mm256_mul_ps (a, b), c);
}

static inline __m256
mulsub_avx (__m256 a, __m256 b, __m256 c)
{
    return _mm256_sub_ps (_mm256_mul_ps (a, b), c);
}

static inline __m256
pow2_int_avx (__m256i x)
{
    __m256i r = _mm256_add_epi32 (x, _mm256_set1_epi32 (0x7F));
    r         = _mm256_slli_epi32 (r, 23);
    return _mm256_castsi256_ps (r);
}

static inline __m256
exp_avx (__m256 x)
{
    const __m256 kOneOverLog2_avx  = _mm256_set1_ps (kOneOverLog2);
    const __m256 kExpLn2Part0f_avx = _mm256_set1_ps (kExpLn2Part0f);
    const __m256 kExpLn2Part1f_avx = _mm256_set1_ps (kExpLn2Part1f);
    const __m256 kExpPoly0_avx     = _mm256_set1_ps (kExpPoly0);
    const __m256 kExpPoly1_avx     = _mm256_set1_ps (kExpPoly1);
    const __m256 kExpPoly2_avx     = _mm256_set1_ps (kExpPoly2);
    const __m256 kExpPoly3_avx     = _mm256_set1_ps (kExpPoly3);
    const __m256 kExpPoly4_avx     = _mm256_set1_ps (kExpPoly4);

    // reduce
    __m256  xlg2 = _mm256_mul_ps (x, kOneOverLog2_avx);
    __m256i q    = _mm256_cvtps_epi32 (xlg2);
    __m256  qf   = _mm256_cvtepi32_ps (q);
    x            = muladd_avx (qf, kExpLn2Part0f_avx, x);
    x            = muladd_avx (qf, kExpLn2Part1f_avx, x);

    // polynomial approximation
    __m256 x2 = _mm256_mul_ps (x, x);
    __m256 x4 = _mm256_mul_ps (x2, x2);
    __m256 f1 = muladd_avx (
        muladd_avx (
            x4,
            kExpPoly4_avx,
            muladd_avx (
                x2,
                muladd_avx (kExpPoly3_avx, x, kExpPoly2_avx),
                muladd_avx (kExpPoly1_avx, x, kExpPoly0_avx))),
        x2,
        x);

    // reconstruct
    __m256       f2  = _mm256_add_ps (f1, _mm256_set1_ps (1.0f));
    __m256i      y   = _mm256_srai_epi32 (q, 1);
    const __m256 py  = pow2_int_avx (y);
    const __m256 pey = pow2_int_avx (_mm256_sub_epi32 (q, y));
    __m256       f3  = _mm256_mul_ps (_mm256_mul_ps (f2, py), pey);
    return f3;
}

static inline __m256
log_avx (__m256 x)
{
    const __m256  kLogLn2Hi_avx = _mm256_set1_ps (kLogLn2Hi);
    const __m256  kLogLn2Lo_avx = _mm256_set1_ps (kLogLn2Lo);
    const __m256  kLogPoly0_avx = _mm256_set1_ps (kLogPoly0);
    const __m256  kLogPoly1_avx = _mm256_set1_ps (kLogPoly1);
    const __m256  kLogPoly2_avx = _mm256_set1_ps (kLogPoly2);
    const __m256  kLogPoly3_avx = _mm256_set1_ps (kLogPoly3);
    const __m256i kLogMagic_avx = _mm256_set1_epi32 (kLogMagic);
    const __m256i kLogExpMagicMask_avx =
        _mm256_set1_epi32 (kLogExpMask - kLogMagic);
    const __m256i kLogManMask_avx = _mm256_set1_epi32 (kLogManMask);

    // reduce x into [sqrt(2)/2, sqrt(2)]
    __m256i bits = _mm256_castps_si256 (x);
    bits         = _mm256_add_epi32 (bits, kLogExpMagicMask_avx);
    __m256i expu = _mm256_srli_epi32 (bits, 23);
    __m256  exp =
        _mm256_cvtepi32_ps (_mm256_sub_epi32 (expu, _mm256_set1_epi32 (0x7f)));
    bits = _mm256_add_epi32 (
        _mm256_and_si256 (bits, kLogManMask_avx), kLogMagic_avx);
    x = _mm256_castsi256_ps (bits);

    // approximate
    const __m256 kOne = _mm256_set1_ps (1.0f);
    __m256       ym1  = _mm256_sub_ps (x, kOne);
    __m256       z    = _mm256_div_ps (ym1, _mm256_add_ps (x, kOne));
    __m256       z2   = _mm256_mul_ps (z, z);
    __m256       z4   = _mm256_mul_ps (z2, z2);
    __m256       zp   = muladd_avx (
        muladd_avx (kLogPoly2_avx, z4, kLogPoly0_avx),
        z2,
        _mm256_mul_ps (muladd_avx (kLogPoly3_avx, z4, kLogPoly1_avx), z4));

    // reconstruct
    return mulsub_avx (
        exp,
        kLogLn2Hi_avx,
        _mm256_sub_ps (
            mulsub_avx (
                z, _mm256_sub_ps (ym1, zp), _mm256_mul_ps (exp, kLogLn2Lo_avx)),
            ym1));
}

// https://fgiesen.wordpress.com/2016/04/03/sse-mind-the-gap/
static inline __m128i
cmpgt_epu16 (__m128i a, __m128i b)
{
    a = _mm_add_epi16 (a, _mm_set1_epi16 ((int16_t) 0x8000));
    b = _mm_add_epi16 (b, _mm_set1_epi16 ((int16_t) 0x8000));
    return _mm_cmpgt_epi16 (a, b);
}

static inline __m128i
from_linear_avx (__m128i x)
{
    // infinity/nan?
    const __m128i expmask = _mm_set1_epi16 (0x7C00);
    __m128i is_inf_nan = _mm_cmpeq_epi16 (_mm_and_si128 (x, expmask), expmask);
    // <= -832.0: exp out of range
    __m128i is_out_of_range = cmpgt_epu16 (x, _mm_set1_epi16 ((short) 0xe27f));
    __m128i ret_zero        = _mm_or_si128 (is_inf_nan, is_out_of_range);

    // >= 8 * log (HALF_MAX)
    __m128i ret_half_max = _mm_and_si128 (
        _mm_cmpgt_epi16 (x, _mm_set1_epi16 (0x558b)),
        _mm_cmpgt_epi16 (x, _mm_setzero_si128 ()));

    // actual calculation
    __m256 f    = _mm256_cvtph_ps (x);
    f           = exp_avx (_mm256_mul_ps (f, _mm256_set1_ps (0.125f)));
    __m128i res = _mm256_cvtps_ph (f, _MM_FROUND_TO_NEAREST_INT);

    // select 0x7BFF where ret_half_max
    res = _mm_blendv_epi8 (res, _mm_set1_epi16 (0x7BFF), ret_half_max);
    // select 0 where ret_zero
    res = _mm_andnot_si128 (ret_zero, res);
    return res;
}

static __m128i
to_linear_avx (__m128i x)
{
    // infinity/nan?
    const __m128i expmask = _mm_set1_epi16 (0x7c00);
    __m128i is_inf_nan = _mm_cmpeq_epi16 (_mm_and_si128 (x, expmask), expmask);
    // negative?
    const __m128i lowmask  = _mm_set1_epi16 (0x7fff);
    __m128i       is_neg   = _mm_cmpgt_epi16 (_mm_setzero_si128 (), x);
    __m128i       ret_zero = _mm_or_si128 (is_inf_nan, is_neg);
    // +/-0: return negative infinity
    __m128i ret_neg_inf =
        _mm_cmpeq_epi16 (_mm_and_si128 (x, lowmask), _mm_setzero_si128 ());

    // actual calculation
    __m256 f    = _mm256_cvtph_ps (x);
    f           = _mm256_mul_ps (_mm256_set1_ps (8.0f), log_avx (f));
    __m128i res = _mm256_cvtps_ph (f, _MM_FROUND_TO_NEAREST_INT);

    // select 0 where ret_zero
    res = _mm_andnot_si128 (ret_zero, res);
    // select 0xfc00 where ret_neg_inf
    res = _mm_blendv_epi8 (res, _mm_set1_epi16 ((int16_t) 0xfc00), ret_neg_inf);
    return res;
}

#endif // IMF_HAVE_AVX2

/**************************************/

// 4x SIMD ARM NEON log and exp implementations, with the same math as above

#ifdef IMF_HAVE_NEON_AARCH64

static inline float32x4_t
muladd_neon (float32x4_t a, float32x4_t b, float32x4_t c)
{
    return vaddq_f32 (vmulq_f32 (a, b), c);
}

static inline float32x4_t
mulsub_neon (float32x4_t a, float32x4_t b, float32x4_t c)
{
    return vsubq_f32 (vmulq_f32 (a, b), c);
}

static inline float32x4_t
pow2_int_neon (int32x4_t x)
{
    int32x4_t r = vaddq_s32 (x, vdupq_n_s32 (0x7F));
    r           = vshlq_s32 (r, vdupq_n_s32 (23));
    return vreinterpretq_f32_s32 (r);
}

static inline float32x4_t
exp_neon (float32x4_t x)
{
    const float32x4_t kOneOverLog2_neon  = vdupq_n_f32 (kOneOverLog2);
    const float32x4_t kExpLn2Part0f_neon = vdupq_n_f32 (kExpLn2Part0f);
    const float32x4_t kExpLn2Part1f_neon = vdupq_n_f32 (kExpLn2Part1f);
    const float32x4_t kExpPoly0_neon     = vdupq_n_f32 (kExpPoly0);
    const float32x4_t kExpPoly1_neon     = vdupq_n_f32 (kExpPoly1);
    const float32x4_t kExpPoly2_neon     = vdupq_n_f32 (kExpPoly2);
    const float32x4_t kExpPoly3_neon     = vdupq_n_f32 (kExpPoly3);
    const float32x4_t kExpPoly4_neon     = vdupq_n_f32 (kExpPoly4);

    // reduce
    float32x4_t xlg2 = vmulq_f32 (x, kOneOverLog2_neon);
    int32x4_t   q    = vcvtnq_s32_f32 (xlg2);
    float32x4_t qf   = vcvtq_f32_s32 (q);
    x                = muladd_neon (qf, kExpLn2Part0f_neon, x);
    x                = muladd_neon (qf, kExpLn2Part1f_neon, x);

    // polynomial approximation
    float32x4_t x2 = vmulq_f32 (x, x);
    float32x4_t x4 = vmulq_f32 (x2, x2);
    float32x4_t f1 = muladd_neon (
        muladd_neon (
            x4,
            kExpPoly4_neon,
            muladd_neon (
                x2,
                muladd_neon (kExpPoly3_neon, x, kExpPoly2_neon),
                muladd_neon (kExpPoly1_neon, x, kExpPoly0_neon))),
        x2,
        x);

    // reconstruct
    float32x4_t       f2  = vaddq_f32 (f1, vdupq_n_f32 (1.0f));
    int32x4_t         y   = vshrq_n_s32 (q, 1);
    const float32x4_t py  = pow2_int_neon (y);
    const float32x4_t pey = pow2_int_neon (vsubq_s32 (q, y));
    float32x4_t       f3  = vmulq_f32 (vmulq_f32 (f2, py), pey);
    return f3;
}

static inline float32x4_t
log_neon (float32x4_t x)
{
    const float32x4_t kLogLn2Hi_neon = vdupq_n_f32 (kLogLn2Hi);
    const float32x4_t kLogLn2Lo_neon = vdupq_n_f32 (kLogLn2Lo);
    const float32x4_t kLogPoly0_neon = vdupq_n_f32 (kLogPoly0);
    const float32x4_t kLogPoly1_neon = vdupq_n_f32 (kLogPoly1);
    const float32x4_t kLogPoly2_neon = vdupq_n_f32 (kLogPoly2);
    const float32x4_t kLogPoly3_neon = vdupq_n_f32 (kLogPoly3);
    const int32x4_t   kLogMagic_neon = vdupq_n_s32 (kLogMagic);
    const int32x4_t   kLogExpMagicMask_neon =
        vdupq_n_s32 (kLogExpMask - kLogMagic);
    const int32x4_t kLogManMask_neon = vdupq_n_s32 (kLogManMask);

    // reduce x into [sqrt(2)/2, sqrt(2)]
    int32x4_t bits = vreinterpretq_s32_f32 (x);
    bits           = vaddq_s32 (bits, kLogExpMagicMask_neon);
    int32x4_t expu =
        vreinterpretq_s32_u32 (vshrq_n_u32 (vreinterpretq_u32_s32 (bits), 23));
    float32x4_t exp = vcvtq_f32_s32 (vsubq_s32 (expu, vdupq_n_s32 (0x7f)));
    bits = vaddq_s32 (vandq_s32 (bits, kLogManMask_neon), kLogMagic_neon);
    x    = vreinterpretq_f32_s32 (bits);

    // approximate
    const float32x4_t kOne = vdupq_n_f32 (1.0f);
    float32x4_t       ym1  = vsubq_f32 (x, kOne);
    float32x4_t       z    = vdivq_f32 (ym1, vaddq_f32 (x, kOne));
    float32x4_t       z2   = vmulq_f32 (z, z);
    float32x4_t       z4   = vmulq_f32 (z2, z2);
    float32x4_t       zp   = muladd_neon (
        muladd_neon (kLogPoly2_neon, z4, kLogPoly0_neon),
        z2,
        vmulq_f32 (muladd_neon (kLogPoly3_neon, z4, kLogPoly1_neon), z4));

    // reconstruct
    return mulsub_neon (
        exp,
        kLogLn2Hi_neon,
        vsubq_f32 (
            mulsub_neon (
                z, vsubq_f32 (ym1, zp), vmulq_f32 (exp, kLogLn2Lo_neon)),
            ym1));
}

static inline float32x4_t
half_to_float_neon (uint32x4_t u32)
{
    uint16x4_t  u16 = vmovn_u32 (u32);
    float16x4_t h4  = vreinterpret_f16_u16 (u16);
    return vcvt_f32_f16 (h4);
}

static inline uint32x4_t
float_to_half_neon (float32x4_t f32)
{
    float16x4_t h16 = vcvt_f16_f32 (f32);
    uint16x4_t  u16 = vreinterpret_u16_f16 (h16);
    return vmovl_u16 (u16);
}

static inline uint32x4_t
from_linear_neon (uint32x4_t x)
{
    // infinity/nan?
    uint32x4_t expmask    = vdupq_n_u32 (0x7c00);
    uint32x4_t is_inf_nan = vceqq_u32 (vandq_u32 (x, expmask), expmask);
    // <= -832.0: exp out of range
    uint32x4_t is_out_of_range = vcgtq_u32 (x, vdupq_n_u32 (0xe27f));
    uint32x4_t ret_zero        = vorrq_u32 (is_inf_nan, is_out_of_range);

    // >= 8 * log (HALF_MAX)
    uint32x4_t ret_half_max = vandq_u32 (
        vcgtq_u32 (x, vdupq_n_u32 (0x558b)),
        vcgtq_u32 (vdupq_n_u32 (0x8000), x));

    // actual calculation
    float32x4_t f  = half_to_float_neon (x);
    f              = exp_neon (vmulq_f32 (f, vdupq_n_f32 (0.125f)));
    uint32x4_t res = float_to_half_neon (f);

    // select 0x7BFF where ret_half_max
    res = vbslq_u32 (ret_half_max, vdupq_n_u32 (0x7BFF), res);
    // select 0 where ret_zero
    res = vbicq_u32 (res, ret_zero);
    return res;
}

static uint32x4_t
to_linear_neon (uint32x4_t x)
{
    // infinity/nan?
    const uint32x4_t expmask    = vdupq_n_u32 (0x7c00);
    uint32x4_t       is_inf_nan = vceqq_u32 (vandq_u32 (x, expmask), expmask);
    // negative?
    const uint32x4_t lowmask  = vdupq_n_u32 (0x7fff);
    uint32x4_t       is_neg   = vcgtq_u32 (x, lowmask);
    uint32x4_t       ret_zero = vorrq_u32 (is_inf_nan, is_neg);
    // +/-0: return negative infinity
    uint32x4_t ret_neg_inf =
        vceqq_u32 (vandq_u32 (x, lowmask), vdupq_n_u32 (0));

    // actual calculation
    float32x4_t f  = half_to_float_neon (x);
    f              = vmulq_f32 (vdupq_n_f32 (8.0f), log_neon (f));
    uint32x4_t res = float_to_half_neon (f);

    // select 0 where ret_zero
    res = vbicq_u32 (res, ret_zero);
    // select 0xfc00 where ret_neg_inf
    res = vbslq_u32 (ret_neg_inf, vdupq_n_u32 (0xfc00), res);
    return res;
}

#endif // IMF_HAVE_NEON_AARCH64

/**************************************/

static inline uint16_t
b44_convertFromLinear (uint16_t x)
{
    if ((x & 0x7c00) == 0x7c00) // infinity/nan?
        return 0;
    if (x >= 0xe280) // <= -832.0: exp out of range
        return 0;
    if (x >= 0x558c && x < 0x8000) // >= 8 * log (HALF_MAX)
        return 0x7bff;             // HALF_MAX

    float f = imath_half_to_float (x);
    f       = exp_scalar (f / 8);
    return imath_float_to_half (f);
}

static inline uint16_t
b44_convertToLinear (uint16_t x)
{
    if ((x & 0x7fff) == 0)      // +/-0
        return 0xfc00;          // -infinity
    if ((x & 0x7c00) == 0x7c00) // infinity/nan?
        return 0;
    if ((x & 0x8000) != 0) // negative?
        return 0;

    float f = imath_half_to_float (x);
    f       = 8 * log_scalar (f);
    return imath_float_to_half (f);
}

void
b44_convertFromLinear_16 (uint16_t s[16])
{
#if defined(IMF_HAVE_AVX2)
    for (int i = 0; i < 2; ++i)
    {
        __m128i v8 = _mm_loadu_si128 ((const __m128i*) s);
        v8         = from_linear_avx (v8);
        _mm_storeu_si128 ((__m128i*) s, v8);
        s += 8;
    }
#elif defined(IMF_HAVE_SSE2)
    for (int i = 0; i < 4; ++i)
    {
        __m128i s4 = _mm_loadu_si64 (s);
        s4         = _mm_unpacklo_epi16 (s4, _mm_setzero_si128 ());
        s4         = from_linear_sse (s4);
        s4         = pack_low16 (s4);
        _mm_storeu_si64 (s, s4);
        s += 4;
    }
#elif defined(IMF_HAVE_NEON_AARCH64)
    for (int i = 0; i < 4; ++i)
    {
        uint16x4_t h4 = vld1_u16 (s);
        uint32x4_t s4 = vmovl_u16 (h4);
        s4            = from_linear_neon (s4);
        h4            = vmovn_u32 (s4);
        vst1_u16 (s, h4);
        s += 4;
    }
#else
    for (int i = 0; i < 16; ++i)
        s[i] = b44_convertFromLinear (s[i]);
#endif
}

void
b44_convertToLinear_16 (uint16_t s[16])
{
#if defined(IMF_HAVE_AVX2)
    for (int i = 0; i < 2; ++i)
    {
        __m128i v8 = _mm_loadu_si128 ((const __m128i*) s);
        v8         = to_linear_avx (v8);
        _mm_storeu_si128 ((__m128i*) s, v8);
        s += 8;
    }
#elif defined(IMF_HAVE_SSE2)
    for (int i = 0; i < 4; ++i)
    {
        __m128i s4 = _mm_loadu_si64 (s);
        s4         = _mm_unpacklo_epi16 (s4, _mm_setzero_si128 ());
        s4         = to_linear_sse (s4);
        s4         = pack_low16 (s4);
        _mm_storeu_si64 (s, s4);
        s += 4;
    }
#elif defined(IMF_HAVE_NEON_AARCH64)
    for (int i = 0; i < 4; ++i)
    {
        uint16x4_t h4 = vld1_u16 (s);
        uint32x4_t s4 = vmovl_u16 (h4);
        s4            = to_linear_neon (s4);
        h4            = vmovn_u32 (s4);
        vst1_u16 (s, h4);
        s += 4;
    }
#else
    for (int i = 0; i < 16; ++i)
        s[i] = b44_convertToLinear (s[i]);
#endif
}
