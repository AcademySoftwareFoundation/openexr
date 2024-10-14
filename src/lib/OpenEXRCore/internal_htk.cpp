/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_compress.h"
#include "internal_decompress.h"

#include "internal_coding.h"
#include "internal_structs.h"

#include "internal_ht_common.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>

#ifdef KDU_AVAILABLE

#    include "openexr_compression.h"
#    include "kdu_elementary.h"
#    include "kdu_params.h"
#    include "kdu_stripe_compressor.h"
#    include "kdu_compressed.h"
#    include "kdu_file_io.h"
#    include "kdu_messaging.h"
#    include "kdu_sample_processing.h"
#    include "kdu_stripe_decompressor.h"
using namespace kdu_supp;

class mem_compressed_target : public kdu_compressed_target
{
public:
    mem_compressed_target () {}

    bool close ()
    {
        this->buf.clear ();
        return true;
    }

    bool write (const kdu_byte* buf, int num_bytes)
    {
        std::copy (buf, buf + num_bytes, std::back_inserter (this->buf));
        return true;
    }

    void set_target_size (kdu_long num_bytes) { this->buf.reserve (num_bytes); }

    bool prefer_large_writes () const { return false; }

    std::vector<uint8_t>& get_buffer () { return this->buf; }

private:
    std::vector<uint8_t> buf;
};

class error_message_handler : public kdu_core::kdu_message
{
public:
    void put_text (const char* msg) { std::cout << msg; }

    virtual void flush (bool end_of_message = false)
    {
        if (end_of_message) { std::cout << std::endl; }
    }
};

static error_message_handler error_handler;

extern "C" exr_result_t
internal_exr_undo_htk (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    exr_result_t rv = EXR_ERR_SUCCESS;

    std::vector<int> cs_to_file_ch (decode->channel_count);
    bool             isRGB = make_channel_map (
        decode->channel_count, decode->channels, cs_to_file_ch);

    std::vector<int> heights (decode->channel_count);
    std::vector<int> sample_offsets (decode->channel_count);

    int32_t width  = decode->chunk.width;
    int32_t height = decode->chunk.height;

    for (int i = 0; i < sample_offsets.size (); i++)
    {
        sample_offsets[i] = cs_to_file_ch[i] * width;
    }

    std::vector<int> row_gaps (decode->channel_count);
    std::fill (
        row_gaps.begin (), row_gaps.end (), width * decode->channel_count);

    kdu_core::kdu_customize_errors (&error_handler);

    kdu_compressed_source_buffered infile (
        (kdu_byte*) (compressed_data), comp_buf_size);

    kdu_codestream cs;
    cs.create (&infile);

    kdu_dims dims;
    cs.get_dims (0, dims, false);

    assert (width == dims.size.x);
    assert (height == dims.size.y);
    assert (decode->channel_count == cs.get_num_components ());
    assert (sizeof (int16_t) == 2);

    kdu_stripe_decompressor d;

    d.start (cs);

    std::fill (heights.begin (), heights.end (), height);
    d.pull_stripe (
        (kdu_int16*) uncompressed_data,
        heights.data (),
        sample_offsets.data (),
        NULL,
        row_gaps.data ());

    d.finish ();

    cs.destroy ();

    return rv;
}

extern "C" exr_result_t
internal_exr_apply_htk (exr_encode_pipeline_t* encode)
{
    exr_result_t rv = EXR_ERR_SUCCESS;

    std::vector<int> cs_to_file_ch (encode->channel_count);
    bool             isRGB = make_channel_map (
        encode->channel_count, encode->channels, cs_to_file_ch);

    int height = encode->chunk.height;
    int width  = encode->chunk.width;

    std::vector<int> heights (encode->channel_count);
    std::vector<int> sample_offsets (encode->channel_count);
    std::vector<int> row_gaps (encode->channel_count);
    std::fill (
        row_gaps.begin (), row_gaps.end (), width * encode->channel_count);

    assert (
        encode->packed_bytes == (encode->channel_count * 2 * height * width));

    siz_params siz;
    siz.set (Scomponents, 0, 0, encode->channel_count);
    siz.set (Sdims, 0, 0, height);
    siz.set (Sdims, 0, 1, width);
    siz.set (Nprecision, 0, 0, 16);
    siz.set (Nsigned, 0, 0, true);
    static_cast<kdu_params&> (siz).finalize ();

    kdu_codestream        codestream;
    mem_compressed_target output;

    // kdu_simple_file_target target("/tmp/out.j2c");
    // codestream.create (&siz, &target);

    codestream.create (&siz, &output);

    codestream.set_disabled_auto_comments (0xFFFFFFFF);

    kdu_params* cod = codestream.access_siz ()->access_cluster (COD_params);

    cod->set (Creversible, 0, 0, true);
    cod->set (Corder, 0, 0, Corder_RPCL);
    cod->set (Cmodes, 0, 0, Cmodes_HT);
    cod->set (Cblk, 0, 0, 32);
    cod->set (Cblk, 0, 1, 128);
    cod->set (Clevels, 0, 0, 5);
    cod->set (Cycc, 0, 0, isRGB);

    kdu_params* nlt = codestream.access_siz ()->access_cluster (NLT_params);

    nlt->set (NLType, 0, 0, NLType_SMAG);

    codestream.access_siz ()->finalize_all ();

    kdu_stripe_compressor compressor;
    compressor.start (codestream);

    std::fill (heights.begin (), heights.end (), height);
    compressor.push_stripe (
        (kdu_int16*) encode->packed_buffer,
        heights.data (),
        sample_offsets.data (),
        NULL,
        row_gaps.data ());

    compressor.finish ();

    codestream.destroy ();

    int compressed_sz = static_cast<size_t> (output.get_buffer ().size ());

    if (compressed_sz < encode->packed_bytes)
    {
        memcpy (
            encode->compressed_buffer,
            output.get_buffer ().data (),
            compressed_sz);
        encode->compressed_bytes = compressed_sz;
    }
    else { encode->compressed_bytes = encode->packed_bytes; }

    return rv;
}

#else

#error

extern "C" exr_result_t
internal_exr_undo_htk (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    return internal_exr_undo_ht (
        decode,
        compressed_data,
        comp_buf_size,
        uncompressed_data,
        uncompressed_size);
}

extern "C" exr_result_t
internal_exr_apply_htk (exr_encode_pipeline_t* encode)
{
    return internal_exr_apply_ht(encode);
}

#endif