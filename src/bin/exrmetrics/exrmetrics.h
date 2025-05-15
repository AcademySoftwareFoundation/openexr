
#ifndef INCLUDED_EXR_METRICS_H
#define INCLUDED_EXR_METRICS_H

//----------------------------------------------------------------------------
//
//	Copy input to output, reporting file size and timing
//
//----------------------------------------------------------------------------

#include "ImfCompression.h"

#include "stdint.h"

#include <vector>

enum PixelMode
{
    PIXELMODE_ORIGINAL,
    PIXELMODE_ALL_HALF,
    PIXELMODE_ALL_FLOAT,
    PIXELMODE_MIXED_HALF_FLOAT
};
std::string modeName (PixelMode p);

struct partSizeData
{
    uint64_t rawSize =
        0; // total size required to store just the pixel data, not including extra space
    uint64_t pixelCount =
        0; // number of pixels in the image, including pixels mipmap levels for tiled images
    uint64_t channelCount = 0;
    uint64_t tileCount    = 0; // for tiled images, the number of tiles
    bool     isDeep       = false;
    bool     isTiled      = false;
    OPENEXR_IMF_NAMESPACE::Compression compression =
        OPENEXR_IMF_NAMESPACE::NUM_COMPRESSION_METHODS;
    std::string partType = "";
};

struct partStats
{
    std::vector<double>
        countReadPerf; // for deep only, time reading the per-pixel sample count
    std::vector<double> readPerf; //time reading the pixel data

    std::vector<double> writePerf; // time to write data (all part types)

    std::vector<double>
        countRereadPerf; // for deep only, time rereading the per-pixel sample count
    std::vector<double>
        rereadPerf; // for deep, times reading the sample count, otherwise times reading the entire data

    partSizeData sizeData;
};

struct fileMetrics
{
    std::vector<partStats> stats;
    partStats              totalStats;
    uint64_t               inputFileSize;
    uint64_t               outputFileSize;
};

fileMetrics exrmetrics (
    const char*                        inFileName,
    const char*                        outFileName,
    int                                part,
    OPENEXR_IMF_NAMESPACE::Compression compression,
    float                              level,
    int                                passes,
    bool                               write,
    bool                               reread,
    PixelMode                          pixelMode,
    bool                               verbose);

#endif
