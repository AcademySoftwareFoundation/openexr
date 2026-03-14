..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

BrightRings.exr
###############

:download:`https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/BrightRings.exr<https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/BrightRings.exr>`

.. image:: https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/BrightRings.jpg
   :target: https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/BrightRings.exr


This RGB image contains a number of rather bright rings, with
pixel values over 1000, on a gray background.  The image is
useful for testing how filtering and resampling algorithms
react to high- dynamic-range data.  (Some filters, for example,
convolution kernels with negative lobes, tend to produce
objectionable artifacts near high-contrast edges.)  Note that
the rings in the image are smooth, although on most displays
clamping of the pixel values introduces aliasing artifacts.  To
see that the rings really are smooth, view the image with
and exposure of -10.

.. list-table::
   :align: left

   * - screenWindowWidth
     - 1
   * - dataWindow
     - (0 0) - (799 799)
   * - screenWindowCenter
     - (0 0)
   * - displayWindow
     - (0 0) - (799 799)
   * - channels
     - B, G, R
   * - lineOrder
     - increasing y
   * - pixelAspectRatio
     - 1
   * - compression
     - zip, multi-scanline blocks
   * - type
     - scanlineimage
