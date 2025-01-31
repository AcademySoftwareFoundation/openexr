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

The ACES image file format defined by `SMPTE ST 2065-4`_ 
is a subset of the OpenEXR file format.  

ACES image container files generally follow these restrictions:

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

Note that perfect adherence to the specification is not strictly 
necessary for many use cases and often diverges in minor ways 
(e.g. the use of PIZ compression in exr2aces is technically off-spec),
but the file generated here is generally suitable for broadly
compatible archival.

Options:
--------

.. describe:: -v, --verbose
   
   verbose mode

.. describe::  -h, --help

   print this message

.. describe:: --version

   print version information

              
.. _SMPTE ST 2065-4: https://doi.org/10.5594/SMPTE.ST2065-4.2013
