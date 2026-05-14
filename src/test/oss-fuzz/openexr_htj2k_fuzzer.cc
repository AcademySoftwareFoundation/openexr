//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

/*
 * This fuzzer specifically targets the High Throughput JPEG 2000 (HTJ2K)
 * compression and decompression paths in OpenEXR.
 *
 * It performs a "round-trip" test:
 * 1. Construction: It creates an EXR file in memory using fuzzed dimensions
 *    and HTJ2K compression settings.
 * 2. Encoding: It fills the image with fuzzed pixel data and encodes it
 *    using the C Core API.
 * 3. Decoding: It then immediately reads the same data back from memory,
 *    exercising the HTJ2K decompression logic.
 *
 * This approach ensures that we test both the compressor and the decompressor
 * in a consistent manner, which is more effective than just fuzzing the
 * decompressor with random bytes that might not represent a valid HTJ2K stream.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <vector>

#include <openexr.h>
#include <fuzzer/FuzzedDataProvider.h>

// Custom write function that stores data in a vector to simulate a file in memory
struct WriteBuffer {
    std::vector<uint8_t> data;
};

static int64_t mem_write(exr_const_context_t ctxt, void* userdata, const void* buffer, uint64_t sz, uint64_t offset, exr_stream_error_func_ptr_t error_cb) {
    WriteBuffer* wb = (WriteBuffer*)userdata;
    if (offset + sz > wb->data.size()) {
        wb->data.resize(offset + sz);
    }
    memcpy(wb->data.data() + offset, buffer, sz);
    return (int64_t)sz;
}

static int64_t mem_read(exr_const_context_t ctxt, void* userdata, void* buffer, uint64_t sz, uint64_t offset, exr_stream_error_func_ptr_t error_cb) {
    WriteBuffer* wb = (WriteBuffer*)userdata;
    if (offset >= wb->data.size()) return 0;
    uint64_t to_read = sz;
    if (offset + sz > wb->data.size()) to_read = wb->data.size() - offset;
    memcpy(buffer, wb->data.data() + offset, to_read);
    return (int64_t)to_read;
}

static int64_t mem_size(exr_const_context_t ctxt, void* userdata) {
    WriteBuffer* wb = (WriteBuffer*)userdata;
    return (int64_t)wb->data.size();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // FuzzedDataProvider is a utility integrated into clang via the
    // compiler-rt project (see: https://github.com/llvm/llvm-project/blob/main/compiler-rt/include/fuzzer/FuzzedDataProvider.h).
    FuzzedDataProvider fdp(data, size);

    WriteBuffer wb;
    exr_context_t f;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.write_fn = mem_write;
    cinit.user_data = &wb;

    // Use a small image to keep the fuzzer fast and responsive
    int32_t width = fdp.ConsumeIntegralInRange<int32_t>(1, 128);
    int32_t height = fdp.ConsumeIntegralInRange<int32_t>(1, 128);

    // Choose between the two HTJ2K compression variants
    exr_compression_t compression = fdp.ConsumeBool() ? EXR_COMPRESSION_HTJ2K256 : EXR_COMPRESSION_HTJ2K32;

    if (exr_start_write(&f, "mem.exr", EXR_WRITE_FILE_DIRECTLY, &cinit) != EXR_ERR_SUCCESS) {
        return 0;
    }

    int part_idx;
    if (exr_add_part(f, "part1", EXR_STORAGE_SCANLINE, &part_idx) != EXR_ERR_SUCCESS) {
        exr_finish(&f);
        return 0;
    }

    if (exr_initialize_required_attr_simple(f, part_idx, width, height, compression) != EXR_ERR_SUCCESS) {
        exr_finish(&f);
        return 0;
    }

    exr_pixel_type_t pixel_type = (exr_pixel_type_t)fdp.PickValueInArray({EXR_PIXEL_HALF, EXR_PIXEL_FLOAT, EXR_PIXEL_UINT});
    int32_t bpp = (pixel_type == EXR_PIXEL_HALF) ? 2 : 4;

    // Add RGB channels
    exr_add_channel(f, part_idx, "R", pixel_type, EXR_PERCEPTUALLY_LOGARITHMIC, 1, 1);
    exr_add_channel(f, part_idx, "G", pixel_type, EXR_PERCEPTUALLY_LOGARITHMIC, 1, 1);
    exr_add_channel(f, part_idx, "B", pixel_type, EXR_PERCEPTUALLY_LOGARITHMIC, 1, 1);

    if (exr_write_header(f) != EXR_ERR_SUCCESS) {
        exr_finish(&f);
        return 0;
    }

    // Write scanlines using the encoding pipeline
    int32_t lines_per_chunk;
    exr_get_scanlines_per_chunk(f, part_idx, &lines_per_chunk);

    for (int y = 0; y < height; y += lines_per_chunk) {
        exr_chunk_info_t cinfo;
        if (exr_write_scanline_chunk_info(f, part_idx, y, &cinfo) != EXR_ERR_SUCCESS) break;

        exr_encode_pipeline_t encoder;
        if (exr_encoding_initialize(f, part_idx, &cinfo, &encoder) != EXR_ERR_SUCCESS) break;

        std::vector<std::vector<uint8_t>> chan_data(encoder.channel_count);
        for (int c = 0; c < encoder.channel_count; ++c) {
            size_t sz = (size_t)encoder.channels[c].width * encoder.channels[c].height * bpp;
            chan_data[c] = fdp.ConsumeBytes<uint8_t>(sz);
            chan_data[c].resize(sz, 0);
            encoder.channels[c].encode_from_ptr = chan_data[c].data();
            encoder.channels[c].user_pixel_stride = bpp;
            encoder.channels[c].user_line_stride = encoder.channels[c].width * bpp;
        }

        if (exr_encoding_choose_default_routines(f, part_idx, &encoder) == EXR_ERR_SUCCESS) {
            exr_encoding_run(f, part_idx, &encoder);
        }
        exr_encoding_destroy(f, &encoder);
    }

    exr_finish(&f);

    // Now read the generated HTJ2K data back to test the decoder
    exr_context_t rf;
    cinit.read_fn = mem_read;
    cinit.size_fn = mem_size;
    if (exr_start_read(&rf, "mem.exr", &cinit) == EXR_ERR_SUCCESS) {
        for (int y = 0; y < height; y += lines_per_chunk) {
            exr_chunk_info_t cinfo;
            if (exr_read_scanline_chunk_info(rf, part_idx, y, &cinfo) != EXR_ERR_SUCCESS) break;

            exr_decode_pipeline_t decoder;
            if (exr_decoding_initialize(rf, part_idx, &cinfo, &decoder) != EXR_ERR_SUCCESS) break;

            std::vector<std::vector<uint8_t>> out_data(decoder.channel_count);
            for (int c = 0; c < decoder.channel_count; ++c) {
                size_t sz = (size_t)decoder.channels[c].width * decoder.channels[c].height * bpp;
                out_data[c].resize(sz);
                decoder.channels[c].decode_to_ptr = out_data[c].data();
                decoder.channels[c].user_pixel_stride = bpp;
                decoder.channels[c].user_line_stride = decoder.channels[c].width * bpp;
            }

            if (exr_decoding_choose_default_routines(rf, part_idx, &decoder) == EXR_ERR_SUCCESS) {
                exr_decoding_run(rf, part_idx, &decoder);
            }
            exr_decoding_destroy(rf, &decoder);
        }
        exr_finish(&rf);
    }

    return 0;
}
