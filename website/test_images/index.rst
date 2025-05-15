Test Images
###########

.. _Test Images:

.. toctree::
   :caption: Test Images
   :maxdepth: 2

   toctree

..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Unusual Pixel Images
####################

A few OpenEXR files with unusual numbers in their pixels.  The files
can be used to test how application programs behave when they
encounter pixels with very small, very large or non-finite pixel
values.


.. raw:: html

   <embed>
   <table>

     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=TestImages/AllHalfValues.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/AllHalfValues.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> AllHalfValues.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The pixels in this RGB HALF image contain all 65,536 possible
             16-bit floating-point numbers, including positive and negative
             numbers, normalized and denormalized numbers, zero, NaNs,
             positive infinity and negative infinity.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=TestImages/BrightRings.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/BrightRings.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> BrightRings.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zip compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
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
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=TestImages/BrightRingsNanInf.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/BrightRingsNanInf.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> BrightRingsNanInf.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zip compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             This image is the same as <b>BrightRings.exr</b>, except for a few
             pixels near the center, which contain NaNs and infinities.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=TestImages/GammaChart.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/GammaChart.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> GammaChart.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> pxr24 compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=TestImages/GrayRampsDiagonal.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/GrayRampsDiagonal.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> GrayRampsDiagonal.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> pxr24 compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=TestImages/GrayRampsHorizontal.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/GrayRampsHorizontal.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> GrayRampsHorizontal.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> pxr24 compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=TestImages/RgbRampsDiagonal.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/RgbRampsDiagonal.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> RgbRampsDiagonal.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> pxr24 compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=TestImages/SquaresSwirls.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/SquaresSwirls.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> SquaresSwirls.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> pxr24 compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             This image contains colored squares and swirling patterns
             against a flat gray background.  Applying lossy compression
             algorithms to this image tends to highlight compression
             artifacts.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=TestImages/WideColorGamut.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/WideColorGamut.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> WideColorGamut.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zip compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             Some pixels in this RGB image have extremely saturated colors,
             outside the gamut that can be displayed on a video monitor
             whose primaries match Rec. ITU-R BT.709.  All RGB triples in
             the image correspond to CIE xyY triples with xy chromaticities
             that represent real colors.  (In a chromaticity diagram, the
             pixels are all inside the spectral locus.)  However, for pixels
             whose chromaticities are outside the triangle defined by the
             chromaticities of the Rec. 709 primaries, at least one of the
             RGB values is negative.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=TestImages/WideFloatRange.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/TestImages/WideFloatRange.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> WideFloatRange.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> pxr24 compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             This image contains only a single G channel of type FLOAT.  The
             values in the pixels span almost the entire range of possible
             32-bit floating-point numbers (roughly -1e38 to +1e38).
          </p>
          </td>
     </tr>

   </table>
   </embed>

..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Scanline Images
###############

Examples of scanline images.


.. raw:: html

   <embed>
   <table>

     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=ScanLines/Blobbies.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/ScanLines/Blobbies.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Blobbies.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zip compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                       </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=ScanLines/CandleGlass.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/ScanLines/CandleGlass.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> CandleGlass.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             Premultiplied alpha with A == 0 and RGB != 0
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=ScanLines/Cannon.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/ScanLines/Cannon.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Cannon.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> b44 compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                       </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=ScanLines/Carrots.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/ScanLines/Carrots.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Carrots.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zip compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             Stored with ACES2065-1 (AP0) chromaticities
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=ScanLines/Desk.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/ScanLines/Desk.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Desk.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                       </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=ScanLines/MtTamWest.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/ScanLines/MtTamWest.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> MtTamWest.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             View from Mount Tamalpais, Marin County, California, U.S.A.
                       </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=ScanLines/PrismsLenses.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/ScanLines/PrismsLenses.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> PrismsLenses.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             Premultiplied alpha with A == 0 and RGB != 0
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=ScanLines/StillLife.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/ScanLines/StillLife.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> StillLife.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                       </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=ScanLines/Tree.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/ScanLines/Tree.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Tree.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                                    </p>
          </td>
     </tr>

   </table>
   </embed>

..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Tiled Images
############

Examples of tiled images.


.. raw:: html

   <embed>
   <table>

     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Tiles/GoldenGate.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Tiles/GoldenGate.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> GoldenGate.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                          View from Hawk Hill towards San Francisco
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Tiles/Ocean.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Tiles/Ocean.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Ocean.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                                    </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Tiles/Spirals.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Tiles/Spirals.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Spirals.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> pxr24 compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                                    </p>
          </td>
     </tr>

   </table>
   </embed>

..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Chromaticities
##############

Four OpenEXR files that can be used to test if a program that displays
images handles the files' chromaticities attribute correctly.

Displayed properly, all four images should look the same.


.. raw:: html

   <embed>
   <table>

     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Chromaticities/Rec709.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Chromaticities/Rec709.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Rec709.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             R, G, B channels with chromaticities according to Rec. ITU-R BT.709-3
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Chromaticities/Rec709_YC.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Chromaticities/Rec709_YC.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Rec709_YC.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             Luminance/chroma encoded image, generated from R, G, B channels
             with chromaticities according to Rec. ITU-R BT.709-3
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Chromaticities/XYZ.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Chromaticities/XYZ.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> XYZ.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             R, G, B channels with chromaticities such that R, G and B
             correspond to CIE X, Y and Z
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Chromaticities/XYZ_YC.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Chromaticities/XYZ_YC.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> XYZ_YC.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             Luminance/chroma encoded image, generated from R, G, B channels
             with chromaticities such that R, G and B correspond to CIE X, Y
             and Z
          </p>
          </td>
     </tr>

   </table>
   </embed>

..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Luminance/Chroma Images
#######################

Examples of images represented as luminance and chroma rather than
RGB.


.. raw:: html

   <embed>
   <table>

     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=LuminanceChroma/CrissyField.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/LuminanceChroma/CrissyField.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> CrissyField.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> b44 compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The Golden Gate without the Bridge
                       </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=LuminanceChroma/Flowers.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/LuminanceChroma/Flowers.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Flowers.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> b44 compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                                    </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=LuminanceChroma/Garden.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/LuminanceChroma/Garden.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Garden.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                                    </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=LuminanceChroma/MtTamNorth.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/LuminanceChroma/MtTamNorth.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> MtTamNorth.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                                    </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=LuminanceChroma/StarField.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/LuminanceChroma/StarField.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> StarField.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                                                 </p>
          </td>
     </tr>

   </table>
   </embed>

..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Display Window
##############

A series of OpenEXR files that can be used to test if a program that
displays images on the screen properly places, crops and pads the
images.  All files contain the same set of 400 by 300 pixels, but the
data window, display window, and pixel aspect ratio differ between
files.


.. raw:: html

   <embed>
   <table>

     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t01.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t01.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t01.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window are the same.  All pixels
             in the data window should be visible in the display window.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t02.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t02.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t02.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window overlap, but they are
             not the same.  Portions of the data window that are outside the
             display window should not be displayed.  Portions of the
             display window that are outside the data window should be
             filled with a suitable background color.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t03.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t03.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t03.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window overlap, but they are
             not the same.  Portions of the data window that are outside the
             display window should not be displayed.  Portions of the
             display window that are outside the data window should be
             filled with a suitable background color.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t04.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t04.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t04.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window overlap, but they are
             not the same.  Portions of the data window that are outside the
             display window should not be displayed.  Portions of the
             display window that are outside the data window should be
             filled with a suitable background color.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t05.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t05.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t05.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window overlap, but they are
             not the same.  Portions of the data window that are outside the
             display window should not be displayed.  Portions of the
             display window that are outside the data window should be
             filled with a suitable background color.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t06.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t06.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t06.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window overlap, but they are
             not the same.  Portions of the data window that are outside the
             display window should not be displayed.  Portions of the
             display window that are outside the data window should be
             filled with a suitable background color.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t07.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t07.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t07.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window overlap, but they are
             not the same.  Portions of the data window that are outside the
             display window should not be displayed.  Portions of the
             display window that are outside the data window should be
             filled with a suitable background color.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t08.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t08.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t08.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window overlap, but they are
             not the same.  Portions of the data window that are outside the
             display window should not be displayed.  Portions of the
             display window that are outside the data window should be
             filled with a suitable background color.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t09.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t09.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t09.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window do not overlap.  The
             entire display window should be filled with the background
             color.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t10.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t10.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t10.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window do not overlap.  The
             entire display window should be filled with the background
             color.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t11.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t11.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t11.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window do not overlap.  The
             entire display window should be filled with the background
             color.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t12.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t12.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t12.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window do not overlap.  The
             entire display window should be filled with the background
             color.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t13.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t13.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t13.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window have only one pixel in
             common.  The data window's lower right pixel should be visible
             in the upper left corner of the display window.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t14.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t14.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t14.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window have only one pixel in
             common.  The data window's upper left pixel should be visible in
             the lower right corner of the display window.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t15.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t15.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t15.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window are the same as in
             </b>t07.exr</b>, but the pixels have an aspect ratio (width divided by
             height) of 1.5.  On a screen with square pixels, both the
             display window and the data window should be stretched
             horizontally.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=DisplayWindow/t16.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/DisplayWindow/t16.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> t16.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             The display window and the data window are the same as in
             </b>t07.exr</b>, but the pixels have an aspect ratio of 0.667.  On a
             screen with square pixels, both the display window and the
             data window should be stretched vertically.
          </p>
          </td>
     </tr>

   </table>
   </embed>

..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Beachball Example Image Sequence
################################

The example images in this folder are singlepart and multipart
versions of the same multichannel image sequence. The multipart
version is compatible only with OpenEXR-2.0 and later. These are
intended to exercise many features of the (regular scanline) InputPart
interface to the library and are recommended test cases for code which
reads them. Code should either read them all correctly, read some
parts of the file correctly, or else at the least report errors
gracefully.

When viewed in a stereo viewing environment, the images form a
sequence of a ball moving towards screen right. The ball should appear
to float in front of the screen, not behind it.

Note: *the content of channels other than RGBAZ in these examples is
arbitrary, and should not be considered a standard approach for
representing and naming conventions for data such as motion vectors,
stereo disparity or grading masks. They have simply been included here
as a realistic example of non-RGBA data.*

Note the following about these images:

* These images are stereo, multiview images, containing data for both
  left and right eyes.
  
  The single part file has a ``multiView`` attribute, the first part of
  the multipart file has a ``view`` attribute.

* The right eye is the **default** or **hero** eye in this case

  The first part in the multipart image has view attribute set to
  right; the single part image lists ``"right"`` as the first view in
  the ``multiView`` attribute.

* **View names are present** in the channel names of the **single part** file,
  except for the right view's RGBAZ views, which have no view names

* View names are not present in the multipart file channel names

* The **first part** of the multipart exr contains the **default channels** of
  the **default view**
  
  Software recompiled against EXR-2.0 which doesn't use the multipart
  API will only load the default channels of the default view

* **Layer names are present** in both the single and multipart file – they
  are not dropped, nor is the part name used to derive layer names.

* **No part in a multipart file can contain channels for multiple
  views**

  In a file with more than one part, the view attribute is used to
  identify the view for all channels in that part, and all channels
  belong to the specified view

* **Parts have consistent ``displayWindow`` attributes**

* **Parts do not have consistent dataWindow attributes**
  
  *The ability to specify different dataWindows for different channels,
  by dividing them into different parts, is one of the motivating
  factors for the EXR-2.0 multipart extension.*
  
  Code reading from different parts must be sure not to read from
  scanlines which are not present in the part's ``dataWindow``, as that
  will result in an exception.

  Reading a channel with a ``dataWindow`` smaller than the memory
  allocated could result in uninitialised memory.

  The first part's ``dataWindow`` is not guaranteed to enclose the
  dataWindow of other parts

* In frames 7 and 8, the **``dataWindow`` extends outside the ``displayWindow``**

* **Division of channels within one view between parts is arbitrary**

  In this case, generally each part contains a separate layer, though
  the RGBAZ layer has been split into across one part for RGBA and another for Z

  The decision of how to "package" channels into different parts is
  generally driven by read performance and filesize requirements. If
  only RGBA channels are commonly read, it would be best to store only
  those channels in a part, as in this case. For realtime playback of
  just RGB, it may be advantageous to store A separately to RGB. Many
  rarely read data channels which have similar data content and
  dataWindows would compress better if stored in the same part. Notice
  the scheme used here leads to files which are approx 20% larger than
  storing all channels in one part.

* The ``disparityR`` and ``disparityL`` channels are **not associated
  with a view**

  There is no view name in the channel names in the single part file,
  even though it has a layer name, indicating it is not view
  associated. The disparity parts in the multipart file do not have a
  view attribute.

  *Arguably, disparity data is associated with the source view - it is
  included here merely to illustrate that channels needn't be
  associated with a view.*

* The **depth** channel is called ``"Z"`` in all cases, in keeping with
  the convention for deep images.

  The convention is optional for regular scanline and tile images, but
  is is practical to maintain it for all image types.

  This means that depth is part of the RGBAZ layer in EXR
  parlance. Many software packages internally associate depth
  differently to RGBA. 

.. raw:: html

   <embed>
   <table>

     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/multipart.0001.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/multipart.0001.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> multipart.0001.exr </b>
            <ul>
                <li> 10 parts </li>
                <li> 118 channels </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/multipart.0002.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/multipart.0002.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> multipart.0002.exr </b>
            <ul>
                <li> 10 parts </li>
                <li> 118 channels </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/multipart.0003.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/multipart.0003.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> multipart.0003.exr </b>
            <ul>
                <li> 10 parts </li>
                <li> 118 channels </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/multipart.0004.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/multipart.0004.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> multipart.0004.exr </b>
            <ul>
                <li> 10 parts </li>
                <li> 118 channels </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/multipart.0005.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/multipart.0005.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> multipart.0005.exr </b>
            <ul>
                <li> 10 parts </li>
                <li> 118 channels </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/multipart.0006.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/multipart.0006.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> multipart.0006.exr </b>
            <ul>
                <li> 10 parts </li>
                <li> 118 channels </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/multipart.0007.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/multipart.0007.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> multipart.0007.exr </b>
            <ul>
                <li> 10 parts </li>
                <li> 118 channels </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/multipart.0008.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/multipart.0008.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> multipart.0008.exr </b>
            <ul>
                <li> 10 parts </li>
                <li> 118 channels </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/singlepart.0001.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/singlepart.0001.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> singlepart.0001.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/singlepart.0002.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/singlepart.0002.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> singlepart.0002.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/singlepart.0003.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/singlepart.0003.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> singlepart.0003.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/singlepart.0004.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/singlepart.0004.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> singlepart.0004.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/singlepart.0005.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/singlepart.0005.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> singlepart.0005.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/singlepart.0006.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/singlepart.0006.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> singlepart.0006.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/singlepart.0007.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/singlepart.0007.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> singlepart.0007.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=Beachball/singlepart.0008.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/Beachball/singlepart.0008.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> singlepart.0008.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>

   </table>
   </embed>

..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Multi-View Images
#################

Multi-view OpenEXR images with a number of formatting variations (scan
lines vs. tiles, image channels, etc.).  All images contain at least a
left-eye and a right-eye view suitable for presentation on a stereo
display.  The images have been prepared for viewing with a pixel
density of approximately 100 pixels per inch; the width of the
displayed images should be about 5 to 10 inches (12 to 25 cm).


.. raw:: html

   <embed>
   <table>

     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiView/Adjuster.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiView/Adjuster.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Adjuster.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> b44a compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             scan lines; R, G, B channels; 3 views; default view is center
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiView/Balls.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiView/Balls.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Balls.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             scan lines; R, G, B channels; 2 views; default view is left
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiView/Fog.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiView/Fog.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Fog.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             scan lines; Y luminance channel only; 2 views; default view is left
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiView/Impact.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiView/Impact.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Impact.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             tiled mip-map;  R, G, B channels; 2 views; default view is left
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiView/LosPadres.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiView/LosPadres.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> LosPadres.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             scan lines; R, G, B channels; 2 views; default view is right
          </p>
          </td>
     </tr>

   </table>
   </embed>

..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Multi-Resolution Images
#######################

Various multi-resolution OpenEXR images.

Regular Images
==============


.. raw:: html

   <embed>
   <table>

     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/Bonita.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/Bonita.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Bonita.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                          Point Bonita in the Marin Headlands, California (mip-map)
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/ColorCodedLevels.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/ColorCodedLevels.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> ColorCodedLevels.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> pxr24 compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                          A mip-map checkerboard image where each resolution level has a
             different color.  If this image is used to texture an object
             during 3D rendering, then the color of the object shows which
             resolution levels are accessed by the renderer as it projects
             the texture onto the object.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/Kapaa.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/Kapaa.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Kapaa.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             Near Kapa'a, Kaua'i, Hawai'i (rip-map)
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/KernerEnvCube.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/KernerEnvCube.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> KernerEnvCube.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
                <li> cube-face map </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             
             Parking lot on Kerner Blvd., San Rafael, California (mip-map, in
             cube-face format)
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/KernerEnvLatLong.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/KernerEnvLatLong.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> KernerEnvLatLong.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
                <li> latitude-longitude map </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             Parking lot on Kerner Blvd., San Rafael, California (mip-map, in
             latitude-longitude format)
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/MirrorPattern.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/MirrorPattern.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> MirrorPattern.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                          Mip-map images that tile seamlessly in "mirror"
             wrap mode.  The image can be used to check if 3D
             renderers correctly implement this wrap modes.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/OrientationCube.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/OrientationCube.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> OrientationCube.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
                <li> cube-face map </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             An environment map, in cube-face format, that indicates the
             directions of the environment's x, y and z axes.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/OrientationLatLong.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/OrientationLatLong.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> OrientationLatLong.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
                <li> latitude-longitude map </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                          An environment map, in latitude-longitude format, that
             indicates the directions of the environment's x, y and z axes.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/PeriodicPattern.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/PeriodicPattern.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> PeriodicPattern.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             A mip-map image that tiles seamlessly in "periodic" wrap mode.
             The image can be used to check if 3D renderers correctly
             implement this wrap mode.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/StageEnvCube.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/StageEnvCube.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> StageEnvCube.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
                <li> cube-face map </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             Stage with props, cameras and other equipment (mip-map, in
             cube-face format)
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/StageEnvLatLong.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/StageEnvLatLong.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> StageEnvLatLong.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
                <li> latitude-longitude map </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             Stage with props, cameras and other equipment (mip-map, in
             latitude-longitude format)
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/WavyLinesCube.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/WavyLinesCube.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> WavyLinesCube.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
                <li> cube-face map </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
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
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/WavyLinesLatLong.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/WavyLinesLatLong.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> WavyLinesLatLong.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> tiledimage </li>
                <li> zip compression </li>
                <li> latitude-longitude map </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
                          An environment map, in latitude-longitude format, that can be
             used to test how an application program, for example, a 3D
             renderer, handles the seams of environment maps.
             The environment image contains multiple sets of wavy lines.
             Each set consists of three parallel lines of equal width.
             Parts of the middle line run along one of the map's seams,
             crossing back and forth over the seam.  In a latitude-longitude
             map, there is a seam along the meridian with longitude +/-pi.
             If the environment map is correctly projected onto a sphere,
             then the seams should be invisible, and all lines should appear
             to have the same uniform width everywhere.  It should be
             impossible or at least difficult to tell where the middle line
             in each set crosses one of the map's seams.
          </p>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=MultiResolution/WavyLinesSphere.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/MultiResolution/WavyLinesSphere.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> WavyLinesSphere.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> piz compression </li>
            </ul>
          </td>
          <td style="vertical-align: top; width:400px">
          <p>
             This image shows what the <b>WavyLinesCube.exr</b> and
             </b>WavyLinesLatLong.exr</b> environment map should look like when
             has been correctly projected onto a sphere.  In this image the
             environment sphere is seen from the outside, not from the
             inside.
          </p>
          </td>
     </tr>

   </table>
   </embed>

..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Stereo Images
#############

Left and right views of each image, at 1920x1080 (Full HD)
resolution. This folder also contains a composited, flattened, image,
with separate views and depth channel, as a regular "scanlineimage"
EXR.  The composited image has four separate parts.



.. raw:: html

   <embed>
   <table>

     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/Stereo/Balls.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/Stereo/Balls.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Balls.exr </b>
            <ul>
                <li> 2 parts </li>
                <li> 28 channels </li>
                <li> deepscanline </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/Stereo/Ground.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/Stereo/Ground.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Ground.exr </b>
            <ul>
                <li> 2 parts </li>
                <li> 28 channels </li>
                <li> deepscanline </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/Stereo/Leaves.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/Stereo/Leaves.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Leaves.exr </b>
            <ul>
                <li> 2 parts </li>
                <li> 28 channels </li>
                <li> deepscanline </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/Stereo/Trunks.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/Stereo/Trunks.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Trunks.exr </b>
            <ul>
                <li> 2 parts </li>
                <li> 28 channels </li>
                <li> deepscanline </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/Stereo/composited.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/Stereo/composited.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> composited.exr </b>
            <ul>
                <li> 4 parts </li>
                <li> 53 channels </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>

   </table>
   </embed>

..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Stereo Left View Images
#######################

These images are the left view only of the above stereo images.




.. raw:: html

   <embed>
   <table>

     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/LeftView/Balls.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/LeftView/Balls.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Balls.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> deepscanline </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/LeftView/Ground.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/LeftView/Ground.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Ground.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> deepscanline </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/LeftView/Leaves.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/LeftView/Leaves.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Leaves.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> deepscanline </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/LeftView/Trunks.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/LeftView/Trunks.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Trunks.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> deepscanline </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>

   </table>
   </embed>

..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Stereo Left View Images, Low Resolution
#######################################

These images are the left view only of the above stereo images,
downsampled by decimation to 1024x576 (there is a visible shift in the
image compared to the 1920x1080 images) This folder also contains a
composited, flattened image, with no depth channel, as a regular
"scanlineimage" EXR.



.. raw:: html

   <embed>
   <table>

     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/LowResLeftView/Balls.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/LowResLeftView/Balls.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Balls.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> deepscanline </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/LowResLeftView/Ground.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/LowResLeftView/Ground.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Ground.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> deepscanline </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/LowResLeftView/Leaves.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/LowResLeftView/Leaves.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Leaves.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> deepscanline </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/LowResLeftView/Trunks.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/LowResLeftView/Trunks.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> Trunks.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> deepscanline </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>
     <tr>
          <td style="vertical-align: top; width:250px">
              <a href=v2/LowResLeftView/composited.html> <img width="250" src="https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main/v2/LowResLeftView/composited.jpg"> </a>
          </td>
          <td style="vertical-align: top; width:250px">
            <b> composited.exr </b>
            <ul>
                <li> single part </li>
                <li> 1 channel </li>
                <li> scanlineimage </li>
                <li> zips compression </li>
            </ul>
          </td>
     </tr>

   </table>
   </embed>

