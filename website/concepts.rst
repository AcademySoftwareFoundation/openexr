..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _OpenEXR Concepts:

OpenEXR Concepts
################

A unique combination of features makes OpenEXR a good fit for
high-quality image processing and storage applications:

**high dynamic range**
  Pixel data are stored as 16-bit or 32-bit floating-point
  numbers. With 16 bits, the representable dynamic range is
  significantly higher than the range of most image capture devices:
  109 or 30 f-stops without loss of precision, and an additional 10
  f-stops at the low end with some loss of precision. Most 8-bit file
  formats have around 7 to 10 stops.

**good color resolution**
  With 16-bit floating-point numbers, color resolution is 1024 steps
  per f-stop, as opposed to somewhere around 20 to 70 steps per f-stop
  for most 8-bit file formats. Even after significant processing (for
  example, extensive color correction) images tend to show no
  noticeable color banding.

**compatible with graphics hardware**
  The 16-bit floating-point data format is fully compatible with the
  16-bit frame-buffer data format used in some new graphics
  hardware. Images can be transferred back and forth between an
  OpenEXR file and a 16-bit floating-point frame buffer without losing
  data.
                                                      
  Most of the data compression methods currently implemented in
  OpenEXR are lossless; repeatedly compressing and uncompressing an
  image does not change the image data. With the lossless compression
  methods, photographic images with significant amounts of film grain
  tend to shrink to somewhere between 35 and 55 percent of their
  uncompressed size. OpenEXR also supports lossy compression, which
  tends to shrink image files more than lossless compression, but
  doesn't preserve the image data exactly. New lossless and lossy
  compression schemes can be added in the future.

**arbitrary image channels**
  OpenEXR images can contain an arbitrary number and combination of
  image channels, for example red, green, blue, and alpha; luminance
  and sub-sampled chroma channels; depth, surface normal directions,
  or motion vectors.

**scan line and tiled images, multi-resolution images**
  Pixels in an OpenEXR file can be stored either as scan lines or as
  tiles. Tiled image files allow random-access to rectangular
  sub-regions of an image. Multiple versions of a tiled image, each
  with a different resolution, can be stored in a single
  multi-resolution OpenEXR file.
                                                      
  Multi-resolution images, often called "mipmaps" or "ripmaps", are
  commonly used as texture maps in 3D rendering programs to accelerate
  filtering during texture lookup, or for operations like stereo image
  matching. Tiled multiresultion images are also useful for
  implementing fast zooming and panning in programs that interactively
  display very large images.

**ability to store additional data**
  Often it is necessary to annotate images with additional data; for
  example, color timing information, process tracking data, or camera
  position and view direction. OpenEXR allows storing of an arbitrary
  number of extra attributes, of arbitrary type, in an image
  file. Software that reads OpenEXR files ignores attributes it does
  not understand.

**easy-to-use C++ and C programming interfaces**
  In order to make writing and reading OpenEXR files easy, the file
  format was designed together with a C++ programming interface. Two
  levels of access to image files are provided: a fully general
  interface for writing and reading files with arbitrary sets of image
  channels, and a specialized interface for the most common case (red,
  green, blue, and alpha channels, or some subset of
  those). Additionally, a C-callable version of the programming
  interface supports reading and writing OpenEXR files from programs
  written in C.
                                                      
  Many application programs expect image files to be scan line
  based. With the OpenEXR programming interface, applications that
  cannot handle tiled images can treat all OpenEXR files as if they
  were scan line based; the interface automatically converts tiles to
  scan lines.
                                                      
  The C++ and C interfaces are implemented in the open-source OpenEXR
  library.

**fast multi-threaded file reading and writing**
  The OpenEXR library supports multi-threaded reading or writing of an
  OpenEXR image file: while one thread performs low-level file input
  or output, multiple other threads simultaneously encode or decode
  individual pieces of the file.

**portability**
  The OpenEXR file format is hardware and operating system
  independent. While implementing the C and C++ programming
  interfaces, an effort was made to use only language features and
  library functions that comply with the C and C++ ISO standards.

**multi-view**
  A “multi-view” image shows the same scene from multiple different
  points of view. A common application is 3D stereo imagery, where a
  left-eye and a right-eye view of a scene are stored in a single
  file.
                                                   
**deep data**
  Support for a new data type has been added: deep data. Deep images
  store an arbitrarily long list of data at each pixel location. This
  is different from multichannel or 'deep channel images' which can
  store a potentially large, but fixed, amount of information at each
  pixel. In a deep image, each pixel stores a different amount of
  data.
          
  This allows for more accurate compositing of objects which occlude
  each other, and provides a method for storing opacity data in the z
  direction (particularly useful for stereo images which have
  atmospheric effects such fog).

**multi-part**
  Multi-part files allow for storing multiple images in one OpenEXR
  file. One important application is to store layers of channels
  separately. This allows for faster access when only a subset of the
  channels needs reading. It also permits layers to have differing
  data layout (for example, for different compression, or different
  layout) and different data windows.
          
  It also allows some layers to be stored as deep data and others as
  regular images. With multi-part files, different views are stored in
  different parts.

.. toctree::
   :caption: Concepts

   TechnicalIntroduction
   StandardAttributes
   MultiViewOpenEXR
   SceneLinear
   InterpretingDeepPixels
   TheoryDeepPixels
   DeepIDsSpecification
   OpenEXRFileLayout
   PortingGuide
   SymbolVisibility
   ImageSizeLimit
