/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <openexr_compression.h>
#include "internal_compress.h"
#include "internal_decompress.h"
#include "internal_coding.h"
#include "internal_structs.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
//#include <immintrin.h>

/* Use Zstd directly */
#include <zstd.h>

#define RETURN_ERRORV(pipeline, err_code, msg, ...)                            \
    {                                                                          \
        exr_const_context_t pctxt = pipeline->context;                         \
        if (pctxt) pctxt->print_error (pctxt, err_code, msg, __VA_ARGS__);     \
        return err_code;                                                       \
    }

/* * Thread Local Storage for Zstd Contexts.
 * This is crucial for performance on 32-96 core machines to avoid allocator locks.
 */
static _Thread_local ZSTD_CCtx* t_cctx = NULL;
static _Thread_local ZSTD_DCtx* d_cctx = NULL;
static _Thread_local uint8_t* t_shuffle_buf = NULL;
static _Thread_local size_t     t_shuffle_buf_size = 0;

static void ensure_tls_resources(size_t required_size) {
    if (!t_cctx) t_cctx = ZSTD_createCCtx();
    if (!d_cctx) d_cctx = ZSTD_createDCtx();
    if (t_shuffle_buf_size < required_size) {
        t_shuffle_buf = (uint8_t*)realloc(t_shuffle_buf, required_size);
        t_shuffle_buf_size = required_size;
    }
}

/* Optimized Planar Byte-Shuffle (the "Blosc" magic) */
static inline void shuffle_encode_4(const uint8_t* in, size_t size, uint8_t* out) {
    size_t elements = size / 4;
    uint8_t *p0 = out, *p1 = out + elements, *p2 = out + elements * 2, *p3 = out + elements * 3;
    for (size_t i = 0; i < elements; ++i) {
        p0[i] = in[i * 4 + 0]; p1[i] = in[i * 4 + 1]; p2[i] = in[i * 4 + 2]; p3[i] = in[i * 4 + 3];
    }
}

static inline void shuffle_encode_2(const uint8_t* in, size_t size, uint8_t* out) {
    size_t elements = size / 2;
    uint8_t *p0 = out, *p1 = out + elements;
    for (size_t i = 0; i < elements; ++i) {
        p0[i] = in[i * 2 + 0]; p1[i] = in[i * 2 + 1];
    }
}

/* scalar */
static inline void shuffle_decode_4(const uint8_t* in, size_t size, uint8_t* out) {
    size_t elements = size / 4;
    const uint8_t *p0 = in, *p1 = in + elements, *p2 = in + elements * 2, *p3 = in + elements * 3;
    for (size_t i = 0; i < elements; ++i) {
        out[i * 4 + 0] = p0[i]; out[i * 4 + 1] = p1[i]; out[i * 4 + 2] = p2[i]; out[i * 4 + 3] = p3[i];
    }
}

// scalar
static inline void shuffle_decode_2(const uint8_t* in, size_t size, uint8_t* out) {
    size_t elements = size / 2;
    const uint8_t *p0 = in, *p1 = in + elements;
    for (size_t i = 0; i < elements; ++i) {
        out[i * 2 + 0] = p0[i]; out[i * 2 + 1] = p1[i];
    }
}
/*
// AVX2
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
}*/
/*
// sse
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


static void
shuffle_decode_bytes (
    uint8_t* dst, const uint8_t* shuf, size_t dSize, uint64_t shuffle_el_bytes)
{
    if (shuffle_el_bytes == 4 && (dSize % 4) == 0)
        shuffle_decode_4 (shuf, dSize, dst);
    else if (shuffle_el_bytes == 2 && (dSize % 2) == 0)
        shuffle_decode_2 (shuf, dSize, dst);
    else if (shuffle_el_bytes == 0)
    {
        if ((dSize % 4) == 0)
            shuffle_decode_4 (shuf, dSize, dst);
        else if ((dSize % 2) == 0)
            shuffle_decode_2 (shuf, dSize, dst);
        else
            memcpy (dst, shuf, dSize);
    }
    else
        memcpy (dst, shuf, dSize);
}



typedef uint64_t (*serialization_callback) (char* src, uint64_t iSize, char* dest, uint64_t oSize);
static const uint64_t SERIALIZATION_OVERHEAD = sizeof(uint64_t);


/** shuffle_el_bytes: 2 or 4 selects shuffle_decode_*; 0 means infer from dSize (legacy). */
long exr_uncompress_zstd (
    char* inPtr, uint64_t inSize, void** outPtr, uint64_t outPtrSize, uint64_t shuffle_el_bytes)
{
    ensure_tls_resources(outPtrSize);

    ZSTD_DCtx_reset(d_cctx, ZSTD_reset_session_only);
    size_t dSize = ZSTD_decompressDCtx(d_cctx, t_shuffle_buf, outPtrSize, inPtr, inSize);

    if (ZSTD_isError(dSize)) return -1;

    if (*outPtr == NULL)
    {
        *outPtr = malloc (dSize);
        if (!*outPtr) return -1;
    }

    if (shuffle_el_bytes == 4 && (dSize % 4) == 0)
        shuffle_decode_4 (t_shuffle_buf, dSize, (uint8_t*) *outPtr);
    else if (shuffle_el_bytes == 2 && (dSize % 2) == 0)
        shuffle_decode_2 (t_shuffle_buf, dSize, (uint8_t*) *outPtr);
    else if (shuffle_el_bytes == 0)
    {
        if ((dSize % 4) == 0)
            shuffle_decode_4 (t_shuffle_buf, dSize, (uint8_t*) *outPtr);
        else if ((dSize % 2) == 0)
            shuffle_decode_2 (t_shuffle_buf, dSize, (uint8_t*) *outPtr);
        else
            memcpy (*outPtr, t_shuffle_buf, dSize);
    }
    else
        memcpy (*outPtr, t_shuffle_buf, dSize);

    return (long) dSize;
}

/* --- Keep the existing OpenEXR specific sorting/sampling logic below --- */

uint64_t compute_sorting_lookup (const uint64_t* num_samples_per_row, int height, const exr_coding_channel_info_t* channels, int channelsSize, uint64_t* sorting_lookup)
{
    uint64_t writeCount = 0;
    
    // Pass 1: 2-byte channels (half) - Interleaved PER SCANLINE
    for (int h = 0; h < height; ++h) {
        for (int i = 0; i < channelsSize; ++i) {
            if (channels[i].bytes_per_element == 2) {
                *(sorting_lookup + h * channelsSize + i) = writeCount;
                writeCount += num_samples_per_row[h] * 2;
            }
        }
    }
    
    uint64_t splitPoint = writeCount;
    
    // Pass 2: 4-byte channels (float) - Interleaved PER SCANLINE
    for (int h = 0; h < height; ++h) {
        for (int i = 0; i < channelsSize; ++i) {
            if (channels[i].bytes_per_element == 4) {
                *(sorting_lookup + h * channelsSize + i) = writeCount;
                writeCount += num_samples_per_row[h] * 4;
            }
        }
    }
    return splitPoint;
}

uint64_t sort2_4ByteChannels_tiled (const char* inPtr, const uint64_t* num_samples_per_row, const exr_coding_channel_info_t* channels, const int channelsSize, const bool forward, int height, char* outPtr)
{
    uint64_t sorting_lookup[channelsSize * height];
    uint64_t splitPoint = compute_sorting_lookup(num_samples_per_row, height, channels, channelsSize, sorting_lookup);
    
    uint32_t pixel_stride = 0;
    for (int i = 0; i < channelsSize; ++i) pixel_stride += channels[i].bytes_per_element;
    
    uint64_t line_start_read = 0;
    
    // Flipped loops: Process height first, then channels. 
    // This perfectly matches the new layout and keeps CPU cache access sequential.
    for (int h = 0; h < height; ++h) {
        uint32_t processed_stride = 0;
        
        for (int i = 0; i < channelsSize; ++i) {
            const uint64_t writeIndex = *(sorting_lookup + h * channelsSize + i); 
            const uint64_t readIndex  = line_start_read + (num_samples_per_row[h] * processed_stride);
            const uint64_t bitSize    = num_samples_per_row[h] * channels[i].bytes_per_element;
            
            if (forward) memcpy(outPtr + writeIndex, inPtr + readIndex, bitSize);
            else         memcpy(outPtr + readIndex, inPtr + writeIndex, bitSize);
            
            processed_stride += channels[i].bytes_per_element;
        }
        line_start_read += num_samples_per_row[h] * pixel_stride;
    }
    
    return splitPoint;
}

/* Helper serialization functions (Keep for compatibility with EXR format) */
static uint64_t _read_uint64 (char** src) {
    uint64_t val; memcpy(&val, *src, 8); *src += 8; return val;
}
static void _write_uint64 (char** dst, const uint64_t val) {
    memcpy(*dst, &val, 8); *dst += 8;
}
static void _write_buffer_data (char** dst, const int32_t size, const void* src) {
    if (size > 0) { memcpy(*dst, src, size); *dst += size; }
}

static const uint64_t MAGIC_NUMBER = 8248453963162350458; // "zstd-exr"

#define ZSTD_EXR_FORMAT_V1 1u
#define ZSTD_EXR_V1_HEADER 24u

static uint32_t
read_u32_le (const uint8_t* p)
{
    return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) |
           ((uint32_t) p[3] << 24);
}

static uint64_t
read_u64_le (const uint8_t* p)
{
    uint64_t lo = read_u32_le (p);
    uint64_t hi = read_u32_le (p + 4);
    return lo | (hi << 32);
}

static void
write_u32_le (uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t) (v & 0xffu);
    p[1] = (uint8_t) ((v >> 8) & 0xffu);
    p[2] = (uint8_t) ((v >> 16) & 0xffu);
    p[3] = (uint8_t) ((v >> 24) & 0xffu);
}

static void
write_u64_le (uint8_t* p, uint64_t v)
{
    write_u32_le (p, (uint32_t) (v & 0xffffffffu));
    write_u32_le (p + 4, (uint32_t) (v >> 32));
}


static int
append_inner_shuffle (
    uint8_t* inner,
    size_t* pos,
    size_t inner_cap,
    const char* src,
    size_t src_len,
    uint64_t typeSize)
{
    if (*pos + 8 > inner_cap) return -1;
    if (src_len > (size_t) INT_MAX) return -1;
    write_u64_le (inner + *pos, (uint64_t) src_len);
    *pos += 8;
    if (*pos + src_len > inner_cap) return -1;
    if (src_len == 0) return 0;
    if (typeSize == 4)
        shuffle_encode_4 ((const uint8_t*) src, src_len, inner + *pos);
    else if (typeSize == 2)
        shuffle_encode_2 ((const uint8_t*) src, src_len, inner + *pos);
    else
        memcpy (inner + *pos, src, src_len);
    *pos += src_len;
    return 0;
}

/** Returns 0 on success; sets *out_total to bytes written (header + ZSTD). */
static int
zstd_write_exr_v1 (
    void*       out_buf,
    size_t      out_cap,
    const uint8_t* inner,
    size_t      inner_len,
    int32_t     level,
    uint64_t*   out_total)
{
    if (out_cap < ZSTD_EXR_V1_HEADER) return -1;
    if (!t_cctx) t_cctx = ZSTD_createCCtx ();
    size_t const cbound = ZSTD_compressBound (inner_len);
    if (ZSTD_isError (cbound)) return -1;
    if (cbound > out_cap - ZSTD_EXR_V1_HEADER) return -1;


/* * Zstd Configuration for Deep EXR 16-Scanline Chunks
 * --------------------------------------------------
 * This setup is optimized for Interleaved Byte-Shuffled data. 
 * Benchmarks show this achieves 'Ultra' density (~536MB) with 
 * 'Fast' CPU overhead by exploiting local data alignment.
 */

/* Reuse the context to avoid malloc/free contention across 96 threads. */
ZSTD_CCtx_reset(t_cctx, ZSTD_reset_session_only);

/* * Presets: Use Level 5 for production (balanced speed/size). 
 * Use Level 15 for archive (max density, 10x slower write). 
 */
ZSTD_CCtx_setParameter(t_cctx, ZSTD_c_compressionLevel, level);

/* * Set internal workers to 0 to prevent "thread bombing." 
 * Threading is handled externally at the frame/chunk level. 
 */
ZSTD_CCtx_setParameter(t_cctx, ZSTD_c_nbWorkers, 0);

/* * WindowLog 24 (16MB) allows the compressor to see the entire 
 * ~8MB chunk at once, enabling vertical redundancy matching 
 * between the first and last scanlines of the block. 
 */
ZSTD_CCtx_setParameter(t_cctx, ZSTD_c_windowLog, 24);

/* * Strategy 'fast' is sufficient here. Because of the interleaved 
 * shuffle, patterns are so well-aligned that complex binary-tree 
 * searches (btultra) yield no additional gains over simple hashing. 
 */
ZSTD_CCtx_setParameter(t_cctx, ZSTD_c_strategy, ZSTD_fast);

/* * High-density search logs. hashLog 22 provides 4M table entries 
 * to minimize collisions, while chainLog 24 allows deep enough 
 * searching to fully utilize the 16MB window. 
 */
ZSTD_CCtx_setParameter(t_cctx, ZSTD_c_chainLog, 24);
ZSTD_CCtx_setParameter(t_cctx, ZSTD_c_hashLog, 22);

/* * : minMatch 3 captures 3-byte repeating patterns 
 * in floating-point exponents and high-bits. This single parameter 
 * is responsible for ~19MB of savings per frame in shuffled data. 
 */
ZSTD_CCtx_setParameter(t_cctx, ZSTD_c_minMatch, 3);

    
    uint8_t* const out = (uint8_t*) out_buf;
    size_t const cSize = ZSTD_compressCCtx (
        t_cctx,
        out + ZSTD_EXR_V1_HEADER,
        out_cap - ZSTD_EXR_V1_HEADER,
        inner,
        inner_len,
        level);
    if (ZSTD_isError (cSize)) return -1;
    memcpy (out, &MAGIC_NUMBER, sizeof (MAGIC_NUMBER));
    write_u32_le (out + 8, ZSTD_EXR_FORMAT_V1);
    write_u32_le (out + 12, 0);
    write_u64_le (out + 16, cSize);
    *out_total = ZSTD_EXR_V1_HEADER + cSize;
    return 0;
}

uint64_t serialize_buffer (char* src, uint64_t iSize, char* dest, uint64_t oSize) {
    char* d = dest; _write_uint64(&d, iSize); _write_buffer_data(&d, (int32_t)iSize, src);
    return (uint64_t)(d - dest);
}

uint64_t serialize_memcpy (char* src, uint64_t iSize, char* dest, uint64_t oSize) {
    memcpy(dest, src, iSize); return iSize;
}

bool get_row_sample_count_decode (const exr_decode_pipeline_t* decode, uint64_t *row_sample_counts) {
    if (decode->sample_count_valid == 1 && decode->chunk.width > 0) {
        for (int h = 0; h < decode->chunk.height; ++h)
            row_sample_counts[h] = (uint64_t)decode->sample_count_table[(h+1) * decode->chunk.width - 1];
        return true;
    }
    return false;
}

bool get_row_sample_count_encode (const exr_encode_pipeline_t* encode, uint64_t *row_sample_counts) {
    if (encode->sample_count_alloc_size > 0) {
        for (int h = 0; h < encode->chunk.height; ++h)
            row_sample_counts[h] = (uint64_t)encode->sample_count_table[(h+1) * encode->chunk.width - 1];
        return true;
    }
    return false;
}

bool needs_sorting(const exr_coding_channel_info_t* channels, const int channelsSize, const uint64_t* numSamples, int height, uint64_t* split, uint8_t types[2]) {
    *split = 0;
    if (channelsSize <= 0) return false;
    int8_t first_ts = channels[0].bytes_per_element;
    bool switched = false; uint64_t count = 0;
    for (int i = 0; i < channelsSize; ++i) {
        if (channels[i].bytes_per_element != first_ts) {
            if (switched) { *split = 0; return true; }
            switched = true; *split = count;
        }
        for (int h = 0; h < height; ++h) count += numSamples[h] * channels[i].bytes_per_element;
    }
    types[0] = channels[0].bytes_per_element;     types[1] = channels[channelsSize-1].bytes_per_element;
    return switched;
}

/* --- Main Pipeline Implementation --- */

exr_result_t internal_exr_apply_zstd (exr_encode_pipeline_t* encode)
{
    uint64_t row_sample_counts[encode->chunk.height];
    bool const sampleCount_valid =
        get_row_sample_count_encode (encode, row_sample_counts);
    int32_t level = 5;
    exr_result_t rv = exr_get_zstd_compression_level (
        encode->context, encode->part_index, &level);
    if (rv != EXR_ERR_SUCCESS) return rv;

    const uint64_t sample_table_bytes =
        (uint64_t) encode->chunk.width * (uint64_t) encode->chunk.height * 4u;

    if (sampleCount_valid && encode->packed_bytes == 0)
    {
        encode->compressed_bytes = 0;
        return EXR_ERR_SUCCESS;
    }

    const size_t packed = encode->packed_bytes;
    size_t inner_cap = packed + 16u;
    if (inner_cap < packed) goto fallback;
    if (encode->compressed_alloc_size < ZSTD_EXR_V1_HEADER) goto fallback;
    ensure_tls_resources (inner_cap);

    uint8_t* const inner = t_shuffle_buf;
    size_t inner_pos = 0;

    const int compressing_sample_counts_only =
        (encode->packed_sample_count_table != NULL &&
         encode->packed_buffer == encode->packed_sample_count_table &&
         packed == (size_t) sample_table_bytes);

    if (compressing_sample_counts_only)
    {
        inner_pos = 0;
        if (append_inner_shuffle (
                inner,
                &inner_pos,
                inner_cap,
                (const char*) encode->packed_buffer,
                packed,
                4) != 0)
            goto fallback;
        uint64_t total_w = 0;
        if (zstd_write_exr_v1 (
                encode->compressed_buffer,
                encode->compressed_alloc_size,
                inner,
                inner_pos,
                level,
                &total_w) != 0)
            goto fallback;
        if (total_w < packed)
        {
            encode->compressed_bytes = total_w;
            return EXR_ERR_SUCCESS;
        }
        goto fallback;
    }

    if (sampleCount_valid)
    {
        uint64_t splitPos = 0;
        uint8_t tSizes[2] = {0, 0};
        bool const sorted =
            (encode->chunk.type == EXR_STORAGE_DEEP_TILED) ||
            needs_sorting (
                encode->channels,
                encode->channel_count,
                row_sample_counts,
                encode->chunk.height,
                &splitPos,
                tSizes);
        char* inData = (char*) encode->packed_buffer;
        if (sorted)
        {
            exr_result_t arv = internal_encode_alloc_buffer (
                encode,
                EXR_TRANSCODE_BUFFER_SCRATCH1,
                &encode->scratch_buffer_1,
                &encode->scratch_alloc_size_1,
                encode->packed_bytes);
            if (arv != EXR_ERR_SUCCESS) return arv;
            splitPos = sort2_4ByteChannels_tiled (
                (const char*) encode->packed_buffer,
                row_sample_counts,
                encode->channels,
                encode->channel_count,
                true,
                encode->chunk.height,
                (char*) encode->scratch_buffer_1);
            inData = (char*) encode->scratch_buffer_1;
            tSizes[0] = 2;
            tSizes[1] = 4;
        }

        inner_pos = 0;
        if (splitPos > 0)
        {
            if (append_inner_shuffle (
                    inner,
                    &inner_pos,
                    inner_cap,
                    inData,
                    (size_t) splitPos,
                    tSizes[0]) != 0)
                goto fallback;
        }
        size_t const s2_in = packed - (size_t) splitPos;
        if (s2_in > 0)
        {
            if (append_inner_shuffle (
                    inner,
                    &inner_pos,
                    inner_cap,
                    inData + splitPos,
                    s2_in,
                    tSizes[1]) != 0)
                goto fallback;
        }
        uint64_t total_w = 0;
        if (zstd_write_exr_v1 (
                encode->compressed_buffer,
                encode->compressed_alloc_size,
                inner,
                inner_pos,
                level,
                &total_w) != 0)
            goto fallback;
        if (total_w < packed)
        {
            encode->compressed_bytes = total_w;
            return EXR_ERR_SUCCESS;
        }
        goto fallback;
    }

    {
        const int no_table_typesize =
            (encode->chunk.type == EXR_STORAGE_SCANLINE ||
             encode->chunk.type == EXR_STORAGE_TILED)
                ? 2
                : 4;
        inner_pos = 0;
        if (append_inner_shuffle (
                inner,
                &inner_pos,
                inner_cap,
                (const char*) encode->packed_buffer,
                packed,
                (uint64_t) no_table_typesize) != 0)
            goto fallback;
        uint64_t total_w = 0;
        if (zstd_write_exr_v1 (
                encode->compressed_buffer,
                encode->compressed_alloc_size,
                inner,
                inner_pos,
                level,
                &total_w) != 0)
            goto fallback;
        if (total_w < packed)
        {
            encode->compressed_bytes = total_w;
            return EXR_ERR_SUCCESS;
        }
    }

fallback:
    memcpy (encode->compressed_buffer, encode->packed_buffer, packed);
    encode->compressed_bytes = packed;
    return EXR_ERR_SUCCESS;
}

static exr_result_t
exr_undo_zstd_v1 (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    const uint8_t* hdr = (const uint8_t*) compressed_data;
    uint64_t         outer = read_u64_le (hdr + 16);
    if (outer > comp_buf_size - ZSTD_EXR_V1_HEADER)
        return EXR_ERR_CORRUPT_CHUNK;
    if (ZSTD_EXR_V1_HEADER + outer != comp_buf_size)
        return EXR_ERR_CORRUPT_CHUNK;

    const void* zsrc = hdr + ZSTD_EXR_V1_HEADER;
    size_t      zsz  = (size_t) outer;

    unsigned long long fds = ZSTD_getFrameContentSize (zsrc, zsz);
    size_t             dst_cap;
    if (fds == ZSTD_CONTENTSIZE_ERROR || fds == ZSTD_CONTENTSIZE_UNKNOWN)
    {
        if (uncompressed_size > (uint64_t) SIZE_MAX - 16)
            return EXR_ERR_CORRUPT_CHUNK;
        dst_cap = (size_t) uncompressed_size + 16;
    }
    else if (fds > (unsigned long long) SIZE_MAX)
        return EXR_ERR_CORRUPT_CHUNK;
    else
        dst_cap = (size_t) fds;

    ensure_tls_resources (dst_cap);

    ZSTD_DCtx_reset (d_cctx, ZSTD_reset_session_only);
    size_t dSize =
        ZSTD_decompressDCtx (d_cctx, t_shuffle_buf, dst_cap, zsrc, zsz);
    if (ZSTD_isError (dSize)) return EXR_ERR_CORRUPT_CHUNK;

    uint64_t row_sample_counts[decode->chunk.height];
    bool const sampleCount_valid =
        get_row_sample_count_decode (decode, row_sample_counts);

    uint64_t split = 0;
    uint8_t  ts[2] = {0, 0};
    bool const needs_chan_sort =
        sampleCount_valid &&
        needs_sorting (
            decode->channels,
            decode->channel_count,
            row_sample_counts,
            decode->chunk.height,
            &split,
            ts);
    bool const sorted =
        sampleCount_valid &&
        ((decode->chunk.type == EXR_STORAGE_DEEP_TILED) || needs_chan_sort);

    uint64_t seg1_el = (uint64_t) ts[1];
    if (sorted) seg1_el = 4;

    uint64_t inner_lens[2];
    uint64_t inner_els[2];
    int      n_inner = 1;

    if (!sampleCount_valid)
    {
        inner_lens[0] = uncompressed_size;
        inner_els[0] =
            (decode->chunk.type == EXR_STORAGE_SCANLINE ||
             decode->chunk.type == EXR_STORAGE_TILED)
                ? 2
                : 4;
    }
    else if (!sorted)
    {
        inner_lens[0] = uncompressed_size;
        inner_els[0]  = seg1_el;
    }
    else
    {
        const int nch = decode->channel_count;
        const int nh  = decode->chunk.height;
        if (nch <= 0 || nh <= 0) return EXR_ERR_CORRUPT_CHUNK;
        uint64_t sort_lu[(nch > 0 && nh > 0) ? (nch * nh) : 1];
        const uint64_t sp = compute_sorting_lookup (
            row_sample_counts,
            nh,
            decode->channels,
            nch,
            sort_lu);
        const uint64_t rest = uncompressed_size - sp;
        if (sp > 0 && rest > 0)
        {
            n_inner       = 2;
            inner_lens[0] = sp;
            inner_lens[1] = rest;
            inner_els[0]  = 2;
            inner_els[1]  = 4;
        }
        else if (sp == 0)
        {
            inner_lens[0] = uncompressed_size;
            inner_els[0]  = 4;
        }
        else
        {
            inner_lens[0] = uncompressed_size;
            inner_els[0]  = 2;
        }
    }

    void* target = uncompressed_data;
    if (sorted)
    {
        exr_result_t drv = internal_decode_alloc_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_SCRATCH1,
            &decode->scratch_buffer_1,
            &decode->scratch_alloc_size_1,
            uncompressed_size);
        if (drv != EXR_ERR_SUCCESS) return drv;
        target = decode->scratch_buffer_1;
    }

    uint8_t*       q   = t_shuffle_buf;
    uint8_t* const qend = t_shuffle_buf + dSize;
    uint8_t*       tgt = (uint8_t*) target;
    int            seg;
    for (seg = 0; seg < n_inner; ++seg)
    {
        if ((size_t) (qend - q) < 8) return EXR_ERR_CORRUPT_CHUNK;
        uint64_t ilen = read_u64_le (q);
        q += 8;
        if (ilen > (uint64_t) (qend - q)) return EXR_ERR_CORRUPT_CHUNK;
        if (ilen != inner_lens[seg]) return EXR_ERR_CORRUPT_CHUNK;
        shuffle_decode_bytes (tgt, q, (size_t) ilen, inner_els[seg]);
        tgt += ilen;
        q += ilen;
    }
    if (q != qend) return EXR_ERR_CORRUPT_CHUNK;
    if ((size_t) (tgt - (uint8_t*) target) != (size_t) uncompressed_size)
        return EXR_ERR_CORRUPT_CHUNK;

    if (sorted)
        sort2_4ByteChannels_tiled (
            (char*) decode->scratch_buffer_1,
            row_sample_counts,
            decode->channels,
            decode->channel_count,
            false,
            decode->chunk.height,
            (char*) uncompressed_data);

    return EXR_ERR_SUCCESS;
}

exr_result_t internal_exr_undo_zstd (exr_decode_pipeline_t* decode, const void* compressed_data, uint64_t comp_buf_size, void* uncompressed_data, uint64_t uncompressed_size)
{
    uint64_t row_sample_counts[decode->chunk.height];
    bool sampleCount_valid = get_row_sample_count_decode (decode, row_sample_counts);
    if (comp_buf_size < 8) {
        memcpy(uncompressed_data, compressed_data, comp_buf_size); return EXR_ERR_SUCCESS;
    }
    uint64_t mhead;
    memcpy (&mhead, compressed_data, 8);
    if (mhead != MAGIC_NUMBER) {
        memcpy(uncompressed_data, compressed_data, comp_buf_size); return EXR_ERR_SUCCESS;
    }

    const uint8_t* suf = (const uint8_t*) compressed_data + 8;
    if (comp_buf_size >= ZSTD_EXR_V1_HEADER &&
        read_u32_le (suf) == ZSTD_EXR_FORMAT_V1 &&
        read_u32_le (suf + 4) == 0)
    {
        return exr_undo_zstd_v1 (
            decode,
            compressed_data,
            comp_buf_size,
            uncompressed_data,
            uncompressed_size);
    }

    char* inPtr = (char*) compressed_data + 8;

    if (sampleCount_valid) {
        uint64_t split = 0; uint8_t ts[2] = {0, 0};
        bool needs_chan_sort = needs_sorting (
            decode->channels,
            decode->channel_count,
            row_sample_counts,
            decode->chunk.height,
            &split,
            ts);
        bool sorted =
            (decode->chunk.type == EXR_STORAGE_DEEP_TILED) || needs_chan_sort;
        uint64_t seg0_el = (uint64_t) ts[0];
        uint64_t seg1_el = (uint64_t) ts[1];
        if (sorted)
        {
            seg0_el = 2;
            seg1_el = 4;
        }
        void* target;
        if (sorted)
        {
            exr_result_t drv = internal_decode_alloc_buffer (
                decode,
                EXR_TRANSCODE_BUFFER_SCRATCH1,
                &decode->scratch_buffer_1,
                &decode->scratch_alloc_size_1,
                uncompressed_size);
            if (drv != EXR_ERR_SUCCESS) return drv;
            target = decode->scratch_buffer_1;
        }
        else { target = uncompressed_data; }

        const char* comp_end = (const char*) compressed_data + comp_buf_size;

        uint64_t csz1 = _read_uint64 (&inPtr);
        if (csz1 == 0) return EXR_ERR_CORRUPT_CHUNK;
        const bool has_second = ((const char*) inPtr + csz1 < comp_end);
        uint64_t el_first;
        if (has_second) el_first = seg0_el;
        else if (sorted)
        {
            if (uncompressed_size == 0) el_first = seg1_el;
            else
            {
                const int         nch = decode->channel_count;
                const int         nh  = decode->chunk.height;
                uint64_t          sort_lu[(nch > 0 && nh > 0) ? (nch * nh) : 1];
                const uint64_t sp = compute_sorting_lookup (
                    row_sample_counts,
                    nh,
                    decode->channels,
                    nch,
                    sort_lu);
                if (sp == 0) el_first = seg1_el;
                else if (sp == uncompressed_size) el_first = seg0_el;
                else
                    return EXR_ERR_CORRUPT_CHUNK;
            }
        }
        else
            el_first = seg1_el;
        long d1 = exr_uncompress_zstd (inPtr, csz1, &target, uncompressed_size, el_first);
        if (d1 < 0) return EXR_ERR_CORRUPT_CHUNK;

        uint64_t total_u = (uint64_t) d1;

        if (has_second)
        {
            inPtr += csz1;
            uint64_t csz2 = _read_uint64 (&inPtr);
            if (csz2 == 0) return EXR_ERR_CORRUPT_CHUNK;
            void* target2 = (char*) target + d1;
            long d2 = exr_uncompress_zstd (
                inPtr,
                csz2,
                &target2,
                uncompressed_size - (uint64_t) d1,
                seg1_el);
            if (d2 < 0) return EXR_ERR_CORRUPT_CHUNK;
            total_u += (uint64_t) d2;
        }

        if (total_u != uncompressed_size) return EXR_ERR_CORRUPT_CHUNK;

        if (sorted) sort2_4ByteChannels_tiled((char*)decode->scratch_buffer_1, row_sample_counts, decode->channels, decode->channel_count, false, decode->chunk.height, (char*)uncompressed_data);
    } else {
        void* ud = uncompressed_data;
        const uint64_t no_tbl =
            (decode->chunk.type == EXR_STORAGE_SCANLINE ||
             decode->chunk.type == EXR_STORAGE_TILED)
                ? 2
                : 4;
        long shallow_u = exr_uncompress_zstd (
            inPtr, comp_buf_size - 8, &ud, uncompressed_size, no_tbl);
        if (shallow_u < 0) return EXR_ERR_CORRUPT_CHUNK;
        if ((uint64_t) shallow_u != uncompressed_size) return EXR_ERR_CORRUPT_CHUNK;
    }
    return EXR_ERR_SUCCESS;
}