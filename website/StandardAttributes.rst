..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _Standard Attributes:

Standard Attributes
###################

.. toctree::
   :caption: Standard Attributes

OpenEXR files store metadata in attributes. The attributes marked below as
"required" are present in every ``.exr`` file and specify values essential to
every image.

The optional attributes store extra information in a conventional form. These
tables give the presumed definitions of the most common data associated with
``.exr`` image files.

Basic Attributes
================

.. raw:: html

   <embed>
   <table>
   
   <tr>
     <td style="vertical-align: top; width:150px; font-weight:bold"> attribute name</td>
     <td style="vertical-align: top; width:100px; font-weight:bold"> type </td>
     <td style="vertical-align: top; width:500px; font-weight:bold"> definition </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> displayWindow </b> </tt> </p>
       <p> <i>required</i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> Box2i </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         The boundaries of an OpenEXR image in pixel space.  The <i>display window</i> is defined
         by the coordinates of the pixels in the upper left and lower right corners. See <a
         href="TechnicalIntroduction.html#overview-of-the-openexr-file-format"> Overview of the
         OpenEXR File Format</a> for more details.
       </p>
     </td>
   </tr>
   <tr style="border-width:3px">
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> dataWindow </b> </tt> </p>
       <p> <i>required</i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> Box2i </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         An OpenEXR file may not have pixel data for all the pixels in the display window, or the
         file may have pixel data beyond the boundaries of the display window. The region for which
         pixel data are available is defined by a second axis-parallel rectangle in pixel space, the
         <i>data window.</i> See <a
         href="TechnicalIntroduction.html#overview-of-the-openexr-file-format"> Overview of the
         OpenEXR File Format</a> for more details.
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> originalDataWindow </b> </tt> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> Box2i </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         If application software crops an image, then it should save the
         data window of the original, un-cropped image in the
         <tt> originalDataWindow </tt> attribute. </tt>
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px"> <tt>
       <p> <b> pixelAspectRatio </b> </tt> <p>
       <p> <i>required</i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Width divided by height of a pixel when the image is displayed 
         with the correct aspect ratio. A pixel's width (height) is the
         distance between the centers of two horizontally (vertically)
         adjacent pixels on the display.
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> screenWindowCenter </b> </tt> </p>
       <p> <i>required</i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> V2f </tt> </td>
     <td style="vertical-align: top; width:500px" rowspan=2>
       <p style="padding-bottom:15px">
         The <tt>screenWindowCenter</tt> and <tt>screenWindowWidth</tt>
         describe the perspective projection that produced the image.
         Programs that deal with images as purely two-dimensional objects may
         not be able so generate a description of a perspective
         projection. Those programs should set <tt>screenWindowWidth</tt> to
         1, and <tt>screenWindowCenter</tt> to (0, 0).
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> screenWindowWidth </b> </tt> </p>
       <p> <i>required</i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> lineOrder </b> </tt> </p>
       <p> <i>required</i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> LineOrder </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px"> Specifies in what order the scan lines in the file are stored in the file:
         <ul>
           <li> <tt> INCREASING_Y </tt> - first scan line has lowest y coordinate </li>
           <li> <tt> DECREASING_Y </tt> - first scan line has highest y coordinate </li>
           <li> <tt> RANDOM_Y </tt> - only for tiled files; tiles are written in random order </li>
         </ul>
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> compression </b> </tt> </p>
       <p> <i>required</i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> Compression </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px"> Specifies the compression method applied to the pixel data of all
           channels in the file.
         <ul>
           <li> <tt> NO_COMPRESSION </tt> - no compression </li>
           <li> <tt> RLE_COMPRESSION </tt> - run length encoding </li>
           <li> <tt> ZIPS_COMPRESSION </tt> - zlib compression, one scan line at a time </li>
           <li> <tt> ZIP_COMPRESSION </tt> - zlib compression, in blocks of 16 scan lines </li>
           <li> <tt> PIZ_COMPRESSION </tt> - piz-based wavelet compression </li>
           <li> <tt> PXR24_COMPRESSION </tt> - lossy 24-bit float compression </li>
           <li> <tt> B44_COMPRESSION </tt> - lossy 4-by-4 pixel block compression, fixed compression rate </li>
           <li> <tt> B44A_COMPRESSION </tt> - lossy 4-by-4 pixel block compression, flat fields are compressed more </li>
           <li> <tt> DWAA_COMPRESSION </tt> - lossy DCT based compression, in blocks of 32 scanlines. More efficient for partial buffer access. </li>
           <li> <tt> DWAB_COMPRESSION </tt> - lossy DCT based compression, in blocks of 256 scanlines. More efficient space wise and faster to decode full frames than <tt>DWAA_COMPRESSION</tt>. </li>
         </ul>
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> channels </b> </tt> <p>
       <p> <i>required</i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> ChannelList </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         A description of the image channels stored in the file.
       </p>
     </td>
   </tr>
   
   </table>
   </embed>

Multi-Part and Deep Data
========================

.. raw:: html

   <embed>
   <table>
   
   <tr>
     <td style="vertical-align: top; width:150px; font-weight:bold"> attribute name</td>
     <td style="vertical-align: top; width:100px; font-weight:bold"> type </td>
     <td style="vertical-align: top; width:500px; font-weight:bold"> definition </td>
   </tr>
   <tr style="border-width:3px">
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> name </b> </tt> </p>
       <p> <i> required for multi-part images </i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         The <tt>name</tt> attribute defines the name of each part. The name of each
         part must be unique. Names may contain <tt>'*.*'</tt> characters to present a
         tree-like structure of the parts in a file.
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> type </b> </tt> </p>
       <p> <i> required for multi-part images </i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Data types are defined by the type attribute. There are four
         types:
         <ul>
           <li> Scan line images: indicated by a value of <tt>scanlineimage</tt> </li>
           <li> Tiled images: indicated by a value of <tt>tiledimage</tt> </li>
           <li> Deep scan line images:  indicated by a value of <tt>deepscanline</tt> </li>
           <li> Deep tiled images:  indicated by a value of <tt>deeptile</tt> </li>      
         </ul>
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> version </b> </tt> </p>
       <p> <i> required for multi-part images </i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> int </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Version 1 data for all part types is described in the section on the 
              <a href="OpenEXRFileLayout.html" class="reference internal ">OpenEXR File Layout</a>.
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> chunkCount </b> </tt> </p>
       <p> <i> required for multi-part images </i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> int </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
        Indicates the number of chunks in this part.  Required if the
        multipart bit (12) is set. This attribute is created
        automatically by the library; the user does not need to
        compute it.
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> <b> tiles </b> </tt> </p>
       <p> <i> required for multi-part images </i> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> TileDescription </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         This attribute is required only for tiled files. It specifies
         the size of the tiles, and the file's level mode. 
       </p>
     </td>
   </tr>

   </table>
   </embed>

Position and Orientation
========================

These attributes describe the position and orientation of the physical or CG
camera at the time of capture. All are optional.

.. raw:: html

   <embed>
   <table>
   
   <tr>
     <td style="vertical-align: top; width:150px; font-weight:bold"> attribute name</td>
     <td style="vertical-align: top; width:100px; font-weight:bold"> type </td>
     <td style="vertical-align: top; width:500px; font-weight:bold"> definition </td>
   </tr>

   <tr style="border-width:3px">
     <td style="vertical-align: top; width:150px"> <tt> <b> worldToCamera </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> M44f </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         For images generated by 3D computer graphics rendering, a matrix
         that transforms 3D points from the world to the camera coordinate
         space of the renderer.
       </p>
       <p style="padding-bottom:15px">
         The camera coordinate space is left-handed.  Its origin indicates the
         location of the camera.  The positive x and y axes correspond to the
         "right" and "up" directions in the rendered image.  The positive z axis
         indicates the camera's viewing direction.  (Objects in front of the
         camera have positive z coordinates.)
       </p>
       <p style="padding-bottom:15px">
         Camera coordinate space in OpenEXR is the same as in Pixar's Renderman.
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> worldToNDC </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> M44f </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         For images generated by 3D computer graphics rendering, a matrix
         that transforms 3D points from the world to the Normalized Device
         Coordinate (NDC) space of the renderer.
       </p>
       <p style="padding-bottom:15px">
         NDC is a 2D coordinate space that corresponds to the image plane,
         with positive x and pointing to the right and y positive pointing
         down.  The coordinates (0, 0) and (1, 1) correspond to the upper
         left and lower right corners of the OpenEXR display window.
       </p>
       <p style="padding-bottom:15px">
         To transform a 3D point in word space into a 2D point in NDC space,
         multiply the 3D point by the worldToNDC matrix and discard the z
         coordinate.
       </p>
       <p style="padding-bottom:15px">
         NDC space in OpenEXR is the same as in Pixar's Renderman.
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> sensorCenterOffset
     </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> V2f </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Horizontal and vertical distances, in microns, of
         the center of the light-sensitive area of the camera's sensor from
         a point on that sensor where a sensor surface normal would intersect
         the center of the lens mount.
       </p>
       <p style="padding-bottom:15px">
         When compared to an image captured with a perfectly centered sensor,
         an image where both horizontal and vertical distances were positive
         would contain more content holding what was at the right and what was
         at the bottom of the scene being captured.
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b>
     sensorOverallDimensions </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> V2f </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Dimensions of the light-sensitive area of the sensor, in millimeters,
         independent of the subset of that region from which image data are
         obtained.
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> sensorPhotositePitch
     </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Distance between centers of sensor photosites, in microns.
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b>
     sensorAcquisitionRectangle </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> Box2i </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         The rectangular area of the sensor containing photosites
         the contents of which are in one-to-one correspondence with
         the captured sensels, for a monochrome sensor, or with the
         reconstructed RGB pixels, for a sensor covered with color
         filter array material in a Bayer or a similar pattern.
       </p>
       <p style="padding-bottom:15px">
         Because understanding the above formal definition is
         critical for many applications, including camera solvers,
         some short definitions:
       </p>
       <ul>
         <li style="padding-bottom:10px">
           a <i>photosite</i> is that optoelectronic component on the
           sensor which, when light hits it, accumulates or
           otherwise registers electric charge
         </li>
         <li style="padding-bottom:10px">
           a <i>sensel</i> is the read-out contents of a single
           photosite
         </li>
         <li style="padding-bottom:10px">
           <i>color filter array material</i> is material deposited
           on top of an array of photosites such that each
           photosite is discretely covered with a material that
           passes photons of certain wavelengths and that blocks
           photons of other wavelengths
         </li>
         <li style="padding-bottom:10px">
           an <i>RGB pixel</i> contains red, green and blue components
           indicating relative exposure values
         </li>
         <li style="padding-bottom:10px">
           <i>RGB pixel reconstruction</i> is the process of taking sensel data from a neighborhood
           of a particular photosite, including that photosite itself, in a photosite covered by
           either red, green or blue CFA material, and combining the read-out sensel data from a
           particular photosite with that of surrounding photosites, said surrounding photosites
           being covered by a variety of red, green or blue CFA materials, to produce an RGB pixel.
       </li>
       </ul>
       <p style="padding-bottom:15px">
         The Wikipedia article on <a
         href=https://en.wikipedia.org/wiki/Demosaicing> demosaicing </a> covers the basics
         of these ideas quite well.
       </p>
       <p style="padding-bottom:15px">
         In the case of sensels read from a monochrome sensor, the idea of a
         one-to-one relationship between sensels read from the photosite array
         and pixels in the data window can be straightforward. Often there is a
         conversion of the sensel data read from the photosites to a different
         representation (e.g. integer to float) along with scaling of the
         individual values.
       </p>
       <p style="padding-bottom:15px">
         A common spatial scaling is from a 2880 x 1620 acquisition format to a
         1920 x 1080 HD format. In this case, a camera solver will want to know
         the original set of photosites that contributed sensel values to the
         downscaler: their number, and their position. Through a combination of
         <tt>sensorAcquisitionRectangle</tt>, <tt>sensorPhotositePitch</tt>,
         <tt>sensorOverallDimensions</tt> and <tt>sensorCenterOffset</tt>, the application can
         know exactly the area on the sensor on which the light fell to create
         the sensel values that produced a monochrome image.
       </p>
       <p style="padding-bottom:15px">
         RGB images are more complicated. RGB pixel reconstruction is a form of
         filtering, and kernels are square, with relatively small span, e.g. 5x5
         or 7x7. Edge handling for the kernel is important; the Wikipedia
         article describing an image processing <a
         href=https://en.wikipedia.org/wiki/Kernel_(image_processing)> kernel </a> covers it well.
       </p>
       <p style="padding-bottom:15px">
         Elements of the reconstruction kernel that are never at the center of
         the kernel are <i>not</i> counted as part of the
         sensorAcquisitionRectangle. Recalling the simple case above of a
         non-spatially-scaled 2880 x 1620 monochrome image being in 1:1
         correspondence with an array of photosites on the sensor, if we are
         instead reading from a CFA-covered sensor to reconstruct a 2880 x 1620
         RGB image, the actual array of all photosites whose sensel values were
         fed into a 5x5 reconstruction kernel would not be 2880 x 1620, but 2884
         x 1624. Nevertheless, the size of the <tt>sensorAcquisitionRectangle</tt> would
         be 2880 x 1620.
       </p>
       <p style="padding-bottom:15px">
         Camera systems differ on how to handle the case where the position of
         the RGB reconstruction kernel is such that one or more elements of the
         kernel do not correspond to physical photosites; these are edge cases
         in every sense of the phrase.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> xDensity </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Horizontal output density, in pixels per inch.  The image's vertical
         output density is <tt> xDensity * pixelAspectRatio</tt>.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> longitude </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px" rowspan=3>
       <p style="padding-bottom:15px">
         For images of real objects, the location where the image was recorded.
         Longitude and latitude are in degrees east of Greenwich and north of the
         equator.  Altitude is in meters above sea level.  For example, Kathmandu,
         Nepal is at longitude 85.317, latitude 27.717, altitude 1305.
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> latitude </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> altitude </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
   </td>

   </table>
   </embed>
     

Camera ID
=========

These attributes identify the camera. All are optional.

.. raw:: html

   <embed>
   <table>
   
   <tr>
     <td style="vertical-align: top; width:150px; font-weight:bold"> attribute name</td>
     <td style="vertical-align: top; width:100px; font-weight:bold"> type </td>
     <td style="vertical-align: top; width:500px; font-weight:bold"> definition </td>
   </tr>

   <tr style="border-width:3px">
     <td style="vertical-align: top; width:150px"> <tt> <b> cameraMake </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Manufacturer or vendor of the camera. If present, the value should
         be UTF-8-encoded and have a nonzero length.
      </p>
     </td>
   </tr>
   
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> cameraModel </b>
     </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Model name or model number of the camera.  If present, the value
         should be UTF-8-encoded and have a nonzero length.
      </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> cameraSerialNumber
     </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Serial number of the camera If present, the value should be
         UTF-8-encoded and have a nonzero length.  Note that despite the
         name, the value can include non-digits as well as digits.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> cameraFirmwareVersion
     </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         The firmware version of the camera. If present, the value should be
         UTF-8-encoded and have a nonzero length.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> cameraUuid </b>
     </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Identifies this camera uniquely among all cameras from all vendors.
       </p>
       <p style="padding-bottom:15px">
         Uniqueness could be accomplished with, e.g., a MAC address, a
         concatenation of <tt>cameraMake</tt>, <tt>cameraModel</tt>,
         <tt>cameraSerialNumber</tt>, etc. The string may have
         arbitrary format; it doesn't need to follow the UUID 128-bit
         string format, even though that is implied by the name.
       </p>
       <p style="padding-bottom:15px">
         If present, the value should be UTF-8-encoded and have a nonzero
         length.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> cameraLabel </b>
     </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Text label identifying how the camera was used or assigned,
         e.g. <tt>"Camera 1 Left"</tt>, <tt>"B Camera"</tt>, <TT>"POV"</TT>, etc
       </p>
       <p style="padding-bottom:15px">
         If present, the value should be UTF-8-encoded and have a nonzero length.
       </p>
     </td>
   </tr>

   </table>
   </embed>

Camera State
============

These attributes describe the camera settings. All are optional.

.. raw:: html

   <embed>
   <table>
   
   <tr>
     <td style="vertical-align: top; width:150px; font-weight:bold"> attribute name</td>
     <td style="vertical-align: top; width:100px; font-weight:bold"> type </td>
     <td style="vertical-align: top; width:500px; font-weight:bold"> definition </td>
   </tr>

   <tr style="border-width:3px">
     <td style="vertical-align: top; width:150px"> <tt> <b> cameraCCTSetting </b>
     </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Color temperature, in Kelvin, configured for the physical or virtual
         camera creating or capturing the image.
       </p>
       <p style="padding-bottom:15px">
         The <tt>cameraCCTSetting</tt> is primarily forensic, and indicates the stated
         color balance of a film stock, the color temperature setting on a
         physical digital camera or the nominal color temperature of the
         scene adopted white as passed to a virtual camera's API.
       </p>
       <p style="padding-bottom:15px">
         A professional digital cinema cameras is not constrained to map
         every supplied correlated color temperature to a point on the curve
         of a Planckian radiator, or map every supplied color temperature to
         a chromaticity corresponding to a combination of the three principal
         components forming a basis for the CIE D series of illuminants.
       </p>
       <p style="padding-bottom:15px">
         Often, lower color temperatures are on the Planckian locus, higher
         color temperatures are on a locus of CIE D series chromaticities,
         and the camera performs a crossfade (typically a linear crossfade)
         between the two for intermediate temperatures. That the start and
         end of the crossfade could differ for every camera vendor -- or even
         across cameras offered by the same vendor -- means that no universal
         algorithm can map a camera color temperature setting (combined with
         a tint setting, see below) into a scene adopted white chromaticity.
       </p>
       <p style="padding-bottom:15px">
         The most common use for the <tt>cameraCCTSetting</tt> attribute is to feed
         its value into a camera-vendor-provided application or API, along
         with a <tt>cameraTintSetting</tt> attribute value, to reproduce the color
         processing done in-camera on set.
       </p>
       <p style="padding-bottom:15px">
         If a <tt>cameraCCTSetting</tt> attribute is provided, and no
         <tt>cameraTintSetting</tt> is provided, then a value of zero should be passed
         to any application or API using the <tt>cameraCCTSetting</tt> and
         <tt>cameraTintSetting</tt>.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> cameraTintSetting
     </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Green/magenta tint configured for the physical or virtual camera
         creating or capturing the image.
       </p>
       <p style="padding-bottom:15px">
         The <tt>cameraTintSetting</tt> is primarily forensic. There is no vendor-
         independent mapping from a unit of tint to a distance on a chromaticity
         diagram. One camera vendor might choose a color space (e.g. the
         CIE 1960 UCS) and have a unit amount of tint represent some delta uv
         distance from the point by the <tt>cammeraCCTSetting</tt> and a tint value of 0.
         Another might choose to express the effect of tint by analogy to a
         traditional unit from a film workflow, e.g. a Kodak or Rosco color
         correction filter. About the only guaranteed commonality is that all
         camera vendor tint schemes have positive values shift the adopted
         scene white towards green, and negative values toward magenta.
       </p>
       <p style="padding-bottom:15px">
         If the camera vendor maps <tt>cameraCCTSetting</tt> to a point defined by
         a linear crossfade between a Planckian blackbody locus and loci of
         CIE D Series illuminants, the slope of the tint isotherm at the
         exact points where the linear crossfade starts and ends can be 
         indeterminate and an inverse mapping from chromaticity to a pair
         of CCT and tint can be one-to-many.
       </p>
       <p style="padding-bottom:15px">
         The most common use for the <tt>cameraTintSetting</tt> attribute is to feed its
         value into a camera-vendor-provided application or API, along with
         a <tt>cameraCCTSetting</tt> attribute value, to reproduce the color processing
         done in-camera on set.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> cameraColorBalance
     </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> V2f </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Chromaticity in CIE 1960 UCS coordinates indicating a color the user
         of the camera would like the camera to treat as neutral, and
         corresponding to a particular camera configuration of make, model,
         camera firmware version, CCT setting and tint setting.
       </p>
       <p style="padding-bottom:15px">         
         Note that this is not necessarily (or even probably) the same
         chromaticity as that of the scene adopted white stored in an
         adoptedNeutral attribute (if present).
       </p>
       <p style="padding-bottom:15px">         
         For example, if a physical digital cinema camera was configured with
         a CCT of 3200K and a tint of -3 (in some camera vendor dependent
         unit), and the camera output had been processed such that the image
         containing this attribute was encoded as per SMPTE ST 2065-4:2023,
         then the <tt>adoptedNeutral</tt> attribute would have the value corresponding
         to the ACES neutral chromaticity, very near that of CIE Illuminant
         D60, whereas the <tt>cameraColorBalance</tt> would have a chromaticity much,
         much warmer than that of the <tt>adoptedNeutral</tt> attribute.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> isoSpeed </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         The ISO speed of the film or the ISO setting of the camera that was
         used to record the image.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> expTime </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Exposure time, in seconds
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> shutterAngle </b>
     </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Shutter angle, in degrees
       </p>
       <p style="padding-bottom:15px">         
         For a physical film or digital camera, changing the shutter angle
         inexorably affects both motion blur and exposure. For a CG camera,
         the parameters to the renderer control whether or not changing the
         shutter angle affects simulation of either or both of these phenomena.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> captureRate </b>
     </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> Rational </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Capture rate, in frames per second, of the image sequence to which
         the image belongs, represented as a rational number
       </p>
       <p style="padding-bottom:15px">         
         For variable frame rates, time-lapse photography, etc. the capture
         rate r is calculated as:
       </p>
       <p style="padding-bottom:15px">
         <tt> r = 1 / (tN - tNm1) </tt>
       </p>
       <p style="padding-bottom:15px">
         where <tt>tn</tt> is the time, in seconds, of the center of frame <tt>N</tt>'s 
         exposure interval, and <tt>tNm1</tt> is the time, in seconds, of the center
         of frame <tt>N-1</tt>'s exposure interval.
       </p>
       <p style="padding-bottom:15px">
         Both the numerator and denominator of <tt>r</tt> must be strictly positive.
       <p style="padding-bottom:15px">
       </p>
     </td>
   </tr>
   
   </table>
   </embed>

Lens ID
=======

These attributes identify the lens. All are optional.

.. raw:: html

   <embed>
   <table>
   
   <tr>
     <td style="vertical-align: top; width:150px; font-weight:bold"> attribute name</td>
     <td style="vertical-align: top; width:100px; font-weight:bold"> type </td>
     <td style="vertical-align: top; width:500px; font-weight:bold"> definition </td>
   </tr>

   <tr style="border-width:3px">
     <td style="vertical-align: top; width:150px"> <tt> <b> lensMake </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Manufacturer or vendor of the lens. If present, the value should be
         UTF-8-encoded and have a nonzero length. 
       <p style="padding-bottom:15px">
       </p>
     </td>
   </tr>
   
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> lensModel </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Model name or model number of the lens.  If present, the value
         should be UTF-8-e coded and have a nonzero length.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> lensSerialNumber </b>
     </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Serial number of the lens
       </p>
       <p style="padding-bottom:15px">
         Note that despite the name, the value can include non-digits
         as well as digits.
       </p>
       <p style="padding-bottom:15px">
         If present, the value should be UTF-8-encoded and have a nonzero length.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> lensFirmwareVersion
     </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Firmware version of the lens. If present, the value should be
         UTF-8-encoded and have a nonzero length.
       </p>
     </td>
   </tr>

   </table>
   </embed>

Lens State
==========

These attributes describe the lens settings. All are optional.

.. raw:: html

   <embed>
   <table>
   
   <tr>
     <td style="vertical-align: top; width:150px; font-weight:bold"> attribute name</td>
     <td style="vertical-align: top; width:100px; font-weight:bold"> type </td>
     <td style="vertical-align: top; width:500px; font-weight:bold"> definition </td>
   </tr>

   <tr style="border-width:3px">
     <td style="vertical-align: top; width:150px"> <tt> <b> nominalFocalLength
     </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Number printed on barrel of a prime lens, or number next to index
         mark on a zoom lens, in units of millimeters.
       </p>
         
       <p style="padding-bottom:15px">
         Nominal focal length is appropriate for asset tracking of lenses (e.g.
         a camera rental house catalogs its lens stock by nominal focal length).
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> pinholeFocalLength
     </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         In the simplest model of image formation, the distance between the
         pinhole and the image plane, in units of millimeters.
       </p>
         
       <p style="padding-bottom:15px">
         When a CGI application supplies a method for an artist to provide
         focal length to some calculation, pinhole focal length is almost
         always the appropriate number to convey to the application.
       </p>
     </td>
   </tr>
         
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> effectiveFocalLength
     </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         In the thick lens model, the effective focal length is the distance
         between the front focal point and the front nodal point, or
         equivalently the back focal point and the back nodal point, in units
         of millimeters.
       </p>
         
       <p style="padding-bottom:15px">
         The effective focal length is an abstraction used in lens design and,
         unless a CGI application is sophisticated enough to be using the thick
         lens model, should not be supplied to the application; for normal
         CGI applications, pinhole focal length should be used.
       </p>
         
       <p style="padding-bottom:15px">
         Note that the forward and back lens nodal points mentioned above are
         distinct in meaning and in position from the forward and back lens
         entrance pupils. A 'no-parallax' rotation is rotation around the
         forward lens entrance pupil.
       </p>
     </td>
   </tr>
         
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> entrancePupilOffset
     </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         The axial distance from the image plane to the entrance pupil, in
         units of millimeters. A larger entrance pupil offset means the
         entrance pupil is closer to the object.
       </p>
         
       <p style="padding-bottom:15px">
         Note that in some lens configurations, the entrance pupil offset can
         be negative.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> aperture </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         The f-number of the lens, computed as the ratio of lens effective
         focal length to the diameter of lens entrance pupil at the time the
         image was created or captured.
       </p>
     </td>
   </tr>

   <tr>
      <td style="vertical-align: top; width:150px"> <tt> <b> tStop </b> </tt>
      </td>
      <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
      <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         The ratio of lens effective focal length to diameter of entrance
         pupil divided by the square root of the transmittance the lens
         presents to a paraxial ray.
         
         Note that <tt>tStop</tt>, like <tt>aperture</tt>, must be strictly positive;
         and that tStop will always be a larger number than aperture.
       </p>
     </td>
   </tr>

   <tr>
      <td style="vertical-align: top; width:150px"> <tt> <b> focus </b> </tt>
      </td>
      <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
      <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         The camera's focus distance, in meters
       </p>
     </td>
   </tr>

   </table>
   </embed>

Editorial
=========

These attribute help to document the image. All are optional.

.. raw:: html

   <embed>
   <table>
   
   <tr>
     <td style="vertical-align: top; width:150px; font-weight:bold"> attribute name</td>
     <td style="vertical-align: top; width:100px; font-weight:bold"> type </td>
     <td style="vertical-align: top; width:500px; font-weight:bold"> definition </td>
   </tr>

   <tr style="border-width:3px">
     <td style="vertical-align: top; width:150px"> <tt> <b> owner </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Name of the owner of the image.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> comments </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Additional image information in human-readable form, for example a
         verbal description of the image.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> capDate </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         The date when the image was created or captured, in local time, and
         formatted as <tt>YYYY:MM:DD hh:mm:ss</tt>, where <TT>YYYY</TT> is the year (4
         digits, e.g. 2003), <TT>MM</TT> is the month (2 digits, 01, 02, ... 12),
         <TT>DD</TT> is the day of the month (2 digits, 01, 02, ... 31), hh is the
         hour (2 digits, 00, 01, ... 23), mm is the minute, and ss is the
         second (2 digits, 00, 01, ... 59).
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> utcOffset </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Universal Coordinated Time (UTC), in seconds: UTC == local time +
         utcOffset.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> keyCode </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> KeyCode </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         For motion picture film frames.  Identifies film manufacturer, film
         type, film roll and frame position within the roll.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> framesPerSecond </b>
     </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> Rational </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Defines the nominal playback frame rate for image sequences, in
         frames per second.  Every image in a sequence should have a
         <tt>framesPerSecond</tt> attribute, and the attribute value should be the
         same for all images in the sequence.  If an image sequence has no
         <tt>framesPerSecond</tt> attribute, playback software should assume that the
         frame rate for the sequence is 24 frames per second.
       </p>

       <p style="padding-bottom:15px">
         In order to allow exact representation of NTSC frame and field
         rates, <tt>framesPerSecond</tt> is stored as a rational number.  A rational
         number is a pair of integers, <tt>n</tt> and <tt>d</tt>, that represents the value
         <tt>n/d</tt>.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> timeCode </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> TimeCode </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Time and control code.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> imageCounter </b>
     </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> int </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         An image number.
       </p>
         
       <p style="padding-bottom:15px">
         For a sequence of images, the image number increases when the images
         are accessed in the intended play order.  <tt>imageCounter</tt> can be used
         to order frames when more standard ordering systems are
         inapplicable, including but not limited to uniquely identifying
         frames of high-speed photography that would have identical time
         codes, ordering sequences of frames where some frames may have been
         captured and discarded due to real-time constraints, or ordering
         frames in a sequence that is intermittently accumulated from devices
         such as security cameras triggered by motion in an environment.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> reelName </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Name for a sequence of unique images. If present, the value should
         be UTF-8-encoded and have a nonzero length.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b>
     ascFramingDecisionList </b> </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         JSON-encoded description of framing decisions associated with the
         captured image, in a format termed 'ASC-FDL', designed and
         documented by the American Society of Cinematographers (ASC).
       </p>

       <p style="padding-bottom:15px">
         If present, the value should be UTF-8-encoded and have a nonzero length.
       </p>
     </td>
   </tr>

   </table>
   </embed>

Encoded Image Color Characteristics
===================================

These attributes describe the color characteristics of the image. All are
optional.

.. raw:: html

   <embed>
   <table>
   
   <tr>
     <td style="vertical-align: top; width:150px; font-weight:bold"> attribute name</td>
     <td style="vertical-align: top; width:100px; font-weight:bold"> type </td>
     <td style="vertical-align: top; width:500px; font-weight:bold"> definition </td>
   </tr>

   <tr style="border-width:3px">
     <td style="vertical-align: top; width:150px"> <tt> <b> chromaticities </b>
     </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> Chromaticities </tt>
     </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         For RGB images, specifies the CIE (x,y) chromaticities of the
         primaries and the white point.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> whiteLuminance </b>
     </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> float </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         For RGB images, defines the luminance, in Nits (candelas per square
         meter) of the RGB value (1.0, 1.0, 1.0).
       <p style="padding-bottom:15px">

       <p style="padding-bottom:15px">
         If the chromaticities and the luminance of an RGB image are
         known, then it is possible to convert the image's pixels from RGB to
         CIE XYZ tristimulus values.
       </p>
     </td>
   </tr>

   <tr>
      <td style="vertical-align: top; width:150px"> <tt> <b> adoptedNeutral </b>
      </tt> </td>
      <td style="vertical-align: top; width:100px"> <tt> V2f </tt> </td>
      <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Specifies the CIE (x,y) coordinates that should be considered
         neutral during color rendering.  Pixels in the image file whose
         (x,y) coordinates match the <tt>adoptedNeutral</tt> value should be mapped to
         neutral values on the display.
       </p>
     </td>
   </tr>

   </table>
   </embed>

Anticipated Use in Pipeline
===========================

These attributes relate to the application of the image in the motion picture
pipeline. All are optional.

.. raw:: html

   <embed>
   <table>
   
   <tr>
     <td style="vertical-align: top; width:150px; font-weight:bold"> attribute name</td>
     <td style="vertical-align: top; width:100px; font-weight:bold"> type </td>
     <td style="vertical-align: top; width:500px; font-weight:bold"> definition </td>
   </tr>

   <tr style="border-width:3px">
      <td style="vertical-align: top; width:150px"> <tt> <b> envmap </b> </tt>
      </td>
      <td style="vertical-align: top; width:100px"> <tt> Envmap </tt> </td>
      <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         If this attribute is present, the image represents an environment
         map.  The attribute's value defines how 3D directions are mapped to
         2D pixel locations.  
       </p>
     </td>
   </tr>
         
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> wrapmodes </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> string </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
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
       </p>

       <ul>
         <li><tt>black</tt> - pixels outside the zero-to-one range are black </li>

         <li><tt>clamp</tt> - texture coordinates less than 0.0 and greater than 1.0 are clamped
           to 0.0 and 1.0 respectively. </li>

         <li><tt>periodic</tt> - the texture image repeats periodically </li>

         <li><tt>mirror</tt> - the texture image repeats periodically, but every other instance
           is mirrored </li>
       </li>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> multiView </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> StringVector </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Defines the view names for multi-view image files.  A multi-view
         image contains two or more views of the same scene, as seen from
         different viewpoints, for example a left-eye and a right-eye view
         for stereo displays.  The <tt>multiView</tt> attribute lists the names of the
         views in an image, and a naming convention identifies the channels
         that belong to each view.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> deepImageState </b>
     </tt> </td>
     <td style="vertical-align: top; width:100px"> <tt> DeepImageState </tt>
     </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Specifies whether the pixels in a deep image are sorted and
         non-overlapping.
       </p>

       <p style="padding-bottom:15px">
         Note: this attribute can be set by application code that writes a
         file in order to tell applications that read the file whether the
         pixel data must be cleaned up prior to image processing operations
         such as flattening.  The OpenEXR library does not verify that the
         attribute is consistent with the actual state of the pixels.
         Application software may assume that the attribute is valid, as long
         as the software will not crash or lock up if any pixels are
         inconsistent with the <tt>deepImageState</tt> attribute. See
         <a href="InterpretingDeepPixels.html" class="reference internal ">Interpreting OpenEXR Deep
         Pixels</a> for more details.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> idManifest </b> </tt>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> CompressedIDManifest
     </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         ID manifest.
       </p>
     </td>
   </tr>

   </table>
   </embed>

Deprecated Attributes
=====================

These attributes are how obsolete and are no longer officially supported by file
format. Note that you can still read and write images that contain these
attributes.

.. raw:: html

   <embed>
   <table>
   
   <tr>
     <td style="vertical-align: top; width:150px; font-weight:bold"> attribute
     name <rat/td>
     <td style="vertical-align: top; width:500px; font-weight:bold"> definition </td>
   </tr>

   <tr style="border-width:3px">
     <td style="vertical-align: top; width:150px"> <tt> <b> dwaCompressionLevel
     </b> </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Sets the quality level for images compressed with the DWAA or DWAB method.
       </p>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> renderingTransform </b> </tt> </td>
     <td style="vertical-align: top; width:500px" rowspan=2>
       <p style="padding-bottom:15px">
         Specify the names of the CTL functions that implements the intended
         color rendering and look modification transforms for this image.
       </p>
     </td>
   </tr>
   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> lookModTransform </b> </tt> </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px"> <tt> <b> maxSamplesPerPixel
     </b> </tt> </td>
     <td style="vertical-align: top; width:500px">
       <p style="padding-bottom:15px">
         Stores the maximum number of samples used by any single pixel within a
         deep image. If this number is small, it may be appropriate to read the
         deep image into a fix-sized buffer for processing. However, this number
         may be very large.
       </p>
       <p>
         Note that the library never actually enforced the correctness
         of this value, so if it appears in legacy files, it should
         not be trusted.
       </p>
     </td>
   </tr>

   </table>
   </embed>
