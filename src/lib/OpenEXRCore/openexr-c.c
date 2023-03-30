
#define ILMTHREAD_THREADING_ENABLED
#define OPENEXR_VERSION_MAJOR 3 //@OpenEXR_VERSION_MAJOR@
#define OPENEXR_VERSION_MINOR 2 //@OpenEXR_VERSION_MINOR@
#define OPENEXR_VERSION_PATCH 0 //@OpenEXR_VERSION_PATCH@

#include "openexr-c.h"

#include "OpenEXRCore/attributes.c"
#include "OpenEXRCore/base.c"
#include "OpenEXRCore/channel_list.c"
#include "OpenEXRCore/chunk.c"
#include "OpenEXRCore/coding.c"
#include "OpenEXRCore/context.c"
#include "OpenEXRCore/debug.c"
#include "OpenEXRCore/decoding.c"
#include "OpenEXRCore/encoding.c"
#include "OpenEXRCore/float_vector.c"
#include "OpenEXRCore/internal_b44_table.c"
#include "OpenEXRCore/internal_b44.c"
#include "OpenEXRCore/internal_dwa.c"
#include "OpenEXRCore/internal_huf.c"
#include "OpenEXRCore/internal_piz.c"
#include "OpenEXRCore/internal_pxr24.c"
#include "OpenEXRCore/internal_rle.c"
#include "OpenEXRCore/internal_structs.c"
#include "OpenEXRCore/internal_zip.c"
#include "OpenEXRCore/memory.c"
#include "OpenEXRCore/opaque.c"
#include "OpenEXRCore/pack.c"
#include "OpenEXRCore/parse_header.c"
#include "OpenEXRCore/part_attr.c"
#include "OpenEXRCore/part.c"
#include "OpenEXRCore/preview.c"
#include "OpenEXRCore/std_attr.c"
#include "OpenEXRCore/string_vector.c"
#include "OpenEXRCore/string.c"
#include "OpenEXRCore/unpack.c"
#include "OpenEXRCore/validation.c"
#include "OpenEXRCore/write_header.c"


#define EXR_FILE "StillLife.exr"
uint64_t gMaxBytesPerScanline = 8000000;
uint64_t gMaxTileBytes        = 8192 * 8192;

exr_result_t
readCoreScanlinePart(
    exr_context_t f, int part, bool reduceMemory, bool reduceTime)
{
    exr_result_t     rv;
    exr_attr_box2i_t datawin;
    rv = exr_get_data_window(f, part, &datawin);
    if (rv != EXR_ERR_SUCCESS) 
        return rv;

    uint64_t width =
        (uint64_t) ((int64_t) datawin.max.x - (int64_t) datawin.min.x + 1);
    uint64_t height =
        (uint64_t) ((int64_t) datawin.max.y - (int64_t) datawin.min.y + 1);

    printf("readCoreScanlinePart: Image size is %llu, %llu\n", width, height);

    uint8_t* imgdata = NULL;

    bool doread = false;
    exr_decode_pipeline_t decoder = EXR_DECODE_PIPELINE_INITIALIZER;

    int32_t lines_per_chunk;
    rv = exr_get_scanlines_per_chunk(f, part, &lines_per_chunk);
    if (rv != EXR_ERR_SUCCESS) return rv;

    for (uint64_t chunk = 0; chunk < height; chunk += lines_per_chunk)
    {
        exr_chunk_info_t cinfo = {0};
        int y = ((int) chunk) + datawin.min.y;

        rv = exr_read_scanline_chunk_info(f, part, y, &cinfo);
        if (rv != EXR_ERR_SUCCESS)
        {
            if (reduceTime) break;
            continue;
        }

        if (decoder.channels == NULL)
        {
            // first time through, allocate the decoder
            rv = exr_decoding_initialize(f, part, &cinfo, &decoder);
            if (rv != EXR_ERR_SUCCESS) break;

            uint64_t bytes = 0;
            for (int c = 0; c < decoder.channel_count; c++)
            {
                exr_coding_channel_info_t* outc = &decoder.channels[c];
                // fake addr for default routines
                outc->decode_to_ptr     = (uint8_t*) 0x1000;
                outc->user_pixel_stride = outc->user_bytes_per_element;
                outc->user_line_stride  = outc->user_pixel_stride * width;
                bytes += width * (uint64_t) outc->user_bytes_per_element *
                         (uint64_t) lines_per_chunk;
            }
/*
            printf("pixel stride = %d, line stride = %d\n", 
                decoder.channels[0].user_pixel_stride, decoder.channels[0].user_line_stride);
            printf("readCoreScanlinePart: Allocating %llu bytes for image data\n", bytes);
            printf(" width * height * bytes per element * decoder.channel_count = %llu (%llu * %llu * %d * %d)\n", 
                    width * height * decoder.channels[0].user_bytes_per_element * decoder.channel_count,
                      width, height, decoder.channels[0].user_bytes_per_element, decoder.channel_count);
*/

            doread = true;
            if (reduceMemory && bytes >= gMaxBytesPerScanline) 
                doread = false;

            if (doread) {
                imgdata = (uint8_t*) malloc(bytes);
            }
            rv = exr_decoding_choose_default_routines(f, part, &decoder);
            if (rv != EXR_ERR_SUCCESS) break;
        }
        else
        {
            // otherwise, just update the chunk info
            rv = exr_decoding_update(f, part, &cinfo, &decoder);
            if (rv != EXR_ERR_SUCCESS)
            {
                if (reduceTime) break;
                continue;
            }
        }

        if (doread)
        {
            printf("readCoreTiledPart: Reading chunk %llu\n", chunk);
            uint8_t* dptr = &(imgdata[0]);
            for (int c = 0; c < decoder.channel_count; c++)
            {
                exr_coding_channel_info_t* outc = &decoder.channels[c];
                outc->decode_to_ptr              = dptr;
                outc->user_pixel_stride          = outc->user_bytes_per_element;
                outc->user_line_stride = outc->user_pixel_stride * width;
                dptr += width * (uint64_t) outc->user_bytes_per_element *
                        (uint64_t) lines_per_chunk;
            }

            rv = exr_decoding_run(f, part, &decoder);
            if (rv != EXR_ERR_SUCCESS)
            {
                if (reduceTime) break;
            }
        }
    }

    exr_decoding_destroy(f, &decoder);
    if (imgdata != NULL)
        free(imgdata);
    return rv;
}

////////////////////////////////////////

exr_result_t
readCoreTiledPart (
    exr_context_t f, int part, bool reduceMemory, bool reduceTime)
{
    exr_result_t rv;

    exr_attr_box2i_t datawin;
    rv = exr_get_data_window(f, part, &datawin);
    if (rv != EXR_ERR_SUCCESS) return rv;

    uint32_t              txsz, tysz;
    exr_tile_level_mode_t levelmode;
    exr_tile_round_mode_t roundingmode;

    rv = exr_get_tile_descriptor(
        f, part, &txsz, &tysz, &levelmode, &roundingmode);
    if (rv != EXR_ERR_SUCCESS) return rv;

    int32_t levelsx, levelsy;
    rv = exr_get_tile_levels(f, part, &levelsx, &levelsy);
    if (rv != EXR_ERR_SUCCESS) return rv;

    bool keepgoing = true;
    for (int32_t ylevel = 0; keepgoing && ylevel < levelsy; ++ylevel)
    {
        for (int32_t xlevel = 0; keepgoing && xlevel < levelsx; ++xlevel)
        {
            int32_t levw, levh;
            rv = exr_get_level_sizes(f, part, xlevel, ylevel, &levw, &levh);
            if (rv != EXR_ERR_SUCCESS)
            {
                if (reduceTime)
                {
                    keepgoing = false;
                    break;
                }
                continue;
            }

            int32_t curtw, curth;
            rv = exr_get_tile_sizes(f, part, xlevel, ylevel, &curtw, &curth);
            if (rv != EXR_ERR_SUCCESS)
            {
                if (reduceTime)
                {
                    keepgoing = false;
                    break;
                }
                continue;
            }

            // we could make this over all levels but then would have to
            // re-check the allocation size, let's leave it here to check when
            // tile size is < full / top level tile size
            uint8_t*              tiledata = NULL;
            bool                  doread = false;
            exr_chunk_info_t      cinfo;
            exr_decode_pipeline_t decoder = EXR_DECODE_PIPELINE_INITIALIZER;

            int tx, ty;
            ty = 0;
            for (int64_t cury = 0; keepgoing && cury < levh;
                 cury += curth, ++ty)
            {
                tx = 0;
                for (int64_t curx = 0; keepgoing && curx < levw;
                     curx += curtw, ++tx)
                {
                    rv = exr_read_tile_chunk_info (
                        f, part, tx, ty, xlevel, ylevel, &cinfo);
                    if (rv != EXR_ERR_SUCCESS)
                    {
                        if (reduceTime) {
                            keepgoing = false;
                            break;
                        }
                        continue;
                    }

                    if (decoder.channels == NULL)
                    {
                        rv =
                            exr_decoding_initialize (f, part, &cinfo, &decoder);
                        if (rv != EXR_ERR_SUCCESS) {
                            keepgoing = false;
                            break;
                        }

                        uint64_t bytes = 0;
                        for (int c = 0; c < decoder.channel_count; c++) {
                            exr_coding_channel_info_t* outc =
                                &decoder.channels[c];
                            // fake addr for default routines
                            outc->decode_to_ptr = (uint8_t*) 0x1000 + bytes;
                            outc->user_pixel_stride =
                                outc->user_bytes_per_element;
                            outc->user_line_stride =
                                outc->user_pixel_stride * curtw;
                            bytes += (uint64_t) curtw *
                                     (uint64_t) outc->user_bytes_per_element *
                                     (uint64_t) curth;
                        }

                        doread = true;
                        if (reduceMemory && bytes >= gMaxTileBytes)
                            doread = false;

                        if (doread) {
                            tiledata = (uint8_t*) malloc(bytes);
                        }
                        rv = exr_decoding_choose_default_routines(f, part, &decoder);
                        if (rv != EXR_ERR_SUCCESS) {
                            keepgoing = false;
                            break;
                        }
                    }
                    else
                    {
                        rv = exr_decoding_update(f, part, &cinfo, &decoder);
                        if (rv != EXR_ERR_SUCCESS)
                        {
                            if (reduceTime) {
                                keepgoing = false;
                                break;
                            }
                            continue;
                        }
                    }

                    if (doread)
                    {
                        uint8_t* dptr = &(tiledata[0]);
                        for (int c = 0; c < decoder.channel_count; c++)
                        {
                            exr_coding_channel_info_t* outc = &decoder.channels[c];
                            outc->decode_to_ptr = dptr;
                            outc->user_pixel_stride = outc->user_bytes_per_element;
                            outc->user_line_stride = outc->user_pixel_stride * curtw;
                            dptr += (uint64_t) curtw *
                                    (uint64_t) outc->user_bytes_per_element *
                                    (uint64_t) curth;
                        }

                        rv = exr_decoding_run(f, part, &decoder);
                        if (rv != EXR_ERR_SUCCESS) {
                            if (reduceTime) {
                                keepgoing = false;
                                break;
                            }
                        }
                    }
                }
            }

            exr_decoding_destroy(f, &decoder);
            free(tiledata);
        }
    }

    return rv;
}


static void
err_cb (exr_const_context_t f, int code, const char* msg)
{
    fprintf(stderr, "err_cb ERROR %d: %s\n", code, msg);
}


nanoexr_Reader_t* nanoexr_new(const char* filename,
                              exr_context_initializer_t* init) {
    nanoexr_Reader_t* reader = (nanoexr_Reader_t*) calloc(1, sizeof(nanoexr_Reader_t));
    exr_get_library_version (&reader->exrSDKVersionMajor,
                             &reader->exrSDKVersionMinor,
                             &reader->exrSDKVersionPatch,
                             &reader->exrSDKExtraInfo);
    printf("OpenEXR Version: %d.%d.%d\n", reader->exrSDKVersionMajor,
           reader->exrSDKVersionMinor, reader->exrSDKVersionPatch);
    if (reader->exrSDKExtraInfo)
        printf("    %s\n", reader->exrSDKExtraInfo);

    reader->filename = strdup(filename);
    reader->f = NULL;
    reader->width = 0;
    reader->height = 0;
    reader->channelCount = 0;
    reader->pixelType = EXR_PIXEL_LAST_TYPE;
    reader->partIndex = 0;
    reader->mipLevels.level = 0;
    reader->isScanline = false;
    reader->tileLevelCount = 0;
    reader->tileLevelInfo = NULL;

    if (init != NULL) {
        reader->init = *init;
    } else {
        reader->init = (exr_context_initializer_t) EXR_DEFAULT_CONTEXT_INITIALIZER;
        reader->init.error_handler_fn          = &err_cb;
    }
    return reader;
}

void nanoexr_close(nanoexr_Reader_t* reader) {
    if (reader && reader->f) {
        exr_finish(&reader->f);
        reader->f = NULL;
    }
}

void nanoexr_delete(nanoexr_Reader_t* reader) {
    nanoexr_close(reader);
    free(reader->filename);
    free(reader->tileLevelInfo);
    free(reader);
}

int nanoexr_open(nanoexr_Reader_t* reader, int partIndex) {
    if (!reader)
        return EXR_ERR_INVALID_ARGUMENT;
    if (reader->f) {
        nanoexr_close(reader);
    }
    int rv = exr_start_read(&reader->f, reader->filename, &reader->init);
    if (rv != EXR_ERR_SUCCESS) {
        nanoexr_close(reader);
        return rv;
    }

    exr_attr_box2i_t datawin;
    rv = exr_get_data_window(reader->f, partIndex, &datawin);
    if (rv != EXR_ERR_SUCCESS) {
        nanoexr_close(reader);
        return rv;
    }
    reader->partIndex = partIndex;
    reader->width = datawin.max.x - datawin.min.x + 1;
    reader->height = datawin.max.y - datawin.min.y + 1;

    exr_storage_t storage;
    rv = exr_get_storage(reader->f, partIndex, &storage);
    if (rv != EXR_ERR_SUCCESS) {
        nanoexr_close(reader);
        return rv;
    }
    reader->isScanline = (storage == EXR_STORAGE_SCANLINE);

    int numMipLevelsX = 1, numMipLevelsY = 1;
    if (reader->isScanline) {
        numMipLevelsX = 1;
        numMipLevelsY = 1;
    } else {
        rv = exr_get_tile_levels(reader->f, partIndex, &numMipLevelsX, &numMipLevelsY);
        if (rv != EXR_ERR_SUCCESS) {
            nanoexr_close(reader);
            return rv;
        }
    }
    if (numMipLevelsX != numMipLevelsY) {
        // current assumption is that mips are only supproted for square images
        // should this be flagged?
        numMipLevelsX = 1;
        numMipLevelsY = 1;
    }
    reader->mipLevels.level = numMipLevelsX;

    if (!reader->isScanline) {
        reader->tileLevelCount = numMipLevelsX;
        reader->tileLevelInfo = (nanoexr_TileMipInfo_t*) calloc(sizeof(nanoexr_TileMipInfo_t), numMipLevelsX);
        for (int i = 0; i < numMipLevelsX; i++) {
            int tileWidth, tileHeight, levelWidth, levelHeight;
            rv = exr_get_tile_sizes(reader->f, partIndex, i, i, &tileWidth, &tileHeight);
            if (rv != EXR_ERR_SUCCESS) {
                nanoexr_close(reader);
                return rv;
            }
            rv = exr_get_level_sizes(reader->f, partIndex, i, i, &levelWidth, &levelHeight);
            if (rv != EXR_ERR_SUCCESS) {
                nanoexr_close(reader);
                return rv;
            }
            reader->tileLevelInfo[i].tileWidth = tileWidth;
            reader->tileLevelInfo[i].tileHeight = tileHeight;
            reader->tileLevelInfo[i].levelWidth = levelWidth;
            reader->tileLevelInfo[i].levelHeight = levelHeight;
        }
    }

    const exr_attr_chlist_t* chlist = NULL;
    rv = exr_get_channels(reader->f, partIndex, &chlist);
    if (rv != EXR_ERR_SUCCESS) {
        nanoexr_close(reader);
        return rv;
    }
    reader->channelCount = chlist->num_channels;
    reader->pixelType = chlist->entries[0].pixel_type;

    return rv;
}


bool nanoexr_isOpen(nanoexr_Reader_t* reader) {
    return reader && (reader->f != NULL);
}

int nanoexr_getWidth(nanoexr_Reader_t* reader) {
    if (!reader || !reader->f)
        return 0;
    return reader->width;
}

int nanoexr_getHeight(nanoexr_Reader_t* reader) {
    if (!reader || !reader->f)
        return 0;
    return reader->height;
}

nanoexr_MipLevel_t nanoexr_getMipLevels(nanoexr_Reader_t* reader) {
    if (!reader || !reader->f)
        return (nanoexr_MipLevel_t){0};
    return reader->mipLevels;
}

int nanoexr_getChannelCount(nanoexr_Reader_t* reader) {
    if (!reader || !reader->f)
        return 0;
    return reader->channelCount;
}

exr_pixel_type_t nanoexr_getPixelType(nanoexr_Reader_t* reader) {
    if (!reader || !reader->f)
        return EXR_PIXEL_LAST_TYPE;
    return reader->pixelType;
}


exr_result_t nanoexr_convertPixelType(exr_pixel_type_t dstType, exr_pixel_type_t srcType,
                                      int pixelCount, int channelCount,
                                      const void* src, void* dst) {
    if (srcType == dstType) {
        memcpy(dst, src, pixelCount * channelCount * nanoexr_getPixelTypeSize(srcType));
        return EXR_ERR_SUCCESS;
    }
    if (srcType == EXR_PIXEL_HALF && dstType == EXR_PIXEL_FLOAT) {
        const uint16_t* src16 = (const uint16_t*) src;
        float* dst32 = (float*) dst;
        for (int i = 0; i < pixelCount * channelCount; ++i) {
            dst32[i] = half_to_float(src16[i]);
        }
        return EXR_ERR_SUCCESS;
    }
    if (srcType == EXR_PIXEL_HALF && dstType == EXR_PIXEL_UINT) {
        const uint16_t* src16 = (const uint16_t*) src;
        uint32_t* dst32 = (uint32_t*) dst;
        for (int i = 0; i < pixelCount * channelCount; ++i) {
            dst32[i] = (uint32_t) (half_to_float(src16[i]) * 4294967295.0f);
        }
        return EXR_ERR_SUCCESS;
    }
    if (srcType == EXR_PIXEL_FLOAT && dstType == EXR_PIXEL_HALF) {
        const float* src32 = (const float*) src;
        uint16_t* dst16 = (uint16_t*) dst;
        for (int i = 0; i < pixelCount * channelCount; ++i) {
            dst16[i] = float_to_half(src32[i]);
        }
        return EXR_ERR_SUCCESS;
    }
    if (srcType == EXR_PIXEL_FLOAT && dstType == EXR_PIXEL_UINT) {
        const float* src32 = (const float*) src;
        uint32_t* dst32 = (uint32_t*) dst;
        for (int i = 0; i < pixelCount * channelCount; ++i) {
            dst32[i] = (uint32_t) (src32[i] * 4294967295.0f);
        }
        return EXR_ERR_SUCCESS;
    }
    if (srcType == EXR_PIXEL_UINT && dstType == EXR_PIXEL_HALF) {
        const uint32_t* src32 = (const uint32_t*) src;
        uint16_t* dst16 = (uint16_t*) dst;
        for (int i = 0; i < pixelCount * channelCount; ++i) {
            dst16[i] = float_to_half(src32[i] / 4294967295.0f);
        }
        return EXR_ERR_SUCCESS;
    }
    if (srcType == EXR_PIXEL_UINT && dstType == EXR_PIXEL_FLOAT) {
        const uint32_t* src32 = (const uint32_t*) src;
        float* dst32 = (float*) dst;
        for (int i = 0; i < pixelCount * channelCount; ++i) {
            dst32[i] = src32[i] / 4294967295.0f;
        }
        return EXR_ERR_SUCCESS;
    }
    return EXR_ERR_INVALID_ARGUMENT;
}


/*
    Read an entire scanline based image
*/
exr_result_t nanoexr_readScanlineData2(nanoexr_Reader_t* reader, nanoexr_ImageData_t* img) {
    exr_decode_pipeline_t decoder;
    memset(&decoder, 0, sizeof(decoder));
    int checkpoint = 0;
    int scanLinesPerChunk;
    int rv = exr_get_scanlines_per_chunk(reader->f, reader->partIndex, &scanLinesPerChunk);
    if (rv != EXR_ERR_SUCCESS)
        goto err;

    exr_attr_box2i_t datawin;
    rv = exr_get_data_window(reader->f, reader->partIndex, &datawin);
    if (rv != EXR_ERR_SUCCESS) 
        goto err;

    int window_width = datawin.max.x - datawin.min.x + 1;
    int window_height = datawin.max.y - datawin.min.y + 1;
    int yoffset = datawin.min.y;

    uint8_t* tempData = NULL;
    size_t output_bpp = nanoexr_getPixelTypeSize(img->pixelType);
    size_t bpp = nanoexr_getPixelTypeSize(reader->pixelType);
    int output_width = window_width;
    if (nanoexr_getPixelType(reader) != img->pixelType) {
        if (!bpp) {
            rv = EXR_ERR_INVALID_ARGUMENT;
            goto err;            
        }
        tempData = (uint8_t*) malloc(window_width * window_height * img->channelCount * output_bpp);
        if (!tempData) {
            rv = EXR_ERR_OUT_OF_MEMORY;
            goto err;
        }
    }
    uint8_t* imageData;
    if (tempData) {
        imageData = tempData;
        bpp = output_bpp;
    }
    else {
        imageData = (uint8_t*) img->data;
    }

    size_t offset = 0;
    size_t outputOffset = 0;
    for (int chunky = 0; chunky < window_height; chunky += scanLinesPerChunk) {
        exr_chunk_info_t chunkInfo = {0};
        int y = (int) chunky + yoffset;
        
        checkpoint = 20;
        rv = exr_read_scanline_chunk_info(reader->f, reader->partIndex, y, &chunkInfo);
        if (rv != EXR_ERR_SUCCESS)
            goto err;

        checkpoint = 30;
        rv = exr_decoding_initialize(reader->f, reader->partIndex, &chunkInfo, &decoder);
        if (rv != EXR_ERR_SUCCESS)
            goto err;
        
        int bytesPerElement = decoder.channels[0].bytes_per_element;
        for (int c = 0; c < decoder.channel_count; ++c) {
            int channelIndex = -1;
            if (strcmp(decoder.channels[c].channel_name, "R") == 0)
                channelIndex = 0;
            else if (strcmp(decoder.channels[c].channel_name, "G") == 0)
                channelIndex = 1;
            else if (strcmp(decoder.channels[c].channel_name, "B") == 0)
                channelIndex = 2;
            else if (strcmp(decoder.channels[c].channel_name, "A") == 0)
                channelIndex = 3;
            else {
                continue;   // skip this unknown channel
            }
            if (channelIndex >= img->channelCount) {
                continue; // skip channels beyond what fits in the output buffer
            }

            checkpoint = 40;
            if (decoder.channels[c].bytes_per_element != bytesPerElement) {
                rv = EXR_ERR_INVALID_ARGUMENT;
                goto err;
            }

            checkpoint = 50;
            if (decoder.channels[c].data_type != reader->pixelType) {
                rv = EXR_ERR_INVALID_ARGUMENT;
                goto err;
            }

            decoder.channels[c].decode_to_ptr = offset + imageData + channelIndex * bytesPerElement;
            decoder.channels[c].user_pixel_stride = img->channelCount * bytesPerElement;
            decoder.channels[c].user_line_stride = decoder.channels[c].user_pixel_stride * window_width;
            decoder.channels[c].user_bytes_per_element = bytesPerElement;
        }
        checkpoint = 60;
        rv = exr_decoding_choose_default_routines(reader->f, 0, &decoder);
        if (rv != EXR_ERR_SUCCESS)
            goto err;

        checkpoint = 70;
        rv = exr_decoding_run(reader->f, reader->partIndex, &decoder);
        if (rv != EXR_ERR_SUCCESS)
            goto err;
        
        checkpoint = 80;
        rv = exr_decoding_destroy(reader->f, &decoder);
        if (rv != EXR_ERR_SUCCESS)
            goto err;

        if (tempData) {
            checkpoint = 90;
            rv = nanoexr_convertPixelType(
                    img->pixelType, reader->pixelType, 
                    window_width * scanLinesPerChunk, 
                    img->channelCount, 
                    offset + imageData, outputOffset + (uint8_t*) img->data);
            if (rv != EXR_ERR_SUCCESS)
                goto err;
        }

        offset += scanLinesPerChunk * reader->width * img->channelCount * bytesPerElement;
        outputOffset += scanLinesPerChunk * reader->width * img->channelCount * bytesPerElement;
    }
    if (tempData)
        free(tempData);
    return rv;
    
err:
    if (tempData)
        free(tempData);
    exr_decoding_destroy(reader->f, &decoder);
    return rv;
}

size_t nanoexr_getPixelTypeSize(exr_pixel_type_t t) {
    switch (t) {
        case EXR_PIXEL_HALF: return 2;
        case EXR_PIXEL_FLOAT: return 4;
        case EXR_PIXEL_UINT: return 4;
        default: return 0;
    }
}


bool nanoexr_isTiled(nanoexr_Reader_t* reader) {
    if (!reader || !reader->f)
        return false;
    return !reader->isScanline;
}

/*
    Read a single tile of a tiled image at a certain miplevel
*/
int nanoexr_readTileData(nanoexr_Reader_t* reader, nanoexr_ImageData_t* img, nanoexr_MipLevel_t mipLevel, int col, int row) {
    if (!reader || !reader->f)
        return EXR_ERR_INVALID_ARGUMENT;
    if (mipLevel.level >= reader->mipLevels.level)
        return EXR_ERR_INVALID_ARGUMENT;
    if (reader->channelCount < 1 || reader->channelCount > 4)
        return EXR_ERR_INVALID_ARGUMENT;
    if (!reader->isScanline)
        return EXR_ERR_INVALID_ARGUMENT;

    int tileWidth = reader->tileLevelInfo[mipLevel.level].tileWidth;
    int tileHeight = reader->tileLevelInfo[mipLevel.level].tileHeight;
    int levelWidth = reader->tileLevelInfo[mipLevel.level].levelWidth;
    int levelHeight = reader->tileLevelInfo[mipLevel.level].levelHeight;

    int numberOfColumns = (levelWidth + tileWidth - 1) / tileWidth;
    int numberOfRows = (levelHeight + tileHeight - 1) / tileHeight;

    if (col >= numberOfColumns || row >= numberOfRows)
        return EXR_ERR_INVALID_ARGUMENT;

    exr_chunk_info_t chunkInfo;
    exr_decode_pipeline_t decoder;
    int rv = exr_read_tile_chunk_info(reader->f, reader->partIndex, 
                col, row, mipLevel.level, mipLevel.level, &chunkInfo);
    if (rv != EXR_ERR_SUCCESS)
        return rv;
    rv = exr_decoding_initialize(reader->f, reader->partIndex, &chunkInfo, &decoder);
    if (rv != EXR_ERR_SUCCESS)
        return rv;
    
    int bytesPerChannel = decoder.channels[0].bytes_per_element;
    int rowPitch = bytesPerChannel * decoder.channel_count * tileWidth;

    int bytesPerTile = rowPitch * tileHeight;
    if (bytesPerTile > img->dataSize)
        return EXR_ERR_INVALID_ARGUMENT;

    for (int c = 0; c < decoder.channel_count; ++c) {
        if (decoder.channels[c].bytes_per_element != bytesPerChannel) {
            exr_decoding_destroy(reader->f, &decoder);
            return EXR_ERR_INVALID_ARGUMENT;
        }

        if (decoder.channels[c].data_type != reader->pixelType) {
            exr_decoding_destroy(reader->f, &decoder);
            return EXR_ERR_INVALID_ARGUMENT;
        }

        int channelIndex = -1;
        if (strcmp(decoder.channels[c].channel_name, "R") == 0)
            channelIndex = 0;
        else if (strcmp(decoder.channels[c].channel_name, "G") == 0)
            channelIndex = 1;
        else if (strcmp(decoder.channels[c].channel_name, "B") == 0)
            channelIndex = 2;
        else if (strcmp(decoder.channels[c].channel_name, "A") == 0)
            channelIndex = 3;
        else {
            exr_decoding_destroy(reader->f, &decoder);
            return EXR_ERR_INVALID_ARGUMENT;
        }

        decoder.channels[c].decode_to_ptr = img->data + channelIndex * bytesPerChannel;
        decoder.channels[c].user_pixel_stride = reader->channelCount * bytesPerChannel;
        decoder.channels[c].user_line_stride = decoder.channels[c].user_pixel_stride * tileWidth;
        decoder.channels[c].user_bytes_per_element = bytesPerChannel;
    }

    rv = exr_decoding_choose_default_routines(reader->f, 0, &decoder);
    if (rv != EXR_ERR_SUCCESS) {
        exr_decoding_destroy(reader->f, &decoder);
        return rv;
    }
    rv = exr_decoding_run(reader->f, reader->partIndex, &decoder);
    if (rv != EXR_ERR_SUCCESS) {
        exr_decoding_destroy(reader->f, &decoder);
        return rv;
    }
    rv = exr_decoding_destroy(reader->f, &decoder);
    return rv;
}

/*
    Read an entire mip level of a tiled image
*/


int nanoexr_readAllTileData(nanoexr_Reader_t* reader, nanoexr_ImageData_t* img, nanoexr_MipLevel_t mip) {
    if (!reader || !reader->f)
        return EXR_ERR_INVALID_ARGUMENT;
    if (mip.level >= reader->mipLevels.level)
        return EXR_ERR_INVALID_ARGUMENT;
    if (reader->channelCount < 1 || reader->channelCount > 4)
        return EXR_ERR_INVALID_ARGUMENT;
    if (reader->isScanline)
        return EXR_ERR_INVALID_ARGUMENT;

    int bytesPerComponent = 0;
    switch (reader->pixelType) {
        case EXR_PIXEL_HALF:
            bytesPerComponent = 2;
            break;
        case EXR_PIXEL_FLOAT:
            bytesPerComponent = 4;
            break;
        case EXR_PIXEL_UINT:
            bytesPerComponent = 4;
            break;

        default:
            return EXR_ERR_INVALID_ARGUMENT;
    }

    int rv = EXR_ERR_SUCCESS;
    int tileWidth = reader->tileLevelInfo[mip.level].tileWidth;
    int tileHeight = reader->tileLevelInfo[mip.level].tileHeight;
    int levelWidth = reader->tileLevelInfo[mip.level].levelWidth;
    int levelHeight = reader->tileLevelInfo[mip.level].levelHeight;
    int tileCountX = (levelWidth + tileWidth - 1) / tileWidth;
    int tileCountY = (levelHeight + tileHeight - 1) / tileHeight;
    int tileCount = tileCountX * tileCountY;
    int bytesPePixel = reader->channelCount * bytesPerComponent;

    for (int row = 0; row < tileCountY; ++ row) {
        int rowOffset = row * levelWidth * tileHeight;
        for (int col = 0; col < tileCountX; ++ col) {
            int colOffset = col * tileWidth;
            // create a window into the image data where the valid memory
            // size has been adjusted so that overruns can be guarded
            nanoexr_ImageData_t window = {
                img->data + (rowOffset + colOffset) * bytesPePixel,
                img->dataSize - (rowOffset + colOffset) * bytesPePixel
            };
            rv = nanoexr_readTileData(reader, &window, mip, col, row);
            if (rv != EXR_ERR_SUCCESS)
                return rv;
        }
    }
    return rv;
}



int main2(int argc, char** argv) {
    int         maj, min, patch;
    const char* extra;
    exr_get_library_version (&maj, &min, &patch, &extra);
    printf("OpenEXR %d.%d.%d %s\n", maj, min, patch, extra? extra: "");

    exr_context_t f;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn          = &err_cb;

    char filename_buff[32768];
    snprintf(filename_buff, sizeof(filename_buff), "%s", EXR_FILE);

    int rval = exr_test_file_header(filename_buff, &cinit);
    if (rval != EXR_ERR_SUCCESS) {
        fprintf(stderr, "could not open %s for reading because %d\n", filename_buff, rval);
        goto err;
    }
    printf("rval is %d\n", rval);

    rval = exr_start_read(&f, filename_buff, &cinit);
    if (rval != EXR_ERR_SUCCESS) {
        fprintf(stderr, "could not start reading %s because %d\n", filename_buff, rval);
        goto err;
    }
    
    EXR_PROMOTE_CONST_CONTEXT_OR_ERROR(f);
    printf (
        "File '%s': ver %d flags%s%s%s%s\n",
        pctxt->filename.str,
        (int) pctxt->version,
        pctxt->is_singlepart_tiled ? " singletile" : "",
        pctxt->max_name_length == EXR_LONGNAME_MAXLEN ? " longnames"
                                                        : " shortnames",
        pctxt->has_nonimage_data ? " deep" : " not-deep ",
        pctxt->is_multipart ? " multipart" : " not-multipart ");
    printf (" parts: %d\n", pctxt->num_parts);

    bool verbose = true;

    for (int partidx = 0; partidx < pctxt->num_parts; ++partidx)
    {
        const struct _internal_exr_part* curpart = pctxt->parts[partidx];
        if (pctxt->is_multipart || curpart->name) {
            printf(
                " part %d: %s\n",
                partidx + 1,
                curpart->name ? curpart->name->string->str : "<single>");

            for (int a = 0; a < curpart->attributes.num_attributes; ++a)
            {
                if (a > 0) printf ("\n");
                printf("  ");
                print_attr(curpart->attributes.entries[a], verbose);
            }
            printf("\n");
        }
        if (curpart->type)
        {
            printf("  ");
            print_attr(curpart->type, verbose);
        }
        printf ("  ");
        print_attr(curpart->compression, verbose);
        if (curpart->tiles)
        {
            printf("\n  ");
            print_attr(curpart->tiles, verbose);
        }
        printf("\n  ");
        print_attr(curpart->displayWindow, verbose);
        printf("\n  ");
        print_attr(curpart->dataWindow, verbose);
        printf("\n  ");
        print_attr(curpart->channels, verbose);
        printf("\n");

        if (curpart->tiles)
        {
            printf(
                "  tiled image has levels: x %d y %d\n",
                curpart->num_tile_levels_x,
                curpart->num_tile_levels_y);
            printf("    x tile count:");
            for (int l = 0; l < curpart->num_tile_levels_x; ++l)
                printf(
                    " %d (sz %d)",
                    curpart->tile_level_tile_count_x[l],
                    curpart->tile_level_tile_size_x[l]);
            printf("\n    y tile count:");
            for (int l = 0; l < curpart->num_tile_levels_y; ++l)
                printf(
                    " %d (sz %d)",
                    curpart->tile_level_tile_count_y[l],
                    curpart->tile_level_tile_size_y[l]);
            printf("\n");
        }
        else {
            printf("This is a scanline encoded file\n");
        }
    }

    {
        // read using the real api, not by peeking under the hood
        // as the info dump above does.
        int numparts = 0;
        rval = exr_get_count(f, &numparts);
        if (rval != EXR_ERR_SUCCESS) {
            fprintf(stderr, "could not fetch the number of parts\n");
            goto err;
        }

        for (int p = 0; p < numparts; ++p)
        {
            exr_storage_t store;
            rval = exr_get_storage(f, p, &store);
            if (rval != EXR_ERR_SUCCESS) {
                fprintf(stderr, "could not fetch the storage kind\n");
                goto err;
            }

            // not supporting deep images
            if (store == EXR_STORAGE_DEEP_SCANLINE || store == EXR_STORAGE_DEEP_TILED)
                continue;

            bool reduceMemory = true;
            bool reduceTime = true;
            if (store == EXR_STORAGE_SCANLINE)
            {
                if (readCoreScanlinePart(f, p, reduceMemory, reduceTime) != EXR_ERR_SUCCESS) {
                    fprintf(stderr, "Could not read a scanline part\n");
                }
            }
            else if (store == EXR_STORAGE_TILED)
            {
                if (readCoreTiledPart(f, p, reduceMemory, reduceTime) != EXR_ERR_SUCCESS) {
                    fprintf(stderr, "Could not read a scanline part\n");
                }
            }
        }

    }


    exr_finish(&f);

    return 0;

err:
    return 1;
}

