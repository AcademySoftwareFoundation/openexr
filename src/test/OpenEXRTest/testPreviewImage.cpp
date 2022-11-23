//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "TestUtilFStream.h"
#include <ImfArray.h>
#include <ImfPreviewImage.h>
#include <ImfRgbaFile.h>
#include <assert.h>
#include <fstream>
#include <stdio.h>

#ifndef ILM_IMF_TEST_IMAGEDIR
#    define ILM_IMF_TEST_IMAGEDIR
#endif

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;
using namespace IMATH_NAMESPACE;

namespace
{

void
readWriteFiles (
    const char fileName1[], const char fileName2[], const char fileName3[])
{
    //
    // Test if the preview image attribute works correctly:
    //
    // Read file1, which does not contain a preview image.
    //
    // Generate a preview image, and store both the pixels
    // from file 1 and the preview image in file 2.
    //
    // Read file 2, and verify that both the preview image, and
    // the main image are exactly what we stored in the file.
    //
    // Write file 3, with the same main image as file 2, but
    // initially leave the preview image blank.  Update the
    // preview image half way through writing the main image's
    // pixels.
    //
    // Compare file 2 and file 3 byte by byte, and verify that
    // the files are identical.
    //

    cout << "reading file " << fileName1 << endl;

    RgbaInputFile file1 (fileName1);

    assert (!file1.header ().hasPreviewImage ());

    const Box2i& dw = file1.dataWindow ();

    int w  = dw.max.x - dw.min.x + 1;
    int h  = dw.max.y - dw.min.y + 1;
    int dx = dw.min.x;
    int dy = dw.min.y;

    Array<OPENEXR_IMF_NAMESPACE::Rgba> pixels1 (w * h);
    file1.setFrameBuffer (pixels1 - dx - dy * w, 1, w);
    file1.readPixels (dw.min.y, dw.max.y);

    cout << "generating preview image" << endl;

    const int PREVIEW_WIDTH  = 128;
    const int PREVIEW_HEIGHT = 64;

    PreviewImage preview1 (PREVIEW_WIDTH, PREVIEW_HEIGHT);

    for (int y = 0; y < PREVIEW_HEIGHT; ++y)
        for (int x = 0; x < PREVIEW_WIDTH; ++x)
            preview1.pixel (x, y) = PreviewRgba (x * 2, y * 4, x + y, 128);

    cout << "writing file " << fileName2 << endl;

    {
        Header header (file1.header ());
        header.setPreviewImage (preview1);

        RgbaOutputFile file2 (fileName2, header);
        file2.setFrameBuffer (pixels1 - dx - dy * w, 1, w);

        for (int y = dw.min.y; y <= dw.max.y; ++y)
            file2.writePixels (1);
    }

    cout << "reading file " << fileName2 << endl;

    {
        RgbaInputFile file2 (fileName2);

        assert (file2.header ().hasPreviewImage ());

        const PreviewImage& preview2 = file2.header ().previewImage ();

        for (int i = 0; i < PREVIEW_WIDTH * PREVIEW_HEIGHT; ++i)
        {
            assert (preview1.pixels ()[i].r == preview2.pixels ()[i].r);
            assert (preview1.pixels ()[i].g == preview2.pixels ()[i].g);
            assert (preview1.pixels ()[i].b == preview2.pixels ()[i].b);
            assert (preview1.pixels ()[i].a == preview2.pixels ()[i].a);
        }

        assert (dw == file2.dataWindow ());

        int w  = dw.max.x - dw.min.x + 1;
        int h  = dw.max.y - dw.min.y + 1;
        int dx = dw.min.x;
        int dy = dw.min.y;

        Array<OPENEXR_IMF_NAMESPACE::Rgba> pixels2 (w * h);
        file2.setFrameBuffer (pixels2 - dx - dy * w, 1, w);
        file2.readPixels (dw.min.y, dw.max.y);

        for (int i = 0; i < w * h; ++i)
        {
            assert (pixels1[i].r == pixels2[i].r);
            assert (pixels1[i].g == pixels2[i].g);
            assert (pixels1[i].b == pixels2[i].b);
            assert (pixels1[i].a == pixels2[i].a);
        }
    }

    cout << "writing file " << fileName3 << endl;

    {
        Header header (file1.header ());
        header.setPreviewImage (PreviewImage (PREVIEW_WIDTH, PREVIEW_HEIGHT));

        RgbaOutputFile file3 (fileName3, header);
        file3.setFrameBuffer (pixels1 - dx - dy * w, 1, w);

        for (int y = dw.min.y; y <= dw.max.y; ++y)
        {
            file3.writePixels (1);

            if (y == (dw.min.y + dw.max.y) / 2)
                file3.updatePreviewImage (preview1.pixels ());
        }
    }

    cout << "comparing files " << fileName2 << " and " << fileName3 << endl;

    {
        ifstream file2, file3;
        testutil::OpenStreamWithUTF8Name (
            file2, fileName2, std::ios_base::in | std::ios_base::binary);
        testutil::OpenStreamWithUTF8Name (
            file3, fileName3, std::ios_base::in | std::ios_base::binary);

        while (true)
        {
            int c2 = file2.get ();
            int c3 = file3.get ();

            if (file2.eof ()) break;

            assert (c2 == c3);
            assert (!!file2 && !!file3);
        }
    }

    remove (fileName2);
    remove (fileName3);
}

} // namespace

void
testPreviewImage (const std::string& tempDir)
{
    std::string filename1 = tempDir + "imf_preview1.exr";
    std::string filename2 = tempDir + "imf_preview2.exr";

    try
    {
        cout << "Testing preview image attribute" << endl;

        readWriteFiles (
            ILM_IMF_TEST_IMAGEDIR "comp_piz.exr",
            filename1.c_str (),
            filename2.c_str ());

        cout << "ok\n" << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
