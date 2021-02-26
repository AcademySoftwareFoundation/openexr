///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004-2012, Industrial Light & Magic, a division of Lucas
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

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "compareB44.h"

#include <ImfOutputFile.h>
#include <ImfInputFile.h>
#include <ImfTiledOutputFile.h>
#include <ImfChannelList.h>
#include <ImfArray.h>
#include <ImathRandom.h>
#include <ImfCompressor.h>
#include <half.h>
#include "compareFloat.h"

#include <stdio.h>
#include <assert.h>
#include <limits>
#include <algorithm>

namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace IMF;
using namespace std;
using namespace IMATH_NAMESPACE;


namespace {

struct pixelArray
{
    Array2D<unsigned int> i;
    Array2D<float> f;
    Array2D<half> h;
    Array2D<half> rgba[4];
    pixelArray( int height , int width ) :
          i(height,width) ,
          f(height,width) ,
          h(height,width)
    {
        for(int c = 0 ; c < 4 ; ++c)
        {
            rgba[c].resizeErase(height, width);
        }
    }
};



void
fillPixels1 (pixelArray& array,
	     int width,
	     int height)
{
    cout << "only zeroes" << endl;

    for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x)
	{
	    array.i[y][x] = 0;
	    array.h[y][x] = 0;
	    array.f[y][x] = 0;
            for(int c = 0 ; c < 4 ; ++c)
            {
                array.rgba[c][y][x] = 0;
            }
	}
}


void
fillPixels2 (pixelArray& array,
	     int width,
	     int height)
{
    cout << "pattern 1" << endl;

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            array.i[y][x] = (x + y) & 1;
            array.h[y][x] = array.i[y][x];
            array.f[y][x] = array.i[y][x] ;
            for(int c = 0 ; c < 4 ; ++c)
            {
                array.rgba[c][y][x] = array.i[y][x];
            }
        }
}


void
fillPixels3 (pixelArray& array,
	     int width,
	     int height)
{
    cout << "pattern 2" << endl;

    for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x)
	{

	    array.i[y][x] = x % 100 + 100 * (y % 100);
	    array.h[y][x] = sin (double (x)) + sin (y * 0.5);
	    array.f[y][x] = sin (double (y)) + sin (x * 0.5);
            for(int c = 0 ; c < 4 ; ++c)
            {
                array.rgba[c][y][x] = sin( double(x+c)) + sin(y*0.5);
            }
	}
}


void
fillPixels4 (pixelArray& array,
	     int width,
	     int height)
{
    cout << "random bits" << endl;

    //
    // Use of a union to extract the bit pattern from a float, as is
    // done below, works only if int and float have the same size.
    //

    assert (sizeof (int) == sizeof (float));

    Rand48 rand;

    for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x)
	{
	    array.i[y][x] = rand.nexti();


	    array.h[y][x].setBits (rand.nexti());
            for(int c = 0 ; c < 4 ; ++c )
            {
                array.rgba[c][y][x].setBits (rand.nexti());
            }

	    union {int i; float f;} u;
	    u.i = rand.nexti();

	    array.f[y][x] = u.f;
	}
}


void
writeRead (pixelArray& array1,
	   const char fileName[],
           bool asTiled,
	   int width,
	   int height,
	   int xOffset,
	   int yOffset,
	   Compression comp,
	   int xs,
	   int ys)
{

    // sampling must be 1 for tiled images
    assert( !asTiled || (xs==1 && ys==1));

    //
    // Write the pixel data in pi1, ph1 and ph2 to an
    // image file using the specified compression type
    // and subsampling rates.  Read the pixel data back
    // from the file and verify that the data did not
    // change.
    //

    if (asTiled)
    {
           cout << "compression " << comp << ", tiled                     :" << flush;
    }
    else
    {
        cout << "compression " << comp <<
                ", x sampling " << xs <<
                ", y sampling " << ys <<
                ":" << flush;
    }

    Header hdr ((Box2i (V2i (0, 0),			// display window
		        V2i (width - 1, height -1))),
		(Box2i (V2i (xOffset, yOffset),		// data window
		        V2i (xOffset + width - 1, yOffset + height - 1))));

    hdr.compression() = comp;

    hdr.channels().insert ("I",			// name
			   Channel (IMF::UINT,	// type
				    xs,		// xSampling
				    ys)		// ySampling
			  );

    static const char* channels[] = {"R","G","B","A","H"};

    for ( int c = 0 ; c < 5 ; ++c )
    {
        hdr.channels().insert (channels[c], // name
                               Channel (IMF::HALF, // type
                                        xs, // xSampling
                                        ys)// ySampling
                              );
    }

    hdr.channels().insert ("F",			// name
			   Channel (IMF::FLOAT,	// type
				    xs,		// xSampling
				    ys)		// ySampling
			  );



    {
	FrameBuffer fb; 

	fb.insert ("I",						// name
		   Slice (IMF::UINT,				// type
			  (char *) &array1.i[-yOffset / ys][-xOffset / xs], // base
			  sizeof (array1.i[0][0]), 			// xStride
			  sizeof (array1.i[0][0]) * (width / xs),	// yStride
			  xs,					// xSampling
			  ys)					// ySampling
		  );
	
	fb.insert ("H",						// name
		   Slice (IMF::HALF,				// type
			  (char *) &array1.h[-yOffset / ys][-xOffset / xs], // base
			  sizeof (array1.h[0][0]), 			// xStride
			  sizeof (array1.h[0][0]) * (width / xs),	// yStride
			  xs,					// xSampling
			  ys)					// ySampling
		  );
	
	fb.insert ("F",						// name
		   Slice (IMF::FLOAT,				// type
			  (char *) &array1.f[-yOffset / ys][-xOffset / xs], // base
			  sizeof (array1.f[0][0]), 			// xStride
			  sizeof (array1.f[0][0]) * (width / xs),	// yStride
			  xs,					// xSampling
			  ys)					// ySampling
		  );
	
        for ( int c = 0 ; c < 4 ; c++)
        {
               fb.insert (channels[c],						// name
    		          Slice (IMF::HALF,				// type
                                 (char *) &array1.rgba[c][-yOffset / ys][-xOffset / xs], // base
			  sizeof (array1.rgba[c][0][0]), 			// xStride
			  sizeof (array1.rgba[c][0][0]) * (width / xs),	// yStride
			  xs,					// xSampling
			  ys)					// ySampling
		  );
        }


	cout << " writing" << flush;

	remove (fileName);

        if (asTiled)
        {
            TileDescription td;
            hdr.setTileDescription(td);
            TiledOutputFile out ( fileName, hdr );
            out.setFrameBuffer( fb );
            out.writeTiles(0,
                           static_cast<int>(ceil( static_cast<float>(width) / static_cast<float>(td.xSize))-1 ) ,
                           0 ,
                           static_cast<int>(ceil( static_cast<float>(height) / static_cast<float>(td.ySize))-1 )
                          );
        }
        else
        {
            OutputFile out (fileName, hdr);
            out.setFrameBuffer (fb);
            out.writePixels (height);
        }
    }

    {
	cout << " reading" << flush;
        InputFile in (fileName);

	const Box2i &dw = hdr.dataWindow();
	int w = dw.max.x - dw.min.x + 1;
	int h = dw.max.y - dw.min.y + 1;
	int dx = dw.min.x;
	int dy = dw.min.y;

        pixelArray array2( h / ys , w / xs );
	FrameBuffer fb;

	{
	    int xs = in.header().channels()["I"].xSampling;
	    int ys = in.header().channels()["I"].ySampling;

	    fb.insert ("I",					// name
		       Slice (IMF::UINT,			// type
			      (char *) &array2.i[-dy / ys][-dx / xs], // base
			      sizeof (array2.i[0][0]), 		// xStride
			      sizeof (array2.i[0][0]) * (w / xs),	// yStride
			      xs,				// xSampling
			      ys)				// ySampling
		      );
	}
	    
	{
	    int xs = in.header().channels()["H"].xSampling;
	    int ys = in.header().channels()["H"].ySampling;

	    fb.insert ("H",					// name
		       Slice (IMF::HALF,			// type
			      (char *) &array2.h[-dy / ys][-dx / xs], // base
			      sizeof (array2.h[0][0]), 		// xStride
			      sizeof (array2.h[0][0]) * (w / xs),	// yStride
			      xs,				// xSampling
			      ys)				// ySampling
		      );
	}
	    

	 for(int c = 0 ; c < 4 ; ++c)
         {
	    int xs = in.header().channels()[ channels[c] ].xSampling;
	    int ys = in.header().channels()[ channels[c] ].ySampling;

	    fb.insert (channels[c],					// name
		       Slice (IMF::HALF,			// type
			      (char *) &array2.rgba[c][-dy / ys][-dx / xs], // base
			      sizeof (array2.rgba[c][0][0]), 		// xStride
			      sizeof (array2.rgba[c][0][0]) * (w / xs),	// yStride
			      xs,				// xSampling
			      ys)				// ySampling
		      );
	}

	{
	    int xs = in.header().channels()["F"].xSampling;
	    int ys = in.header().channels()["F"].ySampling;

	    fb.insert ("F",					// name
		       Slice (IMF::FLOAT,			// type
			      (char *) &array2.f[-dy / ys][-dx / xs], // base
			      sizeof (array2.f[0][0]), 		// xStride
			      sizeof (array2.f[0][0]) * (w / xs),	// yStride
			      xs,				// xSampling
			      ys)				// ySampling
		      );
	}


	in.setFrameBuffer (fb);
	in.readPixels (dw.min.y, dw.max.y);

	cout << " comparing" << flush;

	assert (in.header().displayWindow() == hdr.displayWindow());
	assert (in.header().dataWindow() == hdr.dataWindow());
	assert (in.header().pixelAspectRatio() == hdr.pixelAspectRatio());
	assert (in.header().screenWindowCenter() == hdr.screenWindowCenter());
	assert (in.header().screenWindowWidth() == hdr.screenWindowWidth());
	assert (in.header().lineOrder() == hdr.lineOrder());
	assert (in.header().compression() == hdr.compression());

	ChannelList::ConstIterator hi = hdr.channels().begin();
	ChannelList::ConstIterator ii = in.header().channels().begin();

	while (hi != hdr.channels().end())
	{
	    assert (!strcmp (hi.name(), ii.name()));
	    assert (hi.channel().type == ii.channel().type);
	    assert (hi.channel().xSampling == ii.channel().xSampling);
	    assert (hi.channel().ySampling == ii.channel().ySampling);

	    ++hi;
	    ++ii;
	}

	assert (ii == in.header().channels().end());

	for (int y = 0; y < h / ys; ++y)
	{
	    for (int x = 0; x < w / xs; ++x)
	    {
		assert (array1.i[y][x] == array2.i[y][x]);
		assert (equivalent (array1.f[y][x], array2.f[y][x], comp));

		if (!isLossyCompression(comp))
                {
		    assert (array1.h[y][x].bits() == array2.h[y][x].bits());
                    for (int c = 0 ; c < 4 ; ++c )
                    {
                        assert (array1.rgba[c][y][x].bits() == array2.rgba[c][y][x].bits());
                    }
                }
	    }
	}

	if (comp == B44_COMPRESSION || comp==B44A_COMPRESSION)
	{
            Array2D<half> ph3 (h / ys, w / xs);

            for (int y = 0; y < h / ys; ++y)
               for (int x = 0; x < w / xs; ++x)
                  ph3[y][x] = array1.h[y][x];

            compareB44 (w / xs, h / ys, ph3, array2.h);

            for(int c = 0 ; c < 4 ; ++c )
            {
               for (int y = 0; y < h / ys; ++y)
                   for (int x = 0; x < w / xs; ++x)
                       ph3[y][x] = array1.rgba[c][y][x];

               compareB44 (w / xs, h / ys, ph3, array2.rgba[c]);
            }
	}
	if (comp==DWAA_COMPRESSION || comp==DWAB_COMPRESSION)
        {
            for (int y = 0; y < h / ys; ++y)
               for (int x = 0; x < w / xs; ++x)
                  assert(array1.h[y][x].bits()==array2.h[y][x].bits());

            for(int c = 0 ; c < 4 ; ++c )
            {
               for (int y = 0; y < h / ys; ++y)
                   for (int x = 0; x < w / xs; ++x)
                   {
                       float a1 = array1.rgba[c][y][x];
                       if(!isnan(a1))
                       {
                            float a2 = array2.rgba[c][y][x];
                            float denominator = max(1.f , max(fabs(a2),fabs(a1)) );

                            if (fabs(a1/denominator - a2/denominator) >= 0.1)
                            {
                                std::cerr << "DWA compression detected too big a difference. Got "
                                            << array1.rgba[c][y][x]
                                            << " expected "
                                            << array2.rgba[c][y][x]
                                            << std::endl;
                                assert( fabs(a1/denominator - a2/denominator) < 0.1);
                            }
                       }
                   }
            }
        }
    }

    remove (fileName);
    cout << endl;
}


void
writeRead (const std::string &tempDir,
           pixelArray& array,
	   int w,
	   int h,
	   int dx,
	   int dy)
{
    std::string filename = tempDir + "imf_test_comp.exr";

    for (int xs = 1; xs <= 2; ++xs)
    {
	for (int ys = 1; ys <= 2; ++ys)
	{

	    for (int comp = 0; comp < NUM_COMPRESSION_METHODS; ++comp)
	    {
		writeRead (array,
			   filename.c_str(),
                           false,
			   w  * xs, h  * ys,
			   dx * xs, dy * ys,
			   Compression (comp),
			   xs, ys);

                if (xs==1 && ys==1)
                {
                	writeRead (array,
			   filename.c_str(),
                           true,
			   w  * xs, h  * ys,
			   dx * xs, dy * ys,
			   Compression (comp),
			   xs, ys);
                }
	    }
	}
    }
}

} // namespace


void
testCompression (const std::string &tempDir)
{
    try
    {
	cout << "Testing pixel data types, "
		"subsampling and "
		"compression schemes" << endl;

	const int W = 1371;
	const int H = 159;
	const int DX = 17;
	const int DY = 29;

        pixelArray array(H,W);

	//
	// If the following assertion fails, new pixel types have
	// been added to the Imf library; testing code for the new
	// pixel types should be added to this file.
	//

	assert (NUM_PIXELTYPES == 3);

	fillPixels1 (array, W, H);
	writeRead (tempDir, array, W, H, DX, DY);

	fillPixels2 (array, W, H);
	writeRead (tempDir, array, W, H, DX, DY);

	fillPixels3 (array, W, H);
	writeRead (tempDir, array, W, H, DX, DY);

	fillPixels4 (array, W, H);
	writeRead (tempDir, array, W, H, DX, DY);

	cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
