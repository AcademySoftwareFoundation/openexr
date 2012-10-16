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


//-----------------------------------------------------------------------------
//
//	function blurImage() -- performs a hemispherical blur
//
//-----------------------------------------------------------------------------

#include <blurImage.h>

#include <resizeImage.h>
#include "Iex.h"
#include <iostream>
#include <algorithm>
#include <string.h>

#include <omp.h>

using namespace std;
using namespace Imf;
using namespace Imath;


inline int
toInt (float x)
{
    return int (x + 0.5f);
}


inline double
sqr (double x)
{
    return x * x;
}


void
blurImage (EnvmapImage &image1, int convolutionMethod, double power, bool verbose)
{
    //
    // Ideally we would blur the input image directly by convolving
    // it with a 180-degree wide blur kernel.  Unfortunately this
    // is prohibitively expensive when the input image is large.
    // In order to keep running times reasonable, we perform the
    // blur on a small proxy image that will later be re-sampled
    // to the desired output resolution.
    //
    // Here's how it works:
    //
    // * If the input image is in latitude-longitude format,
    //   convert it into a cube-face environment map.
    //
    // * Repeatedly resample the image, each time shrinking
    //   it to no less than half its current size, until the
    //   width of each cube face is MAX_IN_WIDTH pixels.
    // 
    // * Multiply each pixel by a weight that is proportional
    //   to the solid angle subtended by the pixel as seen
    //   from the center of the environment cube.
    //
    // * Create an output image in cube-face format.
    //   The cube faces of the output image are OUT_WIDTH
    //   pixels wide.
    //
    // * For each pixel of the output image:
    //
    //       Set the output pixel's color to black
    //
    //       Determine the direction, d2, from the center of the
    //       output environment cube to the center of the output
    //	     pixel.
    //   
    //       For each pixel of the input image:
    //
    //           Determine the direction, d1, from the center of
    //           the input environment cube to the center of the
    //           input pixel.
    //    
    //           Multiply the input pixel's color by max (0, d1.dot(d2))
    //           and add the result to the output pixel.
    //

    const int MAX_IN_WIDTH = 40;
    const int OUT_WIDTH = 100;

    if (verbose)
        cout << "blurring map image" << endl;
    
    EnvmapImage image2;
    EnvmapImage *iptr1 = &image1;
    EnvmapImage *iptr2 = &image2;

    int w = image1.dataWindow().max.x - image1.dataWindow().min.x + 1;
    int h = w * 6;

    if (iptr1->type() == ENVMAP_LATLONG)
    {
        //
        // Convert the input image from latitude-longitude
        // to cube-face format.
        //

        if (verbose)
            cout << "    converting to cube-face format" << endl;

        w /= 4;
        h = w * 6;

        Box2i dw (V2i (0, 0), V2i (w - 1, h - 1));
        resizeCube (*iptr1, *iptr2, dw, 1, 7);

        swap (iptr1, iptr2);
    }

    while (w > MAX_IN_WIDTH)
    {
        //
        // Shrink the image.
        //

        if (w >= MAX_IN_WIDTH * 2)
            w /= 2;
        else
            w = MAX_IN_WIDTH;

        h = w * 6;

        if (verbose)
        {
            cout << "    resizing cube faces "
                "to " << w << " by " << w << " pixels" << endl;
        }

        Box2i dw (V2i (0, 0), V2i (w - 1, h - 1));
        resizeCube (*iptr1, *iptr2, dw, 1, 7);

        swap (iptr1, iptr2);
    }

    if (verbose)
	    cout << "    computing pixel weights" << endl;

    { 
        //
        // Multiply each pixel by a weight that is proportional
        // to the solid angle subtended by the pixel.
        //

        Box2i dw = iptr1->dataWindow();
        int sof = CubeMap::sizeOfFace (dw);

        Array2D<Rgba> &pixels = iptr1->pixels();

        double weightTotal = 0;

        for (int f = CUBEFACE_POS_X; f <= CUBEFACE_NEG_Z; ++f)
        {
            if (verbose)
                cout << "        face " << f << endl;

            CubeMapFace face = CubeMapFace (f);
            V3f faceDir (0, 0, 0);
            int ix = 0, iy = 0, iz = 0;

            switch (face)
            {
            case CUBEFACE_POS_X:
                faceDir = V3f (1, 0, 0);
                ix = 0;
                iy = 1;
                iz = 2;
                break;

            case CUBEFACE_NEG_X:
                faceDir = V3f (-1, 0, 0);
                ix = 0;
                iy = 1;
                iz = 2;
                break;

            case CUBEFACE_POS_Y:
                faceDir = V3f (0, 1, 0);
                ix = 1;
                iy = 0;
                iz = 2;
                break;

            case CUBEFACE_NEG_Y:
                faceDir = V3f (0, -1, 0);
                ix = 1;
                iy = 0;
                iz = 2;
                break;

            case CUBEFACE_POS_Z:
                faceDir = V3f (0, 0, 1);
                ix = 2;
                iy = 0;
                iz = 1;
                break;

            case CUBEFACE_NEG_Z:
                faceDir = V3f (0, 0, -1);
                ix = 2;
                iy = 0;
                iz = 1;
                break;
            }

            for (int y = 0; y < sof; ++y)
            {
                bool yEdge = (y == 0 || y == sof - 1);

                for (int x = 0; x < sof; ++x)
                {
                    bool xEdge = (x == 0 || x == sof - 1);

                    double weight;
                    V2f posInFace ((float) x, (float) y);
                    V2f pos = CubeMap::pixelPosition (face, dw, posInFace);
                    V3f dir = CubeMap::direction (face, dw, posInFace).normalized();

                    if (true)//convolutionMethod == 0)
                    {
                        //
                        // The solid angle subtended by pixel (x,y), as seen
                        // from the center of the cube, is proportional to the
                        // square of the distance of the pixel from the center
                        // of the cube and proportional to the dot product of
                        // the viewing direction and the normal of the cube
                        // face that contains the pixel.
                        //

                        weight = (dir ^ faceDir) *
                                 (sqr (dir[iy] / dir[ix]) + sqr (dir[iz] / dir[ix]) + 1);
                    }
                    else
                    {
                        float fx = (float) x;
                        float fy = (float) y;
                        posInFace.x = fx - 0.5f;
                        posInFace.y = fy - 0.5f;
                        V3f dir0 = CubeMap::direction (face, dw, posInFace).normalized();
                        posInFace.x = fx - 0.5f;
                        posInFace.y = fy + 0.5f;
                        V3f dir1 = CubeMap::direction (face, dw, posInFace).normalized();
                        posInFace.x = fx + 0.5f;
                        posInFace.y = fy - 0.5f;
                        V3f dir2 = CubeMap::direction (face, dw, posInFace).normalized();
                        posInFace.x = fx + 0.5f;
                        posInFace.y = fy + 0.5f;
                        V3f dir3 = CubeMap::direction (face, dw, posInFace).normalized();

                        V3f e0 = dir1 - dir0;
                        V3f e1 = dir2 - dir0;
                        V3f e0e1 = e0.cross(e1);
                        float texelArea = 0.5f * sqrtf(e0e1.dot(e0e1));

                        e0 = dir2 - dir1;
                        e1 = dir3 - dir1;
                        e0e1 = e0.cross(e1);
                        texelArea += 0.5f * sqrtf(e0e1.dot(e0e1));

                        // Roughly approximate solid angle from the area of 
                        // two triangles covering the sample point. There are more 
                        // accurate methods.
                        double solidAngle = texelArea;
                            
                        weight = solidAngle;// * brdf;
                    }
                    
                    //
                    // Pixels at the edges and corners of the
                    // cube are duplicated; we must adjust the
                    // pixel weights accordingly.
                    //

                    if (xEdge && yEdge)
                        weight /= 3;
                    else if (xEdge || yEdge)
                        weight /= 2;

                    Rgba &pixel = pixels[toInt (pos.y)][toInt (pos.x)];

                    float fweight = (float) weight;
                    pixel.r *= fweight;
                    pixel.g *= fweight;
                    pixel.b *= fweight;
                    pixel.a *= fweight;

                    weightTotal += weight;
                }
            }
        }

        //
        // The weighting operation above has made the overall image darker.
        // Apply a correction to recover the image's original brightness.
        //

        int w = dw.max.x - dw.min.x + 1;
        int h = dw.max.y - dw.min.y + 1;
        size_t numPixels = w * h;
        double weight = numPixels / weightTotal;
        float fweight = (float) weight;

        Rgba *p = &pixels[0][0];
        Rgba *end = p + numPixels;


        while (p < end)
        {
            p->r *= fweight;
            p->g *= fweight;
            p->b *= fweight;
            p->a *= fweight;

            ++p;
        }
    } 

    { 
        if (verbose)
            cout << "    generating blurred image" << endl;

        Box2i dw1 = iptr1->dataWindow();
        int sof1 = CubeMap::sizeOfFace (dw1);

        Box2i dw2 (V2i (0, 0), V2i (OUT_WIDTH - 1, OUT_WIDTH * 6 - 1));
        int sof2 = CubeMap::sizeOfFace (dw2);

        iptr2->resize (ENVMAP_CUBE, dw2);
        iptr2->clear ();

        Array2D<Rgba> &pixels1 = iptr1->pixels();
        Array2D<Rgba> &pixels2 = iptr2->pixels();

        for (int f2 = CUBEFACE_POS_X; f2 <= CUBEFACE_NEG_Z; ++f2)
        {
            if (verbose)
                cout << "        face " << f2 << endl;

            CubeMapFace face2 = CubeMapFace (f2);

            for (int y2 = 0; y2 < sof2; ++y2)
            {
                for (int x2 = 0; x2 < sof2; ++x2)
                {
                    V2f posInFace2 ((float) x2, (float) y2);

                    V3f dir2 = CubeMap::direction (face2, dw2, posInFace2).normalized();

                    V2f pos2 = CubeMap::pixelPosition (face2, dw2, posInFace2);

                    double weightTotal = 0;
                    double rTotal = 0;
                    double gTotal = 0;
                    double bTotal = 0;
                    double aTotal = 0;

                    Rgba &pixel2 =
                        pixels2[toInt (pos2.y)][toInt (pos2.x)];

                    for (int f1 = CUBEFACE_POS_X; f1 <= CUBEFACE_NEG_Z; ++f1)
                    {
                        CubeMapFace face1 = CubeMapFace (f1);

                        for (int y1 = 0; y1 < sof1; ++y1)
                        {
                            for (int x1 = 0; x1 < sof1; ++x1)
                            {
                                V2f posInFace1 ((float) x1, (float) y1);

                                V3f dir1 = CubeMap::direction (face1, dw1, posInFace1).normalized();
                                V2f pos1 = CubeMap::pixelPosition (face1, dw1, posInFace1);

                                double weight = dir1 ^ dir2;

                                if (weight <= 0)
                                    continue;

                                Rgba &pixel1 = pixels1[toInt (pos1.y)][toInt (pos1.x)];

                                weight = pow(weight, power);

                                weightTotal += weight;
                                rTotal += pixel1.r * weight;
                                gTotal += pixel1.g * weight;
                                bTotal += pixel1.b * weight;
                                aTotal += pixel1.a * weight;
                            }
                        }
                    }

                    pixel2.r = float(rTotal / weightTotal);
                    pixel2.g = float(gTotal / weightTotal);
                    pixel2.b = float(bTotal / weightTotal);
                    pixel2.a = float(aTotal / weightTotal);
                }
            }
        }

        swap (iptr1, iptr2);
    } 

    //
    // Depending on how many times we've re-sampled the image,
    // the result is now either in image1 or in image2.
    // If necessary, copy the result into image1.
    //

    if (iptr1 != &image1)
    {
        if (verbose)
            cout << "    copying" << endl;

        Box2i dw = iptr1->dataWindow();
        image1.resize (ENVMAP_CUBE, dw);

        int w = dw.max.x - dw.min.x + 1;
        int h = dw.max.y - dw.min.y + 1;
        size_t size = w * h * sizeof (Rgba);

        memcpy (&image1.pixels()[0][0], &iptr1->pixels()[0][0], size);
    }
}

// faces are ordered 0-5, as XYZ, posneg
V3f cardinalDirPerFace(int face, int& ix, int& iy, int& iz)
{
    V3f faceDir;
    switch (face)
    {
    case CUBEFACE_POS_X:
        faceDir = V3f (1, 0, 0);
        ix = 0;
        iy = 1;
        iz = 2;
        break;

    case CUBEFACE_NEG_X:
        faceDir = V3f (-1, 0, 0);
        ix = 0;
        iy = 1;
        iz = 2;
        break;

    case CUBEFACE_POS_Y:
        faceDir = V3f (0, 1, 0);
        ix = 1;
        iy = 0;
        iz = 2;
        break;

    case CUBEFACE_NEG_Y:
        faceDir = V3f (0, -1, 0);
        ix = 1;
        iy = 0;
        iz = 2;
        break;

    case CUBEFACE_POS_Z:
        faceDir = V3f (0, 0, 1);
        ix = 2;
        iy = 0;
        iz = 1;
        break;

    case CUBEFACE_NEG_Z:
        faceDir = V3f (0, 0, -1);
        ix = 2;
        iy = 0;
        iz = 1;
        break;
    }
    return faceDir;
}

int
weightIndex(int src_sof, int f, int x, int y)
{
    return (f * src_sof * src_sof) + (y * src_sof) + x;
}


void
blurImage2 (EnvmapImage &image1, int outputWidth, int convolutionMethod, double power, bool verbose)
{
    //
    // Ideally we would blur the input image directly by convolving
    // it with a 180-degree wide blur kernel.  Unfortunately this
    // is prohibitively expensive when the input image is large.
    // In order to keep running times reasonable, we perform the
    // blur on a small proxy image that will later be re-sampled
    // to the desired output resolution.
    //
    // Here's how it works:
    //
    // * If the input image is in latitude-longitude format,
    //   convert it into a cube-face environment map.
    //
    // * Repeatedly resample the image, each time shrinking
    //   it to no less than half its current size, until the
    //   width of each cube face is MAX_IN_WIDTH pixels.
    // 
    // * Multiply each pixel by a weight that is proportional
    //   to the solid angle subtended by the pixel as seen
    //   from the center of the environment cube.
    //
    // * Create an output image in cube-face format.
    //   The cube faces of the output image are OUT_WIDTH
    //   pixels wide.
    //
    // * For each pixel of the output image:
    //
    //       Set the output pixel's color to black
    //
    //       Determine the direction, d2, from the center of the
    //       output environment cube to the center of the output
    //	     pixel.
    //   
    //       For each pixel of the input image:
    //
    //           Determine the direction, d1, from the center of
    //           the input environment cube to the center of the
    //           input pixel.
    //    
    //           Multiply the input pixel's color by max (0, d1.dot(d2))
    //           and add the result to the output pixel.
    //

    if (verbose)
        cout << "blurring map image v2" << endl;
    
    EnvmapImage image2;
    EnvmapImage *iptr1 = &image1;
    EnvmapImage *iptr2 = &image2;

    int inputWidth = image1.dataWindow().max.x - image1.dataWindow().min.x + 1;
    int outputHeight = outputWidth * 6;

    if (iptr1->type() == ENVMAP_LATLONG)
    {
        //
        // Convert the input image from latitude-longitude
        // to cube-face format.
        //

        if (verbose)
            cout << "    converting to cube-face format" << endl;

        int w = inputWidth / 4;             // because input width of lat long views horizontally four cube faces
        int inputHeight = w * 6;            // because resizeCube expects w, w*6
        Box2i dw (V2i (0, 0), V2i (w - 1, inputHeight - 1));
        resizeCube (*iptr1, *iptr2, dw, 1, 7);

        swap (iptr1, iptr2);
    }

    // iptr1 is now the image to be processed, and it has been converted to a cube
    // iptr2 is the left over lat-long map, if there was one, otherwise it's empty


    if (verbose)
	    cout << "    computing pixel weights" << endl;

    //
    // Multiply each pixel by a weight that is proportional
    // to the solid angle subtended by the pixel.
    //
    double* solidAngleWeight;
    float* directions;
    int* positionsInFace;

    {
        Box2i dw = iptr1->dataWindow();
        int src_sof = CubeMap::sizeOfFace (dw);
        solidAngleWeight = (double*) malloc(6 * src_sof * src_sof * sizeof(double));
        directions = (float*) malloc(3 * 6 * src_sof * src_sof * sizeof(float));
        positionsInFace = (int*) malloc(2 * 6 * src_sof * src_sof * sizeof(int));

        double weightTotal = 0;

        for (int f = CUBEFACE_POS_X; f <= CUBEFACE_NEG_Z; ++f)
        {
            if (verbose)
                cout << "        generating weights for face " << f << endl;

            int ix = 0, iy = 0, iz = 0;
            CubeMapFace face = CubeMapFace (f);
            V3f faceDir = cardinalDirPerFace(f, ix, iy, iz);

            for (int y = 0; y < src_sof; ++y)
            {
                bool yEdge = (y == 0 || y == src_sof - 1);

                for (int x = 0; x < src_sof; ++x)
                {
                    bool xEdge = (x == 0 || x == src_sof - 1);

                    int index = weightIndex(src_sof, f, x, y);

                    double weight;
                    V2f posInFace ((float) x, (float) y);
                    V2f pos = CubeMap::pixelPosition (face, dw, posInFace);
                    positionsInFace[index*2] = (int) pos.x;
                    positionsInFace[index*2+1] = (int) pos.y;

                    V3f dir = CubeMap::direction (face, dw, posInFace).normalized();
                    directions[index*3] = dir.x;
                    directions[index*3+1] = dir.y;
                    directions[index*3+2] = dir.z;

                    //
                    // The solid angle subtended by pixel (x,y), as seen
                    // from the center of the cube, is proportional to the
                    // square of the distance of the pixel from the center
                    // of the cube and proportional to the dot product of
                    // the viewing direction and the normal of the cube
                    // face that contains the pixel.
                    //

                    weight = (dir ^ faceDir) *
                                (sqr (dir[iy] / dir[ix]) + sqr (dir[iz] / dir[ix]) + 1);

                    //
                    // Pixels at the edges and corners of the
                    // cube are duplicated; we must adjust the
                    // pixel weights accordingly.
                    //

                    if (xEdge && yEdge)
                        weight /= 3;
                    else if (xEdge || yEdge)
                        weight /= 2;

                    solidAngleWeight[index] = weight;
                }
            }
        }
    }


    { 
        if (verbose)
            cout << "    generating blurred image" << endl;

        Box2i src_dw = iptr1->dataWindow();
        int src_sof = CubeMap::sizeOfFace(src_dw);

        Box2i dest_dw (V2i (0, 0), V2i (outputWidth - 1, outputWidth * 6 - 1));
        int dst_sof = CubeMap::sizeOfFace (dest_dw);

        iptr2->resize(ENVMAP_CUBE, dest_dw);
        iptr2->clear();

        Array2D<Rgba> &src_pixels = iptr1->pixels();
        Array2D<Rgba> &dst_pixels = iptr2->pixels();

        for (int f2 = CUBEFACE_POS_X; f2 <= CUBEFACE_NEG_Z; ++f2)
        {
            if (verbose)
                cout << "        face " << f2 << " phong " << power << endl;

            CubeMapFace face2 = CubeMapFace (f2);

            for (int y2 = 0; y2 < dst_sof; ++y2)
            {
               if (verbose)
                    cout << "        y " << y2 << endl;

                #pragma omp parallel for
                for (int x2 = 0; x2 < dst_sof; ++x2)
                {
                    //if (verbose)
                    //    cout << "        x " << y2 << endl;

                    V2f posInFace2 ((float) x2, (float) y2);

                    V3f dir2 = CubeMap::direction (face2, dest_dw, posInFace2).normalized();
                    V2f pos2 = CubeMap::pixelPosition (face2, dest_dw, posInFace2);

                    double weightTotal = 0;
                    double rTotal = 0;
                    double gTotal = 0;
                    double bTotal = 0;
                    double aTotal = 0;

                    Rgba &pixel2 = dst_pixels[toInt (pos2.y)][toInt (pos2.x)];

#if 0
                    V2f srcPos;
                    Imf::CubeMapFace resultingFace;
                    CubeMap::faceAndPixelPosition(dir2, src_dw, resultingFace, srcPos);
                    V2f srcPixelPos = CubeMap::pixelPosition(resultingFace, src_dw, srcPos);
                    
                    Rgba& copyPixel = src_pixels[toInt(srcPixelPos.y)][toInt(srcPixelPos.x)];
                    rTotal = copyPixel.r;
                    gTotal = copyPixel.g;
                    bTotal = copyPixel.b;
                    aTotal = copyPixel.a;
                    weightTotal = 1.0;
#else
                    for (int f1 = CUBEFACE_POS_X; f1 <= CUBEFACE_NEG_Z; ++f1)
                    {
                        CubeMapFace src_face = CubeMapFace(f1);

                        for (int y1 = 0; y1 < src_sof; ++y1)
                        {
                            for (int x1 = 0; x1 < src_sof; ++x1)
                            {
                                int index = weightIndex(src_sof, f1, x1, y1);
                                V3f dir1(directions[index*3], directions[index*3+1], directions[index*3+2]); 
                                //V2f posInFace1((float) x1, (float) y1);
                                //V3f dir1 = CubeMap::direction(src_face, src_dw, posInFace1).normalized();

                                double weight = dir1 ^ dir2;
                                if (weight <= 0)
                                    continue;

                                weight = pow(weight, power);
                                weight *= solidAngleWeight[index];
                                weightTotal += weight;

                                //V2f pos1 = CubeMap::pixelPosition(src_face, src_dw, posInFace1);
                                //Rgba &pixel1 = src_pixels[toInt(pos1.y)][toInt(pos1.x)];
                                Rgba &pixel1 = src_pixels[positionsInFace[index*2+1]][positionsInFace[index*2]];
                                rTotal += pixel1.r * weight;
                                gTotal += pixel1.g * weight;
                                bTotal += pixel1.b * weight;
                                aTotal += pixel1.a * weight;
                            }
                        }
                    }
#endif
                    double k = weightTotal > 0.0 ? 1.0 / weightTotal : 0.0;
                    pixel2.r = float(rTotal * k);
                    pixel2.g = float(gTotal * k);
                    pixel2.b = float(bTotal * k);
                    pixel2.a = float(aTotal * k);
                }
            }
        }

        swap (iptr1, iptr2);
    } 

    //
    // Depending on how many times we've re-sampled the image,
    // the result is now either in image1 or in image2.
    // If necessary, copy the result into image1.
    //

    if (iptr1 != &image1)
    {
        if (verbose)
            cout << "    copying" << endl;

        Box2i dw = iptr1->dataWindow();
        image1.resize (ENVMAP_CUBE, dw);

        int w = dw.max.x - dw.min.x + 1;
        int h = dw.max.y - dw.min.y + 1;
        size_t size = w * h * sizeof (Rgba);

        memcpy (&image1.pixels()[0][0], &iptr1->pixels()[0][0], size);
    }

    free(solidAngleWeight);
    free(directions);
}
