..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

exrcheck
########

::
   
    exrcheck [options] imagefile [imagefile ...]

Description
-----------
Read exr files to validate their contents and the correct behavior of the software.

Options:
--------

.. describe:: -m

   avoid excessive memory allocation (some files will not be fully checked)

.. describe:: -t

   avoid spending excessive time (some files will not be fully checked)

.. describe:: -s

   use stream API instead of file API

.. describe:: -c

   add core library checks

.. describe:: -h, --help

   print this message

.. describe:: --version

   print version information

