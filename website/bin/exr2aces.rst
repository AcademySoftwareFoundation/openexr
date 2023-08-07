..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

exr2aces
########

::
   
    exr2aces [options] infile outfile

Description
-----------

Read an OpenEXR file from infile and save the contents
in ACES image file outfile.

The ACES image file format is a subset of the OpenEXR file
format.  ACES image files are restricted as follows:

- Images are stored as scanlines; tiles are not allowed.

- Images contain three color channels, either R, G, B (red, green,
  blue) or Y, RY, BY (luminance, sub-sampled chroma)

- Images may optionally contain an alpha channel.

- Only three compression types are allowed:

  * NO_COMPRESSION (file is not compressed)

  * PIZ_COMPRESSION (lossless)

  * B44A_COMPRESSION (lossy)

- The "chromaticities" header attribute must specify
  the ACES RGB primaries and white point.

Options:
--------

.. describe:: -v, --verbose
   
   verbose mode

.. describe::  -h, --help

   print this message

.. describe:: --version

   print version information

              
