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

#if defined(_WIN32) || defined(_WIN64)
#    include <windows.h>
#else
#    include <pthread.h>
#endif

/* Use Zstd directly */
#include <zstd.h>

extern void exr_zstd_shuffle_decode_bytes (
    uint8_t* dst, const uint8_t* shuf, size_t dSize, uint64_t shuffle_el_bytes);
extern void
exr_zstd_shuffle_encode_4 (const uint8_t* in, size_t size, uint8_t* out);
extern void
exr_zstd_shuffle_encode_2 (const uint8_t* in, size_t size, uint8_t* out);

extern void delta_encode_row_u16 (uint8_t* p, uint64_t n);
extern void delta_decode_row_u16 (uint8_t* p, uint64_t n);
extern void delta_encode_row_u32 (uint8_t* p, uint64_t n);
extern void delta_decode_row_u32 (uint8_t* p, uint64_t n);

#define RETURN_ERRORV(pipeline, err_code, msg, ...)                            \
    {                                                                          \
        exr_const_context_t pctxt = pipeline->context;                         \
        if (pctxt) pctxt->print_error (pctxt, err_code, msg, __VA_ARGS__);     \
        return err_code;                                                       \
    }

/* Thread-local ZSTD contexts and shuffle buffer: pthread key / Windows FLS + destructor
 * frees them on thread exit (short-lived worker threads would otherwise leak). */
typedef struct exr_zstd_tls_state
{
    ZSTD_CCtx* cctx;
    ZSTD_DCtx* dctx;
    uint8_t*   shuffle_buf;
    size_t     shuffle_buf_size;
} exr_zstd_tls_state;

#if defined(_WIN32) || defined(_WIN64)

static DWORD       g_exr_zstd_fls_index = FLS_OUT_OF_INDEXES;
static INIT_ONCE   g_exr_zstd_tls_once  = INIT_ONCE_STATIC_INIT;

static VOID CALLBACK
exr_zstd_fls_destructor (PVOID lpFlsData)
{
    exr_zstd_tls_state* s = (exr_zstd_tls_state*) lpFlsData;
    if (!s) return;
    if (s->cctx) ZSTD_freeCCtx (s->cctx);
    if (s->dctx) ZSTD_freeDCtx (s->dctx);
    if (s->shuffle_buf) free (s->shuffle_buf);
    free (s);
}

static BOOL CALLBACK
exr_zstd_init_fls_once (PINIT_ONCE InitOnce, PVOID Parameter, PVOID* Context)
{
    (void) InitOnce;
    (void) Parameter;
    (void) Context;
    g_exr_zstd_fls_index = FlsAlloc (exr_zstd_fls_destructor);
    return (g_exr_zstd_fls_index != FLS_OUT_OF_INDEXES);
}

static exr_zstd_tls_state*
exr_zstd_tls_get_impl (void)
{
    InitOnceExecuteOnce (
        &g_exr_zstd_tls_once, exr_zstd_init_fls_once, NULL, NULL);
    if (g_exr_zstd_fls_index == FLS_OUT_OF_INDEXES) return NULL;

    exr_zstd_tls_state* s =
        (exr_zstd_tls_state*) FlsGetValue (g_exr_zstd_fls_index);
    if (!s)
    {
        s = (exr_zstd_tls_state*) calloc (1, sizeof (*s));
        if (!s) return NULL;
        if (!FlsSetValue (g_exr_zstd_fls_index, s))
        {
            free (s);
            return NULL;
        }
    }
    return s;
}

#else /* !_WIN32 */

static pthread_key_t  g_exr_zstd_tls_key;
static pthread_once_t g_exr_zstd_tls_once = PTHREAD_ONCE_INIT;

static void
exr_zstd_tls_destructor (void* p)
{
    exr_zstd_tls_state* s = (exr_zstd_tls_state*) p;
    if (!s) return;
    if (s->cctx) ZSTD_freeCCtx (s->cctx);
    if (s->dctx) ZSTD_freeDCtx (s->dctx);
    if (s->shuffle_buf) free (s->shuffle_buf);
    free (s);
}

static void
exr_zstd_create_tls_key (void)
{
    pthread_key_create (&g_exr_zstd_tls_key, exr_zstd_tls_destructor);
}

static exr_zstd_tls_state*
exr_zstd_tls_get_impl (void)
{
    pthread_once (&g_exr_zstd_tls_once, exr_zstd_create_tls_key);
    exr_zstd_tls_state* s =
        (exr_zstd_tls_state*) pthread_getspecific (g_exr_zstd_tls_key);
    if (!s)
    {
        s = (exr_zstd_tls_state*) calloc (1, sizeof (*s));
        if (!s) return NULL;
        pthread_setspecific (g_exr_zstd_tls_key, s);
    }
    return s;
}

#endif /* !_WIN32 */

/* Fast path: avoid pthread_getspecific / FlsGetValue on every call within a thread. */
static _Thread_local exr_zstd_tls_state* t_exr_zstd_tls_fast;

static exr_zstd_tls_state*
exr_zstd_tls_get (void)
{
    if (t_exr_zstd_tls_fast) return t_exr_zstd_tls_fast;
    t_exr_zstd_tls_fast = exr_zstd_tls_get_impl ();
    return t_exr_zstd_tls_fast;
}

static void
ensure_tls_resources (size_t required_size)
{
    exr_zstd_tls_state* tls = exr_zstd_tls_get ();
    if (!tls) return;

    if (!tls->cctx) tls->cctx = ZSTD_createCCtx ();
    if (!tls->dctx) tls->dctx = ZSTD_createDCtx ();

    if (tls->shuffle_buf_size < required_size)
    {
        // 1. FREE the old buffer to avoid realloc's internal overhead
        if (tls->shuffle_buf) free (tls->shuffle_buf);

        // 2. OVERSHOOT: Allocate 25% more than needed to prevent
        // repeated reallocations if the next scanline is slightly bigger.
        tls->shuffle_buf_size = required_size + (required_size >> 2);

        // 3. ALIGN: Use aligned_alloc for 64-byte cache line alignment
        // Note: size must be a multiple of alignment for aligned_alloc
        size_t aligned_size = (tls->shuffle_buf_size + 63) & ~63;

        tls->shuffle_buf = (uint8_t*) aligned_alloc (64, aligned_size);
    }
}

/* --- Keep the existing OpenEXR specific sorting/sampling logic below --- */

/** num_samples_grid[h * channelsSize + i] = samples for channel i on chunk line h
 *  (matches default_pack in pack.c: 0 when y_samples skips the line). */
uint64_t
compute_sorting_lookup (
    const uint64_t*                  num_samples_grid,
    int                              height,
    const exr_coding_channel_info_t* channels,
    int                              channelsSize,
    uint64_t*                        sorting_lookup)
{
    uint64_t writeCount = 0;

    for (int h = 0; h < height; ++h)
    {
        for (int i = 0; i < channelsSize; ++i)
        {
            if (channels[i].bytes_per_element == 2)
            {
                *(sorting_lookup + h * channelsSize + i) = writeCount;
                writeCount +=
                    num_samples_grid[h * channelsSize + i] * (uint64_t) 2;
            }
        }
    }

    uint64_t const splitPoint = writeCount;

    for (int h = 0; h < height; ++h)
    {
        for (int i = 0; i < channelsSize; ++i)
        {
            if (channels[i].bytes_per_element == 4)
            {
                *(sorting_lookup + h * channelsSize + i) = writeCount;
                writeCount +=
                    num_samples_grid[h * channelsSize + i] * (uint64_t) 4;
            }
        }
    }
    return splitPoint;
}

/** Apply delta along samples within each channel row (layout from compute_sorting_lookup). */
static int
delta_encode_sorted_layout (
    uint8_t*                         buf,
    uint64_t                         splitPoint,
    size_t                           total_bytes,
    const uint64_t*                  num_samples_grid,
    int                              height,
    const exr_coding_channel_info_t* channels,
    int                              channelsSize)
{
    uint64_t off = 0;
    for (int h = 0; h < height; ++h)
    {
        for (int i = 0; i < channelsSize; ++i)
        {
            if (channels[i].bytes_per_element != 2) continue;
            uint64_t const n = num_samples_grid[h * channelsSize + i];
            if (n > 0) delta_encode_row_u16 (buf + off, n);
            off += (size_t) n * 2u;
        }
    }
    if (off != splitPoint) return -1;
    for (int h = 0; h < height; ++h)
    {
        for (int i = 0; i < channelsSize; ++i)
        {
            if (channels[i].bytes_per_element != 4) continue;
            uint64_t const n = num_samples_grid[h * channelsSize + i];
            if (n > 0) delta_encode_row_u32 (buf + off, n);
            off += (size_t) n * 4u;
        }
    }
    if (off != (uint64_t) total_bytes) return -1;
    return 0;
}

static int
delta_decode_sorted_layout (
    uint8_t*                         buf,
    uint64_t                         splitPoint,
    size_t                           total_bytes,
    const uint64_t*                  num_samples_grid,
    int                              height,
    const exr_coding_channel_info_t* channels,
    int                              channelsSize)
{
    uint64_t off = 0;
    for (int h = 0; h < height; ++h)
    {
        for (int i = 0; i < channelsSize; ++i)
        {
            if (channels[i].bytes_per_element != 2) continue;
            uint64_t const n = num_samples_grid[h * channelsSize + i];
            if (n > 0) delta_decode_row_u16 (buf + off, n);
            off += (size_t) n * 2u;
        }
    }
    if (off != splitPoint) return -1;
    for (int h = 0; h < height; ++h)
    {
        for (int i = 0; i < channelsSize; ++i)
        {
            if (channels[i].bytes_per_element != 4) continue;
            uint64_t const n = num_samples_grid[h * channelsSize + i];
            if (n > 0) delta_decode_row_u32 (buf + off, n);
            off += (size_t) n * 4u;
        }
    }
    if (off != (uint64_t) total_bytes) return -1;
    return 0;
}

/* sort2_4ByteChannels_tiled — copy between “packed” and “sorted” chunk layouts.
 *
 * num_samples_grid and channels match default_pack (see pack.c / unpack.c):
 * for chunk row h and channel i, num_samples_grid[h * channelsSize + i] is how
 * many samples that channel contributes on that line (0 if subsampling skips it).
 *
 * forward == true (encode, ZSTD path before shuffle):
 *
 *   Input, inPtr — packed layout. Walk rows h = 0 .. height-1. On each row,
 *   walk channels i = 0 .. channelsSize-1 in header order and append
 *   num_samples_grid[h,i] * channels[i].bytes_per_element bytes (that channel’s
 *   raw row). Zero-sample channels add nothing on that row.
 *
 *   One row as a picture (each [..] is one channel’s row bytes on that line):
 *
 *       +-------+-------+-----+-------+-------+
 *       | ch 0  | ch 1  | ... | ch n-1|       |
 *       +-------+-------+-----+-------+-------+
 *       packed row h: channels in index order
 *
 *   Output, outPtr — sorted layout. Same total length. Bytes [0, splitPoint)
 *   are every 16-bit channel row in the chunk (all half data), in the order
 *   compute_sorting_lookup assigns. Bytes [splitPoint, end) are every 32-bit
 *   channel row (all float data), same ordering rule. splitPoint is what
 *   compute_sorting_lookup returns (total half bytes in the chunk).
 *
 *   Packed vs sorted for one RGBAZ-style example (R,G,B,A half; Z float;
 *   height 2; 3 samples per channel per row; no subsampling):
 *
 *   Packed input (two rows, each row = R,G,B,A,Z blocks):
 *
 *       row0: [R][G][B][A][Z]     row1: [R][G][B][A][Z]
 *             6+6+6+6+12 bytes per row  -> 72 bytes total
 *
 *   Sorted output: all half rows first, then all float rows (splitPoint = 48):
 *
 *       [R0][G0][B0][A0][R1][G1][B1][A1] | [Z0][Z1]
 *       |<-------- 48 bytes ----------->|
 *
 * forward == false (decode): inPtr is sorted, outPtr is packed; same diagrams
 * with input and output roles swapped.
 */
uint64_t
sort2_4ByteChannels_tiled (
    const char*                      inPtr,
    const uint64_t*                  num_samples_grid,
    const exr_coding_channel_info_t* channels,
    const int                        channelsSize,
    const bool                       forward,
    int                              height,
    char*                            outPtr)
{
    uint64_t sorting_lookup[channelsSize * height];
    uint64_t splitPoint = compute_sorting_lookup (
        num_samples_grid, height, channels, channelsSize, sorting_lookup);

    uint64_t line_start_read = 0;
    for (int h = 0; h < height; ++h)
    {
        uint64_t row_byte_off = 0;
        for (int i = 0; i < channelsSize; ++i)
        {
            const uint64_t n_hi = num_samples_grid[h * channelsSize + i];
            const uint64_t writeIndex =
                *(sorting_lookup + h * channelsSize + i);
            const uint64_t readIndex = line_start_read + row_byte_off;
            const size_t   bitSize =
                (size_t) n_hi * (size_t) channels[i].bytes_per_element;
            if (bitSize > 0)
            {
                if (forward)
                    memcpy (outPtr + writeIndex, inPtr + readIndex, bitSize);
                else
                    memcpy (outPtr + readIndex, inPtr + writeIndex, bitSize);
            }
            row_byte_off += bitSize;
        }
        line_start_read += row_byte_off;
    }

    return splitPoint;
}

static const uint64_t MAGIC_NUMBER = 8248453963162350458; // "zstd-exr"

#define ZSTD_EXR_FORMAT_V1 1u
#define ZSTD_EXR_FORMAT_V2 2u
/** Reserved u32 at header+12; v1 must write 0. */
#define ZSTD_EXR_FLAG_DELTA_AFTER_SORT 1u
#define ZSTD_EXR_V1_HEADER 24u

/** Deep pixel ZSTD wire: 1 = sort+shuffle (header v1); 2 = sort+delta+shuffle (v2). */
#ifndef EXR_ZSTD_SORTED_WIRE_VERSION
#    define EXR_ZSTD_SORTED_WIRE_VERSION 2
#endif

/** Encode order: SORT → DELTA → SHUFFLE → ZSTD; decode reverses ZSTD first. */
typedef enum
{
    EXR_ZSTD_PHASE_SORT = 0,
    EXR_ZSTD_PHASE_DELTA,
    EXR_ZSTD_PHASE_SHUFFLE,
} exr_zstd_phase_t;

typedef struct
{
    uint32_t hdr_format;
    uint32_t hdr_flags;
    bool     apply_sort;
    bool     apply_delta;
    bool     apply_shuffle;
} exr_zstd_pack_pipeline;

/** ZSTD chunks always use sort (± delta) → shuffle before ZSTD; the grid describes
 *  packed sample counts per (chunk line, channel) for that inverse.
 *  (1) Deep sample-count table only — one synthetic UINT32 “channel”, width samples/row.
 *  (2) Deep pixels — same packed row width for every channel (from sample_count_table).
 *  (3) Flat scan/tile — per channel/row counts match default_pack (Y subsampling). */
typedef enum
{
    EXR_ZSTD_GRID_DEEP_SAMPLE_COUNT_TABLE = 0,
    EXR_ZSTD_GRID_DEEP_PIXELS,
    EXR_ZSTD_GRID_FLAT_SCAN_OR_TILE,
} exr_zstd_channel_grid_scenario_t;

static bool
exr_zstd_pipeline_phase_enabled (
    const exr_zstd_pack_pipeline* p, exr_zstd_phase_t ph)
{
    switch (ph)
    {
        case EXR_ZSTD_PHASE_SORT: return p->apply_sort;
        case EXR_ZSTD_PHASE_DELTA: return p->apply_delta;
        case EXR_ZSTD_PHASE_SHUFFLE: return p->apply_shuffle;
        default: return false;
    }
}

static void
exr_zstd_build_encode_pipeline_sorted (exr_zstd_pack_pipeline* out)
{
#if EXR_ZSTD_SORTED_WIRE_VERSION >= 2
    out->hdr_format    = ZSTD_EXR_FORMAT_V2;
    out->hdr_flags     = ZSTD_EXR_FLAG_DELTA_AFTER_SORT;
    out->apply_sort    = true;
    out->apply_delta   = true;
    out->apply_shuffle = true;
#else
    out->hdr_format    = ZSTD_EXR_FORMAT_V1;
    out->hdr_flags     = 0;
    out->apply_sort    = true;
    out->apply_delta   = false;
    out->apply_shuffle = true;
#endif
}

static void
exr_zstd_build_decode_pipeline (
    uint32_t exr_fmt, uint32_t exr_flg, exr_zstd_pack_pipeline* out)
{
    memset (out, 0, sizeof (*out));
    out->hdr_format    = exr_fmt;
    out->hdr_flags     = exr_flg;
    out->apply_shuffle = true;
    out->apply_sort    = true;
    out->apply_delta =
        (exr_fmt == ZSTD_EXR_FORMAT_V2 &&
         (exr_flg & ZSTD_EXR_FLAG_DELTA_AFTER_SORT) != 0);
}

/** True when decompressing the packed sample-count table chunk, not pixel data. */
static bool
exr_zstd_decode_is_sample_count_chunk (
    const exr_decode_pipeline_t* decode,
    const void*                  compressed_data,
    uint64_t                     uncompressed_size)
{
    if (!decode->packed_sample_count_table) return false;
    const uint64_t st_bytes =
        (uint64_t) decode->chunk.width * (uint64_t) decode->chunk.height * 4u;
    return (
        compressed_data == (const void*) decode->packed_sample_count_table &&
        uncompressed_size == st_bytes);
}

static void
exr_zstd_one_channel_u32 (exr_coding_channel_info_t* ch)
{
    memset (ch, 0, sizeof (*ch));
    ch->bytes_per_element = 4;
}

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

/** ZSTD inner stream (pre-ZSTD): u64 LE uncompressed segment length, then shuffled bytes. */
static int
zstd_inner_append_shuffled_segment (
    uint8_t*    inner,
    size_t*     pos,
    size_t      inner_cap,
    const char* src,
    size_t      src_len,
    uint64_t    typeSize)
{
    if (*pos + 8 > inner_cap) return -1;
    if (src_len > (size_t) INT_MAX) return -1;
    write_u64_le (inner + *pos, (uint64_t) src_len);
    *pos += 8;
    if (*pos + src_len > inner_cap) return -1;
    if (src_len == 0) return 0;
    if (typeSize == 4)
        exr_zstd_shuffle_encode_4 ((const uint8_t*) src, src_len, inner + *pos);
    else
        exr_zstd_shuffle_encode_2 ((const uint8_t*) src, src_len, inner + *pos);
    *pos += src_len;
    return 0;
}

/** Read one inner segment: u64 LE length (must equal \a expected_len), then unshuffle. */
static exr_result_t
zstd_inner_read_shuffled_segment (
    const uint8_t** q,
    const uint8_t*  qend,
    uint8_t*        tgt,
    uint64_t        expected_len,
    uint64_t        el_bytes)
{
    if ((size_t) (qend - *q) < 8) return EXR_ERR_CORRUPT_CHUNK;
    uint64_t ilen = read_u64_le (*q);
    *q += 8;
    if (ilen > (uint64_t) (qend - *q)) return EXR_ERR_CORRUPT_CHUNK;
    if (ilen != expected_len) return EXR_ERR_CORRUPT_CHUNK;
    exr_zstd_shuffle_decode_bytes (tgt, *q, (size_t) ilen, el_bytes);
    *q += ilen;
    return EXR_ERR_SUCCESS;
}

/** 24-byte EXR ZSTD prefix: magic @0, hdr_format @8, hdr_flags @12, zstd size @16 LE. */
static void
exr_zstd_write_exr_frame_prefix (
    uint8_t*  out,
    uint32_t  hdr_format,
    uint32_t  hdr_flags,
    size_t    zstd_payload_size,
    uint64_t* out_total)
{
    memcpy (out, &MAGIC_NUMBER, sizeof (MAGIC_NUMBER));
    write_u32_le (out + 8, hdr_format);
    write_u32_le (out + 12, hdr_flags);
    write_u64_le (out + 16, (uint64_t) zstd_payload_size);
    *out_total = ZSTD_EXR_V1_HEADER + (uint64_t) zstd_payload_size;
}

/** On-disk 24-byte EXR ZSTD chunk prefix (all fields little-endian). */
typedef struct
{
    uint64_t magic;
    uint32_t format;
    uint32_t flags;
    uint64_t zstd_payload_size;
} exr_zstd_frame_header_t;

static void
exr_zstd_read_exr_frame_header (const uint8_t* buf, exr_zstd_frame_header_t* h)
{
    h->magic             = read_u64_le (buf);
    h->format            = read_u32_le (buf + 8);
    h->flags             = read_u32_le (buf + 12);
    h->zstd_payload_size = read_u64_le (buf + 16);
}

/** Returns 0 on success; sets *out_total to bytes written (header + ZSTD).
 *  hdr_format / hdr_flags: u32 LE at +8 / +12 after magic (see ZSTD_EXR_FORMAT_*). */
static int
zstd_write_exr_v1 (
    void*          out_buf,
    size_t         out_cap,
    const uint8_t* inner,
    size_t         inner_len,
    int32_t        level,
    uint32_t       hdr_format,
    uint32_t       hdr_flags,
    uint64_t*      out_total)
{
    if (out_cap < ZSTD_EXR_V1_HEADER) return -1;
    exr_zstd_tls_state* tls = exr_zstd_tls_get ();
    if (!tls) return -1;
    if (!tls->cctx) tls->cctx = ZSTD_createCCtx ();
    if (!tls->cctx) return -1;
    size_t const cbound = ZSTD_compressBound (inner_len);
    if (ZSTD_isError (cbound)) return -1;
    if (cbound > out_cap - ZSTD_EXR_V1_HEADER) return -1;

    /* Reuse the context to avoid malloc/free contention across many threads. */
    ZSTD_CCtx_reset (tls->cctx, ZSTD_reset_session_only);

    uint8_t* const out   = (uint8_t*) out_buf;
    size_t const   cSize = ZSTD_compressCCtx (
        tls->cctx,
        out + ZSTD_EXR_V1_HEADER,
        out_cap - ZSTD_EXR_V1_HEADER,
        inner,
        inner_len,
        level);
    if (ZSTD_isError (cSize)) return -1;
    exr_zstd_write_exr_frame_prefix (
        out, hdr_format, hdr_flags, cSize, out_total);
    return 0;
}

bool
get_row_sample_count_decode (
    const exr_decode_pipeline_t* decode, uint64_t* row_sample_counts)
{
    if (decode->sample_count_valid == 1 && decode->chunk.width > 0)
    {
        for (int h = 0; h < decode->chunk.height; ++h)
            row_sample_counts[h] =
                (uint64_t) decode
                    ->sample_count_table[(h + 1) * decode->chunk.width - 1];
        return true;
    }
    return false;
}

bool
get_row_sample_count_encode (
    const exr_encode_pipeline_t* encode, uint64_t* row_sample_counts)
{
    if (encode->sample_count_alloc_size > 0)
    {
        for (int h = 0; h < encode->chunk.height; ++h)
            row_sample_counts[h] =
                (uint64_t) encode
                    ->sample_count_table[(h + 1) * encode->chunk.width - 1];
        return true;
    }
    return false;
}

/** Per (chunk line, channel): number of samples packed for that channel on that
 *  line — matches default_pack in pack.c (Y subsampling skips lines with 0).
 *  Grid is row-major: index = chunk_line * channel_count + channel_index. */
static void
exr_zstd_fill_channel_sample_count_grid (
    exr_zstd_channel_grid_scenario_t scenario,
    int                              chunk_line_count,
    int                              chunk_origin_y,
    int                              chunk_pixel_width,
    int                              channel_count,
    uint64_t*                        channel_sample_count_grid,
    const uint64_t*                  deep_row_end_sample_exclusive,
    const exr_coding_channel_info_t* channels)
{
    switch (scenario)
    {
        case EXR_ZSTD_GRID_DEEP_SAMPLE_COUNT_TABLE:
            for (int chunk_line = 0; chunk_line < chunk_line_count;
                 ++chunk_line)
                channel_sample_count_grid[chunk_line * channel_count + 0] =
                    (uint64_t) chunk_pixel_width;
            break;
        case EXR_ZSTD_GRID_DEEP_PIXELS:
            for (int chunk_line = 0; chunk_line < chunk_line_count;
                 ++chunk_line)
            {
                uint64_t const channel_sample_count_this_row =
                    deep_row_end_sample_exclusive[chunk_line];
                for (int channel_index = 0; channel_index < channel_count;
                     ++channel_index)
                    channel_sample_count_grid
                        [chunk_line * channel_count + channel_index] =
                            channel_sample_count_this_row;
            }
            break;
        case EXR_ZSTD_GRID_FLAT_SCAN_OR_TILE:
            for (int chunk_line = 0; chunk_line < chunk_line_count;
                 ++chunk_line)
            {
                int const image_line_y = chunk_line + chunk_origin_y;
                for (int channel_index = 0; channel_index < channel_count;
                     ++channel_index)
                {
                    const exr_coding_channel_info_t* channel =
                        channels + channel_index;
                    uint64_t channel_sample_count_on_this_line = 0;
                    if (channel->height > 0)
                    {
                        if (channel->y_samples > 1)
                        {
                            if ((image_line_y % channel->y_samples) == 0)
                                channel_sample_count_on_this_line =
                                    (uint64_t) channel->width;
                        }
                        else
                        {
                            channel_sample_count_on_this_line =
                                (uint64_t) channel->width;
                        }
                    }
                    channel_sample_count_grid
                        [chunk_line * channel_count + channel_index] =
                            channel_sample_count_on_this_line;
                }
            }
            break;
    }
}

/* --- Main Pipeline Implementation --- */

/** Store raw packed payload as the chunk (no ZSTD); used only when ZSTD
 *  output (including 24-byte EXR header) is not strictly smaller than `packed`. */
static void
exr_zstd_encode_store_raw_chunk (exr_encode_pipeline_t* encode, size_t packed)
{
    memcpy (encode->compressed_buffer, encode->packed_buffer, packed);
    encode->compressed_bytes = packed;
}

/* EXR-ZSTD chunk layout (the payload for OpenEXR compression type ZSTD).
 * Everything multi-byte is little-endian.
 *
 *   byte offset
 *   |
 *   0         8        12       16        24                    24+N
 *   v         v        v        v         v                       v
 *   +---------+--------+--------+---------+-----------------------+
 *   |  magic  | format | flags  | zstd    |  ZSTD compressed      |
 *   |  u64 LE | u32 LE | u32 LE | payload |  frame (N bytes)      |
 *   |  8 B    | 4 B    | 4 B    | u64 LE  |  N = payload field    |
 *   +---------+--------+--------+---------+-----------------------+
 *
 *   magic (0..7): MAGIC_NUMBER in code (8-byte wire magic).
 *   format (8..11): 1 = wire v1, 2 = wire v2 (ZSTD_EXR_FORMAT_*).
 *   flags (12..15): must be 0 for v1; for v2, bit 0 may request delta-after-sort
 *                   (ZSTD_EXR_FLAG_DELTA_AFTER_SORT); any other flag bits must be 0.
 *   zstd_payload_size (16..23): N, length in bytes of the ZSTD frame only (not the 24-byte header).
 *
 *   Total size of the chunk body: 24 (ZSTD_EXR_V1_HEADER) + N.
 *
 *   Inner stream (bytes ZSTD_decompress writes; this is what ZSTD compresses on encode):
 *
 *   The stream is one or two back-to-back segments. Each segment has the same shape:
 *
 *       +------------------+----------------------+
 *       | u64 LE length L  |  L bytes, shuffled   |
 *       +------------------+----------------------+
 *       8 bytes            L bytes (planar shuffle of the sorted buffer)
 *
 *   "Shuffled" means Blosc-style byte planes (see exr_zstd_shuffle_encode_2 / _4 and
 *   exr_zstd_shuffle_decode_bytes): length L must be a multiple of the element size
 *   (2 or 4 bytes) for that segment.
 *
 *   How many segments, and element width:
 *
 *   - Both half (16-bit) and float (32-bit) channel data in the chunk: two segments.
 *     First segment: L = splitPoint from compute_sorting_lookup (all sorted half bytes),
 *     shuffle element size 2. Second: L = remaining byte count (all sorted float bytes),
 *     shuffle element size 4.
 *
 *   - Only 32-bit channels in the chunk: one segment, L = full uncompressed chunk size,
 *     shuffle element size 4.
 *
 *   - Only 16-bit channels (or float block empty): one segment, L = full uncompressed size,
 *     shuffle element size 2.
 *
 *   On encode, zstd_inner_append_shuffled_segment writes each segment; on decode,
 *   zstd_inner_read_shuffled_segment reads each one and unshuffles into the target buffer.
 */
exr_result_t
internal_exr_apply_zstd (exr_encode_pipeline_t* encode)
{
    uint64_t   row_sample_counts[encode->chunk.height];
    bool const sampleCount_valid =
        get_row_sample_count_encode (encode, row_sample_counts);
    int32_t      level = 5;
    exr_result_t rv    = exr_get_zstd_compression_level (
        encode->context, encode->part_index, &level);
    if (rv != EXR_ERR_SUCCESS) return rv;

    const uint64_t sample_table_bytes = (uint64_t) encode->chunk.width *
                                        (uint64_t) encode->chunk.height *
                                        sizeof (uint32_t);

    if (sampleCount_valid && encode->packed_bytes == 0)
    {
        encode->compressed_bytes = 0;
        return EXR_ERR_SUCCESS;
    }

    const size_t packed    = encode->packed_bytes;
    size_t       inner_cap = packed + 16u;
    if (inner_cap < packed) return EXR_ERR_ARGUMENT_OUT_OF_RANGE;
    if (encode->compressed_alloc_size < ZSTD_EXR_V1_HEADER)
        return EXR_ERR_ARGUMENT_OUT_OF_RANGE;

    ensure_tls_resources (inner_cap);

    exr_zstd_tls_state* tls_enc = exr_zstd_tls_get ();
    if (!tls_enc || !tls_enc->shuffle_buf)
        return EXR_ERR_COMPRESSION_FAILED;

    uint8_t* const inner     = tls_enc->shuffle_buf;
    size_t         inner_pos = 0;

    const int compressing_sample_counts_only =
        (encode->packed_sample_count_table != NULL &&
         encode->packed_buffer == encode->packed_sample_count_table &&
         packed == (size_t) sample_table_bytes);

    const exr_storage_t storage = (exr_storage_t) encode->chunk.type;
    /* Deep sample table without packed_sample_count_table linkage (e.g. C++
     * Imf deep tiled compressTile(tileRange) and the Core deep encode path
     * that calls in to compress the sample-count table directly): same
     * layout as compressing_sample_counts_only when packed == chunk
     * w*h*UINT32. Decoder pairs this via uncompressed_size match. */
    const int deep_sample_table_standalone =
        ((storage == EXR_STORAGE_DEEP_SCANLINE ||
          storage == EXR_STORAGE_DEEP_TILED) &&
         encode->sample_count_table == NULL &&
         packed == (size_t) sample_table_bytes);

    const int use_sample_count_pack_layout =
        compressing_sample_counts_only || deep_sample_table_standalone;

    const int is_flat =
        (storage == EXR_STORAGE_SCANLINE || storage == EXR_STORAGE_TILED);

    const exr_coding_channel_info_t* pack_channels;
    int                              pack_channel_count;
    exr_coding_channel_info_t        pack_one_channel;
    exr_zstd_channel_grid_scenario_t grid_scenario;

    if (use_sample_count_pack_layout)
    {
        /* (1) deep sample-count table: chunk w/h match table (Core or Imf path) */
        exr_zstd_one_channel_u32 (&pack_one_channel);
        pack_one_channel.width  = encode->chunk.width;
        pack_one_channel.height = encode->chunk.height;
        pack_channels           = &pack_one_channel;
        pack_channel_count      = 1;
        grid_scenario           = EXR_ZSTD_GRID_DEEP_SAMPLE_COUNT_TABLE;
    }
    else if (sampleCount_valid)
    {
        pack_channels      = encode->channels;
        pack_channel_count = encode->channel_count;
        grid_scenario      = EXR_ZSTD_GRID_DEEP_PIXELS;
    }
    else if (is_flat)
    {
        pack_channels      = encode->channels;
        pack_channel_count = encode->channel_count;
        grid_scenario      = EXR_ZSTD_GRID_FLAT_SCAN_OR_TILE;
    }
    else
        return EXR_ERR_INVALID_ARGUMENT;

    int const chunk_line_count = encode->chunk.height;
    int const channel_count    = pack_channel_count;
    if (chunk_line_count <= 0 || channel_count <= 0)
        return EXR_ERR_INVALID_ARGUMENT;

    uint64_t channel_sample_count_grid
        [(size_t) chunk_line_count * (size_t) channel_count];

    exr_zstd_fill_channel_sample_count_grid (
        grid_scenario,
        chunk_line_count,
        encode->chunk.start_y,
        encode->chunk.width,
        channel_count,
        channel_sample_count_grid,
        row_sample_counts,
        pack_channels);

    int const pipeline_height = encode->chunk.height;

    uint64_t               splitPos  = 0;
    const uint8_t          tSizes[2] = {2, 4};
    char*                  inData    = (char*) encode->packed_buffer;
    exr_zstd_pack_pipeline pipe;
    memset (&pipe, 0, sizeof (pipe));
    exr_zstd_build_encode_pipeline_sorted (&pipe);
    exr_result_t arv = internal_encode_alloc_buffer (
        encode,
        EXR_TRANSCODE_BUFFER_SCRATCH1,
        &encode->scratch_buffer_1,
        &encode->scratch_alloc_size_1,
        encode->packed_bytes);
    if (arv != EXR_ERR_SUCCESS) return arv;
    if (exr_zstd_pipeline_phase_enabled (&pipe, EXR_ZSTD_PHASE_SORT))
        splitPos = sort2_4ByteChannels_tiled (
            (const char*) encode->packed_buffer,
            channel_sample_count_grid,
            pack_channels,
            pack_channel_count,
            true,
            pipeline_height,
            (char*) encode->scratch_buffer_1);
    if (exr_zstd_pipeline_phase_enabled (&pipe, EXR_ZSTD_PHASE_DELTA))
    {
        if (delta_encode_sorted_layout (
                (uint8_t*) encode->scratch_buffer_1,
                splitPos,
                packed,
                channel_sample_count_grid,
                pipeline_height,
                pack_channels,
                pack_channel_count) != 0)
            return EXR_ERR_COMPRESSION_FAILED;
    }
    inData = (char*) encode->scratch_buffer_1;

    inner_pos = 0;
    if (exr_zstd_pipeline_phase_enabled (&pipe, EXR_ZSTD_PHASE_SHUFFLE))
    {
        if (splitPos > 0)
        {
            if (zstd_inner_append_shuffled_segment (
                    inner,
                    &inner_pos,
                    inner_cap,
                    inData,
                    (size_t) splitPos,
                    tSizes[0]) != 0)
                return EXR_ERR_COMPRESSION_FAILED;
        }
        size_t const s2_in = packed - (size_t) splitPos;
        if (s2_in > 0)
        {
            if (zstd_inner_append_shuffled_segment (
                    inner,
                    &inner_pos,
                    inner_cap,
                    inData + splitPos,
                    s2_in,
                    tSizes[1]) != 0)
                return EXR_ERR_COMPRESSION_FAILED;
        }
    }
    uint64_t total_w = 0;
    if (zstd_write_exr_v1 (
            encode->compressed_buffer,
            encode->compressed_alloc_size,
            inner,
            inner_pos,
            level,
            pipe.hdr_format,
            pipe.hdr_flags,
            &total_w) != 0)
        return EXR_ERR_COMPRESSION_FAILED;
    if (total_w < packed)
    {
        encode->compressed_bytes = total_w;
        return EXR_ERR_SUCCESS;
    }
    exr_zstd_encode_store_raw_chunk (encode, packed);
    return EXR_ERR_SUCCESS;
}

/** Raw packed chunks skip ZSTD entirely (see exr_zstd_encode_store_raw_chunk).
 *  Any EXR-ZSTD frame uses the same sorted-pack inverse (grid + shuffle + delta + sort). */
static exr_result_t
exr_undo_zstd_v1 (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    const uint8_t*          hdr = (const uint8_t*) compressed_data;
    exr_zstd_frame_header_t fr;
    exr_zstd_read_exr_frame_header (hdr, &fr);
    if (fr.magic != MAGIC_NUMBER) return EXR_ERR_CORRUPT_CHUNK;
    if (fr.format == ZSTD_EXR_FORMAT_V1)
    {
        if (fr.flags != 0) return EXR_ERR_CORRUPT_CHUNK;
    }
    else if (fr.format == ZSTD_EXR_FORMAT_V2)
    {
        if ((fr.flags & ~ZSTD_EXR_FLAG_DELTA_AFTER_SORT) != 0)
            return EXR_ERR_CORRUPT_CHUNK;
    }
    else
        return EXR_ERR_CORRUPT_CHUNK;

    if (fr.zstd_payload_size > comp_buf_size - ZSTD_EXR_V1_HEADER)
        return EXR_ERR_CORRUPT_CHUNK;
    if (ZSTD_EXR_V1_HEADER + fr.zstd_payload_size != comp_buf_size)
        return EXR_ERR_CORRUPT_CHUNK;

    const void* zsrc = hdr + ZSTD_EXR_V1_HEADER;
    size_t      zsz  = (size_t) fr.zstd_payload_size;

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

    exr_zstd_tls_state* tls_dec = exr_zstd_tls_get ();
    if (!tls_dec || !tls_dec->shuffle_buf)
        return EXR_ERR_CORRUPT_CHUNK;
    if (!tls_dec->dctx) tls_dec->dctx = ZSTD_createDCtx ();
    if (!tls_dec->dctx) return EXR_ERR_CORRUPT_CHUNK;

    ZSTD_DCtx_reset (tls_dec->dctx, ZSTD_reset_session_only);
    size_t dSize = ZSTD_decompressDCtx (
        tls_dec->dctx, tls_dec->shuffle_buf, dst_cap, zsrc, zsz);
    if (ZSTD_isError (dSize)) return EXR_ERR_CORRUPT_CHUNK;

    uint64_t   row_sample_counts[decode->chunk.height];
    bool const sampleCount_valid =
        get_row_sample_count_decode (decode, row_sample_counts);
    const bool is_sample_count_chunk = exr_zstd_decode_is_sample_count_chunk (
        decode, compressed_data, uncompressed_size);
    const exr_storage_t dstorage = (exr_storage_t) decode->chunk.type;
    const bool          is_flat =
        (dstorage == EXR_STORAGE_SCANLINE || dstorage == EXR_STORAGE_TILED);

    if (!(is_sample_count_chunk || sampleCount_valid || is_flat))
        return EXR_ERR_CORRUPT_CHUNK;

    exr_coding_channel_info_t        pack_one_channel;
    const exr_coding_channel_info_t* pack_channels      = decode->channels;
    int                              pack_channel_count = decode->channel_count;
    exr_zstd_channel_grid_scenario_t grid_scenario;

    if (is_sample_count_chunk)
    {
        exr_zstd_one_channel_u32 (&pack_one_channel);
        pack_channels      = &pack_one_channel;
        pack_channel_count = 1;
        grid_scenario      = EXR_ZSTD_GRID_DEEP_SAMPLE_COUNT_TABLE;
    }
    else if (sampleCount_valid)
    {
        pack_channels      = decode->channels;
        pack_channel_count = decode->channel_count;
        grid_scenario      = EXR_ZSTD_GRID_DEEP_PIXELS;
    }
    else
    {
        pack_channels      = decode->channels;
        pack_channel_count = decode->channel_count;
        grid_scenario      = EXR_ZSTD_GRID_FLAT_SCAN_OR_TILE;
    }

    int const chunk_line_count = decode->chunk.height;
    int const channel_count    = pack_channel_count;

    if (chunk_line_count <= 0 || channel_count <= 0)
        return EXR_ERR_CORRUPT_CHUNK;

    uint64_t channel_sample_count_grid
        [(size_t) chunk_line_count * (size_t) channel_count];

    exr_zstd_fill_channel_sample_count_grid (
        grid_scenario,
        chunk_line_count,
        decode->chunk.start_y,
        decode->chunk.width,
        channel_count,
        channel_sample_count_grid,
        row_sample_counts,
        pack_channels);

    exr_zstd_pack_pipeline pipe;
    exr_zstd_build_decode_pipeline (fr.format, fr.flags, &pipe);

    uint64_t inner_lens[2];
    uint64_t inner_els[2];
    int      n_inner = 1;

    {
        const int      nch = pack_channel_count;
        const int      nh  = decode->chunk.height;
        uint64_t       sort_lu[(size_t) nch * (size_t) nh];
        const uint64_t sp = compute_sorting_lookup (
            channel_sample_count_grid, nh, pack_channels, nch, sort_lu);
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
    if (pipe.apply_sort)
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

    const uint8_t*       q    = (const uint8_t*) tls_dec->shuffle_buf;
    const uint8_t* const qend = (const uint8_t*) tls_dec->shuffle_buf + dSize;
    uint8_t*             tgt  = (uint8_t*) target;
    int                  seg;
    if (exr_zstd_pipeline_phase_enabled (&pipe, EXR_ZSTD_PHASE_SHUFFLE))
    {
        for (seg = 0; seg < n_inner; ++seg)
        {
            exr_result_t seg_rv = zstd_inner_read_shuffled_segment (
                &q, qend, tgt, inner_lens[seg], inner_els[seg]);
            if (seg_rv != EXR_ERR_SUCCESS) return seg_rv;
            tgt += inner_lens[seg];
        }
        if (q != qend) return EXR_ERR_CORRUPT_CHUNK;
        if ((size_t) (tgt - (uint8_t*) target) != (size_t) uncompressed_size)
            return EXR_ERR_CORRUPT_CHUNK;
    }
    else
    {
        if (dSize != uncompressed_size) return EXR_ERR_CORRUPT_CHUNK;
        memcpy (target, tls_dec->shuffle_buf, dSize);
    }

    if (pipe.apply_sort)
    {
        uint64_t split_for_delta = 0;
        if (n_inner == 2)
            split_for_delta = inner_lens[0];
        else if (n_inner == 1)
        {
            if (inner_els[0] == 2)
                split_for_delta = uncompressed_size;
            else
                split_for_delta = 0;
        }
        if (exr_zstd_pipeline_phase_enabled (&pipe, EXR_ZSTD_PHASE_DELTA))
        {
            if (delta_decode_sorted_layout (
                    (uint8_t*) target,
                    split_for_delta,
                    (size_t) uncompressed_size,
                    channel_sample_count_grid,
                    decode->chunk.height,
                    pack_channels,
                    pack_channel_count) != 0)
                return EXR_ERR_CORRUPT_CHUNK;
        }
        if (exr_zstd_pipeline_phase_enabled (&pipe, EXR_ZSTD_PHASE_SORT))
            sort2_4ByteChannels_tiled (
                (char*) decode->scratch_buffer_1,
                channel_sample_count_grid,
                pack_channels,
                pack_channel_count,
                false,
                decode->chunk.height,
                (char*) uncompressed_data);
    }

    return EXR_ERR_SUCCESS;
}

exr_result_t
internal_exr_undo_zstd (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    if (comp_buf_size == 0) return EXR_ERR_SUCCESS;

    const uint8_t* hdr = (const uint8_t*) compressed_data;

    /* Raw-store fallback: encoder skips the 24-byte ZSTD-EXR header when
     * compressed output would not be strictly smaller than the packed input
     * (see exr_zstd_encode_store_raw_chunk). Detect by missing magic. */
    if (comp_buf_size < ZSTD_EXR_V1_HEADER ||
        read_u64_le (hdr) != MAGIC_NUMBER)
    {
        if (comp_buf_size != uncompressed_size) return EXR_ERR_CORRUPT_CHUNK;
        memcpy (uncompressed_data, compressed_data, (size_t) comp_buf_size);
        return EXR_ERR_SUCCESS;
    }

    exr_zstd_frame_header_t fr;
    exr_zstd_read_exr_frame_header (hdr, &fr);
    if (!((fr.format == ZSTD_EXR_FORMAT_V1 && fr.flags == 0) ||
          (fr.format == ZSTD_EXR_FORMAT_V2 &&
           (fr.flags & ~ZSTD_EXR_FLAG_DELTA_AFTER_SORT) == 0)))
        return EXR_ERR_CORRUPT_CHUNK;

    return exr_undo_zstd_v1 (
        decode,
        compressed_data,
        comp_buf_size,
        uncompressed_data,
        uncompressed_size);
}