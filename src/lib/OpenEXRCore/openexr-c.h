
// openexr_c.h and openexr_c.c
// are a "single file" standalone version of the OpenEXRCore library.
// It's not part of the regular OpenEXR build. These two files are meant
// to be added to your project, either as a library with just the single
// standalone files, or added directly to your project.

#ifndef openexr_c_h
#define openexr_c_h

#include "OpenEXRCore/openexr_attr.h"
#include "OpenEXRCore/openexr_context.h"
#include "OpenEXRCore/openexr_part.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int tileWidth,  tileHeight;
    int levelWidth, levelHeight;
} nanoexr_TileMipInfo_t;

typedef struct {
    uint8_t* data;
    size_t dataSize;
    exr_pixel_type_t pixelType;
    int channelCount; // 1 for luminance, 3 for RGB, 4 for RGBA
    int width, height;
} nanoexr_ImageData_t;

// simple struct to force type safety on interface
typedef struct {
    int level;
} nanoexr_MipLevel_t;

typedef struct {
    exr_context_t exr;
    exr_context_initializer_t init;
    char* filename;
    bool isScanline;
    int partIndex;
    exr_pixel_type_t pixelType;
    int channelCount;
    int width, height;
    int tileLevelCount;
    nanoexr_MipLevel_t mipLevels;
    nanoexr_TileMipInfo_t* tileLevelInfo;
    int exrSDKVersionMajor;
    int exrSDKVersionMinor;
    int exrSDKVersionPatch;
    const char* exrSDKExtraInfo;
} nanoexr_Reader_t;

nanoexr_Reader_t* nanoexr_new(const char* filename, 
                              exr_context_initializer_t*);

exr_result_t nanoexr_open(nanoexr_Reader_t* reader, int partIndex);
bool         nanoexr_isOpen(nanoexr_Reader_t* reader);
bool         nanoexr_isTiled(nanoexr_Reader_t* reader);
void         nanoexr_close(nanoexr_Reader_t* reader);
void         nanoexr_delete(nanoexr_Reader_t* reader);
int          nanoexr_getWidth(nanoexr_Reader_t* reader);
int          nanoexr_getHeight(nanoexr_Reader_t* reader);
int          nanoexr_getChannelCount(nanoexr_Reader_t* reader);
nanoexr_MipLevel_t nanoexr_getMipLevels(nanoexr_Reader_t* reader);
exr_pixel_type_t   nanoexr_getPixelType(nanoexr_Reader_t* reader);
size_t             nanoexr_getPixelTypeSize(exr_pixel_type_t t);

// image will be read into the allocation pointed to by img.
// no more than img->height lines will be read. If initialLinesToSkip
// is greater than zero, those lines will not be read into img->data,
// the data in img->data will start at that offset. If sufficient
// lines cannot be read to fill img->data per img->height, the
// remainder of the buffer will be filled with zeroes.
exr_result_t       nanoexr_readScanlineData(nanoexr_Reader_t* reader,
                                            nanoexr_ImageData_t* img,
                                            const char* layerName,
                                            int initialLinesToSkip);

exr_result_t       nanoexr_readTileData(nanoexr_Reader_t* reader,
                                        nanoexr_ImageData_t* img,
                                        nanoexr_MipLevel_t mipLevel,
                                        int col, int row);
exr_result_t       nanoexr_readAllTileData(nanoexr_Reader_t* reader,
                                           nanoexr_ImageData_t* img,
                                           nanoexr_MipLevel_t mip);
bool nanoexr_Gaussian_resample(const nanoexr_ImageData_t* src,
                               nanoexr_ImageData_t* dst);

#ifdef __cplusplus
}
#endif

#endif
