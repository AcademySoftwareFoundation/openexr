//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <ImfHeader.h>
#include <ImfArray.h>
#include <ImfInputFile.h>
#include <ImfFrameBuffer.h>
#include <ImfOutputFile.h>
#include <ImfPreviewImage.h>
#include <ImfRgbaFile.h>
#include <ImfRgba.h>
#include <ImfTiledInputFile.h>
#include <ImfTiledRgbaFile.h>
#include <ImfDeepScanLineInputFile.h>
#include <ImfDeepTiledInputFile.h>
#include <ImfDeepTiledOutputFile.h>
#include <ImfDeepFrameBuffer.h>
#include <ImfStandardAttributes.h>
#include <ImfChannelList.h>
#include <ImfPartType.h>

#include <Iex.h>
#include <ImathFun.h>

#include <cfloat>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _WIN32
#include <sys/mman.h>
#include <unistd.h>
#endif

using namespace IMATH_NAMESPACE;
using namespace OPENEXR_IMF_NAMESPACE;
using namespace IEX_NAMESPACE;

using std::max;

struct GZ
{
    half g;
    float z;
};

int
getPixelSampleCount(int i, int j)
{
    return 0;
}

void
getPixelSampleData(int i, int j, Array2D<float*>& dataZ, Array2D<half*>& dataA)
{
}

int
getSampleCountForTile(int i, int j, Array2D<unsigned int>& sampleCount)
{
    return 0;
}

void
getSampleDataForTile(int i, int j, int tileSizeX, int tileSizeY,
                     Array2D<unsigned int>& sampleCount,
                     Array2D<float*>& dataZ, Array2D<half*>& dataA)
{
}


namespace WEBSITE_SRC {
#include "IStream.cpp"
}

#include "C_IStream.cpp"
#include "C_IStream_clear.cpp"
#include "C_IStream_read.cpp"
#include "C_IStream_seekg.cpp"
#include "C_IStream_tellg.cpp"
#ifndef _WIN32
#include "MemoryMappedIStream.cpp"
#include "MemoryMappedIStream_isMemoryMapped.cpp"
#include "MemoryMappedIStream_destructor.cpp"
#include "MemoryMappedIStream_constructor.cpp"
#include "MemoryMappedIStream_read.cpp"
#include "MemoryMappedIStream_readMemoryMapped.cpp"
#endif
#include "mergeOverlappingSamples.cpp"
#include "readDeepScanLineFile.cpp"
#include "readDeepTiledFile.cpp"
#include "readGZ1.cpp"
#include "readGZ2.cpp"
#include "readHeader.cpp"
#include "readRgba1.cpp"
#include "readRgba2.cpp"
#include "readRgbaFILE.cpp"
#include "readTiled1.cpp"
#include "readTiledRgba1.cpp"
#include "splitVolumeSample.cpp"
#include "writeDeepScanLineFile.cpp"
#include "writeDeepTiledFile.cpp"
#include "writeGZ1.cpp"
#include "writeGZ2.cpp"
#include "writeRgba1.cpp"
#include "writeRgba2.cpp"
#include "readChannelsAndLayers.cpp"
#include "tileDescription.cpp"
#include "validExrFile.cpp"
#include "previewImageExamples.cpp"


void structDefinitions()
{
    #include "structDefinitions.cpp"
}

void multithreading()
{
    #include "multithreading.cpp"
}

void envmap()
{
    #include "envmap.cpp"
}

void compression()
{
    #include "compression.cpp"
}

int
main(int argc, char* argv[])
{
}
