..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright (c) Contributors to the OpenEXR Project.

Standard Optional Attributes
############################

By default, OpenEXR files have the following attributes:

**chromaticities**
  For RGB images, specifies the CIE (x,y) chromaticities of the
  primaries and the white point.

**whiteLuminance**
  For RGB images, defines the luminance, in Nits (candelas per square
  meter) of the RGB value (1.0, 1.0, 1.0).

  If the chromaticities and the whiteLuminance of an RGB image are
  known, then it is possible to convert the image's pixels from RGB to
  CIE XYZ tristimulus values.

**adoptedNeutral**
  Specifies the CIE (x,y) coordinates that should be considered
  neutral during color rendering.  Pixels in the image file whose
  (x,y) coordinates match the adoptedNeutral value should be mapped to
  neutral values on the display.


**renderingTransform**, lookModTransform
  Specify the names of the CTL functions that implements the intended
  color rendering and look modification transforms for this image.

**xDensity**
  Horizontal output density, in pixels per inch.  The image's vertical
  output density is xDensity * pixelAspectRatio.

**owner**
  Name of the owner of the image.

**comments**
  Additional image information in human-readable form, for example a
  verbal description of the image.

**capDate**
  The date when the image was created or captured, in local time, and
  formatted as ``YYYY:MM:DD hh:mm:ss``, where ``YYYY`` is the year (4
  digits, e.g. 2003), ``MM`` is the month (2 digits, 01, 02, ... 12),
  ``DD`` is the day of the month (2 digits, 01, 02, ... 31), hh is the
  hour (2 digits, 00, 01, ... 23), mm is the minute, and ss is the
  second (2 digits, 00, 01, ... 59).

**utcOffset**
  Universal Coordinated Time (UTC), in seconds: UTC == local time +
  utcOffset

**longitude**, **latitude**, **altitude**
  For images of real objects, the location where the image was
  recorded.  Longitude and latitude are in degrees east of Greenwich
  and north of the equator.  Altitude is in meters above sea level.
  For example, Kathmandu, Nepal is at longitude 85.317, latitude
  27.717, altitude 1305.

**focus**
  The camera's focus distance, in meters.

**exposure**
  Exposure time, in seconds.

**aperture**
  The camera's lens aperture, in f-stops (focal length of the lens
  divided by the diameter of the iris opening).

**isoSpeed**
  The ISO speed of the film or image sensor that was used to record
  the image.

**envmap**
  If this attribute is present, the image represents an environment
  map.  The attribute's value defines how 3D directions are mapped to
  2D pixel locations.  

**keyCode**
  For motion picture film frames.  Identifies film manufacturer, film
  type, film roll and frame position within the roll.

**timeCode**
  Time and control code

**wrapmodes**
  Determines how texture map images are extrapolated.  If an OpenEXR
  file is used as a texture map for 3D rendering, texture coordinates
  (0.0, 0.0) and (1.0, 1.0) correspond to the upper left and lower
  right corners of the data window.  If the image is mapped onto a
  surface with texture coordinates outside the zero-to-one range, then
  the image must be extrapolated.  This attribute tells the renderer
  how to do this extrapolation.  The attribute contains either a pair
  of comma-separated keywords, to specify separate extrapolation modes
  for the horizontal and vertical directions; or a single keyword, to
  specify extrapolation in both directions (e.g. "clamp,periodic" or
  "clamp").  Extra white space surrounding the keywords is allowed,
  but should be ignored by the renderer ("clamp, black " is equivalent
  to "clamp,black").  The keywords listed below are predefined; some
  renderers may support additional extrapolation modes:

  **black**
    pixels outside the zero-to-one range are black

  **clamp**
    texture coordinates less than 0.0 and greater than 1.0 are clamped
    to 0.0 and 1.0 respectively.

  **periodic**
    the texture image repeats periodically

  **mirror**
    the texture image repeats periodically, but every other instance
    is mirrored

**framesPerSecond**
  Defines the nominal playback frame rate for image sequences, in
  frames per second.  Every image in a sequence should have a
  framesPerSecond attribute, and the attribute value should be the
  same for all images in the sequence.  If an image sequence has no
  framesPerSecond attribute, playback software should assume that the
  frame rate for the sequence is 24 frames per second.

  In order to allow exact representation of NTSC frame and field
  rates, framesPerSecond is stored as a rational number.  A rational
  number is a pair of integers, n and d, that represents the value
  n/d.

**multiView**
  Defines the view names for multi-view image files.  A multi-view
  image contains two or more views of the same scene, as seen from
  different viewpoints, for example a left-eye and a right-eye view
  for stereo displays.  The multiView attribute lists the names of the
  views in an image, and a naming convention identifies the channels
  that belong to each view.

**worldToCamera**
  For images generated by 3D computer graphics rendering, a matrix
  that transforms 3D points from the world to the camera coordinate
  space of the renderer.

  The camera coordinate space is left-handed.  Its origin indicates
  the location of the camera.  The positive x and y axes correspond to
  the "right" and "up" directions in the rendered image.  The positive
  z axis indicates the camera's viewing direction.  (Objects in front
  of the camera have positive z coordinates.)

  Camera coordinate space in OpenEXR is the same as in Pixar's
  Renderman.

**worldToNDC**
  For images generated by 3D computer graphics rendering, a matrix
  that transforms 3D points from the world to the Normalized Device
  Coordinate (NDC) space of the renderer.

  NDC is a 2D coordinate space that corresponds to the image plane,
  with positive x and pointing to the right and y positive pointing
  down.  The coordinates (0, 0) and (1, 1) correspond to the upper
  left and lower right corners of the OpenEXR display window.

  To transform a 3D point in word space into a 2D point in NDC space,
  multiply the 3D point by the worldToNDC matrix and discard the z
  coordinate.

  NDC space in OpenEXR is the same as in Pixar's Renderman.

**deepImageState**
  Specifies whether the pixels in a deep image are sorted and
  non-overlapping.

  Note: this attribute can be set by application code that writes a
  file in order to tell applications that read the file whether the
  pixel data must be cleaned up prior to image processing operations
  such as flattening.  The OpenEXR library does not verify that the
  attribute is consistent with the actual state of the pixels.
  Application software may assume that the attribute is valid, as long
  as the software will not crash or lock up if any pixels are
  inconsistent with the deepImageState attribute.

**originalDataWindow**
  If application software crops an image, then it should save the data
  window of the original, un-cropped image in the originalDataWindow
  attribute.

**dwaCompressionLevel**
  Sets the quality level for images compressed with the DWAA or DWAB
  method.

**ID Manifest**
  ID manifest.



