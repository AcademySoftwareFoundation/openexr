..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

OpenEXR Tools
#############

The OpenEXR distribution includes a set of basic utility programs that
operate on image files. These do basic conversion between various
flavors of exr files (multiview, multipart, tiled, etc) and they
inspect and edit metadata. For a full-featured set of image processing
operations, consider `oiiotool
<https://openimageio.readthedocs.io/en/latest/oiiotool.html>`_, the
Swiss Army Knife of `OpenImageIO
<https://sites.google.com/site/openimageio/home>`_

The OpenEXR tools are not generally included in most package managers'
distribution of OpenEXR libraries (e.g. via ``yum install``). To build
the tools from source, configure the top-level OpenEXR project build
with the cmake option ``OPENEXR_BUILD_TOOLS=ON``. The tools can only
be built from source as a component of the overall project build, not
separately. To further include the tools in the OpenEXR installation
after build (i.e. ``cmake --target install``), configure with
``OPENEXR_INSTALL_TOOLS=ON``. Both are on by default.

In addition, some tools such as ``exrcheck`` are considered "developer"
tools in that they are useful when developing and debugging OpenEXR
itself. These tools may be helpful in identifying security issues,
and as a result it is not recommended that they be used with untrusted
input files. Although they will still be built and available in the
build directory when the ``OPENEXR_BUILD_TOOLS=ON`` option is used,
a separate ``OPENEXR_INSTALL_DEVELOPER_TOOLS=ON`` option should be
enabled if installation of the developer tools is desired. That option
is off by default.

.. toctree::
   :caption: Tools
   :titlesonly:

   bin/exr2aces
   bin/exrcheck
   bin/exrenvmap
   bin/exrheader
   bin/exrinfo
   bin/exrmakepreview
   bin/exrmaketiled
   bin/exrmanifest
   bin/exrmetrics
   bin/exrmultipart
   bin/exrmultiview
   bin/exrstdattr
