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



//----------------------------------------------------------------------------
//
//	class ImageView
//
//----------------------------------------------------------------------------

#include <ImageView.h>
#include <ImathMath.h>
#include <ImathFun.h>
#include <halfFunction.h>

#if defined PLATFORM_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>
#elif defined PLATFORM_DARWIN_PPC
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#if defined PLATFORM_WIN32
float fmax (float a, float b) 
{ 
    return (a >= b) ? a : b;
}
#endif

ImageView::ImageView (int x, int y,
		      int w, int h,
		      const char label[],
		      const Imf::Rgba pixels[],
		      float exposure,
		      float defog,
		      float kneeLow,
		      float kneeHigh)
:
    Fl_Gl_Window (x, y, w, h, label),
    _exposure (exposure),
    _defog (defog),
    _kneeLow (kneeLow),
    _kneeHigh (kneeHigh),
    _rawPixels (pixels),
    _screenPixels (w * h * 3),
    _fogR (0),
    _fogG (0),
    _fogB (0)
{
    computeFogColor();
    updateScreenPixels();
}


void
ImageView::setExposure (float exposure)
{
    _exposure = exposure;
    updateScreenPixels();
    redraw();
}


void
ImageView::setDefog (float defog)
{
    _defog = defog;
    updateScreenPixels();
    redraw();
}


void
ImageView::setKneeLow (float kneeLow)
{
    _kneeLow = kneeLow;
    updateScreenPixels();
    redraw();
}


void
ImageView::setKneeHigh (float kneeHigh)
{
    _kneeHigh = kneeHigh;
    updateScreenPixels();
    redraw();
}


void
ImageView::draw()
{
    if (!valid())
    {
	glLoadIdentity();
	glViewport (0, 0, w(), h());
	glOrtho(0, w(), h(), 0, -1, 1);
    }

    glColor3f (1, 0, 1);
    glClear (GL_COLOR_BUFFER_BIT);

    for (int y = 0; y < h(); ++y)
    {
	glRasterPos2i (0, y + 1);

	glDrawPixels (w(), 1,
		      GL_RGB,
		      GL_UNSIGNED_BYTE,
		      _screenPixels + y * w() * 3);
    }
}


void
ImageView::computeFogColor ()
{
    _fogR = 0;
    _fogG = 0;
    _fogB = 0;

    for (int j = 0; j < w() * h(); ++j)
    {
	const Imf::Rgba &rp = _rawPixels[j];
	_fogR += rp.r;
	_fogG += rp.g;
	_fogB += rp.b;
    }

    _fogR /= w() * h();
    _fogG /= w() * h();
    _fogB /= w() * h();
}


// static
float
ImageView::knee (float x, float f)
{
    return Imath::Math<float>::log (x * f + 1) / f;
}


// static
float
ImageView::findKneeF (float x, float y)
{
    float f0 = 0;
    float f1 = 1;

    while (knee (x, f1) > y)
    {
	f0 = f1;
	f1 = f1 * 2;
    }

    for (int i = 0; i < 30; ++i)
    {
	float f2 = (f0 + f1) / 2;
	float y2 = knee (x, f2);

	if (y2 < y)
	    f1 = f2;
	else
	    f0 = f2;
    }

    return (f0 + f1) / 2;
}


namespace {

//
// Conversion from raw pixel data to data for the OpenGl frame buffer:
//
//  1) Compensate for fogging by subtracting defog
//     from the raw pixel values.
//
//  2) Multiply the defogged pixel values by
//     2^(exposure + 2.47393).
//
//  3) Values, which are now 1.0, are called "middle gray".
//     If defog and exposure are both set to 0.0, then
//     middle gray corresponds to a raw pixel value of 0.18.
//     In step 6, middle gray values will be mapped to an
//     intensity 3.5 f-stops below the display's maximum
//     intensity.
//
//  4) Apply a knee function.  The knee function has two
//     parameters, kneeLow and kneeHigh.  Pixel values
//     below 2^kneeLow are not changed by the knee
//     function.  Pixel values above kneeLow are lowered
//     according to a logarithmic curve, such that the
//     value 2^kneeHigh is mapped to 2^3.5 (in step 6,
//     this value will be mapped to the the display's
//     maximum intensity).
//
//  5) Gamma-correct the pixel values, assuming that the
//     screen's gamma is 0.4545 (or 1/2.2).
//
//  6) Scale the values such that pixels middle gray
//     pixels are mapped to 84.66 (or 3.5 f-stops below
//     the display's maximum intensity).
//
//  7) Clamp the values to [0, 255].
//


struct Gamma
{
    float m, d, kl, f;

    Gamma (float exposure, float defog, float kneeLow, float kneeHigh);
    unsigned char operator () (half h);
};


Gamma::Gamma (float exposure, float defog, float kneeLow, float kneeHigh):
    m (Imath::Math<float>::pow (2, exposure + 2.47393)),
    d (defog),
    kl (Imath::Math<float>::pow (2, kneeLow)),
    f (ImageView::findKneeF (Imath::Math<float>::pow (2, kneeHigh) - kl, 
			     Imath::Math<float>::pow (2, 3.5) - kl))
{}


unsigned char
Gamma::operator () (half h)
{
    //
    // Defog
    //

#ifdef PLATFORM_WIN32
    float x = fmax (0.f, (h - d)); 
#else
    float x = std::max (0.f, (h - d));
#endif

    //
    // Exposure
    //

    x *= m;

    //
    // Knee
    //

    if (x > kl)
	x = kl + ImageView::knee (x - kl, f);

    //
    // Gamma
    //

    x = Imath::Math<float>::pow (x, 0.4545f);

    //
    // Scale and clamp
    //

    return char (Imath::clamp (x * 84.66f, 0.f, 255.f));
}

} // namespace


void
ImageView::updateScreenPixels ()
{
    halfFunction<unsigned char>
	rGamma (Gamma (_exposure,
		       _defog * _fogR,
		       _kneeLow,
		       _kneeHigh),
		-HALF_MAX, HALF_MAX);

    halfFunction<unsigned char>
	gGamma (Gamma (_exposure,
		       _defog * _fogG,
		       _kneeLow,
		       _kneeHigh),
		-HALF_MAX, HALF_MAX);

    halfFunction<unsigned char>
	bGamma (Gamma (_exposure,
		       _defog * _fogB,
		       _kneeLow,
		       _kneeHigh),
		-HALF_MAX, HALF_MAX);

    for (int j = 0; j < w() * h(); ++j)
    {
	const Imf::Rgba &rp = _rawPixels[j];
	unsigned char *sp = _screenPixels + j * 3;

	sp[0] = rGamma (rp.r);
	sp[1] = gGamma (rp.g);
	sp[2] = bGamma (rp.b);
    }
}
