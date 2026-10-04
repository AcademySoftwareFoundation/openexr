..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _Image Size Limit:

Image Size Limit and Out-of-Memory Failures
###########################################

The OpenEXR file format places no fixed limit on image size, except
that image width and height are represented by signed 32-bit integers
and therefore technically limited to a maximum of 2,147,483,647.

Attempting to read a very large image may result in an "out-of-memory
failure. This is not considered a security vulnerability. The memory
required to decode such an image is inherently proportional to its
pixel count, even if compression reduces the image to a small file
size on disk. Exhausting available memory on a given machine is a
system resource constraint, not a library defect — the same file that
triggers an out-of-memory error on one machine may load successfully
on another with more memory.

The OpenEXR library provides 
``Imf::Header::setMaxImageSize(int maxWidth,int maxHeight)`` and
``Imf::Header:"setMaxTileSize(int maxWidth,int maxHeight)`` (and
``exr_set_default_maximum_image_size()`` and
``exr_set_default_maximum_tile_size()`` in OpenEXRCore) to allow
applications to reject files with dimensions exceeding a configurable
limit before any large allocation occurs. Applications processing
untrusted EXR files should set these limits to values appropriate for
their deployment environment.

