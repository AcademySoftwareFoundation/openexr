///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006, Industrial Light & Magic, a division of Lucas
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

//
// A simple rendering transform that converts the RGB pixels
// of a scene-referred image into XYZ pixels for display.
//
// This rendering transform is a placeholder for the Reference
// Rendering Transform (RRT) that is currently being developed
// by the File Format Committee of the Academy of Motion Picture
// Arts and Sciences.  This transform does not claim to be optimal
// in any sense, or to be a close approximation of any RRT candidate
// under consideration.
//
// The transform consists of three steps:
//
//	- convert from the input image's RGB space to RGB with
//	  primaries and white point according to Rec. 709
//
//	- apply a per-channel lookup table that slightly increases
//	  contrast in darker regions but decreases contrast for
//	  highlights.  This tends to make most images look pleasing,
//	  at least in my opinion, for the images I tried.
//
//	- convert from Rec. 709 RGB to CIE XYZ.
//

const Chromaticities rec709 =
{
    {0.6400, 0.3300},
    {0.3000, 0.6000},
    {0.1500, 0.0600},
    {0.3172, 0.3290}
};

const float lutMin = 0.0;
const float lutMax = 4.0;

const float lut[] =
{
    0.000000, 0.034258, 0.078228, 0.123407,
    0.167658, 0.210146, 0.250543, 0.288758,
    0.324816, 0.358798, 0.390816, 0.420991,
    0.449445, 0.476300, 0.501668, 0.525658,
    0.548368, 0.569891, 0.590312, 0.609707,
    0.628150, 0.645704, 0.662431, 0.678385,
    0.693616, 0.708173, 0.722096, 0.735426,
    0.748199, 0.760448, 0.772204, 0.783497,
    0.794351, 0.804793, 0.814844, 0.824526,
    0.833859, 0.842861, 0.851549, 0.859939,
    0.868046, 0.875884, 0.883465, 0.890803,
    0.897909, 0.904793, 0.911465, 0.917936,
    0.924214, 0.930307, 0.936224, 0.941971,
    0.947557, 0.952987, 0.958269, 0.963408,
    0.968409, 0.973279, 0.978022, 0.982644,
    0.987148, 0.991539, 0.995822, 1.000000
};

void 
transform_RRT 
    (varying half R,				// scene-referred RGB pixels
     varying half G,
     varying half B,
     uniform Chromaticities chromaticities,	// RGB space of input image
     output varying float renderedXYZ[3])	// rendered XYZ pixels
{
    float toRec709[4][4] = mult_f44_f44 (RGBtoXYZ (chromaticities, 1.0), 
					 XYZtoRGB (rec709, 1.0));

    float RGB[3] = {R, G, B};
    RGB = mult_f3_f44 (RGB, toRec709);

    RGB[0] = lookup1D (lut, lutMin, lutMax, RGB[0]);
    RGB[1] = lookup1D (lut, lutMin, lutMax, RGB[1]);
    RGB[2] = lookup1D (lut, lutMin, lutMax, RGB[2]);

    float toXYZ[4][4] = RGBtoXYZ (rec709, 1.0);
    renderedXYZ = mult_f3_f44 (RGB, toXYZ);
}
