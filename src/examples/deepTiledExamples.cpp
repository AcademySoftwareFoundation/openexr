//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//    Code examples that show how class DeepTiledInputFile and
//    class DeepTiledOutputFile can be used to read and write
//    OpenEXR image files with 16-bit floating-point green,
//    and 32-bit floating point depth channels.
//
//-----------------------------------------------------------------------------

#include <ImfArray.h>

#include <ImfHeader.h>
#include <ImfPartType.h>
#include <ImfChannelList.h>
#include <ImfDeepFrameBuffer.h>
#include <ImfDeepTiledInputFile.h>
#include <ImfDeepTiledOutputFile.h>

#include "drawImage.h"

#include "namespaceAlias.h"
using namespace IMF;
using namespace std;
using namespace IMATH_NAMESPACE;


// defined in deepExamples.cpp
extern Array2D<float> testDataZ;
extern Array2D<half> testDataA;
extern unsigned int getPixelSampleCount (int i, int j);
extern void getPixelSampleData(
    int i,
    int j,
    Array2D<float*>& dataZ,
    Array2D<half*>& dataA);


void
readDeepTiledFile (
    const char             filename[],
    Box2i&                 displayWindow,
    Box2i&                 dataWindow,
    Array2D<float*>&       dataZ,
    Array2D<half*>&        dataA,
    Array2D<unsigned int>& sampleCount)
{
    //
    // Read a deep image using class DeepTiledInputFile.  Try to read one
    // channel, A, of type HALF, and one channel, Z,
    // of type FLOAT.  Store the A, and Z pixels in two
    // separate memory buffers.
    //
    //    - open the file
    //    - allocate memory for the pixels
    //    - describe the layout of the A, and Z pixel buffers
    //    - read the sample counts from the file
    //    - allocate the memory requred to store the samples
    //    - read the pixels from the file
    //
    
    DeepTiledInputFile file (filename);
    
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
        "Z",
        DeepSlice (
            FLOAT,
            (char*) (&dataZ[0][0] - dataWindow.min.x - dataWindow.min.y * width),
            sizeof (float*) * 1,     // xStride for pointer array
            sizeof (float*) * width, // yStride for pointer array
            sizeof (float) * 1));    // stride for samples
    
    frameBuffer.insert (
        "A",
        DeepSlice (
            HALF,
            (char*) (&dataA[0][0] - dataWindow.min.x - dataWindow.min.y * width),
            sizeof (half*) * 1,     // xStride for pointer array
            sizeof (half*) * width, // yStride for pointer array
            sizeof (half) * 1));    // stride for samples
    
    file.setFrameBuffer (frameBuffer);
    
    int numXTiles = file.numXTiles (0);
    int numYTiles = file.numYTiles (0);
    
    file.readPixelSampleCounts (0, numXTiles - 1, 0, numYTiles - 1);
    
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            dataZ[i][j] = new float[sampleCount[i][j]];
            dataA[i][j] = new half[sampleCount[i][j]];
        }
    }
        
    file.readTiles (0, numXTiles - 1, 0, numYTiles - 1);
    
    // (after read data is processed, data must be freed:)
    
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            delete[] dataZ[i][j];
            delete[] dataA[i][j];
        }
    }
}

void getSampleDataForTile (
   int i,
   int j,
   int tileSizeX,
   int tileSizeY,
   Array2D<unsigned int>& sampleCount,
   Array2D<float*>& dataZ,
   Array2D<half*>& dataA)
{
    for (int k = 0; k < tileSizeY; k++)
    {
        int y = j * tileSizeY + k;
        if (y >= sampleCount.height()) break;
        
        for (int l = 0; l < tileSizeX; l++)
        {
            int x = i * tileSizeX + l;
            if (x >= sampleCount.width()) break;
            
            sampleCount[y][x] = getPixelSampleCount(y, x);
            
            dataZ[y][x] = new float[sampleCount[y][x]];
            dataA[y][x] = new half [sampleCount[y][x]];
            
            getPixelSampleData(y, x, dataZ, dataA);
        }
    }
}

void
writeDeepTiledFile (
    const char filename[],
    Box2i      displayWindow,
    Box2i      dataWindow,
    int        tileSizeX,
    int        tileSizeY)
{
    //
    // Write a deep image with only a A (alpha) and a Z (depth) channel,
    // using class DeepTiledOutputFile.
    //
    //    - create a file header
    //    - add A and Z channels to the header
    //    - open the file, and store the header in the file
    //    - describe the memory layout of the A and Z pixels
    //    - store the pixels in the file
    //
    
    int height = dataWindow.max.y - dataWindow.min.y + 1;
    int width  = dataWindow.max.x - dataWindow.min.x + 1;

    Header header (displayWindow, dataWindow);
    header.channels ().insert ("Z", Channel (FLOAT));
    header.channels ().insert ("A", Channel (HALF));
    header.setType (DEEPTILE);
    header.compression () = ZIPS_COMPRESSION;

    header.setTileDescription (
        TileDescription (tileSizeX, tileSizeY, ONE_LEVEL));

    Array2D<float*> dataZ;
    dataZ.resizeErase (height, width);

    Array2D<half*> dataA;
    dataA.resizeErase (height, width);

    Array2D<unsigned int> sampleCount;
    sampleCount.resizeErase (height, width);

    DeepTiledOutputFile file (filename, header);

    DeepFrameBuffer frameBuffer;

    frameBuffer.insertSampleCountSlice (Slice (
        UINT,
        (char*) (&sampleCount[0][0] - dataWindow.min.x - dataWindow.min.y * width),
        sizeof (unsigned int) * 1,       // xStride
        sizeof (unsigned int) * width)); // yStride

    frameBuffer.insert (
        "Z",
        DeepSlice (
            FLOAT,
            (char*) (&dataZ[0][0] - dataWindow.min.x - dataWindow.min.y * width),
            sizeof (float*) * 1,     // xStride for pointer array
            sizeof (float*) * width, // yStride for pointer array
            sizeof (float) * 1));    // stride for samples

    frameBuffer.insert (
        "A",
        DeepSlice (
            HALF,
            (char*) (&dataA[0][0] - dataWindow.min.x - dataWindow.min.y * width),
            sizeof (half*) * 1,     // xStride for pointer array
            sizeof (half*) * width, // yStride for pointer array
            sizeof (half) * 1));    // stride for samples

    file.setFrameBuffer (frameBuffer);

    for (int j = 0; j < file.numYTiles (0); j++)
    {
        for (int i = 0; i < file.numXTiles (0); i++)
        {
            // Generate data for sampleCount, dataZ and dataA.
            getSampleDataForTile (i, j, tileSizeX, tileSizeY, sampleCount, dataZ, dataA);
            file.writeTile (i, j, 0);
        }
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

void deepTiledExamples()
{
    int w = 800;
    int h = 600;
    
    int tileSizeX = 64;
    int tileSizeY = 64;
    
    Box2i window;
    window.min.setValue(0, 0);
    window.max.setValue(w - 1, h - 1);
    
    Array2D<float *> dataZ;
    dataZ.resizeErase(h, w);
    
    Array2D<half *> dataA;
    dataA.resizeErase(h, w);
    
    Array2D<unsigned int> sampleCount;
    sampleCount.resizeErase(h, w);
    
    // Create an image to be used as a source for deep data
    testDataA.resizeErase(h, w);
    testDataZ.resizeErase(h, w);
    drawImage2(testDataA, testDataZ, w, h);
    
    writeDeepTiledFile("testTiled.deep.exr", window, window, tileSizeX, tileSizeY);
    readDeepTiledFile ("testTiled.deep.exr", window, window, dataZ, dataA, sampleCount);
}
