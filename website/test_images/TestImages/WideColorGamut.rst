..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

WideColorGamut.exr
##################

:download:`https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/WideColorGamut.exr<https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/WideColorGamut.exr>`

.. image:: https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/WideColorGamut.jpg
   :target: https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/WideColorGamut.exr


Some pixels in this RGB image have extremely saturated colors,
outside the gamut that can be displayed on a video monitor
whose primaries match Rec. ITU-R BT.709.  All RGB triples in
the image correspond to CIE xyY triples with xy chromaticities
that represent real colors.  (In a chromaticity diagram, the
pixels are all inside the spectral locus.)  However, for pixels
whose chromaticities are outside the triangle defined by the
chromaticities of the Rec. 709 primaries, at least one of the
RGB values is negative.

.. list-table::
   :align: left

   * - screenWindowWidth
     - 1
   * - dataWindow
     - (0 0) - (799 799)
   * - chromaticities
     - red  (0.64 0.33), green (0.3 0.6), blue (0.15 0.06), white (0.3127 0.329)
   * - displayWindow
     - (0 0) - (799 799)
   * - screenWindowCenter
     - (0 0)
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
