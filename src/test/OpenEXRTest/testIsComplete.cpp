//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <ImfRgbaFile.h>
#include <ImfTiledRgbaFile.h>
#include <ImfArray.h>
#include <stdio.h>
#include <assert.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;
using namespace IMATH_NAMESPACE;


namespace {

void
writeFiles (const char completeScanLinesName[],
	    const char incompleteScanLinesName[],
	    const char completeTilesName[],
	    const char incompleteTilesName[],
	    int width,
	    int height,
	    int tileXSize,
	    int tileYSize)
{
    Array2D <Rgba> pixels (height, width);

    for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x)
	    pixels[y][x] = Rgba (x, y, 0.0, 1.0);

    {
	RgbaOutputFile out (completeScanLinesName, width, height);
	out.setFrameBuffer (&pixels[0][0], 1, width);
	out.writePixels (height);
    }

    {
	RgbaOutputFile out (incompleteScanLinesName, width, height);
	out.setFrameBuffer (&pixels[0][0], 1, width);
	out.writePixels (height - 1);
    }

    {
	TiledRgbaOutputFile out (completeTilesName,
				 width, height,
				 tileXSize, tileYSize,
				 ONE_LEVEL);

	out.setFrameBuffer (&pixels[0][0], 1, width);
        out.writeTiles (0, out.numXTiles() - 1, 0, out.numYTiles() - 1);
    }

    {
	TiledRgbaOutputFile out (incompleteTilesName,
				 width, height,
				 tileXSize, tileYSize,
				 ONE_LEVEL);

	out.setFrameBuffer (&pixels[0][0], 1, width);
        out.writeTiles (0, out.numXTiles() - 1, 0, out.numYTiles() - 2);
    }
}


void
checkFiles (const char completeScanLinesName[],
	    const char incompleteScanLinesName[],
	    const char completeTilesName[],
	    const char incompleteTilesName[])
{
    {
	RgbaInputFile in (completeScanLinesName);
	assert (in.isComplete());
    }

    {
	RgbaInputFile in (incompleteScanLinesName);
	assert (!in.isComplete());
    }

    {
	RgbaInputFile in (completeTilesName);
	assert (in.isComplete());
    }

    {
	RgbaInputFile in (incompleteTilesName);
	assert (!in.isComplete());
    }

    {
	TiledRgbaInputFile in (completeTilesName);
	assert (in.isComplete());
    }

    {
	TiledRgbaInputFile in (incompleteTilesName);
	assert (!in.isComplete());
    }
}

} // namespace


void
testIsComplete (const std::string &tempDir)
{
    try
    {
	cout << "Testing isComplete() function" << endl;

	std::string csl = tempDir + "imf_test_complete_sl.exr";
	std::string icsl = tempDir + "imf_test_incomplete_sl.exr";
	std::string ct = tempDir + "imf_test_complete_t.exr";
	std::string ict = tempDir + "imf_test_incomplete_t.exr";

	writeFiles (csl.c_str(), icsl.c_str(), ct.c_str(), ict.c_str(), 327, 289, 17, 17);
	checkFiles (csl.c_str(), icsl.c_str(), ct.c_str(), ict.c_str());

	remove (csl.c_str());
	remove (icsl.c_str());
	remove (ct.c_str());
	remove (ict.c_str());

	cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
