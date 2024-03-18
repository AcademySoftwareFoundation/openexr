..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

ColorCodedLevels.exr
####################

:download:`https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/ColorCodedLevels.exr<https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/ColorCodedLevels.exr>`

.. image:: https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/ColorCodedLevels.jpg
   :target: https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/ColorCodedLevels.exr


A mip-map checkerboard image where each resolution level has a
different color.  If this image is used to texture an object
during 3D rendering, then the color of the object shows which
resolution levels are accessed by the renderer as it projects
the texture onto the object.

.. list-table::
   :align: left

   * - screenWindowWidth
     - 1
   * - dataWindow
     - (0 0) - (511 511)
   * - owner
     - Copyright 2005 Industrial Light & Magic
   * - wrapmodes
     - periodic,periodic
   * - comments
     - a mip-map image with color-coded levels
   * - screenWindowCenter
     - (0 0)
   * - displayWindow
     - (0 0) - (511 511)
   * - tiles
     - mip-map, tile size 64 by 64 pixels, level sizes rounded down
   * - channels
     - A, B, G, R
   * - lineOrder
     - increasing y
   * - pixelAspectRatio
     - 1
   * - compression
     - pxr24
   * - type
     - tiledimage
