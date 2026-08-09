//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	Optional Standard Attributes
//
//-----------------------------------------------------------------------------

#include "ImfStandardAttributes.h"

#include "openexr.h"

#include <stdint.h>
#include <string.h>

#if defined(_MSC_VER)
// suppress warning about non-exported base classes
#    pragma warning(disable : 4251)
#    pragma warning(disable : 4275)
#endif

#define IMF_STRING(name) #name

#define IMF_STD_ATTRIBUTE_IMP(name, suffix, type)                              \
                                                                               \
    void IMF_ADD_SUFFIX (suffix) (Header & header, const type& value)          \
    {                                                                          \
        header.insert (IMF_STRING (name), TypedAttribute<type> (value));       \
    }                                                                          \
                                                                               \
    bool IMF_HAS_SUFFIX (suffix) (const Header& header)                        \
    {                                                                          \
        return header.findTypedAttribute<TypedAttribute<type>> (               \
                   IMF_STRING (name)) != 0;                                    \
    }                                                                          \
                                                                               \
    const TypedAttribute<type>& IMF_NAME_ATTRIBUTE (name) (                    \
        const Header& header)                                                  \
    {                                                                          \
        return header.typedAttribute<TypedAttribute<type>> (                   \
            IMF_STRING (name));                                                \
    }                                                                          \
                                                                               \
    TypedAttribute<type>& IMF_NAME_ATTRIBUTE (name) (Header & header)          \
    {                                                                          \
        return header.typedAttribute<TypedAttribute<type>> (                   \
            IMF_STRING (name));                                                \
    }                                                                          \
                                                                               \
    const type& name (const Header& header)                                    \
    {                                                                          \
        return IMF_NAME_ATTRIBUTE (name) (header).value ();                    \
    }                                                                          \
                                                                               \
    type& name (Header& header)                                                \
    {                                                                          \
        return IMF_NAME_ATTRIBUTE (name) (header).value ();                    \
    }

#include "ImfNamespace.h"

using namespace IMATH_NAMESPACE;
using namespace std;

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

IMF_STD_ATTRIBUTE_IMP (originalDataWindow, OriginalDataWindow, Box2i)
IMF_STD_ATTRIBUTE_IMP (worldToCamera, WorldToCamera, M44f)
IMF_STD_ATTRIBUTE_IMP (worldToNDC, WorldToNDC, M44f)
IMF_STD_ATTRIBUTE_IMP (sensorCenterOffset, SensorCenterOffset, V2f)
IMF_STD_ATTRIBUTE_IMP (sensorOverallDimensions, SensorOverallDimensions, V2f)
IMF_STD_ATTRIBUTE_IMP (sensorPhotositePitch, SensorPhotositePitch, float)
IMF_STD_ATTRIBUTE_IMP (
    sensorAcquisitionRectangle, SensorAcquisitionRectangle, Box2i)
IMF_STD_ATTRIBUTE_IMP (ascFramingDecisionList, AscFramingDecisionList, string)
IMF_STD_ATTRIBUTE_IMP (xDensity, XDensity, float)
IMF_STD_ATTRIBUTE_IMP (longitude, Longitude, float)
IMF_STD_ATTRIBUTE_IMP (latitude, Latitude, float)
IMF_STD_ATTRIBUTE_IMP (altitude, Altitude, float)
IMF_STD_ATTRIBUTE_IMP (cameraMake, CameraMake, string)
IMF_STD_ATTRIBUTE_IMP (cameraModel, CameraModel, string)
IMF_STD_ATTRIBUTE_IMP (cameraSerialNumber, CameraSerialNumber, string)
IMF_STD_ATTRIBUTE_IMP (cameraFirmwareVersion, CameraFirmwareVersion, string)
IMF_STD_ATTRIBUTE_IMP (cameraUuid, CameraUuid, string)
IMF_STD_ATTRIBUTE_IMP (cameraLabel, CameraLabel, string)
IMF_STD_ATTRIBUTE_IMP (cameraCCTSetting, CameraCCTSetting, float)
IMF_STD_ATTRIBUTE_IMP (cameraTintSetting, CameraTintSetting, float)
IMF_STD_ATTRIBUTE_IMP (cameraColorBalance, CameraColorBalance, V2f)
IMF_STD_ATTRIBUTE_IMP (isoSpeed, IsoSpeed, float)
IMF_STD_ATTRIBUTE_IMP (expTime, ExpTime, float)
IMF_STD_ATTRIBUTE_IMP (shutterAngle, ShutterAngle, float)
IMF_STD_ATTRIBUTE_IMP (captureRate, CaptureRate, Rational)
IMF_STD_ATTRIBUTE_IMP (lensMake, LensMake, string)
IMF_STD_ATTRIBUTE_IMP (lensModel, LensModel, string)
IMF_STD_ATTRIBUTE_IMP (lensSerialNumber, LensSerialNumber, string)
IMF_STD_ATTRIBUTE_IMP (lensFirmwareVersion, LensFirmwareVersion, string)
IMF_STD_ATTRIBUTE_IMP (nominalFocalLength, NominalFocalLength, float)
IMF_STD_ATTRIBUTE_IMP (pinholeFocalLength, PinholeFocalLength, float)
IMF_STD_ATTRIBUTE_IMP (effectiveFocalLength, EffectiveFocalLength, float)
IMF_STD_ATTRIBUTE_IMP (entrancePupilOffset, EntrancePupilOffset, float)
IMF_STD_ATTRIBUTE_IMP (aperture, Aperture, float)
IMF_STD_ATTRIBUTE_IMP (tStop, TStop, float)
IMF_STD_ATTRIBUTE_IMP (focus, Focus, float)
IMF_STD_ATTRIBUTE_IMP (owner, Owner, string)
IMF_STD_ATTRIBUTE_IMP (comments, Comments, string)
IMF_STD_ATTRIBUTE_IMP (capDate, CapDate, string)
IMF_STD_ATTRIBUTE_IMP (utcOffset, UtcOffset, float)
IMF_STD_ATTRIBUTE_IMP (keyCode, KeyCode, KeyCode)
IMF_STD_ATTRIBUTE_IMP (timeCode, TimeCode, TimeCode)
IMF_STD_ATTRIBUTE_IMP (framesPerSecond, FramesPerSecond, Rational)
IMF_STD_ATTRIBUTE_IMP (imageCounter, ImageCounter, int)
IMF_STD_ATTRIBUTE_IMP (reelName, ReelName, string)
IMF_STD_ATTRIBUTE_IMP (chromaticities, Chromaticities, Chromaticities)
IMF_STD_ATTRIBUTE_IMP (colorInteropID, ColorInteropID, string)
IMF_STD_ATTRIBUTE_IMP (whiteLuminance, WhiteLuminance, float)
IMF_STD_ATTRIBUTE_IMP (adoptedNeutral, AdoptedNeutral, V2f)
IMF_STD_ATTRIBUTE_IMP (acesImageContainerFlag, AcesImageContainerFlag, int)

#if defined(_MSC_VER)
    __pragma(warning(push))
    __pragma(warning(disable: 4996))
#elif defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
IMF_STD_ATTRIBUTE_IMP (renderingTransform, RenderingTransform, string)
IMF_STD_ATTRIBUTE_IMP (lookModTransform, LookModTransform, string)
IMF_STD_ATTRIBUTE_IMP (dwaCompressionLevel, DwaCompressionLevel, float)
#if defined(_MSC_VER)
    __pragma(warning(pop))
#elif defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

IMF_STD_ATTRIBUTE_IMP (envmap, Envmap, Envmap)
IMF_STD_ATTRIBUTE_IMP (wrapmodes, Wrapmodes, string)
IMF_STD_ATTRIBUTE_IMP (multiView, MultiView, StringVector)
IMF_STD_ATTRIBUTE_IMP (deepImageState, DeepImageState, DeepImageState)
IMF_STD_ATTRIBUTE_IMP (idManifest, IDManifest, CompressedIDManifest)

//
// Color space metadata consistency checking
//
// The rules live in OpenEXRCore so that the C API, the C++ API and exrinfo
// all apply the same ones; these functions adapt a Header to that call.
//

namespace
{

//
// The two enumerations are declared separately so that neither public
// header has to include the other's, which makes it possible for them to
// drift. Use a compile-time assert to validate consistency.
//

#define IMF_ASSERT_SAME_WARNING(imfName, coreName)                             \
    static_assert (                                                            \
        static_cast<int> (imfName) == static_cast<int> (coreName),             \
        #imfName " and " #coreName " have diverged")

IMF_ASSERT_SAME_WARNING (COLOR_METADATA_OK, EXR_COLOR_METADATA_OK);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_EMPTY_INTEROP_ID, EXR_COLOR_METADATA_EMPTY_INTEROP_ID);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_CHROMATICITIES_DIFFER,
    EXR_COLOR_METADATA_CHROMATICITIES_DIFFER);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_DATA_HAS_CHROMATICITIES,
    EXR_COLOR_METADATA_DATA_HAS_CHROMATICITIES);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_DATA_HAS_WHITE_LUMINANCE,
    EXR_COLOR_METADATA_DATA_HAS_WHITE_LUMINANCE);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_DATA_HAS_ADOPTED_NEUTRAL,
    EXR_COLOR_METADATA_DATA_HAS_ADOPTED_NEUTRAL);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_INTEROP_ID_NOT_SHARED,
    EXR_COLOR_METADATA_INTEROP_ID_NOT_SHARED);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_ACES_FLAG_NOT_ONE, EXR_COLOR_METADATA_ACES_FLAG_NOT_ONE);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0,
    EXR_COLOR_METADATA_ACES_FLAG_INTEROP_ID_NOT_AP0);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_ACES_FLAG_NO_CHROMATICITIES,
    EXR_COLOR_METADATA_ACES_FLAG_NO_CHROMATICITIES);
IMF_ASSERT_SAME_WARNING (
    COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0,
    EXR_COLOR_METADATA_ACES_FLAG_CHROMATICITIES_NOT_AP0);

#undef IMF_ASSERT_SAME_WARNING

//
// Describe a header to the Core check. The struct only holds a pointer to
// the chromaticities, so the caller supplies the storage they live in; the
// strings point into the header's attributes.
//

void
fillColorMetadata (
    const Header&              header,
    exr_color_metadata_t&      values,
    exr_attr_chromaticities_t& chromaBuffer)
{
    memset (&values, 0, sizeof (values));

    if (hasColorInteropID (header))
        values.color_interop_id = colorInteropID (header).c_str ();

    if (hasChromaticities (header))
    {
        const Chromaticities& c = chromaticities (header);

        chromaBuffer.red_x   = c.red.x;
        chromaBuffer.red_y   = c.red.y;
        chromaBuffer.green_x = c.green.x;
        chromaBuffer.green_y = c.green.y;
        chromaBuffer.blue_x  = c.blue.x;
        chromaBuffer.blue_y  = c.blue.y;
        chromaBuffer.white_x = c.white.x;
        chromaBuffer.white_y = c.white.y;

        values.chromaticities = &chromaBuffer;
    }

    values.has_white_luminance = hasWhiteLuminance (header) ? 1 : 0;
    values.has_adopted_neutral = hasAdoptedNeutral (header) ? 1 : 0;

    if (hasAcesImageContainerFlag (header))
    {
        values.has_aces_image_container_flag = 1;
        values.aces_image_container_flag = acesImageContainerFlag (header);
    }
    else if (header.find ("acesImageContainerFlag") != header.end ())
    {
        //
        // Present, but not an int: hasAcesImageContainerFlag() only matches
        // the expected type. A wrong type is reported the same way as a
        // wrong value, so any non-conforming value will do here.
        //

        values.has_aces_image_container_flag = 1;
        values.aces_image_container_flag     = 0;
    }
}

} // namespace

unsigned int
checkColorMetadata (const Header& header)
{
    exr_color_metadata_t      values;
    exr_attr_chromaticities_t chromaBuffer;
    uint32_t                  warnings = EXR_COLOR_METADATA_OK;

    fillColorMetadata (header, values, chromaBuffer);

    exr_check_color_metadata_values (&values, &warnings);

    return warnings;
}

unsigned int
checkColorMetadata (const Header& header, const Header& firstPart)
{
    exr_color_metadata_t      values;
    exr_attr_chromaticities_t chromaBuffer;
    uint32_t                  warnings = EXR_COLOR_METADATA_OK;

    fillColorMetadata (header, values, chromaBuffer);

    values.compare_to_first_part = 1;

    if (hasColorInteropID (firstPart))
        values.first_part_color_interop_id =
            colorInteropID (firstPart).c_str ();

    exr_check_color_metadata_values (&values, &warnings);

    return warnings;
}

const char*
colorMetadataWarningToString (ColorMetadataWarning warning)
{
    return exr_get_color_metadata_warning_as_string (
        static_cast<exr_color_metadata_warning_t> (warning));
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
