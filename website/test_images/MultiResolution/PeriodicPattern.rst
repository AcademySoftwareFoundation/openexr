..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

PeriodicPattern.exr
###################

:download:`https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/PeriodicPattern.exr<https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/PeriodicPattern.exr>`

.. image:: https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/PeriodicPattern.jpg
   :target: https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/PeriodicPattern.exr


A mip-map image that tiles seamlessly in "periodic" wrap mode.
The image can be used to check if 3D renderers correctly
implement this wrap mode.

.. list-table::
   :align: left

   * - screenWindowWidth
     - 1
   * - dataWindow
     - (0 0) - (516 516)
   * - owner
     - Copyright 2004 Industrial Light & Magic
   * - wrapmodes
     - periodic,periodic
   * - screenWindowCenter
     - (0 0)
   * - displayWindow
     - (0 0) - (516 516)
   * - tiles
     - mip-map, tile size 64 by 64 pixels, level sizes rounded down
   * - channels
     - B, G, R
   * - lineOrder
     - increasing y
   * - pixelAspectRatio
     - 1
   * - compression
     - zip, multi-scanline blocks
   * - type
     - tiledimage
