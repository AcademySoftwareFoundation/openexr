..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

exrmaketiled
############

::

    exrmaketiled [options] infile outfile

Description
-----------

Read an OpenEXR image from infile, produce a tiled
version of the image, and save the result in outfile.

Options:
--------

.. describe:: -o            

              produces a ONE_LEVEL image (default)

.. describe:: -m            

              produces a MIPMAP_LEVELS multiresolution image

.. describe:: -r            

              produces a RIPMAP_LEVELS multiresolution image

.. describe:: -f c          

              when a MIPMAP_LEVELS or RIPMAP_LEVELS image
              is produced, image channel c will be resampled
              without low-pass filtering.  This option can
              be specified multiple times to disable low-pass
              filtering for multiple channels.

.. describe:: -e x y        

              when a MIPMAP_LEVELS or RIPMAP_LEVELS image
              is produced, low-pass filtering takes samples
              outside the image's data window.  This requires
              extrapolating the image.  Option -e specifies
              how the image is extrapolated horizontally and
              vertically (black/clamp/periodic/mirror, default
              is clamp).

.. describe:: -t x y        

              sets the tile size in the output image to
              x by y pixels (default is 64 by 64)

.. describe:: -d            

              sets level size rounding to ROUND_DOWN (default)

.. describe:: -u            

              sets level size rounding to ROUND_UP

.. describe:: -z x          

              sets the data compression method to x
              (none/rle/zip/piz/pxr24/b44/b44a/dwaa/dwab,
              default is zip)

.. describe:: -v            

              verbose mode

.. describe:: -h, --help    

              print this message

.. describe:: --version  

              print version information

.. describe:: -p i          

              part number, default is 0

