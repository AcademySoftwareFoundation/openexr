/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_HT_COMMON_H
#define OPENEXR_PRIVATE_HT_COMMON_H

#include <vector>
#include <float.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <half.h>
#include "openexr_coding.h"

/**
 *
 * Routines are common to the three HTJ2K compression methods:
 *  - EXR_COMPRESSION_HTJ2K256: lossless coding in blocks of 256 scanlines,
 *    using the High-Throughput block coder specified in Rec. ITU-T T.814 and
 *    ISO/IEC 15444-15.
 *  - EXR_COMPRESSION_HTJ2K32: same lossless coding as HTJ2K256, but in blocks
 *    of 32 scanlines, trading some coding efficiency for cheaper partial-buffer
 *    access.
 *  - EXR_COMPRESSION_LJ2K: same block coder and block size as HTJ2K256, but
 *    with lossy coding, producing smaller files at the cost of distortion
 *    controlled via a quality level parameter.
 *
 * EXR_COMPRESSION_LJ2K
 * --------------------
 *
 * The lossy LJ2K quality level ranges from 1 to 150 and controls the level of
 * distortion in the compressed image. Values below 97 are passed through
 * directly as the Qfactor scheme described in "Controlling JPEG 2000 image
 * quality using a single parameter (Qfactor) v2.0". Qfactor is defined over [1,
 * 100] and is tuned for lower-quality imagery. Values from 97 to 150 are an
 * OpenEXR-specific extension of that range, added to reach the higher quality
 * levels needed for OpenEXR's 32-bit float imagery. In that extended range, the
 * quality value is mapped to an explicit quantization step value via an
 * exponential model. See internal_ht.cpp for the actual Qfactor-to-Qstep
 * computation. Values around 108 are generally visually lossless over the
 * entire range of half-float values.
 *
 * As with DWAA/DWAB, lossy RGB channels are first transformed to a non-linear
 * domain before being quantized for coding (see tf_from_linear()/
 * tf_to_linear()). The transformation compresses highlights logarithmically
 * above |x| == 1. *EXPERIMENTAL* this transformation is performed by OpenJPH as
 * a native NLT LUT (type 4, tracking
 * https://github.com/aous72/OpenJPH/pull/356).
 *
 * LJ2K apply lossy compression to RGB channels only, and use lossless
 * compression otherwise.
 */

/** Indicates the kind of samples carried by a channel */
enum J2KChannelKind {
    visual,
    data
};

/** Maps a JPEG 2000 codestream component index to the corresponding OpenEXR
 *  file channel index and its byte offset within a packed raster line. */
struct CodestreamChannelInfo
{
    J2KChannelKind kind;
    int            file_index;
    size_t         raster_line_offset;
    int            scratch; // used when reading from a file
};

/** Build a codestream-to-file channel map for @p channel_count OpenEXR
 *  channels.
 *
 *  When the channels include a matching RGB triplet (e.g. R/G/B, Red/Green/Blue,
 *  or layer-prefixed variants), the first three entries of @p cs_to_file_ch are
 *  assigned to R, G, and B respectively so that the JPEG 2000 Reversible Color
 *  Transform (RCT) can be applied by the encoder.  All remaining channels follow
 *  in their original order.  When no RGB triplet is detected the channels are
 *  mapped in their original order.
 *
 *  @param channel_count  Number of channels in @p channels.
 *  @param channels       OpenEXR per-channel coding descriptors.
 *  @param cs_to_file_ch  Output map from J2K component index to file channel.
 *                        Resized to @p channel_count on return.
 *  @return true  if an RGB triplet was detected (RCT will be applied),
 *          false otherwise.
 */
bool make_channel_map (
    int                                 channel_count,
    exr_coding_channel_info_t*          channels,
    std::vector<CodestreamChannelInfo>& cs_to_file_ch);

/** Write an HTJ2K chunk header into @p buffer.
 *
 *  The header encodes the channel map so that a decoder can reconstruct the
 *  original OpenEXR channel ordering from the JPEG 2000 codestream.  It has
 *  the following on-disk layout (all integers big-endian):
 *  @code
 *    uint16_t  MAGIC  = 0x4854 ('H','T')
 *    uint32_t  PLEN          // payload length in bytes
 *    uint16_t  NCH           // number of entries in the channel map
 *    uint16_t  CS_TO_F[NCH]  // OpenEXR channel index for each J2K component
 *    // optional opaque extension bytes up to PLEN
 *    // JPEG 2000 codestream follows immediately after the header
 *  @endcode
 *
 *  @param buffer   Destination buffer; must be at least @p max_sz bytes.
 *  @param max_sz   Capacity of @p buffer in bytes.
 *  @param map      Channel map produced by make_channel_map().
 *  @return Number of bytes written; the JPEG 2000 codestream should be
 *          placed at this offset within @p buffer.
 */
size_t write_header (
    uint8_t*                                  buffer,
    size_t                                    max_sz,
    const std::vector<CodestreamChannelInfo>& map);

/** Parse an HTJ2K chunk header from @p buffer and populate the channel map.
 *
 *  Validates the magic number, reads the payload length, and decodes the
 *  per-component OpenEXR channel indices.  Extension bytes inside the payload
 *  (beyond the channel map) are silently skipped.
 *
 *  @param buffer   Chunk data; must be at least @p max_sz bytes.
 *  @param max_sz   Number of readable bytes starting at @p buffer.
 *  @param map      Populated with one entry per J2K component on success.
 *  @return Byte offset of the JPEG 2000 codestream within @p buffer, i.e. the
 *          total size of the header including its payload.
 *  @throws std::runtime_error if the magic number is absent, the header is
 *          larger than @p max_sz.
 */
size_t read_header (
    void*                               buffer,
    size_t                              max_sz,
    std::vector<CodestreamChannelInfo>& map);


/** Transforms samples on lossy RGB channels of LJ2K before they
 *  are quantized to integers for the block coder (see build_nlt_lut()).
 *
 *  This transfer function is equivalent to that used by the DWA compressor.
 *
 *  This is the inverse of tf_to_linear().
 *
 *  @param x  Linear-light value, positive, negative, or zero.
 *  @return   Value in the transfer function domain, with the same sign as
 *            @p x.
 */
inline double tf_from_linear(double x)
{
    double sign = x < 0 ? -1.0f : 1.0f;
    double v = fabs(x);
    if (v <= 1.0f)
    {
        return sign * pow(v, 1.0f / 2.2f);
    }
    return sign * (log(v) / 2.2f + 1.0f);
}

/** Inverse of tf_from_linear().
 *
 *  @param x  Value in the transfer function domain.
 *  @return   Linear-light value, with the same sign as @p x.
 */
inline double tf_to_linear(double x)
{
    double sign = x < 0 ? -1.0f : 1.0f;
    double v = fabs(x);
    if (v <= 1.0f)
    {
        return sign * pow(v, 2.2f);
    }
    return sign * exp(2.2f * (v - 1.0f));
}

/**
 * Native OpenJPH NLT type 4, (LUT + binary-complement-to-sign- magnitude)
 */

/** Implements sign-to-magnitude conversion as specified in Rec. ITU-T T.801 */
static inline int64_t nlt_smag(int64_t v, int bit_depth)
{
    int64_t bias = ((int64_t) 1 << (bit_depth - 1)) + 1;
    return (v >= 0) ? v : (-v - bias);
}

/** Implements the decoding transfer function tf_from_linear() as a LUT
 *  appropriate for 16-bit samples (float) and for
 *  ojph::param_nlt::set_nonlinear_transform() 
 *
 *  @param num_points  Number of table entries between 2 and 8192.
 *  @return            num_points table entries.
 */
inline std::vector<uint16_t> build_nlt_lut_16(int32_t num_points)
{
    static const int32_t BIT_DEPTH_LIMIT = 1 << 16;
    const double max_linear = 65504.0; // half's max finite value

    const double INT16_HALF_FACTOR = 32767.0 / tf_from_linear (max_linear);
    const double d_min  = -32767.0;
    const double d_max  = 32767.0;
    const double delta  = (d_max - d_min) / (double) (num_points - 1);

    std::vector<uint16_t> points(num_points);
    for (int32_t i = 0; i < num_points; i++)
    {
        double s      = d_min + (double) i * delta;
        double linear = tf_to_linear (s / INT16_HALF_FACTOR);

        int32_t raw_bits_signed = (int16_t) half ((float) linear).bits ();
        int32_t v_after_f       = (int32_t) nlt_smag (raw_bits_signed, 16);

        double  z = (double) v_after_f / (double) BIT_DEPTH_LIMIT + 0.5;
        points[i] = (uint16_t) llround (z * (double) (BIT_DEPTH_LIMIT - 1));
    }
    return points;
}

/** Implements the decoding transfer function tf_from_linear() as a LUT
 *  appropriate for 32-bit samples (float) and for
 *  ojph::param_nlt::set_nonlinear_transform()
 *
 *  Unlike build_nlt_lut_16(), which spans half's actual finite range, @p
 *  max_linear is caller-supplied so it can be set from the actual (or
 *  robustly-estimated) magnitude of the samples being coded, e.g. per chunk.
 *  Using a fixed max_linear such as FLT_MAX wastes nearly all of the table's
 *  resolution on magnitudes no real image reaches.
 *
 *  @param num_points   Number of table entries between 2 and 8192. Denser is
 *                      not better: OpenJPH inverts the table for encoding and
 *                      caps that inverse at 8192 entries.
 *  @param max_linear   Largest finite linear-light magnitude the table needs to
 *                      represent without saturating.
 *  @return             num_points table entries.
 */
inline std::vector<uint32_t> build_nlt_lut_32(int32_t num_points, double max_linear)
{
    static const int64_t BIT_DEPTH_LIMIT = (int64_t) 1 << 32;

    const double INT32_FLOAT_FACTOR = (double) INT32_MAX / tf_from_linear (max_linear);
    const double d_min  = -(double) INT32_MAX;
    const double d_max  = (double) INT32_MAX;
    const double delta  = (d_max - d_min) / (double) (num_points - 1);

    std::vector<uint32_t> points(num_points);
    for (int32_t i = 0; i < num_points; i++)
    {
        double s      = d_min + (double) i * delta;
        double linear = tf_to_linear (s / INT32_FLOAT_FACTOR);

        float    f = (float) linear;
        uint32_t unsigned_bits;
        memcpy (&unsigned_bits, &f, sizeof (unsigned_bits));
        int64_t raw_bits_signed = (int32_t) unsigned_bits;
        int64_t v_after_f       = nlt_smag (raw_bits_signed, 32);

        double z = (double) v_after_f / (double) BIT_DEPTH_LIMIT + 0.5;
        points[i] = (uint32_t) llround (z * (double) (BIT_DEPTH_LIMIT - 1));
    }
    return points;
}

#endif /* OPENEXR_PRIVATE_HT_COMMON_H */
