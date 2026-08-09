//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_CHROMATICITIES_H
#define INCLUDED_IMF_CHROMATICITIES_H

//-----------------------------------------------------------------------------
//
//	CIE (x,y) chromaticities, and conversions between
//	RGB triples and CIE XYZ tristimulus values.
//
//-----------------------------------------------------------------------------

#include "ImfExport.h"
#include "ImfNamespace.h"

#include <Imath/ImathMatrix.h>
#include <Imath/ImathVec.h>

#include <string>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

struct IMF_EXPORT_TYPE Chromaticities
{
    //-----------------------------------------------
    // The CIE x and y coordinates of the RGB triples
    // (1,0,0), (0,1,0), (0,0,1) and (1,1,1).
    //-----------------------------------------------

    IMATH_NAMESPACE::V2f red;
    IMATH_NAMESPACE::V2f green;
    IMATH_NAMESPACE::V2f blue;
    IMATH_NAMESPACE::V2f white;

    //--------------------------------------------
    // Default constructor produces chromaticities
    // according to Rec. ITU-R BT.709-3
    //--------------------------------------------

    IMF_EXPORT
    Chromaticities (
        const IMATH_NAMESPACE::V2f& red =
            IMATH_NAMESPACE::V2f (0.6400f, 0.3300f),
        const IMATH_NAMESPACE::V2f& green =
            IMATH_NAMESPACE::V2f (0.3000f, 0.6000f),
        const IMATH_NAMESPACE::V2f& blue =
            IMATH_NAMESPACE::V2f (0.1500f, 0.0600f),
        const IMATH_NAMESPACE::V2f& white =
            IMATH_NAMESPACE::V2f (0.3127f, 0.3290f));

    //---------
    // Equality
    //---------

    IMF_EXPORT
    bool operator== (const Chromaticities& v) const;
    IMF_EXPORT
    bool operator!= (const Chromaticities& v) const;
};

//
// Conversions between RGB and CIE XYZ
//
// RGB to XYZ:
//
// 	Given a set of chromaticities, c, and the luminance, Y, of the RGB
// 	triple (1,1,1), or "white", RGBtoXYZ(c,Y) computes a matrix, M, so
// 	that multiplying an RGB value, v, with M produces an equivalent
// 	XYZ value, w.  (w == v * M)
//
// 	If we define that
//
// 	   (Xr, Yr, Zr) == (1, 0, 0) * M
// 	   (Xg, Yg, Zg) == (0, 1, 0) * M
// 	   (Xb, Yb, Zb) == (0, 0, 1) * M
// 	   (Xw, Yw, Zw) == (1, 1, 1) * M,
//
// 	then the following statements are true:
//
// 	   Xr / (Xr + Yr + Zr) == c.red.x
// 	   Yr / (Xr + Yr + Zr) == c.red.y
//
// 	   Xg / (Xg + Yg + Zg) == c.green.x
// 	   Yg / (Xg + Yg + Zg) == c.green.y
//
// 	   Xb / (Xb + Yb + Zb) == c.blue.x
// 	   Yb / (Xb + Yb + Zb) == c.blue.y
//
// 	   Xw / (Xw + Yw + Zw) == c.white.x
// 	   Yw / (Xw + Yw + Zw) == c.white.y
//
// 	   Yw == Y.
//
// XYZ to RGB:
//
// 	XYZtoRGB(c,Y) returns RGBtoXYZ(c,Y).inverse().
//

IMF_EXPORT IMATH_NAMESPACE::M44f
           RGBtoXYZ (const Chromaticities& chroma, float Y);
IMF_EXPORT IMATH_NAMESPACE::M44f
           XYZtoRGB (const Chromaticities& chroma, float Y);

//
// Conversions between the chromaticities attribute and the colorInteropID
// attribute
//
// The Color Interop Forum recommendation for OpenEXR files defines a
// mapping between chromaticities and the subset of color interop IDs that
// denote linear, scene-referred RGB color spaces. Only these six IDs have
// a defined mapping:
//
//     lin_rec709_scene     lin_ap0_scene       lin_ap1_scene
//     lin_p3d65_scene      lin_rec2020_scene   lin_adobergb_scene
//
// See <https://github.com/AcademySoftwareFoundation/ColorInterop/blob/main/
// Recommendations/04_OpenEXRFiles/OpenEXRFiles.md>.
//
// Both functions return false and leave their output argument unmodified
// for any other value, including "unknown" and "data". An unsupported
// value is an ordinary outcome rather than an error, so neither function
// throws.
//
// Note that the recommendation is that the chromaticities attribute not be
// set when setting the colorInteropID, other than for ST 2065-4
// compliance. colorInteropIDToChromaticities is intended for feeding
// legacy consumers that understand only chromaticities.
//
// chromaticitiesToColorInteropID matches a set of chromaticities against
// the table within +/- tolerance in x and y, comparing all four
// coordinates. The default tolerance is the value the recommendation
// suggests. 
//

IMF_EXPORT bool
colorInteropIDToChromaticities (const std::string& id, Chromaticities& chroma);

IMF_EXPORT bool chromaticitiesToColorInteropID (
    const Chromaticities& chroma, std::string& id, float tolerance = 0.001f);

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
