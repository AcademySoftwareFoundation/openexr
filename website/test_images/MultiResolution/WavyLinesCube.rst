..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

WavyLinesCube.exr
#################

:download:`https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/WavyLinesCube.exr<https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/WavyLinesCube.exr>`

.. image:: https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/WavyLinesCube.jpg
   :target: https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/WavyLinesCube.exr


An environment map, in cube-face format, that can be used to
test how an application program, for example, a 3D renderer,
handles the seams of environment maps.
The environment image contains multiple sets of wavy lines.
Each set consists of three parallel lines of equal width.
Parts of the middle line run along one of the map's seams,
crossing back and forth over the seam.  In a cube-face map,
seams occur along the edges of the six faces of the cube. 
If the environment map is correctly projected onto a sphere,
then the seams should be invisible, and all lines should appear
to have the same uniform width everywhere.  It should be
impossible or at least difficult to tell where the middle line
in each set crosses one of the map's seams.

.. list-table::
   :align: left

   * - envmap
     - cube-face map
   * - screenWindowWidth
     - 1
   * - dataWindow
     - (0 0) - (255 1535)
   * - owner
     - Copyright 2005 Industrial Light & Magic
   * - screenWindowCenter
     - (0 0)
   * - displayWindow
     - (0 0) - (255 1535)
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
