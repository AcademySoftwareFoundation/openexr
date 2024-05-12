//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//    Code examples that show how class MultiPartInputFile and
//    class MultiPartOutputFile can be used to read and write
//    OpenEXR image files containing multiple images.
//
//-----------------------------------------------------------------------------

#include <ImfHeader.h>
#include <ImfPartType.h>
#include <ImfChannelList.h>
#include <ImfArray.h>

#include <ImfFrameBuffer.h>
#include <ImfDeepFrameBuffer.h>

#include <ImfMultiPartInputFile.h>
#include <ImfMultiPartOutputFile.h>

#include <ImfInputPart.h>
#include <ImfTiledInputPart.h>
#include <ImfDeepScanLineInputPart.h>
#include <ImfDeepTiledInputPart.h>

#include <ImfOutputPart.h>
#include <ImfTiledOutputPart.h>
#include <ImfDeepScanLineOutputPart.h>
#include <ImfDeepTiledOutputPart.h>

#include <vector>
#include <list>

#include "namespaceAlias.h"
using namespace IMF;
using namespace std;
using namespace IMATH_NAMESPACE;

template <class InputPartType, class OutputPartType>
void
copyPixels (
    MultiPartInputFile&  inputFile,
    MultiPartOutputFile& outputFile,
    int                  inputPartNumber,
    int                  outputPartNumber)
{
    InputPartType  inPart (inputFile, inputPartNumber);
    OutputPartType outPart (outputFile, outputPartNumber);
    outPart.copyPixels (inPart);
}

void
copyPixels (
    const std::string&   type,
    MultiPartInputFile&  inputFile,
    MultiPartOutputFile& outputFile,
    int                  inputPartNumber,
    int                  outputPartNumber)
{
    //
    // Copy pixels from a given part of a multipart input file
    // to a given part of a multipart output file.
    //

    if (type == SCANLINEIMAGE)
    {
        copyPixels<InputPart, OutputPart> (
            inputFile, outputFile, inputPartNumber, outputPartNumber);
    }
    else if (type == TILEDIMAGE)
    {
        copyPixels<TiledInputPart, TiledOutputPart> (
            inputFile, outputFile, inputPartNumber, outputPartNumber);
    }
    else if (type == DEEPSCANLINE)
    {
        copyPixels<DeepScanLineInputPart, DeepScanLineOutputPart> (
            inputFile, outputFile, inputPartNumber, outputPartNumber);
    }
    else if (type == DEEPTILE)
    {
        copyPixels<DeepTiledInputPart, DeepTiledOutputPart> (
            inputFile, outputFile, inputPartNumber, outputPartNumber);
    }
}

void
combineFiles ()
{
    //
    // Read multiple single-part input files and write them as a multi-part file.
    // If an input file is multi-part, only one part is copied.
    // All input files dimentions must be the same.
    //

    std::vector<MultiPartInputFile*> inputFiles;
    std::vector<Header>              headers;

    const char* filenames[] = {
        "gz1.exr", "tiledgz1.exr", "test.deep.exr", "testTiled.deep.exr"};

    for (size_t i = 0; i < sizeof (filenames) / sizeof (filenames[0]); i++)
    {
        MultiPartInputFile* in_file = new MultiPartInputFile (filenames[i]);
        Header              header  = in_file->header (0);

        if (!header.hasName ()) { header.setName (filenames[i]); }

        inputFiles.push_back (in_file);
        headers.push_back (header);
    }

    MultiPartOutputFile outputFile (
        "multipart.exr", headers.data (), (int) headers.size ());

    for (size_t i = 0; i < sizeof (filenames) / sizeof (filenames[0]); i++)
    {
        Header&       header = headers[i];
        const string& type   = header.type ();
        copyPixels (type, *inputFiles[i], outputFile, 0, i);
        delete inputFiles[i];
    }
}

void
splitFiles ()
{
    //
    // Read a multi-part input file and write all parts as separate files.
    //

    MultiPartInputFile inputFile ("modified.exr");

    for (int i = 0; i < inputFile.parts (); i++)
    {
        Header header = inputFile.header (i);
        string out_path =
            string ("split_part_") + to_string (i) + string (".exr");
        const string&       type = header.type ();
        MultiPartOutputFile outputFile (out_path.c_str (), &header, 1);
        copyPixels (type, inputFile, outputFile, i, 0);
    }
}

template <typename ValueType>
void
insertSlice (
    Box2i&                    dataWindow,
    FrameBuffer&              frameBuffer,
    const char                name[],
    PixelType                 pixelType,
    list<Array2D<ValueType>>& channels)
{
    //
    // Allocate a pixel buffer and describe the layout of
    // the buffer in the FrameBuffer object.
    //

    size_t width  = dataWindow.max.x - dataWindow.min.x + 1;
    size_t height = dataWindow.max.y - dataWindow.min.y + 1;

    channels.emplace_back ();
    Array2D<ValueType>& buffer = channels.back ();
    buffer.resizeErase (height, width);

    char* base =
        (char*) (&buffer[0][0] - dataWindow.min.x - dataWindow.min.y * width);

    frameBuffer.insert (
        name, // name
        Slice (
            pixelType,                 // type
            base,                      // base
            sizeof (ValueType) * 1,    // xStride
            sizeof (ValueType) * width // yStride
            ));
}

template <typename ValueType>
void
insertDeepSlice (
    Box2i&                     dataWindow,
    DeepFrameBuffer&           frameBuffer,
    const char                 name[],
    PixelType                  pixelType,
    list<Array2D<ValueType*>>& channels)
{
    //
    // Allocate a pixel buffer and describe the layout of
    // the buffer in the DeepFrameBuffer object.
    //

    size_t width  = dataWindow.max.x - dataWindow.min.x + 1;
    size_t height = dataWindow.max.y - dataWindow.min.y + 1;

    channels.emplace_back ();
    Array2D<ValueType*>& buffer = channels.back ();
    buffer.resizeErase (height, width);

    char* base =
        (char*) (&buffer[0][0] - dataWindow.min.x - dataWindow.min.y * width);

    frameBuffer.insert (
        name, // name
        DeepSlice (
            pixelType,                   // type
            base,                        // base
            sizeof (ValueType*) * 1,     // xStride
            sizeof (ValueType*) * width, // yStride
            sizeof (ValueType)           // sample stride
            ));
}

FrameBuffer
setupFramebuffer (
    const Header&            header,
    list<Array2D<uint32_t>>& intChannels,
    list<Array2D<half>>&     halfChannels,
    list<Array2D<float>>&    floatChannels)
{
    //
    // Allocate pixel buffers for all channels specified in the header, describe the layout of
    // the buffers in the FrameBuffer object.
    //

    FrameBuffer frameBuffer;
    Box2i       dataWindow = header.dataWindow ();

    for (auto i = header.channels ().begin (); i != header.channels ().end ();
         i++)
    {
        if (i.channel ().type == UINT)
        {
            insertSlice<uint32_t> (
                dataWindow,
                frameBuffer,
                i.name (),
                i.channel ().type,
                intChannels);
        }
        else if (i.channel ().type == HALF)
        {
            insertSlice<half> (
                dataWindow,
                frameBuffer,
                i.name (),
                i.channel ().type,
                halfChannels);
        }
        else if (i.channel ().type == FLOAT)
        {
            insertSlice<float> (
                dataWindow,
                frameBuffer,
                i.name (),
                i.channel ().type,
                floatChannels);
        }
    }

    return frameBuffer;
}

DeepFrameBuffer
setupDeepFramebuffer (
    const Header&             header,
    Array2D<uint32_t>&        sampleCount,
    list<Array2D<uint32_t*>>& intChannels,
    list<Array2D<half*>>&     halfChannels,
    list<Array2D<float*>>&    floatChannels)
{
    //
    // Allocate pixel buffers for all channels specified in the header, describe the layout of
    // the buffers in the DeepFrameBuffer object.
    //

    DeepFrameBuffer frameBuffer;

    Box2i dataWindow = header.dataWindow ();

    size_t width  = dataWindow.max.x - dataWindow.min.x + 1;
    size_t height = dataWindow.max.y - dataWindow.min.y + 1;

    sampleCount.resizeErase (height, width);

    frameBuffer.insertSampleCountSlice (Slice (
        UINT,
        (char*) (&sampleCount[0][0] - dataWindow.min.x -
                 dataWindow.min.y * width),
        sizeof (unsigned int) * 1,    // xStride
        sizeof (unsigned int) * width // yStride
        ));

    for (auto i = header.channels ().begin (); i != header.channels ().end ();
         i++)
    {
        if (i.channel ().type == UINT)
        {
            insertDeepSlice<uint32_t> (
                dataWindow,
                frameBuffer,
                i.name (),
                i.channel ().type,
                intChannels);
        }
        else if (i.channel ().type == HALF)
        {
            insertDeepSlice<half> (
                dataWindow,
                frameBuffer,
                i.name (),
                i.channel ().type,
                halfChannels);
        }
        else if (i.channel ().type == FLOAT)
        {
            insertDeepSlice<float> (
                dataWindow,
                frameBuffer,
                i.name (),
                i.channel ().type,
                floatChannels);
        }
    }

    return frameBuffer;
}

template <typename T>
void
resizeDeepBuffers (Array2D<uint32_t>& sampleCount, list<Array2D<T*>>& channels)
{
    //
    // Allocate memory for samples in all pixel buffers according to the data in sampleCount buffer.
    //

    for (auto i = channels.begin (); i != channels.end (); i++)
    {
        Array2D<T*>& channel = *i;
        for (int y = 0; y < channel.height (); y++)
        {
            for (int x = 0; x < channel.width (); x++)
            {
                uint32_t count = sampleCount[y][x];
                if (count)
                    channel[y][x]  = new T[count];
                else
                    channel[y][x]  = nullptr;
            }
        }
    }
}

template <typename T>
void
freeDeepBuffers (list<Array2D<T*>>& channels)
{
    for (auto i = channels.begin (); i != channels.end (); i++)
    {
        Array2D<T*>& channel = *i;
        for (int y = 0; y < channel.height (); y++)
            for (int x = 0; x < channel.width (); x++)
                delete[] channel[y][x];
    }
}

template <typename T>
void
modifyChannels (list<Array2D<T>>& channels, T delta)
{
    //
    // Dummy code modifying each pixel by incrementing every channel by a given delta.
    //

    for (auto i = channels.begin (); i != channels.end (); i++)
    {
        Array2D<T>& channel = *i;

        for (int y = 0; y < channel.height (); y++)
        {
            for (int x = 0; x < channel.width (); x++)
            {
                channel[y][x] += delta;
            }
        }
    }
}

template <typename T>
void
modifyDeepChannels (
    Array2D<uint32_t>& sampleCount, list<Array2D<T*>>& channels, T delta)
{
    //
    // Dummy code modifying each deep pixel by incrementing every sample of each channel by a given delta.
    //

    for (auto i = channels.begin (); i != channels.end (); i++)
    {
        Array2D<T*>& channel = *i;

        for (int y = 0; y < channel.height (); y++)
        {
            for (int x = 0; x < channel.width (); x++)
            {
                uint32_t count = sampleCount[y][x];
                for (uint32_t j = 0; j < count; j++)
                    channel[y][x][j] += delta;
            }
        }
    }
}

void
modifyMultipart ()
{
    //
    // Read all channels from a multi-part file, modify each pixel value, write the modified data as a multi-part file.
    // The parts in the file can be scanline- or tile-based, either flat or deep.
    // Every channel of the input file gets modified.
    //

    MultiPartInputFile inputFile ("multipart.exr");

    std::vector<Header> headers (inputFile.parts ());

    for (int i = 0; i < inputFile.parts (); i++)
    {
        headers[i] = inputFile.header (i);
    }

    MultiPartOutputFile outputFile (
        "modified.exr", headers.data (), (int) headers.size ());

    for (int i = 0; i < inputFile.parts (); i++)
    {
        Header& header = headers[i];

        const string& type = header.type ();

        if (type == SCANLINEIMAGE || type == TILEDIMAGE)
        {
            list<Array2D<uint32_t>> intChannels;
            list<Array2D<half>>     halfChannels;
            list<Array2D<float>>    floatChannels;

            FrameBuffer frameBuffer = setupFramebuffer (
                header, intChannels, halfChannels, floatChannels);

            if (type == SCANLINEIMAGE)
            {
                InputPart inputPart (inputFile, i);
                inputPart.setFrameBuffer (frameBuffer);
                inputPart.readPixels (
                    header.dataWindow ().min.y, header.dataWindow ().max.y);
            }
            else
            {
                TiledInputPart inputPart (inputFile, i);
                inputPart.setFrameBuffer (frameBuffer);
                inputPart.readTiles (
                    0,
                    inputPart.numXTiles () - 1,
                    0,
                    inputPart.numYTiles () - 1);
            }

            modifyChannels<uint32_t> (intChannels, 1);
            modifyChannels<half> (halfChannels, 0.3);
            modifyChannels<float> (floatChannels, 0.5);

            if (type == SCANLINEIMAGE)
            {
                Box2i      dataWindow = header.dataWindow ();
                OutputPart outputPart (outputFile, i);
                outputPart.setFrameBuffer (frameBuffer);
                outputPart.writePixels (
                    dataWindow.max.y - dataWindow.min.y + 1);
            }
            else
            {
                TiledOutputPart outputPart (outputFile, i);
                outputPart.setFrameBuffer (frameBuffer);
                outputPart.writeTiles (
                    0,
                    outputPart.numXTiles () - 1,
                    0,
                    outputPart.numYTiles () - 1);
            }
        }
        else if (type == DEEPSCANLINE || type == DEEPTILE)
        {
            Array2D<uint32_t>        sampleCount;
            list<Array2D<uint32_t*>> intChannels;
            list<Array2D<half*>>     halfChannels;
            list<Array2D<float*>>    floatChannels;

            DeepFrameBuffer frameBuffer = setupDeepFramebuffer (
                header, sampleCount, intChannels, halfChannels, floatChannels);

            if (type == DEEPSCANLINE)
            {
                DeepScanLineInputPart inputPart (inputFile, i);
                inputPart.setFrameBuffer (frameBuffer);
                inputPart.readPixelSampleCounts (
                    header.dataWindow ().min.y, header.dataWindow ().max.y);

                resizeDeepBuffers<uint32_t> (sampleCount, intChannels);
                resizeDeepBuffers<half> (sampleCount, halfChannels);
                resizeDeepBuffers<float> (sampleCount, floatChannels);

                inputPart.readPixels (
                    header.dataWindow ().min.y, header.dataWindow ().max.y);
            }
            else
            {
                DeepTiledInputPart inputPart (inputFile, i);
                inputPart.setFrameBuffer (frameBuffer);
                inputPart.readPixelSampleCounts (
                    0,
                    inputPart.numXTiles () - 1,
                    0,
                    inputPart.numYTiles () - 1);

                resizeDeepBuffers<uint32_t> (sampleCount, intChannels);
                resizeDeepBuffers<half> (sampleCount, halfChannels);
                resizeDeepBuffers<float> (sampleCount, floatChannels);

                inputPart.readTiles (
                    0,
                    inputPart.numXTiles () - 1,
                    0,
                    inputPart.numYTiles () - 1);
            }

            modifyDeepChannels<uint32_t> (sampleCount, intChannels, 1);
            modifyDeepChannels<half> (sampleCount, halfChannels, 0.3);
            modifyDeepChannels<float> (sampleCount, floatChannels, 0.5);

            if (type == DEEPSCANLINE)
            {
                Box2i                  dataWindow = header.dataWindow ();
                DeepScanLineOutputPart outputPart (outputFile, i);
                outputPart.setFrameBuffer (frameBuffer);
                outputPart.writePixels (
                    dataWindow.max.y - dataWindow.min.y + 1);
            }
            else
            {
                DeepTiledOutputPart outputPart (outputFile, i);
                outputPart.setFrameBuffer (frameBuffer);
                outputPart.writeTiles (
                    0,
                    outputPart.numXTiles () - 1,
                    0,
                    outputPart.numYTiles () - 1);
            }

            freeDeepBuffers (intChannels);
            freeDeepBuffers (halfChannels);
            freeDeepBuffers (floatChannels);
        }
    }
}

void
multipartExamples ()
{
    // Read multiple single-part files and write them out as a single multi-part file.
    combineFiles ();

    // Read all parts from a multi-part file, modify each channel of every pixel by incrementing its value, write out as a multi-part file.
    modifyMultipart ();

    // Read a multi-part file and write out as multiple single-part files.
    splitFiles ();
}
