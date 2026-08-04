//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	CIE (x,y) chromaticities, and conversions between
//	RGB triples and CIE XYZ tristimulus values.
//
//-----------------------------------------------------------------------------

#include "ImfNamespace.h"
#include "ImfChromaticities.h"
#include <string.h>

#include <cmath>
#include <float.h>
#include <stdexcept>

#if defined(_MSC_VER)
// suppress warning about non-exported base classes
#    pragma warning(disable : 4251)
#endif

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

Chromaticities::Chromaticities (
    const IMATH_NAMESPACE::V2f& red,
    const IMATH_NAMESPACE::V2f& green,
    const IMATH_NAMESPACE::V2f& blue,
    const IMATH_NAMESPACE::V2f& white)
    : red (red), green (green), blue (blue), white (white)
{
    // empty
}

bool
Chromaticities::operator== (const Chromaticities& c) const
{
    return red == c.red && green == c.green && blue == c.blue &&
           white == c.white;
}

bool
Chromaticities::operator!= (const Chromaticities& c) const
{
    return red != c.red || green != c.green || blue != c.blue ||
           white != c.white;
}

IMATH_NAMESPACE::M44f
RGBtoXYZ (const Chromaticities& chroma, float Y)
{
    //
    // For an explanation of how the color conversion matrix is derived,
    // see Roy Hall, "Illumination and Color in Computer Generated Imagery",
    // Springer-Verlag, 1989, chapter 3, "Perceptual Response"; and
    // Charles A. Poynton, "A Technical Introduction to Digital Video",
    // John Wiley & Sons, 1996, chapter 7, "Color science for video".
    //

    //
    // X and Z values of RGB value (1, 1, 1), or "white"
    //

    // prevent a division that rounds to zero
    if (std::abs (chroma.white.y) <= 1.f &&
        std::abs (chroma.white.x * Y) >= std::abs (chroma.white.y) * FLT_MAX)
    {
        throw std::invalid_argument (
            "Bad chromaticities: white.y cannot be zero");
    }

    float X = chroma.white.x * Y / chroma.white.y;
    float Z = (1 - chroma.white.x - chroma.white.y) * Y / chroma.white.y;

    //
    // Scale factors for matrix rows, compute numerators and common denominator
    //

    float d = chroma.red.x * (chroma.blue.y - chroma.green.y) +
              chroma.blue.x * (chroma.green.y - chroma.red.y) +
              chroma.green.x * (chroma.red.y - chroma.blue.y);

    float SrN =
        (X * (chroma.blue.y - chroma.green.y) -
         chroma.green.x * (Y * (chroma.blue.y - 1) + chroma.blue.y * (X + Z)) +
         chroma.blue.x * (Y * (chroma.green.y - 1) + chroma.green.y * (X + Z)));

    float SgN =
        (X * (chroma.red.y - chroma.blue.y) +
         chroma.red.x * (Y * (chroma.blue.y - 1) + chroma.blue.y * (X + Z)) -
         chroma.blue.x * (Y * (chroma.red.y - 1) + chroma.red.y * (X + Z)));

    float SbN =
        (X * (chroma.green.y - chroma.red.y) -
         chroma.red.x * (Y * (chroma.green.y - 1) + chroma.green.y * (X + Z)) +
         chroma.green.x * (Y * (chroma.red.y - 1) + chroma.red.y * (X + Z)));

    if (std::abs (d) < 1.f && (std::abs (SrN) >= std::abs (d) * FLT_MAX ||
                               std::abs (SgN) >= std::abs (d) * FLT_MAX ||
                               std::abs (SbN) >= std::abs (d) * FLT_MAX))
    {
        // cannot generate matrix if all RGB primaries have the same y value
        // or if they all have the an x value of zero
        // in both cases, the primaries are colinear, which makes them unusable
        throw std::invalid_argument (
            "Bad chromaticities: RGBtoXYZ matrix is degenerate");
    }

    float Sr = SrN / d;
    float Sg = SgN / d;
    float Sb = SbN / d;

    //
    // Assemble the matrix
    //

    IMATH_NAMESPACE::M44f M;

    M[0][0] = Sr * chroma.red.x;
    M[0][1] = Sr * chroma.red.y;
    M[0][2] = Sr * (1 - chroma.red.x - chroma.red.y);

    M[1][0] = Sg * chroma.green.x;
    M[1][1] = Sg * chroma.green.y;
    M[1][2] = Sg * (1 - chroma.green.x - chroma.green.y);

    M[2][0] = Sb * chroma.blue.x;
    M[2][1] = Sb * chroma.blue.y;
    M[2][2] = Sb * (1 - chroma.blue.x - chroma.blue.y);

    return M;
}

IMATH_NAMESPACE::M44f
XYZtoRGB (const Chromaticities& chroma, float Y)
{
    return RGBtoXYZ (chroma, Y).inverse ();
}

namespace
{

//
// The chromaticities of the linear, scene-referred color interop IDs, per
// the Color Interop Forum recommendation for OpenEXR files. This is the
// library's single definition of these primaries; acesChromaticities()
// takes the lin_ap0_scene entry from here.
//

// Held as plain floats rather than Chromaticities so that the table is
// constant-initialized into read-only data, with no run-time construction.

struct ColorInteropIDChromaticities
{
    const char* id;
    float       red[2];
    float       green[2];
    float       blue[2];
    float       white[2];
};

const ColorInteropIDChromaticities colorInteropIDChromaticitiesTable[] = {
    {"lin_rec709_scene",
     {0.6400f, 0.3300f},
     {0.3000f, 0.6000f},
     {0.1500f, 0.0600f},
     {0.3127f, 0.3290f}},
    {"lin_ap0_scene",
     {0.73470f, 0.26530f},
     {0.00000f, 1.00000f},
     {0.00010f, -0.07700f},
     {0.32168f, 0.33767f}},
    {"lin_ap1_scene",
     {0.7130f, 0.2930f},
     {0.1650f, 0.8300f},
     {0.1280f, 0.0440f},
     {0.32168f, 0.33767f}},
    {"lin_p3d65_scene",
     {0.6800f, 0.3200f},
     {0.2650f, 0.6900f},
     {0.1500f, 0.0600f},
     {0.3127f, 0.3290f}},
    {"lin_rec2020_scene",
     {0.7080f, 0.2920f},
     {0.1700f, 0.7970f},
     {0.1310f, 0.0460f},
     {0.3127f, 0.3290f}},
    {"lin_adobergb_scene",
     {0.6400f, 0.3300f},
     {0.2100f, 0.7100f},
     {0.1500f, 0.0600f},
     {0.3127f, 0.3290f}}};

const size_t numColorInteropIDChromaticities =
    sizeof (colorInteropIDChromaticitiesTable) /
    sizeof (colorInteropIDChromaticitiesTable[0]);

bool
withinTolerance (const float a[2], const IMATH_NAMESPACE::V2f& b, float tolerance)
{
    return std::abs (a[0] - b.x) <= tolerance &&
           std::abs (a[1] - b.y) <= tolerance;
}

} // namespace

bool
colorInteropIDToChromaticities (const std::string& id, Chromaticities& chroma)
{
    for (size_t i = 0; i < numColorInteropIDChromaticities; i++)
    {
        const ColorInteropIDChromaticities& e =
            colorInteropIDChromaticitiesTable[i];

        if (id == e.id)
        {
            chroma = Chromaticities (
                IMATH_NAMESPACE::V2f (e.red[0], e.red[1]),
                IMATH_NAMESPACE::V2f (e.green[0], e.green[1]),
                IMATH_NAMESPACE::V2f (e.blue[0], e.blue[1]),
                IMATH_NAMESPACE::V2f (e.white[0], e.white[1]));
            return true;
        }
    }

    return false;
}

bool
chromaticitiesToColorInteropID (
    const Chromaticities& chroma, std::string& id, float tolerance)
{
    for (size_t i = 0; i < numColorInteropIDChromaticities; i++)
    {
        const ColorInteropIDChromaticities& e =
            colorInteropIDChromaticitiesTable[i];

        if (withinTolerance (e.red, chroma.red, tolerance) &&
            withinTolerance (e.green, chroma.green, tolerance) &&
            withinTolerance (e.blue, chroma.blue, tolerance) &&
            withinTolerance (e.white, chroma.white, tolerance))
        {
            id = e.id;
            return true;
        }
    }

    return false;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
