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

#include "openexr.h"

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

//
// The table of interop IDs and their chromaticities lives in OpenEXRCore,
// so that the C API, the C++ API and exrinfo all share one definition of
// these primaries. These two functions are adapters over it.
//

bool
colorInteropIDToChromaticities (const std::string& id, Chromaticities& chroma)
{
    //
    // An embedded NUL would make c_str() below compare as a shorter, and
    // possibly valid, ID.
    //

    if (id.find ('\0') != std::string::npos) return false;

    exr_attr_chromaticities_t c;

    if (EXR_ERR_SUCCESS !=
        exr_color_interop_id_to_chromaticities (id.c_str (), &c))
        return false;

    chroma = Chromaticities (
        IMATH_NAMESPACE::V2f (c.red_x, c.red_y),
        IMATH_NAMESPACE::V2f (c.green_x, c.green_y),
        IMATH_NAMESPACE::V2f (c.blue_x, c.blue_y),
        IMATH_NAMESPACE::V2f (c.white_x, c.white_y));

    return true;
}

bool
chromaticitiesToColorInteropID (
    const Chromaticities& chroma, std::string& id, float tolerance)
{
    const exr_attr_chromaticities_t c = {
        chroma.red.x,
        chroma.red.y,
        chroma.green.x,
        chroma.green.y,
        chroma.blue.x,
        chroma.blue.y,
        chroma.white.x,
        chroma.white.y};

    const char* match = nullptr;

    if (EXR_ERR_SUCCESS !=
        exr_chromaticities_to_color_interop_id (&c, tolerance, &match))
        return false;

    id = match;

    return true;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
