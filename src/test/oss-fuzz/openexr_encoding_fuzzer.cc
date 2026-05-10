//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

/*
 * This fuzzer targets the OpenEXR C Core encoding pipeline.
 *
 * It uses FuzzedDataProvider to explore the vast parameter space of EXR
 * encoding, including different storage types, compression methods,
 * pixel types, and channel configurations.
 *
 * The fuzzer:
 * 1. Randomizes the storage type (scanline, tiled, deep).
 * 2. Randomizes the compression method (including HTJ2K, ZIP, PIZ, etc.).
 * 3. Randomizes image dimensions and channel count.
 * 4. Initializes an encoding pipeline and fills it with fuzzed pixel data.
 * 5. Runs the encoder to exercise the various compression codecs and
 *    the core encoding logic.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <vector>
#include <iostream>

#include <openexr.h>
#include <fuzzer/FuzzedDataProvider.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // FuzzedDataProvider is a utility integrated into clang via the
    // compiler-rt project (see: https://github.com/llvm/llvm-project/blob/main/compiler-rt/include/fuzzer/FuzzedDataProvider.h).
    FuzzedDataProvider fdp(data, size);

    exr_context_t f;
    exr_context_initializer_t cinit;
    memset(&cinit, 0, sizeof(exr_context_initializer_t));
    cinit.size = sizeof(exr_context_initializer_t);
    cinit.zip_level = -2;
    cinit.dwa_quality = -1.0f;

    // Custom write function that discards data
    cinit.write_fn = [](exr_const_context_t ctxt, void* userdata, const void* buffer, uint64_t sz, uint64_t offset, exr_stream_error_func_ptr_t error_cb) -> int64_t {
        return (int64_t)sz;
    };

    if (exr_start_write(&f, "dummy.exr", EXR_WRITE_FILE_DIRECTLY, &cinit) != EXR_ERR_SUCCESS) {
        return 0;
    }

    // Fuzz the storage type
    exr_storage_t storage = (exr_storage_t)fdp.PickValueInArray({
        EXR_STORAGE_SCANLINE,
        EXR_STORAGE_TILED,
        EXR_STORAGE_DEEP_SCANLINE,
        EXR_STORAGE_DEEP_TILED
    });

    int part_idx;
    if (exr_add_part(f, "part1", storage, &part_idx) != EXR_ERR_SUCCESS) {
        exr_finish(&f);
        return 0;
    }

    // Fuzz the compression method
    exr_compression_t compression = (exr_compression_t)fdp.PickValueInArray({
        EXR_COMPRESSION_NONE,
        EXR_COMPRESSION_RLE,
        EXR_COMPRESSION_ZIPS,
        EXR_COMPRESSION_ZIP,
        EXR_COMPRESSION_PIZ,
        EXR_COMPRESSION_PXR24,
        EXR_COMPRESSION_B44,
        EXR_COMPRESSION_B44A,
        EXR_COMPRESSION_DWAA,
        EXR_COMPRESSION_DWAB,
        EXR_COMPRESSION_HTJ2K256,
        EXR_COMPRESSION_HTJ2K32
    });

    int32_t width = fdp.ConsumeIntegralInRange<int32_t>(1, 256);
    int32_t height = fdp.ConsumeIntegralInRange<int32_t>(1, 256);

    if (exr_initialize_required_attr_simple(f, part_idx, width, height, compression) != EXR_ERR_SUCCESS) {
        exr_finish(&f);
        return 0;
    }

    // Randomly add 1 to 4 channels with different pixel types
    int num_channels = fdp.ConsumeIntegralInRange<int>(1, 4);
    for (int i = 0; i < num_channels; ++i) {
        char chan_name[10];
        snprintf(chan_name, sizeof(chan_name), "C%d", i);
        exr_pixel_type_t ptype = (exr_pixel_type_t)fdp.PickValueInArray({
            EXR_PIXEL_UINT,
            EXR_PIXEL_HALF,
            EXR_PIXEL_FLOAT
        });
        exr_add_channel(f, part_idx, chan_name, ptype, EXR_PERCEPTUALLY_LOGARITHMIC, 1, 1);
    }

    if (storage == EXR_STORAGE_TILED || storage == EXR_STORAGE_DEEP_TILED) {
        exr_set_tile_descriptor(f, part_idx, 32, 32, EXR_TILE_ONE_LEVEL, EXR_TILE_ROUND_DOWN);
    }

    if (exr_write_header(f) != EXR_ERR_SUCCESS) {
        exr_finish(&f);
        return 0;
    }

    // Initialize encoding for a single chunk (the first one)
    exr_chunk_info_t cinfo;
    if (storage == EXR_STORAGE_SCANLINE || storage == EXR_STORAGE_DEEP_SCANLINE) {
        if (exr_write_scanline_chunk_info(f, part_idx, 0, &cinfo) != EXR_ERR_SUCCESS) {
            exr_finish(&f);
            return 0;
        }
    } else {
        if (exr_write_tile_chunk_info(f, part_idx, 0, 0, 0, 0, &cinfo) != EXR_ERR_SUCCESS) {
            exr_finish(&f);
            return 0;
        }
    }

    exr_encode_pipeline_t encoder;
    if (exr_encoding_initialize(f, part_idx, &cinfo, &encoder) != EXR_ERR_SUCCESS) {
        exr_finish(&f);
        return 0;
    }

    // Provide fuzzed pixel data for each channel
    std::vector<std::vector<uint8_t>> channel_data(encoder.channel_count);
    for (int i = 0; i < encoder.channel_count; ++i) {
        size_t chan_size = (size_t)encoder.channels[i].width * encoder.channels[i].height * 4; // Max 4 bytes per pixel
        if (chan_size > 1024*1024) chan_size = 1024*1024; // Limit size to prevent OOM
        channel_data[i] = fdp.ConsumeBytes<uint8_t>(chan_size);
        channel_data[i].resize(chan_size, 0); // Ensure the buffer is fully populated
        encoder.channels[i].encode_from_ptr = channel_data[i].data();
        encoder.channels[i].user_pixel_stride = 4; // Assume 4 for simplicity
        encoder.channels[i].user_line_stride = (int32_t)encoder.channels[i].width * 4;
    }

    if (storage == EXR_STORAGE_DEEP_SCANLINE || storage == EXR_STORAGE_DEEP_TILED) {
        // For deep data, we also need fuzzed sample counts per pixel
        size_t num_pixels = (size_t)cinfo.width * cinfo.height;
        std::vector<int32_t> sample_counts(num_pixels);
        for (size_t i = 0; i < num_pixels; ++i) {
            sample_counts[i] = fdp.ConsumeIntegralInRange<int32_t>(0, 10);
        }
        encoder.sample_count_table = sample_counts.data();
    }

    // Run the encoding pipeline
    if (exr_encoding_choose_default_routines(f, part_idx, &encoder) == EXR_ERR_SUCCESS) {
        exr_encoding_run(f, part_idx, &encoder);
    }

    exr_encoding_destroy(f, &encoder);
    exr_finish(&f);

    return 0;
}
