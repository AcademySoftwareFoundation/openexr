..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

exrmakepreview
##############

::

    exrmakepreview [options] infile outfile

Description
-----------

Read an OpenEXR image from infile, generate a preview
image, add it to the image's header, and save the result
in outfile.  Infile and outfile must not refer to the same
file (the program cannot edit an image file "in place").

Options:
--------

.. describe:: -w x          

              sets the width of the preview image to x pixels
              (default is 100)

.. describe:: -e s          

              adjusts the preview image's exposure by s f-stops
              (default is 0).  Positive values make the image
              brighter, negative values make it darker.

.. describe:: -v            

              verbose mode

.. describe:: -h, --help    

              print this message

.. describe:: --version

              print version information

