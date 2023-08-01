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
   bin/exrmultipart
   bin/exrmultiview
   bin/exrstdattr
