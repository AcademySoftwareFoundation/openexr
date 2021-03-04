//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <IlmThread.h>
#include <stdio.h>
#include <assert.h>

#ifndef ILM_IMF_TEST_IMAGEDIR
    #define ILM_IMF_TEST_IMAGEDIR
#endif


using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;
using namespace IMATH_NAMESPACE;


namespace {

void
readImage (const char fileName[], unsigned int correctChecksum)
{
    cout << "file " << fileName << " " << flush;

    OPENEXR_IMF_NAMESPACE::RgbaInputFile in (fileName);

    cout << "version " << in.version() << " " << flush;

    const Box2i &dw = in.dataWindow();

    int w = dw.max.x - dw.min.x + 1;
    int h = dw.max.y - dw.min.y + 1;
    int dx = dw.min.x;
    int dy = dw.min.y;

    Array<OPENEXR_IMF_NAMESPACE::Rgba> pixels (w * h);
    in.setFrameBuffer (pixels - dx - dy * w, 1, w);
    in.readPixels (in.dataWindow().min.y, in.dataWindow().max.y);

    unsigned int checksum = 0;

    for (int i = 0; i < w * h; ++i)
    {
	checksum ^= pixels[i].r.bits();
	checksum ^= pixels[i].g.bits();
	checksum ^= pixels[i].b.bits();
	checksum ^= pixels[i].a.bits();
    }

    cout << "checksum = " << checksum << endl;

    assert (checksum == correctChecksum);
}


bool
approximatelyEqual (float x, float y)
{
    float z = (x + 0.01f) / (y + 0.01f);
    return z >= 0.99f && z <= 1.01f;
}


void
compareImages (const char fileName1[], const char fileName2[])
{
    cout << "comparing files " << fileName1 << " and " << fileName2 << endl;

    OPENEXR_IMF_NAMESPACE::RgbaInputFile in1 (fileName1);
    OPENEXR_IMF_NAMESPACE::RgbaInputFile in2 (fileName1);

    assert (in1.dataWindow() == in2.dataWindow());

    const Box2i &dw = in1.dataWindow();

    int w = dw.max.x - dw.min.x + 1;
    int h = dw.max.y - dw.min.y + 1;
    int dx = dw.min.x;
    int dy = dw.min.y;

    Array<OPENEXR_IMF_NAMESPACE::Rgba> pixels1 (w * h);
    Array<OPENEXR_IMF_NAMESPACE::Rgba> pixels2 (w * h);

    in1.setFrameBuffer (pixels1 - dx - dy * w, 1, w);
    in2.setFrameBuffer (pixels2 - dx - dy * w, 1, w);

    in1.readPixels (in1.dataWindow().min.y, in1.dataWindow().max.y);
    in2.readPixels (in2.dataWindow().min.y, in2.dataWindow().max.y);

    for (int i = 0; i < w * h; ++i)
    {
	assert (approximatelyEqual (pixels1[i].r, pixels2[i].r));
	assert (approximatelyEqual (pixels1[i].g, pixels2[i].g));
	assert (approximatelyEqual (pixels1[i].b, pixels2[i].b));
	assert (approximatelyEqual (pixels1[i].a, pixels2[i].a));
    }
}

} // namespace


void
testSampleImages (const std::string&)
{
    try
    {
	cout << "Testing sample image files" << endl;

	readImage (ILM_IMF_TEST_IMAGEDIR "comp_none.exr", 24988);
	readImage (ILM_IMF_TEST_IMAGEDIR "comp_rle.exr",  24988);
	readImage (ILM_IMF_TEST_IMAGEDIR "comp_zips.exr", 24988);
	readImage (ILM_IMF_TEST_IMAGEDIR "comp_zip.exr",  24988);
	readImage (ILM_IMF_TEST_IMAGEDIR "comp_piz.exr",  24988);
        
        for (int i = 0; i < 5; i++)
        {
            if (ILMTHREAD_NAMESPACE::supportsThreads ())
            {
                setGlobalThreadCount (i);

                readImage (ILM_IMF_TEST_IMAGEDIR "lineOrder_increasing.exr",
			   46515);

                readImage (ILM_IMF_TEST_IMAGEDIR "lineOrder_decreasing.exr",
			   46515);
            }
        }

	compareImages (ILM_IMF_TEST_IMAGEDIR "comp_b44.exr",
		       ILM_IMF_TEST_IMAGEDIR "comp_b44_piz.exr");

	compareImages (ILM_IMF_TEST_IMAGEDIR "comp_dwaa_v1.exr",
		       ILM_IMF_TEST_IMAGEDIR "comp_dwaa_piz.exr");
	compareImages (ILM_IMF_TEST_IMAGEDIR "comp_dwaa_v2.exr",
		       ILM_IMF_TEST_IMAGEDIR "comp_dwaa_piz.exr");

	compareImages (ILM_IMF_TEST_IMAGEDIR "comp_dwab_v1.exr",
		       ILM_IMF_TEST_IMAGEDIR "comp_dwab_piz.exr");
	compareImages (ILM_IMF_TEST_IMAGEDIR "comp_dwab_v2.exr",
		       ILM_IMF_TEST_IMAGEDIR "comp_dwab_piz.exr");


	cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
