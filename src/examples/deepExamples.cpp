//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//    Code examples that show how class DeepScanLineInputFile and
//    class DeepScanLineOutputFile can be used to read and write
//    OpenEXR image files with 16-bit floating-point green,
//    and 32-bit floating point depth channels.
//
//-----------------------------------------------------------------------------

#include <ImfArray.h>

#include <ImfHeader.h>
#include <ImfPartType.h>
#include <ImfChannelList.h>
#include <ImfDeepFrameBuffer.h>
#include <ImfDeepScanLineInputFile.h>
#include <ImfDeepScanLineOutputFile.h>

#include "drawImage.h"

#include "namespaceAlias.h"
using namespace IMF;
using namespace std;
using namespace IMATH_NAMESPACE;

void
readDeepScanlineFile (
    const char             filename[],
    Box2i&                 displayWindow,
    Box2i&                 dataWindow,
    Array2D<float*>&       dataZ,
    Array2D<half*>&        dataA,
    Array2D<unsigned int>& sampleCount)
{
    DeepScanLineInputFile file (filename);

    const Header& header = file.header ();
    dataWindow           = header.dataWindow ();
    displayWindow        = header.displayWindow ();

    int width  = dataWindow.max.x - dataWindow.min.x + 1;
    int height = dataWindow.max.y - dataWindow.min.y + 1;

    sampleCount.resizeErase (height, width);

    dataZ.resizeErase (height, width);
    dataA.resizeErase (height, width);

    DeepFrameBuffer frameBuffer;

    frameBuffer.insertSampleCountSlice (Slice (
        UINT,
        (char*) (&sampleCount[0][0] - dataWindow.min.x - dataWindow.min.y * width),
        sizeof (unsigned int) * 1,       // xStride
        sizeof (unsigned int) * width)); // yStride

    frameBuffer.insert (
        "dataZ",
        DeepSlice (
            FLOAT,
            (char*) (&dataZ[0][0] - dataWindow.min.x - dataWindow.min.y * width),

            sizeof (float*) * 1,     // xStride for pointer array
            sizeof (float*) * width, // yStride for pointer array
            sizeof (float) * 1));    // stride for Z data sample

    frameBuffer.insert (
        "dataA",
        DeepSlice (
            HALF,
            (char*) (&dataA[0][0] - dataWindow.min.x - dataWindow.min.y * width),
            sizeof (half*) * 1,     // xStride for pointer array
            sizeof (half*) * width, // yStride for pointer array
            sizeof (half) * 1));    // stride for O data sample

    file.setFrameBuffer (frameBuffer);

    file.readPixelSampleCounts (dataWindow.min.y, dataWindow.max.y);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {

            dataZ[i][j] = new float[sampleCount[i][j]];
            dataA[i][j] = new half[sampleCount[i][j]];
        }
    }

    file.readPixels (dataWindow.min.y, dataWindow.max.y);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            delete[] dataZ[i][j];
            delete[] dataA[i][j];
        }
    }
}

unsigned int getPixelSampleCount (int i, int j)
{
    return 1;
}

Array2D<float> testDataZ;
Array2D<half> testDataA;

void getPixelSampleData(
    int i,
    int j,
    Array2D<float*>& dataZ,
    Array2D<half*>& dataA)
{
    dataZ[i][j][0] = testDataZ[i][j];
    dataA[i][j][0] = testDataA[i][j];
}

void
writeDeepScanlineFile (
    const char       filename[],
    Box2i            displayWindow,
    Box2i            dataWindow,
    Array2D<float*>& dataZ,

    Array2D<half*>& dataA,

    Array2D<unsigned int>& sampleCount)

{
    int height = dataWindow.max.y - dataWindow.min.y + 1;
    int width  = dataWindow.max.x - dataWindow.min.x + 1;

    Header header (displayWindow, dataWindow);

    header.channels ().insert ("Z", Channel (FLOAT));
    header.channels ().insert ("A", Channel (HALF));
    header.setType (DEEPSCANLINE);
    header.compression () = ZIPS_COMPRESSION;

    DeepScanLineOutputFile file (filename, header);

    DeepFrameBuffer frameBuffer;

    frameBuffer.insertSampleCountSlice (Slice (
        UINT,
        (char*) (&sampleCount[0][0] - dataWindow.min.x - dataWindow.min.y * width),
        sizeof (unsigned int) * 1, // xS

        sizeof (unsigned int) * width)); // yStride

    frameBuffer.insert (
        "Z",
        DeepSlice (
            FLOAT,
            (char*) (&dataZ[0][0] - dataWindow.min.x - dataWindow.min.y * width),
            sizeof (float*) * 1, // xStride for pointer

            sizeof (float*) * width, // yStride for pointer array
            sizeof (float) * 1));    // stride for Z data sample

    frameBuffer.insert (
        "A",
        DeepSlice (
            HALF,
            (char*) (&dataA[0][0] - dataWindow.min.x - dataWindow.min.y * width),
            sizeof (half*) * 1,     // xStride for pointer array
            sizeof (half*) * width, // yStride for pointer array
            sizeof (half) * 1));    // stride for A data sample

    file.setFrameBuffer (frameBuffer);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            sampleCount[i][j] = getPixelSampleCount (i, j);
            dataZ[i][j]       = new float[sampleCount[i][j]];
            dataA[i][j]       = new half[sampleCount[i][j]];
            // Generate data for dataZ and dataA.
            getPixelSampleData(i, j, dataZ, dataA);
        }

        file.writePixels (1);
    }

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            delete[] dataZ[i][j];
            delete[] dataA[i][j];
        }
    }
}


void deepExamples()
{
    int w = 800;
    int h = 600;
    
    Box2i window;
    window.min.setValue(1, 1);
    window.max.setValue(w, h);
    
    Array2D<float *> dataZ;
    dataZ.resizeErase(h, w);
    
    Array2D<half *> dataA;
    dataA.resizeErase(h, w);
    
    Array2D<unsigned int> sampleCount;
    sampleCount.resizeErase(h, w);
    
    testDataA.resizeErase(h, w);
    testDataZ.resizeErase(h, w);
    drawImage2(testDataA, testDataZ, w, h);
    
    writeDeepScanlineFile("test.deep.exr", window, window, dataZ, dataA, sampleCount);
    readDeepScanlineFile ("test.deep.exr", window, window, dataZ, dataA, sampleCount);
}
