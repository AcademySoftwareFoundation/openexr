
#define ILMTHREAD_THREADING_ENABLED
#define OPENEXR_CORE_STANDALONE
#define OPENEXR_VERSION_MAJOR 3 //@OpenEXR_VERSION_MAJOR@
#define OPENEXR_VERSION_MINOR 2 //@OpenEXR_VERSION_MINOR@
#define OPENEXR_VERSION_PATCH 0 //@OpenEXR_VERSION_PATCH@

#include "openexr-c.h"

#include "attributes.c"
#include "base.c"
#include "channel_list.c"
#include "chunk.c"
#include "coding.c"
#include "context.c"
#include "debug.c"
#include "decoding.c"
#include "encoding.c"
#include "float_vector.c"
#include "internal_b44_table.c"
#include "internal_b44.c"
#include "internal_dwa.c"
#include "internal_huf.c"
#include "internal_piz.c"
#include "internal_pxr24.c"
#include "internal_rle.c"
#include "internal_structs.c"
#include "internal_zip.c"
#include "memory.c"
#include "opaque.c"
#include "pack.c"
#include "parse_header.c"
#include "part_attr.c"
#include "part.c"
#include "preview.c"
#include "std_attr.c"
#include "string_vector.c"
#include "string.c"
#include "unpack.c"
#include "validation.c"
#include "write_header.c"

#include <ctype.h>
#include <math.h>

#define EXR_FILE "StillLife.exr"
uint64_t gMaxBytesPerScanline = 8000000;
uint64_t gMaxTileBytes        = 8192 * 8192;


static float integrate_gaussian(float x, float sigma)
{
  float p1 = erf((x - 0.5f) / sigma * sqrtf(0.5f));
  float p2 = erf((x + 0.5f) / sigma * sqrtf(0.5f));
  return (p2-p1) * 0.5f;
}

bool nanoexr_Gaussian_resample(const nanoexr_ImageData_t* src,
                               nanoexr_ImageData_t* dst)
{
    if (src->pixelType != EXR_PIXEL_FLOAT && dst->pixelType != EXR_PIXEL_FLOAT)
        return false;
    if (src->channelCount != dst->channelCount)
        return false;
    
    const int srcWidth  = src->width;
    const int dstWidth  = dst->width;
    const int srcHeight = src->height;
    const int dstHeight = dst->height;
    if (srcWidth == dstWidth && srcHeight == dstHeight) {
        memcpy(dst->data, src->data, src->channelCount * srcWidth * srcHeight * sizeof(float));
        return true;
    }
    
    float* srcData = (float*)src->data;
    float* dstData = (float*)dst->data;

    // two pass image resize

    // Create a Gaussian filter, per:
    // https://bartwronski.com/2021/10/31/practical-gaussian-filter-binomial-filter-and-small-sigma-gaussians
    // chose sigma to suppress high frequencies that can't be represented in the downsampled image
    const float ratio = (float)dstWidth / (float)srcWidth;
    const float sigma = 1.f / 2.f * ratio;
    const float support = 0.995f;
    float radius = ceilf(sqrtf(-2.0f * sigma * sigma * logf(1.0f - support)));
    int filterSize = (int)radius;
    float* filter = (float*) malloc(sizeof(float) * 1 + filterSize * 2);
    float sum = 0.0f;
    for (int i = 0; i <= filterSize; i++) {
        int idx = i + filterSize;
        filter[idx] = integrate_gaussian((float) i, sigma);
        if (i > 0)
            sum += 2 * filter[idx];
        else
            sum = filter[idx];
    }
    for (int i = 0; i <= filterSize; ++i) {
        filter[i + filterSize] /= sum;
    }
    for (int i = 0; i < filterSize; ++i) {
        filter[filterSize - i - 1] = filter[i + filterSize + 1];
    }
    int fullFilterSize = filterSize * 2 + 1;

    // first pass: resize horizontally
    int srcFloatsPerLine = src->channelCount * srcWidth;
    int dstFloatsPerLine = src->channelCount * dstWidth;
    float* firstPass = (float*)malloc(dstWidth * src->channelCount * srcHeight * sizeof(float));
    for (int y = 0; y < srcHeight; ++y) {
        for (int x = 0; x < dstWidth; ++x) {
            for (int c = 0; c < src->channelCount; ++c) {
                float sum = 0.0f;
                for (int i = 0; i < fullFilterSize; ++i) {
                    int srcX = (int)((x + 0.5f) / ratio - 0.5f) + i - filterSize;
                    if (srcX < 0 || srcX >= srcWidth)
                        continue;
                    int idx = y * srcFloatsPerLine + (srcX * src->channelCount) + c;
                    sum += srcData[idx] * filter[i];
                }
                firstPass[y * dstFloatsPerLine + (x * src->channelCount) + c] = sum;
            }
        }
    }

    // second pass: resize vertically
    float* secondPass = dstData;
    for (int y = 0; y < dstHeight; ++y) {
        for (int x = 0; x < dstWidth; ++x) {
            for (int c = 0; c < src->channelCount; ++c) {
                float sum = 0.0f;
                for (int i = 0; i < fullFilterSize; ++i) {
                    int srcY = (int)((y + 0.5f) / ratio - 0.5f) + i - filterSize;
                    if (srcY < 0 || srcY >= srcHeight)
                        continue;
                    int idx = src->channelCount * srcY * dstWidth + (x * src->channelCount) + c;
                    sum += firstPass[idx] * filter[i];
                }
                secondPass[src->channelCount * y * dstWidth + (x * src->channelCount) + c] = sum;
            }
        }
    }
    free(filter);
    free(firstPass);
    return true;
}


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

    reader->filename = strdup(filename);
    reader->exr = NULL;
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
    if (reader && reader->exr) {
        exr_finish(&reader->exr);
        reader->exr = NULL;
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
    if (reader->exr) {
        nanoexr_close(reader);
    }
    int rv = exr_start_read(&reader->exr, reader->filename, &reader->init);
    if (rv != EXR_ERR_SUCCESS) {
        nanoexr_close(reader);
        return rv;
    }

    exr_attr_box2i_t datawin;
    rv = exr_get_data_window(reader->exr, partIndex, &datawin);
    if (rv != EXR_ERR_SUCCESS) {
        nanoexr_close(reader);
        return rv;
    }
    reader->partIndex = partIndex;
    reader->width = datawin.max.x - datawin.min.x + 1;
    reader->height = datawin.max.y - datawin.min.y + 1;

    exr_storage_t storage;
    rv = exr_get_storage(reader->exr, partIndex, &storage);
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
        rv = exr_get_tile_levels(reader->exr, partIndex, &numMipLevelsX, &numMipLevelsY);
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
            rv = exr_get_tile_sizes(reader->exr, partIndex, i, i, &tileWidth, &tileHeight);
            if (rv != EXR_ERR_SUCCESS) {
                nanoexr_close(reader);
                return rv;
            }
            rv = exr_get_level_sizes(reader->exr, partIndex, i, i, &levelWidth, &levelHeight);
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
    rv = exr_get_channels(reader->exr, partIndex, &chlist);
    if (rv != EXR_ERR_SUCCESS) {
        nanoexr_close(reader);
        return rv;
    }
    reader->channelCount = chlist->num_channels;
    reader->pixelType = chlist->entries[0].pixel_type;

    return rv;
}

bool nanoexr_isOpen(nanoexr_Reader_t* reader) {
    return reader && (reader->exr != NULL);
}

bool nanoexr_isTiled(nanoexr_Reader_t* reader) {
    if (!reader || !reader->exr)
        return false;
    return !reader->isScanline;
}

int nanoexr_getWidth(nanoexr_Reader_t* reader) {
    if (!reader || !reader->exr)
        return 0;
    return reader->width;
}

int nanoexr_getHeight(nanoexr_Reader_t* reader) {
    if (!reader || !reader->exr)
        return 0;
    return reader->height;
}

nanoexr_MipLevel_t nanoexr_getMipLevels(nanoexr_Reader_t* reader) {
    if (!reader || !reader->exr)
        return (nanoexr_MipLevel_t){0};
    return reader->mipLevels;
}

int nanoexr_getChannelCount(nanoexr_Reader_t* reader) {
    if (!reader || !reader->exr)
        return 0;
    return reader->channelCount;
}

exr_pixel_type_t nanoexr_getPixelType(nanoexr_Reader_t* reader) {
    if (!reader || !reader->exr)
        return EXR_PIXEL_LAST_TYPE;
    return reader->pixelType;
}

size_t nanoexr_getPixelTypeSize(exr_pixel_type_t t)
{
    switch (t) {
        case EXR_PIXEL_HALF: return 2;
        case EXR_PIXEL_UINT: return 4;
        case EXR_PIXEL_FLOAT: return 4;
        default: return 0;
    }
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

static bool strIsRed(const char* layerName, const char* str) {
    if (layerName && (strncmp(layerName, str, strlen(layerName)) != 0))
        return false;

    // check if the case folded string is R or RED, or if it ends in .R or .RED
    char* folded = strdup(str);
    for (int i = 0; folded[i]; ++i) {
        folded[i] = tolower(folded[i]);
    }
    if (strcmp(folded, "r") == 0 || strcmp(folded, "red") == 0)
        return true;
    size_t l = strlen(folded);
    if ((l > 2) && (folded[l - 2] == '.') && (folded[l - 1] == 'r'))
        return true;
    if (l < 4)
        return false;
    return strcmp(folded + l - 4, ".red");
}

static bool strIsGreen(const char* layerName, const char* str) {
    if (layerName && (strncmp(layerName, str, strlen(layerName)) != 0))
        return false;

    // check if the case folded string is G or GREEN, or if it ends in .G or .GREEN
    char* folded = strdup(str);
    for (int i = 0; folded[i]; ++i) {
        folded[i] = tolower(folded[i]);
    }
    if (strcmp(folded, "g") == 0 || strcmp(folded, "green") == 0)
        return true;
    size_t l = strlen(folded);
    if ((l > 2) && (folded[l - 2] == '.') && (folded[l - 1] == 'g'))
        return true;
    if (l < 6)
        return false;
    return strcmp(folded + l - 6, ".green");
}

static bool strIsBlue(const char* layerName, const char* str) {
    if (layerName && (strncmp(layerName, str, strlen(layerName)) != 0))
        return false;

    // check if the case folded string is B or BLUE, or if it ends in .B or .BLUE
    char* folded = strdup(str);
    for (int i = 0; folded[i]; ++i) {
        folded[i] = tolower(folded[i]);
    }
    if (strcmp(folded, "b") == 0 || strcmp(folded, "blue") == 0)
        return true;
    size_t l = strlen(folded);
    if ((l > 2) && (folded[l - 2] == '.') && (folded[l - 1] == 'b'))
        return true;
    if (l < 5)
        return false;
    return strcmp(folded + l - 5, ".blue");
}

static bool strIsAlpha(const char* layerName, const char* str) {
    if (layerName && (strncmp(layerName, str, strlen(layerName)) != 0))
        return false;

    // check if the case folded string is A or ALPHA, or if it ends in .A or .ALPHA
    char* folded = strdup(str);
    for (int i = 0; folded[i]; ++i) {
        folded[i] = tolower(folded[i]);
    }
    if (strcmp(folded, "a") == 0 || strcmp(folded, "alpha") == 0)
        return true;
    size_t l = strlen(folded);
    if ((l > 2) && (folded[l - 2] == '.') && (folded[l - 1] == 'a'))
        return true;
    if (l < 6)
        return false;
    return strcmp(folded + l - 6, ".alpha");
}

/*
    Read an entire scanline based image
*/

exr_result_t nanoexr_readScanlineData(nanoexr_Reader_t* reader, 
                                      nanoexr_ImageData_t* img,
                                      const char* layerName,
                                      int cropTop)
{
    uint8_t* chunk_buffer = NULL;
    exr_decode_pipeline_t decoder;
    memset(&decoder, 0, sizeof(decoder));
    int checkpoint = 0;
    int scanLinesPerChunk;
    int rv = exr_get_scanlines_per_chunk(reader->exr, reader->partIndex, &scanLinesPerChunk);
    if (rv != EXR_ERR_SUCCESS)
        goto err;

    checkpoint = 1;
    exr_attr_box2i_t datawin;
    rv = exr_get_data_window(reader->exr, reader->partIndex, &datawin);
    if (rv != EXR_ERR_SUCCESS) 
        goto err;

    int window_width = datawin.max.x - datawin.min.x + 1;
    int window_height = datawin.max.y - datawin.min.y + 1;

    // allocate a space large enough for a chunk of scanlines of type float
    chunk_buffer = (uint8_t*) malloc(scanLinesPerChunk * window_width * reader->channelCount * sizeof(float));
    if (!chunk_buffer) {
        checkpoint = 2;
        rv = EXR_ERR_OUT_OF_MEMORY;
        goto err;
    }
    
    size_t output_bpp = nanoexr_getPixelTypeSize(img->pixelType);
    size_t bpp = nanoexr_getPixelTypeSize(reader->pixelType);
    int output_width = window_width;
    size_t outputOffset = 0;
    int bytesPerElement = 0;
    int rgbaIndex[4] = {-1, -1, -1, -1};
    int linesWritten = 0;
    
    for (int chunky = datawin.min.y; chunky < datawin.max.y; chunky += scanLinesPerChunk) {

        if (cropTop > scanLinesPerChunk) {
            cropTop -= scanLinesPerChunk;
            continue;
        }

        exr_chunk_info_t chunkInfo = {0};
        
        checkpoint = 20;
        rv = exr_read_scanline_chunk_info(reader->exr, reader->partIndex, chunky, &chunkInfo);
        if (rv != EXR_ERR_SUCCESS)
            goto err;

        if (decoder.channels == NULL) {
            checkpoint = 30;
        
            rv = exr_decoding_initialize(reader->exr, reader->partIndex, &chunkInfo, &decoder);
            if (rv != EXR_ERR_SUCCESS)
                goto err;
            
            bytesPerElement = decoder.channels[0].bytes_per_element;
            for (int c = 0; c < decoder.channel_count; ++c) {
                int channelIndex = -1;
                if (strIsRed(layerName, decoder.channels[c].channel_name)) {
                    rgbaIndex[0] = c;
                    channelIndex = 0;
                }
                else if (strIsGreen(layerName, decoder.channels[c].channel_name)) {
                    rgbaIndex[1] = c;
                    channelIndex = 1;
                }
                else if (strIsBlue(layerName, decoder.channels[c].channel_name)) {
                    rgbaIndex[2] = c;
                    channelIndex = 2;
                }
                else if (strIsAlpha(layerName, decoder.channels[c].channel_name)) {
                    rgbaIndex[3] = c;
                    channelIndex = 3;
                }
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

                decoder.channels[c].decode_to_ptr = NULL; // assume we won't decode this channel
                decoder.channels[c].user_pixel_stride = img->channelCount * bytesPerElement;
                decoder.channels[c].user_line_stride = decoder.channels[c].user_pixel_stride * window_width;
                decoder.channels[c].user_bytes_per_element = bytesPerElement;
            }
            checkpoint = 60;
            rv = exr_decoding_choose_default_routines(reader->exr, 0, &decoder);
            if (rv != EXR_ERR_SUCCESS)
                goto err;
        }
        else {
       	    // Reuse existing pipeline
            checkpoint = 40;
	        rv = exr_decoding_update(reader->exr, reader->partIndex, &chunkInfo, &decoder);
	        if (rv != EXR_ERR_SUCCESS)
                goto err;
        }

        // Set pixmap pointers for this chunk
        for (int c = 0; c < decoder.channel_count; c++) {
            if (rgbaIndex[c] >= 0)
                decoder.channels[c].decode_to_ptr = chunk_buffer + rgbaIndex[c] * bytesPerElement;
        }

        checkpoint = 70;
        rv = exr_decoding_run(reader->exr, reader->partIndex, &decoder);
        if (rv != EXR_ERR_SUCCESS)
            goto err;

        checkpoint = 90;

        uint8_t* copy_from_here;
        if (!cropTop)
            copy_from_here = chunk_buffer;
        else
            copy_from_here = chunk_buffer + cropTop * window_width * img->channelCount * bytesPerElement;

        int linesWrittenThisChunk = scanLinesPerChunk - cropTop;
        cropTop = 0;

        if (linesWritten + linesWrittenThisChunk > img->height)
            linesWrittenThisChunk = img->height - linesWritten;
        
        // this will just be a memcpy if in & out match
        rv = nanoexr_convertPixelType(
                img->pixelType, reader->pixelType,
                window_width * linesWrittenThisChunk,
                img->channelCount,
                copy_from_here, outputOffset + (uint8_t*) img->data);


        if (rv != EXR_ERR_SUCCESS)
            goto err;

        outputOffset += linesWrittenThisChunk * reader->width * img->channelCount * bytesPerElement;
        linesWritten += linesWrittenThisChunk;
        
        if (linesWritten >= img->height) {
            break;
        }
    }

    // zero out any remaining lines
    if (linesWritten < img->height) {
        memset(outputOffset + (uint8_t*) img->data, 0, (img->height - linesWritten) * img->width * img->channelCount * bytesPerElement);
    }
        
    // fall through and return rv
err:
    if (chunk_buffer)
        free(chunk_buffer);
    exr_decoding_destroy(reader->exr, &decoder);
    return rv;
}

/*
    Read a single tile of a tiled image at a certain miplevel
*/
int nanoexr_readTileData(nanoexr_Reader_t* reader, nanoexr_ImageData_t* img, nanoexr_MipLevel_t mipLevel, int col, int row)
{
    if (!reader || !reader->exr)
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
    int rv = exr_read_tile_chunk_info(reader->exr, reader->partIndex, 
                col, row, mipLevel.level, mipLevel.level, &chunkInfo);
    if (rv != EXR_ERR_SUCCESS)
        return rv;
    rv = exr_decoding_initialize(reader->exr, reader->partIndex, &chunkInfo, &decoder);
    if (rv != EXR_ERR_SUCCESS)
        return rv;
    
    int bytesPerChannel = decoder.channels[0].bytes_per_element;
    int rowPitch = bytesPerChannel * decoder.channel_count * tileWidth;

    int bytesPerTile = rowPitch * tileHeight;
    if (bytesPerTile > img->dataSize)
        return EXR_ERR_INVALID_ARGUMENT;

    for (int c = 0; c < decoder.channel_count; ++c) {
        if (decoder.channels[c].bytes_per_element != bytesPerChannel) {
            exr_decoding_destroy(reader->exr, &decoder);
            return EXR_ERR_INVALID_ARGUMENT;
        }

        if (decoder.channels[c].data_type != reader->pixelType) {
            exr_decoding_destroy(reader->exr, &decoder);
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
            exr_decoding_destroy(reader->exr, &decoder);
            return EXR_ERR_INVALID_ARGUMENT;
        }

        decoder.channels[c].decode_to_ptr = img->data + channelIndex * bytesPerChannel;
        decoder.channels[c].user_pixel_stride = reader->channelCount * bytesPerChannel;
        decoder.channels[c].user_line_stride = decoder.channels[c].user_pixel_stride * tileWidth;
        decoder.channels[c].user_bytes_per_element = bytesPerChannel;
    }

    rv = exr_decoding_choose_default_routines(reader->exr, 0, &decoder);
    if (rv != EXR_ERR_SUCCESS) {
        exr_decoding_destroy(reader->exr, &decoder);
        return rv;
    }
    rv = exr_decoding_run(reader->exr, reader->partIndex, &decoder);
    if (rv != EXR_ERR_SUCCESS) {
        exr_decoding_destroy(reader->exr, &decoder);
        return rv;
    }
    rv = exr_decoding_destroy(reader->exr, &decoder);
    return rv;
}

/*
    Read an entire mip level of a tiled image
*/


int nanoexr_readAllTileData(nanoexr_Reader_t* reader, nanoexr_ImageData_t* img, nanoexr_MipLevel_t mip) {
    if (!reader || !reader->exr)
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

