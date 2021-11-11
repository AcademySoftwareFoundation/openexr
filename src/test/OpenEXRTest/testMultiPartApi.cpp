//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tmpDir.h"
#include "testMultiPartApi.h"
#include "random.h"

#include <ImfPartType.h>
#include <ImfMultiPartInputFile.h>
#include <ImfMultiPartOutputFile.h>
#include <ImfOutputFile.h>
#include <ImfTiledOutputFile.h>
#include <ImfGenericOutputFile.h>
#include <ImfArray.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfOutputPart.h>
#include <ImfInputPart.h>
#include <ImfTiledOutputPart.h>
#include <ImfTiledInputPart.h>
#include <ImfTiledMisc.h>

namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace IMF;
using namespace std;
using namespace IMATH_NAMESPACE;

namespace
{

const int height = 263;
const int width = 197;

struct Task
{
        int partNumber;
        int tx, ty, lx, ly;

        Task(int partNumber):
            partNumber(partNumber)
        {}

        Task(int partNumber, int tx, int ty, int lx, int ly):
            partNumber(partNumber),
            tx(tx),
            ty(ty),
            lx(lx),
            ly(ly)
        {}
};

vector<Header> headers;
vector<int> pixelTypes;
vector<int> partTypes;
vector<int> levelModes;

template <class T>
void fillPixels (Array2D<T> &ph, int width, int height)
{
    ph.resizeErase(height, width);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            //
            // We do this because half cannot store number bigger than 2048 exactly.
            //
            ph[y][x] = (y * width + x) % 2049;
        }
}

template <class T>
bool checkPixels (Array2D<T> &ph, int lx, int rx, int ly, int ry, int width)
{
    for (int y = ly; y <= ry; ++y)
        for (int x = lx; x <= rx; ++x)
            if (ph[y][x] != static_cast<T>(((y * width + x) % 2049)))
            {
                cout << "value at " << x << ", " << y << ": " << ph[y][x]
                     << ", should be " << (y * width + x) % 2049 << endl << flush;
                return false;
            }
    return true;
}

template <class T>
bool checkPixels (Array2D<T> &ph, int width, int height)
{
    return checkPixels<T> (ph, 0, width - 1, 0, height - 1, width);
}

void generateRandomHeaders(int partCount, vector<Header>& headers, vector<Task>& taskList)
{
    headers.clear();
    for (int i = 0; i < partCount; i++)
    {
        Header header(width, height);
        int pixelType = random_int(3);
        int partType = random_int(2);
        pixelTypes[i] = pixelType;
        partTypes[i] = partType;

        stringstream ss;
        ss << i;
        header.setName(ss.str());

        switch (pixelType)
        {
            case 0:
                header.channels().insert("UINT", Channel(IMF::UINT));
                break;
            case 1:
                header.channels().insert("FLOAT", Channel(IMF::FLOAT));
                break;
            case 2:
                header.channels().insert("HALF", Channel(IMF::HALF));
                break;
        }

        switch (partType)
        {
            case 0:
                header.setType(SCANLINEIMAGE);
                break;
            case 1:
                header.setType(TILEDIMAGE);
                break;
        }

        int tileX;
        int tileY;
        int levelMode = 0;
        if (partType == 1)
        {
            tileX = random_int(width) + 1;
            tileY = random_int(height) + 1;
            levelMode = random_int(3);
            levelModes[i] = levelMode;
            LevelMode lm = NUM_LEVELMODES;
            switch (levelMode)
            {
                case 0:
                    lm = ONE_LEVEL;
                    break;
                case 1:
                    lm = MIPMAP_LEVELS;
                    break;
                case 2:
                    lm = RIPMAP_LEVELS;
                    break;
            }
            header.setTileDescription(TileDescription(tileX, tileY, lm));
        }

        //
        // Add lines or tiles to task list.
        //
        if (partType == 0)
        {
            for (int j = 0; j < height; j++)
                taskList.push_back(Task(i));
        }
        else
        {
            int numXLevel;
            int numYLevel;
            int* numXTiles;
            int* numYTiles;
            precalculateTileInfo (header.tileDescription(),
                                  0, width - 1,
                                  0, height - 1,
                                  numXTiles, numYTiles,
                                  numXLevel, numYLevel);

            for (int lx = 0; lx < numXLevel; lx++)
                for (int ly = 0; ly < numYLevel; ly++)
                {
                    if (levelMode == 1)
                        if (lx != ly) continue;

                    // Get all tasks for this level.
                    for (int tx = 0; tx < numXTiles[lx]; tx++)
                        for (int ty = 0; ty < numYTiles[ly]; ty++)
                            taskList.push_back(Task(i, tx, ty, lx, ly));
                }

            delete[] numXTiles;
            delete[] numYTiles;
        }

//        if (partType == 0)
//        {
//            cout << "pixelType = " << pixelType << " partType = " << partType
//                 << endl << flush;
//        }
//        else
//        {
//            cout << "pixelType = " << pixelType << " partType = " << partType
//                 << " levelMode = " << levelModes[i] << endl << flush;
//        }

        headers.push_back(header);
    }
}

void setOutputFrameBuffer(FrameBuffer& frameBuffer, int pixelType,
                          Array2D<unsigned int>& uData, Array2D<float>& fData,
                          Array2D<half>& hData, int width)
{
    switch (pixelType)
    {
        case 0:
            frameBuffer.insert ("UINT",
                                Slice (IMF::UINT,
                                (char *) (&uData[0][0]),
                                sizeof (uData[0][0]) * 1,
                                sizeof (uData[0][0]) * width));
            break;
        case 1:
            frameBuffer.insert ("FLOAT",
                                Slice (IMF::FLOAT,
                                (char *) (&fData[0][0]),
                                sizeof (fData[0][0]) * 1,
                                sizeof (fData[0][0]) * width));
            break;
        case 2:
            frameBuffer.insert ("HALF",
                                Slice (IMF::HALF,
                                (char *) (&hData[0][0]),
                                sizeof (hData[0][0]) * 1,
                                sizeof (hData[0][0]) * width));
            break;
    }
}

void setInputFrameBuffer(FrameBuffer& frameBuffer, int pixelType,
                         Array2D<unsigned int>& uData, Array2D<float>& fData,
                         Array2D<half>& hData, int width, int height)
{
    switch (pixelType)
    {
        case 0:
            uData.resizeErase(height, width);
            frameBuffer.insert ("UINT",
                                Slice (IMF::UINT,
                                (char *) (&uData[0][0]),
                                sizeof (uData[0][0]) * 1,
                                sizeof (uData[0][0]) * width,
                                1, 1,
                                0));
            break;
        case 1:
            fData.resizeErase(height, width);
            frameBuffer.insert ("FLOAT",
                                Slice (IMF::FLOAT,
                                (char *) (&fData[0][0]),
                                sizeof (fData[0][0]) * 1,
                                sizeof (fData[0][0]) * width,
                                1, 1,
                                0));
            break;
        case 2:
            hData.resizeErase(height, width);
            frameBuffer.insert ("HALF",
                                Slice (IMF::HALF,
                                (char *) (&hData[0][0]),
                                sizeof (hData[0][0]) * 1,
                                sizeof (hData[0][0]) * width,
                                1, 1,
                                0));
            break;
    }
}

void
generateRandomFile (int partCount, const std::string & fn)
{
    //
    // Init data.
    //
    Array2D<half> halfData;
    Array2D<float> floatData;
    Array2D<unsigned int> uintData;
    fillPixels<unsigned int>(uintData, width, height);
    fillPixels<half>(halfData, width, height);
    fillPixels<float>(floatData, width, height);

    Array2D< Array2D< half > >* tiledHalfData = new Array2D< Array2D< half > >[partCount];
    Array2D< Array2D< float > >* tiledFloatData = new Array2D< Array2D< float > >[partCount];
    Array2D< Array2D< unsigned int > >* tiledUintData = new Array2D< Array2D< unsigned int > >[partCount];

    vector<GenericOutputFile*> outputfiles;
    vector<Task> taskList;

    pixelTypes.resize(partCount);
    partTypes.resize(partCount);
    levelModes.resize(partCount);

    //
    // Generate headers and data.
    //
    cout << "Generating headers and data " << flush;
    generateRandomHeaders(partCount, headers, taskList);

    //
    // Shuffle tasks.
    //
    cout << "Shuffling " << taskList.size() << " tasks " << flush;
    int taskListSize = taskList.size();
    for (int i = 0; i < taskListSize; i++)
    {
        int a, b;
        a = random_int(taskListSize);
        b = random_int(taskListSize);
        swap(taskList[a], taskList[b]);
    }

    remove(fn.c_str());
    MultiPartOutputFile file(fn.c_str(), &headers[0],headers.size());

    //
    // Writing tasks.
    //
    cout << "Writing tasks " << flush;

    //
    // Pre-generating frameBuffers.
    //
    vector<void *> parts;
    vector<FrameBuffer> frameBuffers(partCount);
    Array<Array2D<FrameBuffer> > tiledFrameBuffers(partCount);
    for (int i = 0; i < partCount; i++)
    {
        if (partTypes[i] == 0)
        {
            OutputPart* part = new OutputPart(file, i);
            parts.push_back((void*) part);

            FrameBuffer& frameBuffer = frameBuffers[i];

            setOutputFrameBuffer(frameBuffer, pixelTypes[i], uintData, floatData, halfData, width);

            part->setFrameBuffer(frameBuffer);
        }
        else
        {
            TiledOutputPart* part = new TiledOutputPart(file, i);
            parts.push_back((void*) part);

            int numXLevels = part->numXLevels();
            int numYLevels = part->numYLevels();

            // Allocating space.
            tiledUintData[i].resizeErase(numYLevels, numXLevels);
            tiledFloatData[i].resizeErase(numYLevels, numXLevels);
            tiledHalfData[i].resizeErase(numYLevels, numXLevels);

            tiledFrameBuffers[i].resizeErase(numYLevels, numXLevels);

            for (int xLevel = 0; xLevel < numXLevels; xLevel++)
                for (int yLevel = 0; yLevel < numYLevels; yLevel++)
                {
                    if (!part->isValidLevel(xLevel, yLevel))
                        continue;

                    int w = part->levelWidth(xLevel);
                    int h = part->levelHeight(yLevel);

                    FrameBuffer& frameBuffer = tiledFrameBuffers[i][yLevel][xLevel];

                    switch (pixelTypes[i])
                    {
                        case 0:
                            fillPixels<unsigned int>(tiledUintData[i][yLevel][xLevel], w, h);
                            break;
                        case 1:
                            fillPixels<float>(tiledFloatData[i][yLevel][xLevel], w, h);
                            break;
                        case 2:
                            fillPixels<half>(tiledHalfData[i][yLevel][xLevel], w, h);
                            break;
                    }
                    setOutputFrameBuffer(frameBuffer, pixelTypes[i],
                                         tiledUintData[i][yLevel][xLevel],
                                         tiledFloatData[i][yLevel][xLevel],
                                         tiledHalfData[i][yLevel][xLevel],
                                         w);
                }
        }
    }

    //
    // Writing tasks.
    //
    for (int i = 0; i < taskListSize; i++)
    {
        int partNumber = taskList[i].partNumber;
        int partType = partTypes[partNumber];

        if (partType == 0)
        {
            OutputPart* part = (OutputPart*) parts[partNumber];
            part->writePixels();
        }
        else
        {
            int tx = taskList[i].tx;
            int ty = taskList[i].ty;
            int lx = taskList[i].lx;
            int ly = taskList[i].ly;
            TiledOutputPart* part = (TiledOutputPart*) parts[partNumber];
            part->setFrameBuffer(tiledFrameBuffers[partNumber][ly][lx]);
            part->writeTile(tx, ty, lx, ly);
        }
    }

    for (size_t i = 0 ; i < parts.size() ; ++i )
    {
        int partType = partTypes[i];

        if (partType == 0)
        {
            delete (OutputPart*) parts[i];
        }
        else
        {
            delete (TiledOutputPart*) parts[i];
        }

    }

    delete[] tiledHalfData;
    delete[] tiledUintData;
    delete[] tiledFloatData;
}

void
readWholeFiles (const std::string & fn)
{
    Array2D<unsigned int> uData;
    Array2D<float> fData;
    Array2D<half> hData;

    MultiPartInputFile file(fn.c_str());
    for (size_t i = 0; i < static_cast<size_t>(file.parts()); i++)
    {
        const Header& header = file.header(i);
        assert (header.displayWindow() == headers[i].displayWindow());
        assert (header.dataWindow() == headers[i].dataWindow());
        assert (header.pixelAspectRatio() == headers[i].pixelAspectRatio());
        assert (header.screenWindowCenter() == headers[i].screenWindowCenter());
        assert (header.screenWindowWidth() == headers[i].screenWindowWidth());
        assert (header.lineOrder() == headers[i].lineOrder());
        assert (header.compression() == headers[i].compression());

        //
        // It rarely fails here. Added code to see what's wrong when it happens.
        //
        ChannelList::ConstIterator i1 = header.channels().begin();
        ChannelList::ConstIterator i2 = headers[i].channels().begin();
        Channel c1 = i1.channel();
        Channel c2 = i2.channel();
        if (!(c1 == c2))
        {
            cout << " type is " << c1.type << ", should be " << c2.type
                 << " xSampling is " << c1.xSampling << ", should be " << c2.xSampling
                 << " ySampling is " << c1.ySampling << ", should be " << c2.ySampling
                 << " pLinear is " << c1.pLinear << ", should be " << c2.pLinear << flush;
        }

        assert (header.channels() == headers[i].channels());
        assert (header.name() == headers[i].name());
        assert (header.type() == headers[i].type());
    }

    cout << "Reading whole files " << flush;

    //
    // Shuffle part numbers.
    //
    vector<int> shuffledPartNumber;
    int nHeaders = static_cast<int>(headers.size());
    for (int i = 0; i < nHeaders; i++)
        shuffledPartNumber.push_back(i);
    for (int i = 0; i < nHeaders; i++)
    {
        int a = random_int(nHeaders);
        int b = random_int(nHeaders);
        swap (shuffledPartNumber[a], shuffledPartNumber[b]);
    }

    //
    // Start reading whole files.
    //
    int i;
    int partNumber;
    try
    {
        for (i = 0; i < nHeaders; i++)
        {
            partNumber = shuffledPartNumber[i];
            if (partTypes[partNumber] == 0)
            {
                FrameBuffer frameBuffer;
                setInputFrameBuffer(frameBuffer, pixelTypes[partNumber],
                                    uData, fData, hData, width, height);

                InputPart part(file, partNumber);
                part.setFrameBuffer(frameBuffer);
                part.readPixels(0, height - 1);
                switch (pixelTypes[partNumber])
                {
                    case 0:
                        assert(checkPixels<unsigned int>(uData, width, height));
                        break;
                    case 1:
                        assert(checkPixels<float>(fData, width, height));
                        break;
                    case 2:
                        assert(checkPixels<half>(hData, width, height));
                        break;
                }
            }
            else
            {
                FrameBuffer frameBuffer;
                TiledInputPart part(file, partNumber);
                int numXLevels = part.numXLevels();
                int numYLevels = part.numYLevels();
                for (int xLevel = 0; xLevel < numXLevels; xLevel++)
                    for (int yLevel = 0; yLevel < numYLevels; yLevel++)
                    {
                        if (!part.isValidLevel(xLevel, yLevel))
                            continue;

                        int w = part.levelWidth(xLevel);
                        int h = part.levelHeight(yLevel);

                        setInputFrameBuffer(frameBuffer, pixelTypes[partNumber],
                                            uData, fData, hData, width, height);

                        part.setFrameBuffer(frameBuffer);
                        int numXTiles = part.numXTiles(xLevel);
                        int numYTiles = part.numYTiles(yLevel);
                        part.readTiles(0, numXTiles - 1, 0, numYTiles - 1, xLevel, yLevel);
                        switch (pixelTypes[partNumber])
                        {
                            case 0:
                                assert(checkPixels<unsigned int>(uData, w, h));
                                break;
                            case 1:
                                assert(checkPixels<float>(fData, w, h));
                                break;
                            case 2:
                                assert(checkPixels<half>(hData, w, h));
                                break;
                        }
                    }
            }
        }
    }
    catch (...)
    {
        cout << "Error while reading part " << partNumber << endl << flush;
        throw;
    }
}

void
readPartialFiles (int randomReadCount, const std::string & fn)
{
    Array2D<unsigned int> uData;
    Array2D<float> fData;
    Array2D<half> hData;

    cout << "Reading partial files " << flush;
    MultiPartInputFile file(fn.c_str());
    //const vector<Header>& headers = file.parts();
    for (int i = 0; i < randomReadCount; i++)
    {
        int partNumber = random_int(headers.size());
        int partType = partTypes[partNumber];
        int pixelType = pixelTypes[partNumber];
        int levelMode = levelModes[partNumber];

        if (partType == 0)
        {
            int l1, l2;
            l1 = random_int(height);
            l2 = random_int(height);
            if (l1 > l2) swap(l1, l2);

            InputPart part(file, partNumber);

            FrameBuffer frameBuffer;
            setInputFrameBuffer(frameBuffer, pixelType,
                                uData, fData, hData, width, height);

            part.setFrameBuffer(frameBuffer);
            part.readPixels(l1, l2);

            switch (pixelType)
            {
                case 0:
                    assert(checkPixels<unsigned int>(uData, 0, width - 1, l1, l2, width));
                    break;
                case 1:
                    assert(checkPixels<float>(fData, 0, width - 1, l1, l2, width));
                    break;
                case 2:
                    assert(checkPixels<half>(hData, 0, width - 1, l1, l2, width));
                    break;
            }
        }
        else
        {
            int tx1, tx2, ty1, ty2;
            int lx, ly;

            TiledInputPart part(file, partNumber);

            int numXLevels = part.numXLevels();
            int numYLevels = part.numYLevels();

            lx = random_int(numXLevels);
            ly = random_int(numYLevels);
            if (levelMode == 1) ly = lx;

            int w = part.levelWidth(lx);
            int h = part.levelHeight(ly);

            int numXTiles = part.numXTiles(lx);
            int numYTiles = part.numYTiles(ly);
            tx1 = random_int(numXTiles);
            tx2 = random_int(numXTiles);
            ty1 = random_int(numYTiles);
            ty2 = random_int(numYTiles);
            if (tx1 > tx2) swap(tx1, tx2);
            if (ty1 > ty2) swap(ty1, ty2);

            FrameBuffer frameBuffer;
            setInputFrameBuffer(frameBuffer, pixelType,
                                uData, fData, hData, w, h);

            part.setFrameBuffer(frameBuffer);
            part.readTiles(tx1, tx2, ty1, ty2, lx, ly);

            Box2i b1 = part.dataWindowForTile(tx1, ty1, lx, ly);
            Box2i b2 = part.dataWindowForTile(tx2, ty2, lx, ly);

            switch (pixelType)
            {
                case 0:
                    assert(checkPixels<unsigned int>(uData, b1.min.x, b2.max.x, b1.min.y, b2.max.y,
                                                     w));
                    break;
                case 1:
                    assert(checkPixels<float>(fData, b1.min.x, b2.max.x, b1.min.y, b2.max.y,
                                              w));
                    break;
                case 2:
                    assert(checkPixels<half>(hData, b1.min.x, b2.max.x, b1.min.y, b2.max.y,
                                             w));
                    break;
            }
        }
    }
}

void
testWriteRead (int partNumber,
               int runCount,
               int randomReadCount,
               const std::string & tempDir)
{
    cout << "Testing file with " << partNumber << " part(s)." << endl << flush;

    std::string fn = tempDir +  "imf_test_multipart_api.exr";

    for (int i = 0; i < runCount; i++)
    {
        generateRandomFile (partNumber, fn);
        readWholeFiles (fn);
        readPartialFiles (randomReadCount, fn);

        remove (fn.c_str());

        cout << endl << flush;
    }
}

} // namespace


void testMultiPartApi (const std::string & tempDir)
{
    try
    {
        cout << "Testing the multi part APIs for normal use" << endl;

        random_reseed(1);

        testWriteRead ( 1, 2,   5, tempDir);
        testWriteRead ( 2, 5,  10, tempDir);
        testWriteRead ( 5, 1,  25, tempDir);
        testWriteRead (50, 2, 100, tempDir);

        cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
        cerr << "ERROR -- caught exception: " << e.what() << endl;
        assert (false);
    }
}
