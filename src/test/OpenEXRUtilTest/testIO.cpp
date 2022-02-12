//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <Iex.h>
#include <ImfDeepImage.h>
#include <ImfFlatImage.h>
#include <ImfHeader.h>
#include <ImfImageIO.h>
#include <ImfStandardAttributes.h>

#include <cassert>
#include <cstdio>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;
using namespace IEX_NAMESPACE;
using namespace std;

namespace
{

void
testFlatScanLineImage1 (const string& fileName)
{
    FlatImage img1 (Box2i (V2i (0, 0), V2i (5, 5)), ONE_LEVEL, ROUND_DOWN);
    img1.insertChannel ("H", HALF);
    saveImage (fileName, img1);

    Header hdr;
    Image* img2 = loadImage (fileName, hdr);

    assert (dynamic_cast<FlatImage*> (img2));
    assert (img2->levelMode () == ONE_LEVEL);
    assert (img2->levelRoundingMode () == ROUND_DOWN);
    assert (!hdr.hasTileDescription ());

    delete img2;
}

void
testFlatScanLineImage2 (const string& fileName)
{
    FlatImage img1 (Box2i (V2i (0, 0), V2i (5, 5)), ONE_LEVEL, ROUND_DOWN);
    img1.insertChannel ("H", HALF);

    Header hdr1;
    addComments (hdr1, "it's raining");

    saveImage (fileName, hdr1, img1);

    Header hdr2;
    Image* img2 = loadImage (fileName, hdr2);

    assert (dynamic_cast<FlatImage*> (img2));
    assert (img2->levelMode () == ONE_LEVEL);
    assert (img2->levelRoundingMode () == ROUND_DOWN);
    assert (!hdr2.hasTileDescription ());
    assert (hasComments (hdr2));
    assert (comments (hdr2) == "it's raining");

    delete img2;
}

void
testFlatTiledImage1 (const string& fileName)
{
    FlatImage img1 (Box2i (V2i (0, 0), V2i (5, 5)), MIPMAP_LEVELS, ROUND_DOWN);
    img1.insertChannel ("H", HALF);
    saveImage (fileName, img1);

    Header hdr;
    Image* img2 = loadImage (fileName, hdr);

    assert (dynamic_cast<FlatImage*> (img2));
    assert (img2->levelMode () == MIPMAP_LEVELS);
    assert (img2->levelRoundingMode () == ROUND_DOWN);
    assert (hdr.hasTileDescription ());

    delete img2;
}

#if 0
void
testFlatTiledImage2 (const string &fileName)
{
    FlatImage img1 (Box2i (V2i (0, 0), V2i (5, 5)), ONE_LEVEL, ROUND_DOWN);
    img1.insertChannel ("H", HALF);

    Header hdr1;
    hdr1.setTileDescription (TileDescription (32, 32));

    saveImage (fileName, hdr1, img1);

    Header hdr2;
    Image* img2 = loadImage (fileName, hdr2);

    assert (dynamic_cast <FlatImage *> (img2));
    assert (img2->levelMode() == ONE_LEVEL);
    assert (img2->levelRoundingMode() == ROUND_DOWN);
    assert (hdr2.hasTileDescription());

    delete img2;
}
#endif

void
testDeepScanLineImage1 (const string& fileName)
{
    DeepImage img1 (Box2i (V2i (0, 0), V2i (5, 5)), ONE_LEVEL, ROUND_DOWN);
    img1.insertChannel ("H", HALF);
    saveImage (fileName, img1);

    Header hdr;
    Image* img2 = loadImage (fileName, hdr);

    assert (dynamic_cast<DeepImage*> (img2));
    assert (img2->levelMode () == ONE_LEVEL);
    assert (img2->levelRoundingMode () == ROUND_DOWN);
    assert (!hdr.hasTileDescription ());

    delete img2;
}

void
testDeepScanLineImage2 (const string& fileName)
{
    DeepImage img1 (Box2i (V2i (0, 0), V2i (5, 5)), ONE_LEVEL, ROUND_DOWN);
    img1.insertChannel ("H", HALF);

    Header hdr1;
    addComments (hdr1, "it's raining");

    saveImage (fileName, hdr1, img1);

    Header hdr2;
    Image* img2 = loadImage (fileName, hdr2);

    assert (dynamic_cast<DeepImage*> (img2));
    assert (img2->levelMode () == ONE_LEVEL);
    assert (img2->levelRoundingMode () == ROUND_DOWN);
    assert (!hdr2.hasTileDescription ());
    assert (hasComments (hdr2));
    assert (comments (hdr2) == "it's raining");

    delete img2;
}

void
testDeepTiledImage1 (const string& fileName)
{
    DeepImage img1 (Box2i (V2i (0, 0), V2i (5, 5)), MIPMAP_LEVELS, ROUND_DOWN);
    img1.insertChannel ("H", HALF);
    saveImage (fileName, img1);

    Header hdr;
    Image* img2 = loadImage (fileName, hdr);

    assert (dynamic_cast<DeepImage*> (img2));
    assert (img2->levelMode () == MIPMAP_LEVELS);
    assert (img2->levelRoundingMode () == ROUND_DOWN);
    assert (hdr.hasTileDescription ());

    delete img2;
}

void
testDeepTiledImage2 (const string& fileName)
{
    DeepImage img1 (Box2i (V2i (0, 0), V2i (5, 5)), ONE_LEVEL, ROUND_DOWN);
    img1.insertChannel ("H", HALF);

    Header hdr1;
    hdr1.setTileDescription (TileDescription (32, 32));

    saveImage (fileName, hdr1, img1);

    Header hdr2;
    Image* img2 = loadImage (fileName, hdr2);

    assert (dynamic_cast<DeepImage*> (img2));
    assert (img2->levelMode () == ONE_LEVEL);
    assert (img2->levelRoundingMode () == ROUND_DOWN);
    assert (hdr2.hasTileDescription ());

    delete img2;
}

} // namespace

void
testIO (const string& tempDir)
{
    try
    {
        cout << "Testing I/O based on image and file type" << endl;

        testFlatScanLineImage1 (tempDir + "io.exr");
        testFlatScanLineImage2 (tempDir + "io.exr");
        testFlatTiledImage1 (tempDir + "io.exr");
        testFlatTiledImage1 (tempDir + "io.exr");
        testDeepScanLineImage1 (tempDir + "io.exr");
        testDeepScanLineImage2 (tempDir + "io.exr");
        testDeepTiledImage1 (tempDir + "io.exr");
        testDeepTiledImage2 (tempDir + "io.exr");

        cout << "ok\n" << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
    remove ((tempDir + "io.exr").c_str ());
}
