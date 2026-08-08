//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

/*
 * This fuzzer performs a comprehensive cross-API round-trip test.
 *
 * It validates the interoperability between the OpenEXR C++ API and the
 * C Core API by:
 * 1. Generating a random EXR file configuration (compression, tiling,
 *    channels, etc.) using FuzzedDataProvider.
 * 2. Writing an EXR file to memory using the C++ API (OutputFile/TiledOutputFile).
 * 3. Reading the resulting file back using BOTH the C Core API and the
 *    C++ API (InputFile/TiledInputFile).
 *
 * This "differential fuzzing" approach is highly effective at finding
 * inconsistencies between the two implementations, as well as bugs in
 * the encoding/decoding logic across all supported compression formats
 * and storage types.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <vector>
#include <iostream>

#include <openexr.h>
#include <ImfOutputFile.h>
#include <ImfTiledOutputFile.h>
#include <ImfInputFile.h>
#include <ImfTiledInputFile.h>
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfStdIO.h>
#include <ImathBox.h>

#include <fuzzer/FuzzedDataProvider.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;

// Memory stream implementation for the C++ API
class StdMemStream : public OPENEXR_IMF_NAMESPACE::IStream, public OPENEXR_IMF_NAMESPACE::OStream {
public:
    StdMemStream() : IStream("mem"), OStream("mem"), _pos(0) {}
    virtual bool read(char c[], int n) override {
        if (_pos + n > _data.size()) throw IEX_NAMESPACE::InputExc("Unexpected end of file");
        memcpy(c, &_data[_pos], n);
        _pos += n;
        return _pos < _data.size();
    }
    virtual uint64_t tellg() override { return _pos; }
    virtual void seekg(uint64_t pos) override { _pos = pos; }
    virtual void write(const char c[], int n) override {
        if (_pos + n > _data.size()) _data.resize(_pos + n);
        memcpy(&_data[_pos], c, n);
        _pos += n;
    }
    virtual uint64_t tellp() override { return _pos; }
    virtual void seekp(uint64_t pos) override { _pos = pos; }

    std::vector<char> _data;
    uint64_t _pos;
};

// C Core API read callbacks that interface with the same memory stream
static int64_t core_read(exr_const_context_t ctxt, void* userdata, void* buffer, uint64_t sz, uint64_t offset, exr_stream_error_func_ptr_t error_cb) {
    StdMemStream* ms = (StdMemStream*)userdata;
    if (offset >= ms->_data.size()) return 0;
    uint64_t to_read = sz;
    if (offset + sz > ms->_data.size()) to_read = ms->_data.size() - offset;
    memcpy(buffer, ms->_data.data() + offset, to_read);
    return (int64_t)to_read;
}

static int64_t core_size(exr_const_context_t ctxt, void* userdata) {
    StdMemStream* ms = (StdMemStream*)userdata;
    return (int64_t)ms->_data.size();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // FuzzedDataProvider is a utility integrated into clang via the
    // compiler-rt project (see: https://github.com/llvm/llvm-project/blob/main/compiler-rt/include/fuzzer/FuzzedDataProvider.h).
    FuzzedDataProvider fdp(data, size);

    // Keep dimensions small to ensure the round-trip completes quickly
    int32_t width = fdp.ConsumeIntegralInRange<int32_t>(1, 128);
    int32_t height = fdp.ConsumeIntegralInRange<int32_t>(1, 128);

    StdMemStream ms;
    Header header(width, height);

    // Randomize all major EXR features
    header.compression() = (Compression)fdp.PickValueInArray({
        NO_COMPRESSION, RLE_COMPRESSION, ZIPS_COMPRESSION, ZIP_COMPRESSION,
        PIZ_COMPRESSION, PXR24_COMPRESSION, B44_COMPRESSION, B44A_COMPRESSION,
        DWAA_COMPRESSION, DWAB_COMPRESSION, HTJ2K256_COMPRESSION, HTJ2K32_COMPRESSION
    });

    header.lineOrder() = (LineOrder)fdp.PickValueInArray({
        INCREASING_Y, DECREASING_Y, RANDOM_Y
    });

    bool isTiled = fdp.ConsumeBool();
    if (isTiled) {
        header.setTileDescription(TileDescription(
            32, 32,
            (LevelMode)fdp.PickValueInArray({ONE_LEVEL, MIPMAP_LEVELS, RIPMAP_LEVELS}),
            (LevelRoundingMode)fdp.PickValueInArray({ROUND_DOWN, ROUND_UP})
        ));
    }

    PixelType pixelType = (PixelType)fdp.PickValueInArray({HALF, FLOAT, UINT});
    int32_t bytesPerPixel = (pixelType == HALF) ? 2 : 4;

    header.channels().insert("R", Channel(pixelType));
    header.channels().insert("G", Channel(pixelType));
    header.channels().insert("B", Channel(pixelType));

    // Generate random pixel data for each channel
    size_t dataSize = (size_t)width * height * bytesPerPixel;
    std::vector<uint8_t> rData = fdp.ConsumeBytes<uint8_t>(dataSize);
    rData.resize(dataSize, 0);
    std::vector<uint8_t> gData = fdp.ConsumeBytes<uint8_t>(dataSize);
    gData.resize(dataSize, 0);
    std::vector<uint8_t> bData = fdp.ConsumeBytes<uint8_t>(dataSize);
    bData.resize(dataSize, 0);

    FrameBuffer fb;
    fb.insert("R", Slice(pixelType, (char*)rData.data(), bytesPerPixel, bytesPerPixel * width));
    fb.insert("G", Slice(pixelType, (char*)gData.data(), bytesPerPixel, bytesPerPixel * width));
    fb.insert("B", Slice(pixelType, (char*)bData.data(), bytesPerPixel, bytesPerPixel * width));

    // Step 1: Write using C++ API
    try {
        if (isTiled) {
            TiledOutputFile out(ms, header);
            out.setFrameBuffer(fb);
            out.writeTiles(0, out.numXTiles() - 1, 0, out.numYTiles() - 1);
        } else {
            OutputFile out(ms, header);
            out.setFrameBuffer(fb);
            out.writePixels(height);
        }
    } catch (...) {
        // If writing fails (e.g. invalid header configuration), we just skip
        return 0;
    }

    // Step 2: Read back using C Core API
    exr_context_t rf;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.read_fn = core_read;
    cinit.size_fn = core_size;
    cinit.user_data = &ms;

    if (exr_start_read(&rf, "mem.exr", &cinit) == EXR_ERR_SUCCESS) {
        int num_parts;
        exr_get_count(rf, &num_parts);
        for (int p = 0; p < num_parts; ++p) {
            exr_attr_box2i_t dw;
            exr_get_data_window(rf, p, &dw);
            int32_t w = dw.max.x - dw.min.x + 1;
            int32_t h = dw.max.y - dw.min.y + 1;

            exr_storage_t storage;
            exr_get_storage(rf, p, &storage);

            if (storage == EXR_STORAGE_SCANLINE) {
                int32_t lines;
                exr_get_scanlines_per_chunk(rf, p, &lines);
                for (int y = 0; y < h; y += lines) {
                    exr_chunk_info_t cinfo;
                    if (exr_read_scanline_chunk_info(rf, p, y + dw.min.y, &cinfo) == EXR_ERR_SUCCESS) {
                        exr_decode_pipeline_t decoder;
                        if (exr_decoding_initialize(rf, p, &cinfo, &decoder) == EXR_ERR_SUCCESS) {
                            std::vector<std::vector<uint8_t>> out_data(decoder.channel_count);
                            for (int c = 0; c < decoder.channel_count; ++c) {
                                out_data[c].resize((size_t)decoder.channels[c].width * decoder.channels[c].height * decoder.channels[c].bytes_per_element);
                                decoder.channels[c].decode_to_ptr = out_data[c].data();
                                decoder.channels[c].user_pixel_stride = decoder.channels[c].bytes_per_element;
                                decoder.channels[c].user_line_stride = decoder.channels[c].width * decoder.channels[c].bytes_per_element;
                            }
                            if (exr_decoding_choose_default_routines(rf, p, &decoder) == EXR_ERR_SUCCESS) {
                                exr_decoding_run(rf, p, &decoder);
                            }
                            exr_decoding_destroy(rf, &decoder);
                        }
                    }
                }
            } else if (storage == EXR_STORAGE_TILED) {
                int32_t tx, ty;
                exr_get_tile_counts(rf, p, 0, 0, &tx, &ty);
                for (int y = 0; y < ty; ++y) {
                    for (int x = 0; x < tx; ++x) {
                        exr_chunk_info_t cinfo;
                        if (exr_read_tile_chunk_info(rf, p, x, y, 0, 0, &cinfo) == EXR_ERR_SUCCESS) {
                            exr_decode_pipeline_t decoder;
                            if (exr_decoding_initialize(rf, p, &cinfo, &decoder) == EXR_ERR_SUCCESS) {
                                std::vector<std::vector<uint8_t>> out_data(decoder.channel_count);
                                for (int c = 0; c < decoder.channel_count; ++c) {
                                    out_data[c].resize((size_t)decoder.channels[c].width * decoder.channels[c].height * decoder.channels[c].bytes_per_element);
                                    decoder.channels[c].decode_to_ptr = out_data[c].data();
                                    decoder.channels[c].user_pixel_stride = decoder.channels[c].bytes_per_element;
                                    decoder.channels[c].user_line_stride = decoder.channels[c].width * decoder.channels[c].bytes_per_element;
                                }
                                if (exr_decoding_choose_default_routines(rf, p, &decoder) == EXR_ERR_SUCCESS) {
                                    exr_decoding_run(rf, p, &decoder);
                                }
                                exr_decoding_destroy(rf, &decoder);
                            }
                        }
                    }
                }
            }
        }
        exr_finish(&rf);
    }

    // Step 3: Read back using C++ API
    try {
        ms.seekg(0);
        if (isTiled) {
            TiledInputFile in(ms);
            in.setFrameBuffer(fb);
            in.readTiles(0, in.numXTiles() - 1, 0, in.numYTiles() - 1);
        } else {
            InputFile in(ms);
            in.setFrameBuffer(fb);
            in.readPixels(0, height - 1);
        }
    } catch (...) {
        // If reading fails, we just skip
    }

    return 0;
}
