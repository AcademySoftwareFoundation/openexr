..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

exrmultipart
############

::

    exrmultipart -combine -i input.exr[:partnum][::partname] [input2.exr[:partnum]][::partname] [...] -o outfile.exr [options]

    or: exrmultipart -separate -i infile.exr -o outfileBaseName [options]

    or: exrmultipart -convert -i infile.exr -o outfile.exr [options]

Description
-----------

Combine or split multipart data

Options:
--------

.. describe:: -override [0/1]

              * 0 = do not override conflicting shared attributes [default]

              * 1 = override conflicting shared attributes

.. describe:: -view name
              
              (after specifying -i) assign following inputs to view 'name'

.. describe:: -h, --help

              print this message

.. describe:: --version

              print version information

