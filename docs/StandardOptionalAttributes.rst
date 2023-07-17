..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Standard Optional Attributes
############################

.. toctree::
   :caption: Standard Optional Attributes

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

**sensorCenterOffset**
  Horizontal and vertical distances, in microns, of
  the center of the light-sensitive area of the camera's sensor from
  a point on that sensor where a sensor surface normal would intersect
  the center of the lens mount.

  When compared to an image captured with a perfectly centered sensor,
  an image where both horizontal and vertical distances were positive
  would contain more content holding what was at the right and what was
  at the bottom of the scene being captured.

**sensorOverallDimensions**
  Dimensions of the light-sensitive area of the sensor, in millimeters,
  independent of the subset of that region from which image data are
  obtained.

**sensorPhotositePitch**
  Ddistance between centers of sensor photosites, in microns.

**sensorAcquisitionRectangle**
  The rectangular area of the sensor containing photosites
  the contents of which are in one-to-one correspondence with
  the captured sensels, for a monochrome sensor, or with the
  reconstructed RGB pixels, for a sensor covered with color
  filter array material in a Bayer or a similar pattern.

  Because understanding the above formal definition is
  critical for many applications, including camera solvers,
  some short definitions:
  - a *photosite* is that optoelectronic component on the
    sensor which, when light hits it, accumulates or
    otherwise registers electric charge
  - a *sensel* is the read-out contents of a single
    photosite
  - *color filter array material* is material deposited
    on top of an array of photosites such that each
    photosite is discretely covered with a material that
    passes photons of certain wavelengths and that blocks
    photons of other wavelengths
  - an *RGB pixel* contains red, green and blue components
    indicating relative exposure values
  - *RGB pixel reconstruction* is the process of taking
    sensel data from a neighborhood of a particular
    photosite, including that photosite itself, in a
    photosite covered by either red, green or blue CFA
    material, and combining the read-out sensel data
    from a particular photosite with that of surrounding
    photosites, said surrounding photosites being covered
    by a variety of red, green or blue CFA materials, to
    produce an RGB pixel.

  The Wikipedia article on demosaicing_ covers the basics
  of these ideas quite well.

  In the case of sensels read from a monochrome sensor,
  the idea of a one-to-one relationship between sensels
  read from the photosite array and pixels in the data
  window can be straightforward. Often there is a
  conversion of the sensel data read from the photosites
  to a different representation (e.g. integer to float)
  along with scaling of the individual values.

  A common spatial scaling is from a 2880 x 1620 acquisition
  format to a 1920 x 1080 HD format. In this case, a camera
  solver will want to know the original set of photosites
  that contributed sensel values to the downscaler: their
  number, and their position. Through a combination of
  sensorAcquisitionRectangle, sensorPhotositePitch,
  sensorOverallDimensions and sensorCenterOffset,
  the application can know exactly the area on the sensor
  on which the light fell to create the sensel values that
  produced a monochrome image.

  RGB images are more complicated. RGB pixel reconstruction
  is a form of filtering, and kernels are square, with
  relatively small span, e.g. 5x5 or 7x7. Edge handling for the
  kernel is important; the Wikipedia article describing an
  image processing kernel_ covers it pretty well.

  Elements of the reconstruction kernel that are never
  at the center of the kernel are **not** counted as part
  of the sensorAcquisitionRectangle. Recalling the simple
  case above of a non-spatially-scaled 2880 x 1620 monochrome
  image being in 1:1 correspondence with an array of
  photosites on the sensor, if we are instead reading from a
  CFA-covered sensor to reconstruct a 2880 x 1620 RGB image,
  the actual array of all photosites whose sensel values
  were fed into a 5x5 reconstruction kernel would not be
  2880 x 1620, but 2884 x 1624. Nevertheless, the size of
  the sensorAcquisitionRectangle would be 2880 x 1620.

  Camera systems differ on how to handle the case where the
  position of the RGB reconstruction kernel is such that one
  or more elements of the kernel do not correspond to
  physical photosites; these are edge cases in every sense
  of the phrase.

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

.. _demosaicing: https://en.wikipedia.org/wiki/Demosaicing

.. _kernel: https://en.wikipedia.org/wiki/Kernel_(image_processing)