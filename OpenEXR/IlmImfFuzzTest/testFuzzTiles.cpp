///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2007, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////


#include <tmpDir.h>
#include <fuzzFile.h>

#include <ImfTiledRgbaFile.h>
#include <ImfArray.h>
#include <ImfThreading.h>
#include <IlmThread.h>
#include <Iex.h>
#include <iostream>
#include <cassert>

using namespace std;
using namespace Imath;
using namespace Imf;

namespace {

void
fillPixels (Array2D<Rgba> &pixels, int w, int h)
{
    for (int y = 0; y < h; ++y)
    {
	for (int x = 0; x < w; ++x)
	{
	    Rgba &p = pixels[y][x];

	    p.r = 0.5 + 0.5 * sin (0.1 * x + 0.1 * y);
	    p.g = 0.5 + 0.5 * sin (0.1 * x + 0.2 * y);
	    p.b = 0.5 + 0.5 * sin (0.1 * x + 0.3 * y);
	    p.a = (p.r + p.b + p.g) / 3.0;
	}
    }
}


void
writeImageONE (const char fileName[],
	       int width, int height,
	       int xSize, int ySize,
	       Compression comp)
{
    cout << "levelMode 0" << ", compression " << comp << endl;

    Header header (width, height);
    header.lineOrder() = INCREASING_Y;
    header.compression() = comp;
    
    Array2D<Rgba> pixels (height, width);
    fillPixels (pixels, width, height);

    TiledRgbaOutputFile out (fileName, header, WRITE_RGBA,
			     xSize, ySize, ONE_LEVEL);
        
    out.setFrameBuffer (&pixels[0][0], 1, width);
    out.writeTiles (0, out.numXTiles() - 1, 0, out.numYTiles() - 1);
}


void
readImageONE (const char fileName[])
{
    //
    // Try to read the specified one-level file, which may be damaged.
    // Reading should either succeed or throw an exception, but it
    // should not cause a crash.
    //

    try
    {
        TiledRgbaInputFile in (fileName);
        const Box2i &dw = in.dataWindow();

        int w = dw.max.x - dw.min.x + 1;
        int h = dw.max.y - dw.min.y + 1;
        int dwx = dw.min.x;
        int dwy = dw.min.y;

        Array2D<Rgba> pixels (h, w);
        in.setFrameBuffer (&pixels[-dwy][-dwx], 1, w);
        in.readTiles (0, in.numXTiles() - 1, 0, in.numYTiles() - 1);
    }
    catch (...)
    {
	// empty
    }
}


void
writeImageMIP (const char fileName[],
	       int width, int height,
	       int xSize, int ySize,
	       Compression comp)
{
    cout << "levelMode 1" << ", compression " << comp << endl;

    Header header (width, height);
    header.lineOrder() = INCREASING_Y;
    header.compression() = comp;
    
    Array < Array2D<Rgba> > levels;

    TiledRgbaOutputFile out (fileName, header, WRITE_RGBA,
			     xSize, ySize, MIPMAP_LEVELS, ROUND_DOWN);
    
    int numLevels = out.numLevels();
    levels.resizeErase (numLevels);

    for (int level = 0; level < out.numLevels(); ++level)
    {
	int levelWidth  = out.levelWidth(level);
	int levelHeight = out.levelHeight(level);
	levels[level].resizeErase(levelHeight, levelWidth);
	fillPixels (levels[level], levelWidth, levelHeight);
    
	out.setFrameBuffer (&(levels[level])[0][0], 1, levelWidth);
	out.writeTiles (0, out.numXTiles(level) - 1,
			0, out.numYTiles(level) - 1, level);
    }
}


void
readImageMIP (const char fileName[])
{
    //
    // Try to read the specified mipmap file, which may be damaged.
    // Reading should either succeed or throw an exception, but it
    // should not cause a crash.
    //

    try
    {
        TiledRgbaInputFile in (fileName);
        const Box2i &dw = in.dataWindow();
        int dwx = dw.min.x;
        int dwy = dw.min.y;

        int numLevels = in.numLevels();
        Array < Array2D<Rgba> > levels2 (numLevels);
        
        for (int level = 0; level < numLevels; ++level)
        {
            int levelWidth = in.levelWidth(level);
            int levelHeight = in.levelHeight(level);
            levels2[level].resizeErase(levelHeight, levelWidth);
        
            in.setFrameBuffer (&(levels2[level])[-dwy][-dwx], 1, levelWidth);
            in.readTiles (0, in.numXTiles(level) - 1,
                          0, in.numYTiles(level) - 1, level);
        }
    }
    catch (...)
    {
	// empty
    }
}


void
writeImageRIP (const char fileName[],
	       int width, int height,
	       int xSize, int ySize,
	       Compression comp)
{
    cout << "levelMode 2" << ", compression " << comp << endl;

    Header header (width, height);
    header.lineOrder() = INCREASING_Y;
    header.compression() = comp;
    
    Array2D < Array2D<Rgba> > levels;

    TiledRgbaOutputFile out (fileName, header, WRITE_RGBA,
			     xSize, ySize, RIPMAP_LEVELS, ROUND_UP);

    levels.resizeErase (out.numYLevels(), out.numXLevels());

    for (int ylevel = 0; ylevel < out.numYLevels(); ++ylevel)
    {            
	for (int xlevel = 0; xlevel < out.numXLevels(); ++xlevel)
	{
	    int levelWidth = out.levelWidth(xlevel);
	    int levelHeight = out.levelHeight(ylevel);
	    levels[ylevel][xlevel].resizeErase(levelHeight, levelWidth);
	    fillPixels (levels[ylevel][xlevel], levelWidth, levelHeight);

	    out.setFrameBuffer (&(levels[ylevel][xlevel])[0][0], 1,
				levelWidth); 
	    out.writeTiles (0, out.numXTiles(xlevel) - 1,
			    0, out.numYTiles(ylevel) - 1, xlevel, ylevel);
	}
    }
}


void
readImageRIP (const char fileName[])
{
    //
    // Try to read the specified ripmap file, which may be damaged.
    // Reading should either succeed or throw an exception, but it
    // should not cause a crash.
    //

    try
    {
        TiledRgbaInputFile in (fileName);
        const Box2i &dw = in.dataWindow();
        int dwx = dw.min.x;
        int dwy = dw.min.y;        
        
        int numXLevels = in.numXLevels();
        int numYLevels = in.numYLevels();
	Array2D < Array2D<Rgba> > levels2 (numYLevels, numXLevels);
        
        for (int ylevel = 0; ylevel < numYLevels; ++ylevel)
        {
            for (int xlevel = 0; xlevel < numXLevels; ++xlevel)
            {
                int levelWidth  = in.levelWidth(xlevel);
                int levelHeight = in.levelHeight(ylevel);
                levels2[ylevel][xlevel].resizeErase(levelHeight, levelWidth);
                in.setFrameBuffer (&(levels2[ylevel][xlevel])[-dwy][-dwx], 1,
                                   levelWidth);
                                   
                in.readTiles (0, in.numXTiles(xlevel) - 1,
                              0, in.numYTiles(ylevel) - 1, xlevel, ylevel);
            }
        }
    }
    catch (...)
    {
	// empty
    }
}


void
fuzzTiles (int numThreads, Rand48 &random)
{
    if (IlmThread::supportsThreads())
    {
	setGlobalThreadCount (numThreads);
	cout << "\nnumber of threads: " << globalThreadCount() << endl;
    }

    Header::setMaxImageSize (10000, 10000);
    Header::setMaxTileSize (10000, 10000);

    const int W = 217;
    const int H = 197;
    const int TW = 64;
    const int TH = 64;

    const char *goodFile = IMF_TMP_DIR "imf_test_file_fuzz_good.exr";
    const char *brokenFile = IMF_TMP_DIR "imf_test_file_fuzz_broken.exr";

    for (int comp = 0; comp < NUM_COMPRESSION_METHODS; ++comp)
    {
	writeImageONE (goodFile, W, H, TW, TH, Compression (comp));
	fuzzFile (goodFile, brokenFile, readImageONE, 5000, 3000, random);

	writeImageMIP (goodFile, W, H, TW, TH, Compression (comp));
	fuzzFile (goodFile, brokenFile, readImageMIP, 5000, 3000, random);

	writeImageRIP (goodFile, W, H, TW, TH, Compression (comp));
	fuzzFile (goodFile, brokenFile, readImageRIP, 5000, 3000, random);
    }

    remove (goodFile);
    remove (brokenFile);
}

} // namespace


void
testFuzzTiles ()
{
    try
    {
	cout << "Testing tile-based files "
		"with randomly inserted errors" << endl;

	Rand48 random (5);

	fuzzTiles (0, random);

	if (IlmThread::supportsThreads())
	    fuzzTiles (2, random);

	cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
