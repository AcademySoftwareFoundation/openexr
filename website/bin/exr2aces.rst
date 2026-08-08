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

The ACES Image Container File Format specified in SMPTE ST 2065-4 
is a subset of the OpenEXR file format defined by the implementation 
of the time (roughly OpenEXR 1.6). 

This imposes considerable restrictions on what image data and metadata 
can be transported from a modern OpenEXR file into an ACES Image 
Container File, including (but not limited to) the following:

- The version field must be either be 2 or 1026; a value of 2 implies 
  attribute names cannot be longer than 31 characters in length, 
  as was the case prior to OpenEXR 1.7.

- Images must be stored as scanlines; tiles are not allowed.

- Images must contain RGB and possibly an A channel if they are monoscopic; 
  images representing stereo pairs would add another three (or four if A is 
  present) channels. Other channels are not permitted, including channels 
  that otherwise might represent a combination of a luminance channel and 
  chromaticity channels.

- The image must not be compressed.

- The `chromaticities` attribute must specify the ACES RGB primaries and 
  the ACES neutral as specified in SMPTE ST 2065-1.

- The `acesImageContainerFlag` flat must be present and have the value 1.

For the full set of restrictions, see `SMTPE ST 2065-4  <https://doi.org/10.5594/SMPTE.ST2065-4.2013>`_ (or any 
superseding later version of that standard).

In practice, facilities and productions often use the term "ACES file" 
to mean OpenEXR files containing linear scene data expressed as 
combinations of R, G and B whose chromaticities match those found in 
ST 2065-1, where equal amounts of those primaries produce a color the 
chromaticity of which matches that of the ACES neutral. The image data 
might be compressed using one of the OpenEXR's library's built-in compression 
functions, even though strict compliance with ST 2065-4 would forbid such 
compression; the `acesImageContainerFile` flag might be missing; and the 
`chromaticities` attribute might contain chromaticities that do not actually 
match those found in ST 2065-1.

Options:
--------

.. describe:: -v, --verbose
   
   verbose mode

.. describe::  -h, --help

   print this message

.. describe:: --version

   print version information

              
