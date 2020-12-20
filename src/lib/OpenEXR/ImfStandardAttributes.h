///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004, Industrial Light & Magic, a division of Lucas
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


#ifndef INCLUDED_IMF_STANDARD_ATTRIBUTES_H
#define INCLUDED_IMF_STANDARD_ATTRIBUTES_H

//-----------------------------------------------------------------------------
//
//	Optional Standard Attributes -- these attributes are "optional"
//	because not every image file header has them, but they define a
//	"standard" way to represent commonly used data in the file header.
//
//	For each attribute, with name "foo", and type "T", the following
//	functions are automatically generated via macros:
//
//	void			   addFoo (Header &header, const T &value);
//	bool			   hasFoo (const Header &header);
//	const TypedAttribute<T> &  fooAttribute (const Header &header);
//	TypedAttribute<T> &	   fooAttribute (Header &header);
//	const T &		   foo (const Header &Header);
//	T &			   foo (Header &Header);
//
//-----------------------------------------------------------------------------

#include "ImfHeader.h"
#include "ImfBoxAttribute.h"
#include "ImfChromaticitiesAttribute.h"
#include "ImfEnvmapAttribute.h"
#include "ImfDeepImageStateAttribute.h"
#include "ImfFloatAttribute.h"
#include "ImfKeyCodeAttribute.h"
#include "ImfMatrixAttribute.h"
#include "ImfRationalAttribute.h"
#include "ImfStringAttribute.h"
#include "ImfStringVectorAttribute.h"
#include "ImfTimeCodeAttribute.h"
#include "ImfVecAttribute.h"
#include "ImfNamespace.h"
#include "ImfExport.h"

#define IMF_ADD_SUFFIX(suffix) add##suffix
#define IMF_HAS_SUFFIX(suffix) has##suffix
#define IMF_NAME_ATTRIBUTE(name) name##Attribute

#define IMF_STD_ATTRIBUTE_DEF(name,suffix,object)                            \
                                                                             \
    OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER                              \
    IMF_EXPORT void           IMF_ADD_SUFFIX(suffix) (Header &header, const object &v); \
    IMF_EXPORT bool           IMF_HAS_SUFFIX(suffix) (const Header &header);     \
    IMF_EXPORT const TypedAttribute<object> &                                \
                              IMF_NAME_ATTRIBUTE(name) (const Header &header); \
    IMF_EXPORT TypedAttribute<object> &                                      \
                              IMF_NAME_ATTRIBUTE(name) (Header &header);     \
    IMF_EXPORT const object &                                                \
                              name (const Header &header);                   \
    IMF_EXPORT object &       name (Header &header);                         \
    OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT                               \

//
// acesImageContainerFlag -- indicates that the file and all attributes
// comply with SMPTE ST 2065-4:202X, "ACES Image Container File Layout".
// If present, the value is always 1.
//
// To comply with SMPTE ST 2065-4 an image must
// - have a chromaticities attribute with values matching those given in
//   SMPTE ST 2065-1:202X, "Academy Color Encoding Specification (ACES)"
// - be uncompressed, indicating this with a compression attribute with
//   value 0
// - contain all the required attributes present in an OpenEXR file,
//   those being channels, dataWindow, displayWindow, lineOrder,
//   pixelAspectRatio, screenWindowCenter, and screenWindowWidth.
// - have an adoptedNeutral value, if the adoptedNeutral attribute is
//   present, matching that given in SMPTE ST 2065-1:202X.
//

IMF_STD_ATTRIBUTE_DEF (acesImageContainerFlag, AcesImageContainerFlag, int)


//
// originalImageFlag -- indicates whether or not the pixel data is an
// unaltered original from a source such as an electronic camera, a film
// scanner, or a computer graphics rendering engine. A value of 1
// indicates the image is an unaltered orginal, and a value of 0 
// indicates that the image has been altered in some way. Values other
// than 0 or 1 are undefined and should not be used.
//
// 'Unaltered original' does not usually mean 'raw image sensor data'.
// Some amount of processing will be required to convert raw data from
// the sensor in an electronic camera or film scanner into an OpenEXR
// file, but this type of processing is not considered altering the
// image.
// 
// An original image that is copied, with all image data identical in
// the copy, with identical sets of attributes each of which has an
// identical value as its counterpart, but in which the attributes are
// stored in a different order, can still be considered an original
// image file.

IMF_STD_ATTRIBUTE_DEF (originalImageFlag, OriginalImageFlag, int)


//
// metadataDigestMD5 -- MD5 message digest, as specified in IETF RFC 1321,
// of the bytes in the closed interval that starts with the first byte in
// the file and ends with the last byte in the line offset table. In the
// input to the digest computation, the metadataDigestMD5 attribute value
// is considered to be a 128-bit integer with all bits set to zero.
//
// Note that equality of metadataDigestMD5 attribute values is a sufficient
// but not a neccessary indicator that two images have equal metadata;
// since OpenEXR does not specify a particular order to metadata attribute
// storage, two images might have equal metadata but might have different
// calculated metadataDigestMD5 values.
//

IMF_STD_ATTRIBUTE_DEF (metadataDigestMD5, MetadataDigestMD5, std::string)

//
// chromaticities -- for RGB images, specifies the CIE (x,y)
// chromaticities of the primaries and the white point
//

IMF_STD_ATTRIBUTE_DEF (chromaticities, Chromaticities, Chromaticities)


//
// whiteLuminance -- for RGB images, defines the luminance, in Nits
// (candelas per square meter) of the RGB value (1.0, 1.0, 1.0).
//
// If the chromaticities and the whiteLuminance of an RGB image are
// known, then it is possible to convert the image's pixels from RGB
// to CIE XYZ tristimulus values (see function RGBtoXYZ() in header
// file ImfChromaticities.h).
// 
//

IMF_STD_ATTRIBUTE_DEF (whiteLuminance, WhiteLuminance, float)


//
// adoptedNeutral -- specifies the CIE (x,y) coordinates that should
// be considered neutral during color rendering.  Pixels in the image
// file whose (x,y) coordinates match the adoptedNeutral value should
// be mapped to neutral values on the display.
//

IMF_STD_ATTRIBUTE_DEF (adoptedNeutral, AdoptedNeutral, IMATH_NAMESPACE::V2f)


//
// renderingTransform, lookModTransform -- specify the names of the
// CTL functions that implements the intended color rendering and look
// modification transforms for this image.
// 

IMF_STD_ATTRIBUTE_DEF (renderingTransform, RenderingTransform, std::string)
IMF_STD_ATTRIBUTE_DEF (lookModTransform, LookModTransform, std::string)


//
// xDensity -- horizontal output density, in pixels per inch.
// The image's vertical output density is xDensity * pixelAspectRatio.
//

IMF_STD_ATTRIBUTE_DEF (xDensity, XDensity, float)


//
// owner -- name of the owner of the image
//

IMF_STD_ATTRIBUTE_DEF (owner, Owner, std::string)
   

//
// creator -- name of the creator of the image
//

IMF_STD_ATTRIBUTE_DEF (creator, Creator, std::string)


//
// comments -- additional image information in human-readable
// form, for example a verbal description of the image
//

IMF_STD_ATTRIBUTE_DEF (comments, Comments, std::string)


//
// capDate -- the date when the image was created or captured,
// in local time, and formatted as
//
//    YYYY:MM:DD hh:mm:ss
//
// where YYYY is the year (4 digits, e.g. 2003), MM is the month
// (2 digits, 01, 02, ... 12), DD is the day of the month (2 digits,
// 01, 02, ... 31), hh is the hour (2 digits, 00, 01, ... 23), mm
// is the minute, and ss is the second (2 digits, 00, 01, ... 59).
//
//

IMF_STD_ATTRIBUTE_DEF (capDate, CapDate, std::string)


//
// utcOffset -- offset of local time at capDate from
// Universal Coordinated Time (UTC), in seconds:
//
//    UTC == local time + utcOffset
//

IMF_STD_ATTRIBUTE_DEF (utcOffset, UtcOffset, float)


//
// longitude, latitude, altitude -- for images of real objects, the
// location where the image was recorded.  Longitude and latitude are
// in degrees east of Greenwich and north of the equator.  Altitude
// is in meters above sea level.  For example, Kathmandu, Nepal is
// at longitude 85.317, latitude 27.717, altitude 1305.
//

IMF_STD_ATTRIBUTE_DEF (longitude, Longitude, float)
IMF_STD_ATTRIBUTE_DEF (latitude, Latitude, float)
IMF_STD_ATTRIBUTE_DEF (altitude, Altitude, float)


//
// cameraFirmwareVersion -- the firmware version of the camera
//

IMF_STD_ATTRIBUTE_DEF (cameraFirmwareVersion, CameraFirmwareVersion, std::string)


//
// cameraIdentifier -- identifies this camera uniquely among all
// cameras from all vendors. This could be accomplished with, e.g.,
// a MAC address, a concatenation of cameraMake, cameraModel,
// cameraSerialNumber, etc.
//

IMF_STD_ATTRIBUTE_DEF (cameraIdentifier, CameraIdentifier, std::string)


//
// cameraLabel -- text label identifying how the camera was used or
// assigned, e.g. "Camera 1 Left", "B Camera", "POV", etc.
//

IMF_STD_ATTRIBUTE_DEF (cameraLabel, CameraLabel, std::string)


//
// cameraMake -- manufacturer or vendor of the camera.
//

IMF_STD_ATTRIBUTE_DEF (cameraMake, CameraMake, std::string)


//
// cameraModel -- model name or model number of the camera.
//

IMF_STD_ATTRIBUTE_DEF (cameraModel, CameraModel, std::string)


//
// cameraSerialNumber -- serial number of the camera
//

IMF_STD_ATTRIBUTE_DEF (cameraSerialNumber, CameraSerialNumber, std::string)


//
// colorTemperature -- correlated color temperature in Kelvin
// selected for the camera at the time of image was created or
// captured
//

IMF_STD_ATTRIBUTE_DEF (colorTemperature, ColorTemperature, float)


//
// tint - the green / magenta tint selected for the camera at the
// time the image was created or captured. Positive units of tint
// correspond to a green shift equal to that effected by a Kodak
// CC035G or Rosco E-Colour Eighth Plus Green filter; negative
// units of tint correspond to a magenta shift equal to that effected
// by a Kodak CC035M or Rosco E-Colour Eighth Minus Green filter.
//

IMF_STD_ATTRIBUTE_DEF (tint, Tint, float)


//
// lensMake -- manufacturer or vendor of the lens
//

IMF_STD_ATTRIBUTE_DEF (lensMake, LensMake, std::string)


//
// lensModel -- model name or model number of the lens
//

IMF_STD_ATTRIBUTE_DEF (lensModel, LensModel, std::string)


//
// lensSerialNumber -- serial number of the lens
//

IMF_STD_ATTRIBUTE_DEF (lensSerialNumber, LensSerialNumber, std::string)


//
// lensFirmwareVersion -- the firmware version of the lens
//

IMF_STD_ATTRIBUTE_DEF (lensFirmwareVersion, LensFirmwareVersion, std::string)


//
// lensTemperature -- Celsius temperature of the lens
//

IMF_STD_ATTRIBUTE_DEF (lensTemperature, LensTemperature, float)


//
// lensAttributes -- lens metadata not specified in other predefined
// attributes (such as aperture, focus, focalLength, lensMake, etc);
// not to be used in place of any predefined lens-related attribute.
//

IMF_STD_ATTRIBUTE_DEF (lensAttributes, LensAttributes, std::string)


//
// focalPlaneImageSize -- width and height of the area on the focal
// plane corresponding to the pixels in the data window, with x
// holding the width and y holding the height
//

IMF_STD_ATTRIBUTE_DEF (focalPlaneImageSize, FocalPlaneImageSize, IMATH_NAMESPACE::V2f)


//
// focus -- the camera's focus distance, in meters
//

IMF_STD_ATTRIBUTE_DEF (focus, Focus, float)


//
// convergenceDistance -- distance in meters from the baseline of the
// two lens entrance pupils to the point where the lens optical center
// axes cross each other
//

IMF_STD_ATTRIBUTE_DEF (convergenceDistance, ConvergenceDistance, float)


//
// interocularDistance -- distance in meters between centers of entrance
// pupils of two lenses (a negative value can reflect a 'flip' of the
// eye positions for each camera)
//

IMF_STD_ATTRIBUTE_DEF (interocularDistance, InterocularDistance, float)


//
// entrancePupilOffset -- distance in millimeters between the film or
// sensor plane and the center of the entrance pupil of the lens.
//

IMF_STD_ATTRIBUTE_DEF (entrancePupilOffset, EntrancePupilOffset, float)


//
// captureRate -- capture rate, in frames per second, of the image
// sequence to which the image belongs, represented as a rational
// number. For variable frame rates, time-lapse photography, etc. the
// capture rate r is calculated as
//
//   r = 1 / (tN - tNm1)
//
// where tn is the time, in seconds, of the center of frame N's 
// exposure interval, and tNm1 is the time, in seconds, of the center
// of frame N-1's exposure interval.
//
// Both the numerator and denominator of r must be strictly positive.

IMF_STD_ATTRIBUTE_DEF (captureRate, CaptureRate, Rational)


//
// exposure -- exposure time, in seconds
//

IMF_STD_ATTRIBUTE_DEF (expTime, ExpTime, float)


//
// effectiveFocalLength -- focal length in millimeters of a hypothetical
// equivalent lens exhibiting no focus 'breathing'
//

IMF_STD_ATTRIBUTE_DEF (effectiveFocalLength, EffectiveFocalLength, float)


//
// aperture -- the camera's lens aperture, in f-stops (focal length
// of the lens divided by the diameter of the iris opening)
// 
// In ACES Container files conforming to SMPTE ST 2065-4, the attribute
// named 'aperture' can contain either the T-stop of the lens, or the
// F-number of the lens.
//
// If the T-stop is known, it is preferable to use the tStop attribute
// to indicate this unambiguously.
//
// If the F-number is known, it is preferable to use the fNumber
// attribute to indicate this unambiguously to OpenEXR readers following
// strict ACES Container semantics.
// 
// If a number is provided to an application creating an OpenEXR file
// but it's unclear whether that number is a T-stop or an F-number, the
// aperture attribute should be used to carry that metadatum.

IMF_STD_ATTRIBUTE_DEF (aperture, Aperture, float)


//
// fStop -- ratio of lens focal length to diameter
// 

IMF_STD_ATTRIBUTE_DEF (fStop, FStop, float)


//
// tStop -- ratio of lens focal length to diameter, adjusted for
// transmittance

IMF_STD_ATTRIBUTE_DEF (tStop, TStop, float)


//
// isoSpeed -- the ISO speed of the film or image sensor
// that was used to record the image
//

IMF_STD_ATTRIBUTE_DEF (isoSpeed, IsoSpeed, float)


//
// recorderMake -- manufacturer or vendor of the device that
// recorded the image
//

IMF_STD_ATTRIBUTE_DEF (recorderMake, RecorderMake, std::string)


//
// recorderModel -- model name or model number of the device
// that recorded the image
//

IMF_STD_ATTRIBUTE_DEF (recorderModel, RecorderModel, std::string)


//
// recorderSerialNumber -- the serial number of the device that
// recorded the image
//

IMF_STD_ATTRIBUTE_DEF (recorderSerialNumber, RecorderSerialNumber, std::string)


//
// recorderFirmwareVersion -- firmware version of the device that
// recorded the image
//

IMF_STD_ATTRIBUTE_DEF (recorderFirmwareVersion, RecorderFirmwareVersion, std::string)


//
// storageMediaSerialNumber -- serial number of the physical medium
// on which the camera output is recorded
//

IMF_STD_ATTRIBUTE_DEF (storageMediaSerialNumber, StorageMediaSerialNumber, std::string)


//
// reelName -- name for a sequence of unique images.
//

IMF_STD_ATTRIBUTE_DEF (reelName, ReelName, std::string)


//
// envmap -- if this attribute is present, the image represents
// an environment map.  The attribute's value defines how 3D
// directions are mapped to 2D pixel locations.  For details
// see header file ImfEnvmap.h
//

IMF_STD_ATTRIBUTE_DEF (envmap, Envmap, Envmap)


//
// uuid -- universally unique identifier (UUID) as specified in
// IETF RFC 4122, as a 128-bit-long string. The value of this 
// attribute should be such that, with a very high probability,
// no other image will ever be assigned the same identifier.

IMF_STD_ATTRIBUTE_DEF (uuid, Uuid, std::string)


//
// keyCode -- for motion picture film frames.  Identifies film
// manufacturer, film type, film roll and frame position within
// the roll.
//

IMF_STD_ATTRIBUTE_DEF (keyCode, KeyCode, KeyCode)


//
// timeCode -- time and control code
//

IMF_STD_ATTRIBUTE_DEF (timeCode, TimeCode, TimeCode)


//
// timecodeRate -- the timecode rate of the image sequence to which
// the image belongs, specified in timecodes per second.
//

IMF_STD_ATTRIBUTE_DEF (timecodeRate, TimecodeRate, int)


//
// imageCounter -- an image number; for a sequence of images, the
// image number shall be increasing when the images are accessed in
// the intended play order. Can be used to uniquely identify frames
// of high-speed photography that would have identical timecode.
//

IMF_STD_ATTRIBUTE_DEF (imageCounter, ImageCounter, int)


//
// wrapmodes -- determines how texture map images are extrapolated.
// If an OpenEXR file is used as a texture map for 3D rendering,
// texture coordinates (0.0, 0.0) and (1.0, 1.0) correspond to
// the upper left and lower right corners of the data window.
// If the image is mapped onto a surface with texture coordinates
// outside the zero-to-one range, then the image must be extrapolated.
// This attribute tells the renderer how to do this extrapolation.
// The attribute contains either a pair of comma-separated keywords,
// to specify separate extrapolation modes for the horizontal and
// vertical directions; or a single keyword, to specify extrapolation
// in both directions (e.g. "clamp,periodic" or "clamp").  Extra white
// space surrounding the keywords is allowed, but should be ignored
// by the renderer ("clamp, black " is equivalent to "clamp,black").
// The keywords listed below are predefined; some renderers may support
// additional extrapolation modes:
//
//	black		pixels outside the zero-to-one range are black
//
//	clamp		texture coordinates less than 0.0 and greater
//			than 1.0 are clamped to 0.0 and 1.0 respectively
//
//	periodic	the texture image repeats periodically
//
//	mirror		the texture image repeats periodically, but
//			every other instance is mirrored
//

IMF_STD_ATTRIBUTE_DEF (wrapmodes, Wrapmodes, std::string)


//
// framesPerSecond -- defines the nominal playback frame rate for image
// sequences, in frames per second.  Every image in a sequence should
// have a framesPerSecond attribute, and the attribute value should be
// the same for all images in the sequence.  If an image sequence has
// no framesPerSecond attribute, playback software should assume that
// the frame rate for the sequence is 24 frames per second.
//
// In order to allow exact representation of NTSC frame and field rates,
// framesPerSecond is stored as a rational number.  A rational number is
// a pair of integers, n and d, that represents the value n/d.
//
// For the exact values of commonly used frame rates, please see header
// file ImfFramesPerSecond.h.
//

IMF_STD_ATTRIBUTE_DEF (framesPerSecond, FramesPerSecond, Rational)


//
// multiView -- defines the view names for multi-view image files.
// A multi-view image contains two or more views of the same scene,
// as seen from different viewpoints, for example a left-eye and
// a right-eye view for stereo displays.  The multiView attribute
// lists the names of the views in an image, and a naming convention
// identifies the channels that belong to each view.
//
// For details, please see header file ImfMultiView.h
//

IMF_STD_ATTRIBUTE_DEF (multiView, MultiView, StringVector)


//
// framelines -- set of rectangles used for creative purposes, e.g. to
// indicate safe areas, cropping for derivative image sequences, etc.
// The way in which the format encodes these rectangles is not defined;
// the encoding might be camera-vendor-specific, or it might be formally
// or informally standardized (e.g. an ASC Frameline Decision List)
//

IMF_STD_ATTRIBUTE_DEF (framelines, Framelines, std::string)


//
// imageRotation -- rotation of the image in degrees, in the closed interval
// [-45, +45], with the rotation around the center of the display window, a
// positive value specifying a rotation from x-axis to y-axis.
//

IMF_STD_ATTRIBUTE_DEF (imageRotation, ImageRotation, float)


//
// cameraPosition -- x, y, z position of the camera, in meters. The attribute
// may record the center of the camera's sensor, or the center of the
// entrance pupil of the lens. The coordinate system for the camera's
// position is fixed in relation to the set, and is a right-handed Cartesian
// coordinate system with the z-axis pointing upwards and the y-axis pointing
// 90 degrees to the left of the x-axis direction.
//
// Recording the position of the center of the entrance pupil provides the
// most usable value in some circumstances, e.g. as the camera pans. This
// position changes, however, with lens and focus. For cases when this
// precision is not needed, recording the center of the sensor is provided
// as an alternative.
//
// For higher precision the chosen coordinate system is usually NOT the GPS
// system, but an on-set coordinate system. Any coordinate system can be 
// chosen for recording the camera's position, as long as its axes are
// orthogonal, and z points up. The coordinate system can be moving with
// the set, for example, when on a ship. Its reference point (usually
// assignged position 0,0,0) can be within or outside the set.

IMF_STD_ATTRIBUTE_DEF (cameraPosition, CameraPosition, IMATH_NAMESPACE::V3f)


//
// cameraUpDirection -- the 'up' direction of the camera. If present, must
// be accompanied by a cameraViewingDirection attribute. The 'up' direction
// shall be perpendicular to the direction specified in the
// cameraViewingDirection attribute. The coordinate system in which the
// 'up' direction is expressed is that defined for the cameraViewingDirection
// attribute.

IMF_STD_ATTRIBUTE_DEF (cameraUpDirection, CameraUpDirection, IMATH_NAMESPACE::V3f)


//
// cameraViewingDirection -- the viewing direction of the camera, i.e. the
// direction of the optical axis of the lens, as a right-handed Cartesian
// coordinate system with the z-axis pointing upwards and the y-axis
// pointing 90 degrees to the left of the x-axis direction.
// 
// If present, must be accompanied by a cameraUpDirection attribute.
//
// If the cameraPosition attribute is present, the axes of the
// cameraViewingDirection attribute are parallel to, and point in the same
// direction as, those of the cameraPosition attribute; if the cameraPosition
// attribute is not present then the directions of the axes in the coordinate
// shall be fixed in relation to the set.
//
// If the cameraViewingDirection attribute is not present, then the camera's
// viewing and up directions are undefined.

IMF_STD_ATTRIBUTE_DEF (cameraViewingDirection, CameraViewingDirection, IMATH_NAMESPACE::V3f)
// 
// worldToCamera -- for images generated by 3D computer graphics rendering,
// a matrix that transforms 3D points from the world to the camera coordinate
// space of the renderer.
// 
// The camera coordinate space is left-handed.  Its origin indicates the
// location of the camera.  The positive x and y axes correspond to the
// "right" and "up" directions in the rendered image.  The positive z
// axis indicates the camera's viewing direction.  (Objects in front of
// the camera have positive z coordinates.)
// 
// Camera coordinate space in OpenEXR is the same as in Pixar's Renderman.
// 

IMF_STD_ATTRIBUTE_DEF (worldToCamera, WorldToCamera, IMATH_NAMESPACE::M44f)


// 
// worldToNDC -- for images generated by 3D computer graphics rendering, a
// matrix that transforms 3D points from the world to the Normalized Device
// Coordinate (NDC) space of the renderer.
// 
// NDC is a 2D coordinate space that corresponds to the image plane, with
// positive x and pointing to the right and y positive pointing down.  The
// coordinates (0, 0) and (1, 1) correspond to the upper left and lower right
// corners of the OpenEXR display window.
// 
// To transform a 3D point in word space into a 2D point in NDC space,
// multiply the 3D point by the worldToNDC matrix and discard the z
// coordinate.
// 
// NDC space in OpenEXR is the same as in Pixar's Renderman.
// 

IMF_STD_ATTRIBUTE_DEF (worldToNDC, WorldToNDC, IMATH_NAMESPACE::M44f)


//
// deepImageState -- specifies whether the pixels in a deep image are
// sorted and non-overlapping.
//
// Note: this attribute can be set by application code that writes a file
// in order to tell applications that read the file whether the pixel data
// must be cleaned up prior to image processing operations such as flattening. 
// The OpenEXR library does not verify that the attribute is consistent with
// the actual state of the pixels.  Application software may assume that the
// attribute is valid, as long as the software will not crash or lock up if
// any pixels are inconsistent with the deepImageState attribute.
//

IMF_STD_ATTRIBUTE_DEF (deepImageState, DeepImageState, DeepImageState)


//
// originalDataWindow -- if application software crops an image, then it
// should save the data window of the original, un-cropped image in the
// originalDataWindow attribute.
//

IMF_STD_ATTRIBUTE_DEF
    (originalDataWindow, OriginalDataWindow, IMATH_NAMESPACE::Box2i)


//
// dwaCompressionLevel -- sets the quality level for images compressed
// with the DWAA or DWAB method.
//

IMF_STD_ATTRIBUTE_DEF (dwaCompressionLevel, DwaCompressionLevel, float)


#endif
