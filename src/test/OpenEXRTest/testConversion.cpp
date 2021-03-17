//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <ImfOutputFile.h>
#include <ImfInputFile.h>
#include <ImfTiledOutputFile.h>
#include <ImfTiledInputFile.h>
#include <ImfArray.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfConvert.h>
#include <half.h>
#include "compareFloat.h"

#include <stdio.h>
#include <assert.h>


namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace IMF;
using namespace std;
using namespace IMATH_NAMESPACE;


namespace {

//
// Functions to test conversion of individual numbers
//

void
testFloatToUint (unsigned int floatVal, unsigned int uintVal)
{
    union {unsigned int ui; float f;} u;
    u.ui = floatVal;
    cout << "floatToUint (" << u.f << ") == " << floatToUint(u.f) << endl;
    assert (floatToUint (u.f) == uintVal);
}


void
testHalfToUint (unsigned int halfVal, unsigned int uintVal)
{
    half h;
    h.setBits (halfVal);
    cout << "halfToUint (" << h << ") == " << halfToUint(h) << endl;
    assert (halfToUint (h) == uintVal);
}


void
testFloatToHalf (unsigned int floatVal, unsigned int halfVal)
{
    union {unsigned int ui; float f;} u;
    u.ui = floatVal;
    cout << "floatToHalf (" << u.f << ") == " << floatToHalf(u.f) << endl;
    assert (floatToHalf(u.f).bits() == halfVal);
}


void
testUintToHalf (unsigned int uintVal, unsigned int halfVal)
{
    cout << "uintToHalf (" << uintVal << ") == " << uintToHalf(uintVal) << endl;
    assert (uintToHalf(uintVal).bits() == halfVal);
}

void
testNumbers ()
{
    testFloatToUint (0x00000000, 0);		//  0.0
    testFloatToUint (0x3f000000, 0);		// +0.5
    testFloatToUint (0xbf000000, 0);		// -0.5
    testFloatToUint (0x42f82000, 124);		// +124.0625
    testFloatToUint (0xc2f82000, 0);		// -124.0625
    testFloatToUint (0x58635fa9, 0xffffffff);	// +1.0e15
    testFloatToUint (0xd8635fa9, 0);		// -1.0e15
    testFloatToUint (0x7f800000, 0xffffffff);	// +infinity
    testFloatToUint (0xff800000, 0);		// -infinity
    testFloatToUint (0x7fffffff, 0);		//  NaN
    testFloatToUint (0xffffffff, 0);		//  NaN

    testHalfToUint  (0x0000, 0);		//  0.0
    testHalfToUint  (0x3800, 0);		// +0.5
    testHalfToUint  (0xb800, 0);		// -0.5
    testHalfToUint  (0x57c1, 124);		// +124.0625
    testHalfToUint  (0xd7c1, 0);		// -124.0625
    testHalfToUint  (0x7c00, 0xffffffff);	// +infinity
    testHalfToUint  (0xfc00, 0);		// -infinity
    testHalfToUint  (0x7fff, 0);		//  NaN
    testHalfToUint  (0xffff, 0);		//  NaN

    testFloatToHalf (0x00000000, 0x0000);	//  0.0
    testFloatToHalf (0x3f000000, 0x3800);	// +0.5
    testFloatToHalf (0xbf000000, 0xb800);	// -0.5
    testFloatToHalf (0x42f82000, 0x57c1);	// +124.0625
    testFloatToHalf (0xc2f82000, 0xd7c1);	// -124.0625
    testFloatToHalf (0x58635fa9, 0x7c00);	// +1.0e15
    testFloatToHalf (0xd8635fa9, 0xfc00);	// -1.0e15
    testFloatToHalf (0x7f800000, 0x7c00);	// +infinity
    testFloatToHalf (0xff800000, 0xfc00);	// -infinity
    testFloatToHalf (0x7fffffff, 0x7fff);	//  NaN
    testFloatToHalf (0xffffffff, 0xffff);	//  NaN

    testUintToHalf  (0,          0x0000);	
    testUintToHalf  (1,          0x3c00);
    testUintToHalf  (124,        0x57c0);
    testUintToHalf  (1000000,    0x7c00);
    testUintToHalf  (0xffffffff, 0x7c00);
}


template <class T>
bool
isEquivalent (T t1, T t2, Compression compression)
{
    return t1 == t2;
}


template<>
bool
isEquivalent (float t1, float t2, Compression compression)
{
    return equivalent (t1, t2, compression);
}


template <class OutType, PixelType OutTypeTag,
          class InType,  PixelType InTypeTag>
void
testScanLineImageChannel (const char fileName[],
		          int width, int height,
			  Compression compression)
{
    cout << "scan lines, "
	    "compression " << compression << ", " <<
	    "output type " << OutTypeTag << ", " <<
	    "input type " << InTypeTag << ":\n    " << flush;

    Array2D<OutType> outPixels (height, width);

    for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x)
	    outPixels[y][x] = x * 10 + y;

    {
	Header hdr (width, height);

	hdr.compression() = compression;

	hdr.channels().insert ("X", 				// name
			       Channel (OutTypeTag));		// type

	FrameBuffer fb;

	fb.insert ("X",						// name
		   Slice (OutTypeTag,				// type
		          (char *) &outPixels[0][0],		// base
			  sizeof (outPixels[0][0]),		// xStride
			  sizeof (outPixels[0][0]) * width));	// yStride

	cout << "writing " << flush;

	OutputFile out (fileName, hdr);
	out.setFrameBuffer (fb);
	out.writePixels (height);
    }

    Array2D<InType> inPixels (height, width);

    {
	cout << "reading " << flush;

	FrameBuffer fb;

	fb.insert ("X",						// name
		   Slice (InTypeTag,				// type
		          (char *) &inPixels[0][0],		// base
			  sizeof (inPixels[0][0]),		// xStride
			  sizeof (inPixels[0][0]) * width));	// yStride

	InputFile in (fileName);
	in.setFrameBuffer (fb);
	in.readPixels (0, height - 1);
    }

    cout << "comparing" << flush;

    for (int y = 0; y < height; ++y)
    {
	for (int x = 0; x < width; ++x)
	{
	    assert (isEquivalent (inPixels[y][x],
				  InType (outPixels[y][x]),
				  compression));
	}
    }

    cout << endl;

    remove (fileName);
}


template <class OutType, PixelType OutTypeTag,
          class InType,  PixelType InTypeTag>
void
testTiledImageChannel (const char fileName[],
		       int width, int height,
		       Compression compression)
{
    cout << "tiles, "
	    "compression " << compression << ", " <<
	    "output type " << OutTypeTag << ", " <<
	    "input type " << InTypeTag << ":\n    " << flush;

    Array2D<OutType> outPixels (height, width);

    for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x)
	    outPixels[y][x] = x * 10 + y;

    {
	Header hdr (width, height);

	hdr.setTileDescription (TileDescription (67, 67, ONE_LEVEL));

	hdr.compression() = compression;

	hdr.channels().insert ("X", 				// name
			       Channel (OutTypeTag));		// type

	FrameBuffer fb;

	fb.insert ("X",						// name
		   Slice (OutTypeTag,				// type
		          (char *) &outPixels[0][0],		// base
			  sizeof (outPixels[0][0]),		// xStride
			  sizeof (outPixels[0][0]) * width));	// yStride

	cout << "writing " << flush;

	TiledOutputFile out (fileName, hdr);
	out.setFrameBuffer (fb);
        out.writeTiles (0, out.numXTiles() - 1, 0, out.numYTiles() - 1);
    }

    Array2D<InType> inPixels (height, width);

    {
	cout << "reading " << flush;

	FrameBuffer fb;

	fb.insert ("X",						// name
		   Slice (InTypeTag,				// type
		          (char *) &inPixels[0][0],		// base
			  sizeof (inPixels[0][0]),		// xStride
			  sizeof (inPixels[0][0]) * width));	// yStride

	TiledInputFile in (fileName);
	in.setFrameBuffer (fb);
        in.readTiles (0, in.numXTiles() - 1, 0, in.numYTiles() - 1);
    }

    cout << "comparing" << flush;

    for (int y = 0; y < height; ++y)
    {
	for (int x = 0; x < width; ++x)
	{
	    assert (isEquivalent (inPixels[y][x],
				  InType (outPixels[y][x]),
				  compression));
	}
    }

    cout << endl;

    remove (fileName);
}


template <class OutType, PixelType OutTypeTag,
          class InType,  PixelType InTypeTag>
void
testImageChannel (const std::string &fileName,
		  int width, int height,
		  Compression compression)
{
    testScanLineImageChannel <OutType, OutTypeTag, InType, InTypeTag>
	(fileName.c_str(), width, height, compression);

    testTiledImageChannel <OutType, OutTypeTag, InType, InTypeTag>
	(fileName.c_str(), width, height, compression);
}



} // namespace


void
testConversion (const std::string &tempDir)
{
    try
    {
	cout << "Testing conversion between pixel data types" << endl;

	cout << "individual numbers" << endl;

	testNumbers();

	cout << "conversion of image channels while reading a file " << endl;

	for (int comp = 0; comp < NUM_COMPRESSION_METHODS; ++comp)
	{
	    if (comp == B44_COMPRESSION ||
                comp == B44A_COMPRESSION)
            {
		continue;
            }

	    testImageChannel <unsigned int, IMF::UINT, unsigned int, IMF::UINT>
			     (tempDir + "imf_test_conv.exr",
			      317, 539,
			      Compression (comp));

	    testImageChannel <unsigned int, IMF::UINT, half, IMF::HALF>
			     (tempDir + "imf_test_conv.exr",
			      317, 539,
			      Compression (comp));

	    testImageChannel <unsigned int, IMF::UINT, float, IMF::FLOAT>
			     (tempDir + "imf_test_conv.exr",
			      317, 539,
			      Compression (comp));

	    testImageChannel <half, IMF::HALF, unsigned int, IMF::UINT>
			     (tempDir + "imf_test_conv.exr",
			      317, 539,
			      Compression (comp));

	    testImageChannel <half, IMF::HALF, half, IMF::HALF>
			     (tempDir + "imf_test_conv.exr",
			      317, 539,
			      Compression (comp));

	    testImageChannel <half, IMF::HALF, float, IMF::FLOAT>
			     (tempDir + "imf_test_conv.exr",
			      317, 539,
			      Compression (comp));

	    testImageChannel <float, IMF::FLOAT, unsigned int, IMF::UINT>
			     (tempDir + "imf_test_conv.exr",
			      317, 539,
			      Compression (comp));

	    testImageChannel <float, IMF::FLOAT, half, IMF::HALF>
			     (tempDir + "imf_test_conv.exr",
			      317, 539,
			      Compression (comp));

	    testImageChannel <float, IMF::FLOAT, float, IMF::FLOAT>
			     (tempDir + "imf_test_conv.exr",
			      317, 539,
			      Compression (comp));

	}
	cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
