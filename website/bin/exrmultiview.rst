..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

exrmultiview
############

::

    exrmultiview [options] viewname1 infile1 viewname2 infile2 ... outfile

Description
-----------

Combine two or more single-view OpenEXR image files into
a single multi-view image file.  On the command line,
each single-view input image is specified together with
a corresponding view name.  The first view on the command
line becomes the default view.  Example:

::

    exrmultiview left imgL.exr right imgR.exr imgLR.exr

Here, ``imgL.exr`` and ``imgR.exr`` become the left and right
views in output file ``imgLR.exr``.  The left view becomes
the default view.

Options:
--------

.. describe:: -z x          

              sets the data compression method to x
              (none/rle/zip/piz/pxr24/b44/b44a/dwaa/dwab,
              default is piz)

.. describe:: -v            

              verbose mode

.. describe:: -h, --help    

              print this message

.. describe:: --version  

              print version information

