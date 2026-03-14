//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "compareB44.h"
#include "compareDwa.h"

#include <IlmThread.h>
#include <ImfArray.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfOutputFile.h>
#include <ImfCRgbaFile.h>
#include <ImfThreading.h>

#include <string>

#include <assert.h>
#include <stdio.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;
using namespace IMATH_NAMESPACE;

namespace
{
void
fillPixels (Array2D<ImfRgba>& pixels, int w, int h)
{
    for (int y = 0; y < h; ++y) 
    {
        for (int x = 0; x < w; ++x) 
        {
            half r = 0.5 + 0.5 * sin (0.1 * x + 0.1 * y);
            half g = 0.5 + 0.5 * sin (0.1 * x + 0.2 * y);
            half b = 0.5 + 0.5 * sin (0.1 * x + 0.3 * y);
            ImfFloatToHalf (r, &pixels[y][x].r);
            ImfFloatToHalf (g, &pixels[y][x].g);
            ImfFloatToHalf (b, &pixels[y][x].b);
            ImfFloatToHalf ((r + g + b)/3.0, &pixels[y][x].a);
        }
    }
}

void 
convertImfRgbaToRgbaPixels (const Array2D<ImfRgba>& ImfRgbaPixels,
                            Array2D<Rgba>& RgbaPixels, 
                            int w, 
                            int h)
{
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            RgbaPixels[y][x].r = ImfHalfToFloat (ImfRgbaPixels[y][x].r);
            RgbaPixels[y][x].g = ImfHalfToFloat (ImfRgbaPixels[y][x].g);
            RgbaPixels[y][x].b = ImfHalfToFloat (ImfRgbaPixels[y][x].b);
            RgbaPixels[y][x].a = ImfHalfToFloat (ImfRgbaPixels[y][x].a);
        }
    }
}

void
writeReadCRGBA (
    const char           fileName[],
    int                  width,
    int                  height,
    const Array2D<ImfRgba>& p1,
    RgbaChannels         channels,
    LineOrder            lorder,
    Compression          comp)
{
    // 
    // Save the selected channels of RGBA image p1; save the
    // scan lines in the specified order.  Read the image back
    // from the file, and compare the data with the original.
    //

    cout << "channels " << ((channels & IMF_WRITE_R) ? "R" : "")
         << ((channels & IMF_WRITE_G) ? "G" : "")
         << ((channels & IMF_WRITE_B) ? "B" : "")
         << ((channels & IMF_WRITE_A) ? "A" : "") << ", line order " << lorder
         << ", compression " << comp << endl;

    ImfHeader* headerPtr = ImfNewHeader ();
    ImfHeaderSetLineOrder (headerPtr, lorder);
    ImfHeaderSetCompression (headerPtr, comp);
    ImfHeaderSetDataWindow (headerPtr, 0, 0, width - 1, height - 1);
    ImfHeaderSetDisplayWindow (headerPtr, 0, 0, width - 1, height - 1);

    // Get screenWindowCenter for comparison
    float xScreenWindowCenterOut, yScreenWindowCenterOut;
    ImfHeaderScreenWindowCenter (headerPtr, &xScreenWindowCenterOut, &yScreenWindowCenterOut);

    cout << "writing ";
    cout.flush ();

    {
        remove (fileName);
        ImfOutputFile* out = ImfOpenOutputFile (fileName, headerPtr, channels);
        ImfOutputSetFrameBuffer (out, &p1[0][0], 1, width);
        ImfOutputWritePixels (out, height);
        int outputFileClosed = ImfCloseOutputFile (out);
        assert (outputFileClosed == 1);
    }

    cout << "reading ";
    cout.flush ();

    {
        ImfInputFile* in = ImfOpenInputFile (fileName);
        assert (in != NULL);
        const ImfHeader* inputFileHeaderPtr = ImfInputHeader (in);
        int xMin, yMin, xMax, yMax;
        ImfHeaderDataWindow (inputFileHeaderPtr, &xMin, &yMin, &xMax, &yMax);

        int w  = xMax - xMin + 1;
        int h  = yMax - yMin + 1;
        int dx = xMin;
        int dy = yMin;

        Array2D<ImfRgba> p2 (h, w);
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                p2[y][x] = {0, 0, 0, 1};
            }
        }
        int successSetFrameBuffer = ImfInputSetFrameBuffer (in, &p2[-dy][-dx], 1, w);
        assert (successSetFrameBuffer == 1);
        int successReadPixels = ImfInputReadPixels (in, yMin, yMax);
        assert (successReadPixels == 1);

        // Get inputFile dataWindow
        int xMinDataWindow, yMinDataWindow, xMaxDataWindow, yMaxDataWindow;
        ImfHeaderDataWindow (inputFileHeaderPtr, &xMinDataWindow, &yMinDataWindow, &xMaxDataWindow, &yMaxDataWindow);

        // Get inputFile displayWindow
        int xMinDisplayWindow, yMinDisplayWindow, xMaxDisplayWindow, yMaxDisplayWindow;
        ImfHeaderDisplayWindow (inputFileHeaderPtr, &xMinDisplayWindow, &yMinDisplayWindow, &xMaxDisplayWindow, &yMaxDisplayWindow);

        // Get inputFile screenWindowCenter
        float xScreenWindowCenter, yScreenWindowCenter;
        ImfHeaderScreenWindowCenter (inputFileHeaderPtr, &xScreenWindowCenter, &yScreenWindowCenter);

        // Compare displayWindow
        assert ((xMinDisplayWindow == 0) && (yMinDisplayWindow == 0) && (xMaxDisplayWindow == width - 1) && (yMaxDisplayWindow == height - 1));
        // Compare dataWindow
        assert ((xMinDataWindow == 0) && (yMinDataWindow == 0) && (xMaxDataWindow == width - 1) && (yMaxDataWindow == height - 1));
        // Compare pixelAspectRatio
        assert (ImfHeaderPixelAspectRatio(inputFileHeaderPtr) == ImfHeaderPixelAspectRatio (headerPtr));
        // Compare screenWindowCenter
        assert ((xScreenWindowCenter == xScreenWindowCenterOut) && (yScreenWindowCenter == yScreenWindowCenterOut));
        // Compare screenWindowWidth
        assert (ImfHeaderScreenWindowWidth (inputFileHeaderPtr) == ImfHeaderScreenWindowWidth (headerPtr));
        // Compare lineOrder
        assert (ImfHeaderLineOrder (inputFileHeaderPtr) == ImfHeaderLineOrder (headerPtr));
        // Compare compression
        assert (ImfHeaderCompression (inputFileHeaderPtr) == ImfHeaderCompression (headerPtr));
        // Compare channels
        assert(ImfInputChannels (in) == channels);

        if (ImfHeaderCompression(inputFileHeaderPtr) == IMF_B44_COMPRESSION ||
            ImfHeaderCompression(inputFileHeaderPtr) == IMF_B44A_COMPRESSION)
        {
            Array2D<Rgba> p1_temp (h,w);
            Array2D<Rgba> p2_temp (h,w);
            convertImfRgbaToRgbaPixels (p1, p1_temp, w, h);
            convertImfRgbaToRgbaPixels (p2, p2_temp, w, h);
            compareB44 (width, height, p1_temp, p2_temp, channels);
        }
        else if (
            ImfHeaderCompression (inputFileHeaderPtr) == IMF_DWAA_COMPRESSION ||
            ImfHeaderCompression (inputFileHeaderPtr) == IMF_DWAB_COMPRESSION)
        {
            Array2D<Rgba> p1_temp (h, w);
            Array2D<Rgba> p2_temp (h, w);
            convertImfRgbaToRgbaPixels (p1, p1_temp, w, h);
            convertImfRgbaToRgbaPixels (p2, p2_temp, w, h);
            compareDwa (width, height, p1_temp, p2_temp, channels);
        }
        else
        {
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {  
                    if (channels & IMF_WRITE_R)
                        assert (p2[y][x].r == p1[y][x].r);
                    else
                        assert (ImfHalfToFloat(p2[y][x].r) == 0.0);

                    if (channels & IMF_WRITE_G)
                        assert (p2[y][x].g == p1[y][x].g);
                    else
                        assert (ImfHalfToFloat(p2[y][x].g) == 0.0);

                    if (channels & IMF_WRITE_B)
                        assert (p2[y][x].b == p1[y][x].b);
                    else
                        assert (ImfHalfToFloat(p2[y][x].b) == 0.0);

                    if (channels & IMF_WRITE_A) 
                        assert (p2[y][x].a == p1[y][x].a);
                    else
                        assert (ImfHalfToFloat(p2[y][x].a) == 1.0);       
                }
            }
        }

        int inputFileClosed = ImfCloseInputFile (in);
        assert (inputFileClosed == 1);
    }
    ImfDeleteHeader(headerPtr);
    remove (fileName);
}

} // namespace

void
testCRgba (const std::string& tempDir)
{
    try
    {
        const int W = 237;
        const int H = 119;

        Array2D<ImfRgba> p1 (H, W);
        fillPixels (p1, W, H);
       
        int maxThreads = ILMTHREAD_NAMESPACE::supportsThreads () ? 3 : 0;

        for (int n = 0; n <= maxThreads; ++n) // Vary num threads 0 to max(3)
        {
            if (ILMTHREAD_NAMESPACE::supportsThreads ())
            {
                setGlobalThreadCount (n);
                cout << "\nnumber of threads: " << globalThreadCount () << endl;
            }

            for (int lorder = 0; lorder < RANDOM_Y; ++lorder) // Vary line order
            {
                for (int comp = 0; comp < NUM_COMPRESSION_METHODS; ++comp) // Vary compression
                {
                    writeReadCRGBA (
                        (tempDir + "imf_test_crgba.exr").c_str (),
                        W,
                        H,
                        p1,
                        WRITE_RGBA,
                        LineOrder (lorder),
                        Compression (comp));

                    writeReadCRGBA (
                        (tempDir + "imf_test_crgba.exr").c_str (),
                        W,
                        H,
                        p1,
                        WRITE_RGB,
                        LineOrder (lorder),
                        Compression (comp));

                    writeReadCRGBA (
                        (tempDir + "imf_test_crgba.exr").c_str (),
                        W,
                        H,
                        p1,
                        WRITE_A,
                        LineOrder (lorder),
                        Compression (comp));

                    writeReadCRGBA (
                        (tempDir + "imf_test_crgba.exr").c_str (),
                        W,
                        H,
                        p1,
                        RgbaChannels (WRITE_R | WRITE_B),
                        LineOrder (lorder),
                        Compression (comp));
                }
            }
        }
        cout << "ok\n" << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
