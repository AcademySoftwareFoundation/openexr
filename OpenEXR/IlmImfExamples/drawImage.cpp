///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
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
//	Functions that algorithmically generate images, so that
//	the code examples for image file reading and writing have
//	data they can work with.
//	Note that it is not necessary to study the code below in
//	order to understand how the file I/O code examples work.
//
//-----------------------------------------------------------------------------


#include <drawImage.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <algorithm>

using namespace Imf;
using namespace std;

#if defined PLATFORM_WIN32 && _MSC_VER < 1300
namespace
{
template<class T>
inline T min (const T &a, const T &b) { return (a <= b) ? a : b; }

template<class T>
inline T max (const T &a, const T &b) { return (a >= b) ? a : b; }
}
#endif


float
pw (float x, int y)
{
    float p = 1;

    while (y)
    {
	if (y & 1)
	    p *= x;

	x *= x;
	y >>= 1;
    }

    return p;
}


void
sp (Array2D<Rgba> &px, int w, int h,
    float xc, float yc, float rc,
    float rd, float gn, float bl, float lm)
{
    int x1 = int (max ((float) floor (xc - rc),  0.0f));
    int x2 = int (min ((float) ceil  (xc + rc), w - 1.0f));
    int y1 = int (max ((float) floor (yc - rc),  0.0f));
    int y2 = int (min ((float) ceil  (yc + rc), h - 1.0f));

    for (int y = y1; y <= y2; ++y)
    {
	for (int x = x1; x <= x2; ++x)
	{
	    float xl = (x - xc) / rc;
	    float yl = (y - yc) / rc;
	    float r  = sqrt (xl * xl + yl * yl);

	    if (r >= 1)
		continue;

	    float a = 1;

	    if (r * rc > rc - 1)
		a = rc - r * rc;

	    float zl = sqrt (1 - r * r);
	    float dl = xl * 0.42426 - yl * 0.56568 + zl * 0.70710;

	    if (dl < 0)
		dl *= -0.1;

	    float hl = pw (dl, 50) * 4;
	    float dr = (dl + hl) * rd;
	    float dg = (dl + hl) * gn;
	    float db = (dl + hl) * bl;

	    Rgba &p = px[y][x];
	    p.r = p.r * (1 - a) + dr * lm * a;
	    p.g = p.g * (1 - a) + dg * lm * a;
	    p.b = p.b * (1 - a) + db * lm * a;
	    p.a = 1 - (1 - p.a) * (1 - a);
	}
    }
}


void
zsp (Array2D<half> &gpx, Array2D<float> &zpx, int w, int h,
     float xc, float yc, float zc, float rc, float gn)
{
    int x1 = int (max ((float) floor (xc - rc),  0.0f));
    int x2 = int (min ((float) ceil  (xc + rc), w - 1.0f));
    int y1 = int (max ((float) floor (yc - rc),  0.0f));
    int y2 = int (min ((float) ceil  (yc + rc), h - 1.0f));

    for (int x = x1; x <= x2; ++x)
    {
	for (int y = y1; y <= y2; ++y)
	{
	    float xl = (x - xc) / rc;
	    float yl = (y - yc) / rc;
	    float r  = sqrt (xl * xl + yl * yl);

	    if (r >= 1)
		continue;

	    float a = 1;

	    if (r * rc > rc - 1)
		a = rc - r * rc;

	    float zl = sqrt (1 - r * r);
	    float zp = zc - rc * zl;

	    if (zp >= zpx[y][x])
		continue;

	    float dl = xl * 0.42426 - yl * 0.56568 + zl * 0.70710;

	    if (dl < 0)
		dl *= -0.1;

	    float hl = pw (dl, 50) * 4;
	    float dg = (dl + hl) * gn;

	    gpx[y][x] = dg;
	    zpx[y][x] = zp;
	}
    }
}


void
drawImage1 (Array2D<Rgba> &px, int w, int h)
{
    for (int y = 0; y < h; ++y)
    {
	for (int x = 0; x < w; ++x)
	{
	    Rgba &p = px[y][x];
	    p.r = 0;
	    p.g = 0;
	    p.b = 0;
	    p.a = 0;
	}
    }

    int n = 5600;

    for (int i = 0; i < n; ++i)
    {
	float t = (i * 2.0 * M_PI) / n;
	float xp = sin (t * 2.0) + 0.2 * sin (t * 15.0);
	float yp = cos (t * 3.0) + 0.2 * cos (t * 15.0);
	float r = float (i + 1) / float (n);
	float xq = xp + 0.3 * r * sin (t * 80.0);
	float yq = yp + 0.3 * r * cos (t * 80.0);
	float xr = xp + 0.3 * r * sin (t * 80.0 + M_PI / 2);
	float yr = yp + 0.3 * r * cos (t * 80.0 + M_PI / 2);

	if (i % 10 == 0)
	    sp (px, w, h,
		xp * w / 3 + w / 2, yp * h / 3 + h / 2,
		w * 0.05 * r,
		2.0, 0.8, 0.1,
		0.5 * r * r);

	sp (px, w, h,
	    xq * w / 3 + w / 2, yq * h / 3 + h / 2,
	    w * 0.01 * r,
	    0.7, 0.2, 2.0,
	    0.5 * r * r);

	sp (px, w, h,
	    xr * w / 3 + w / 2, yr * h / 3 + h / 2,
	    w * 0.01 * r,
	    0.2, 1.5, 0.1,
	    0.5 * r * r);
    }
}


void
drawImage2 (Array2D<half> &gpx, Array2D<float> &zpx, int w, int h)
{
    for (int y = 0; y < h; ++y)
    {
	for (int x = 0; x < w; ++x)
	{
	    gpx[y][x] = 0;
	    zpx[y][x] = FLT_MAX;
	}
    }

    int n = 2000;

    for (int i = 0; i < n; ++i)
    {
	float t = (i * 2.0 * M_PI) / n;
	float xp = sin (t * 4.0) + 0.2 * sin (t * 15.0);
	float yp = cos (t * 3.0) + 0.2 * cos (t * 15.0);
	float zp = sin (t * 5.0);
	float rd = 0.7 + 0.3 * sin (t * 15.0);
	float gn = 0.5 - 0.5 * zp + 0.2;

	zsp (gpx, zpx, w, h,
	     xp * w / 3 + w / 2,
	     yp * h / 3 + h / 2,
	     zp * w + 3 * w,
	     w * rd * 0.05,
	     2.5 * gn * gn);
    }
}
