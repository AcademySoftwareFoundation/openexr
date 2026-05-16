//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

/*
 * This fuzzer targets the attribute-setting and header-writing functionality
 * of the OpenEXR C Core API.
 *
 * It uses FuzzedDataProvider to systematically explore different combinations
 * of EXR file structures and metadata, rather than just parsing a raw byte
 * stream as an EXR file. This "structured fuzzing" approach helps reach
 * deep code paths in the header-creation and attribute-validation logic.
 *
 * Specifically, the fuzzer:
 * 1. Randomizes the number of parts in the EXR file.
 * 2. For each part, it picks a storage type (scanline, tiled, deep, etc.).
 * 3. It then generates a random number of attributes with random names
 *    and types (int, float, string, v2i, box2i), filling them with fuzzed data.
 * 4. Finally, it attempts to write the constructed header.
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
    // It splits the fuzzer input (raw bytes) into various data types
    // (integers, strings, etc.) to guide the fuzzer through different
    // API calls and logic branches.
    FuzzedDataProvider fdp(data, size);

    exr_context_t f;
    exr_context_initializer_t cinit;
    memset(&cinit, 0, sizeof(exr_context_initializer_t));
    cinit.size = sizeof(exr_context_initializer_t);

    // Use a custom write function that just discards data, as we are
    // testing the internal state management and header construction,
    // not actual I/O performance.
    cinit.write_fn = [](exr_const_context_t ctxt, void* userdata, const void* buffer, uint64_t sz, uint64_t offset, exr_stream_error_func_ptr_t error_cb) -> int64_t {
        return (int64_t)sz;
    };

    if (exr_start_write(&f, "dummy.exr", EXR_WRITE_FILE_DIRECTLY, &cinit) != EXR_ERR_SUCCESS) {
        return 0;
    }

    // Fuzz the number of parts (1 to 3)
    int num_parts = fdp.ConsumeIntegralInRange<int>(1, 3);
    for (int p = 0; p < num_parts; ++p) {
        char part_name[20];
        snprintf(part_name, sizeof(part_name), "part%d", p);

        // Pick a random storage type
        exr_storage_t storage = (exr_storage_t)fdp.PickValueInArray({
            EXR_STORAGE_SCANLINE,
            EXR_STORAGE_TILED,
            EXR_STORAGE_DEEP_SCANLINE,
            EXR_STORAGE_DEEP_TILED
        });

        int part_idx;
        if (exr_add_part(f, part_name, storage, &part_idx) != EXR_ERR_SUCCESS) {
            continue;
        }

        exr_initialize_required_attr_simple(f, part_idx, 64, 64, EXR_COMPRESSION_NONE);

        // Fuzz additional attributes (0 to 5)
        int num_attrs = fdp.ConsumeIntegralInRange<int>(0, 5);
        for (int a = 0; a < num_attrs; ++a) {
            std::string attr_name = fdp.ConsumeRandomLengthString(16);
            if (attr_name.empty()) continue;

            // Pick a random attribute type to set and fill it with fuzzed data
            int choice = fdp.ConsumeIntegralInRange<int>(0, 5);
            switch (choice) {
                case 0:
                    exr_attr_set_int(f, part_idx, attr_name.c_str(), fdp.ConsumeIntegral<int32_t>());
                    break;
                case 1:
                    exr_attr_set_float(f, part_idx, attr_name.c_str(), fdp.ConsumeFloatingPoint<float>());
                    break;
                case 2:
                {
                    std::string s = fdp.ConsumeRandomLengthString(32);
                    exr_attr_set_string(f, part_idx, attr_name.c_str(), s.c_str());
                }
                break;
                case 3:
                {
                    exr_attr_v2i_t v = {fdp.ConsumeIntegral<int32_t>(), fdp.ConsumeIntegral<int32_t>()};
                    exr_attr_set_v2i(f, part_idx, attr_name.c_str(), &v);
                }
                break;
                case 4:
                {
                    exr_attr_box2i_t b = {{fdp.ConsumeIntegral<int32_t>(), fdp.ConsumeIntegral<int32_t>()},
                                          {fdp.ConsumeIntegral<int32_t>(), fdp.ConsumeIntegral<int32_t>()}};
                    exr_attr_set_box2i(f, part_idx, attr_name.c_str(), &b);
                }
                break;
                case 5:
                {
                    const char* choices[] = {"increasingY", "decreasingY", "randomY"};
                    exr_attr_set_string(f, part_idx, attr_name.c_str(), fdp.PickValueInArray(choices));
                }
                break;
            }
        }
    }

    // This calls the internal header-writing and validation logic
    exr_write_header(f);
    exr_finish(&f);

    return 0;
}
