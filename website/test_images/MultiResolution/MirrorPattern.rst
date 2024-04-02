..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

MirrorPattern.exr
#################

:download:`https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/MirrorPattern.exr<https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/MirrorPattern.exr>`

.. image:: https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/MirrorPattern.jpg
   :target: https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/MirrorPattern.exr


Mip-map images that tile seamlessly in "mirror"
wrap mode.  The image can be used to check if 3D
renderers correctly implement this wrap modes.

.. list-table::
   :align: left

   * - screenWindowWidth
     - 1
   * - dataWindow
     - (0 0) - (511 511)
   * - owner
     - Copyright 2004 Industrial Light & Magic
   * - wrapmodes
     - mirror,mirror
   * - screenWindowCenter
     - (0 0)
   * - displayWindow
     - (0 0) - (511 511)
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
