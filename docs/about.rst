..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _About OpenEXR:

About OpenEXR
=============

.. toctree::
   :caption: About
             
.. sidebar::

     .. image:: images/OpenEXR_Reel_2020.png
        :target: https://www.youtube.com/embed/X0khNMmEJEI
        :width: 325

OpenEXR is a project of the `Academy Software Foundation
<https://www.aswf.io>`_.  The format and library were originally
developed at Industrial Light & Magic and first released as open
source in 2003.  Weta Digital, Walt Disney Animation Studios, Sony
Pictures Imageworks, Pixar Animation Studios, DreamWorks, and other
studios, companies, and individuals have made contributions to the
code base.

Read the origin story of OpenEXR on the `ASWF Blog
<https://www.aswf.io/news/aswf-deep-dive-openexr-origin-story-part-1>`_. 

OpenEXR is included in the `VFX Reference Platform <https://vfxplatform.com>`_.

OpenEXR Features
----------------

* High dynamic range and color precision.
* Support for 16-bit floating-point, 32-bit floating-point, and
  32-bit integer pixels.
* Multiple image compression algorithms, both lossless and lossy. Some of
  the included codecs can achieve 2:1 lossless compression ratios on images
  with film grain.  The lossy codecs have been tuned for visual quality and
  decoding performance.
* Extensibility. New compression codecs and image types can easily be added
  by extending the C++ classes included in the OpenEXR software distribution.
  New image attributes (strings, vectors, integers, etc.) can be added to
  OpenEXR image headers without affecting backward compatibility with
  existing OpenEXR applications. 
* Support for stereoscopic image workflows and a generalization
  to multi-views.
* Flexible support for deep data: pixels can store a variable-length list
  of samples and, thus, it is possible to store multiple values at different
  depths for each pixel. Hard surfaces and volumetric data representations
  are accommodated.
* Multipart: ability to encode separate, but related, images in one file.
  This allows for access to individual parts without the need to read other
  parts in the file.
* Versioning: OpenEXR source allows for user configurable C++
  namespaces to provide protection when using multiple versions of the
  library in the same process space.

OpenEXR and Imath Version 3
----------------------------

With the release of OpenEXR 3, the Imath library formerly distributed
via the IlmBase component of OpenEXR is now an independent library
dependency, available for download from
https://github.com/AcademySoftwareFoundation/Imath.  You can choose to
build OpenEXR against an external installation of Imath, or the
default CMake configuration will download and build it automatically
during the OpenEXR build process.  Note that the half 16-bit floating
point data type is included in Imath.

See :doc:`PortingGuide` for details about differences from previous
releases and how to address them. Also refer to the porting guide for
details about changes to Imath.

New Features in OpenEXR v3.1
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The 3.1 release of OpenEXR introduces a new library, OpenEXRCore,
which is the result of a significant re-thinking of how OpenEXR
manages file I/O and provides access to image data. It begins to
address long-standing scalability issues with multithreaded image
reading and writing.

The OpenEXRCore library provides thread-safe, non-blocking access to
files, which was not possible with the current API, where the
framebuffer management is separate from read requests. It is written
entirely in C and provides a new C-language API alongside the existing
C++ API. This new low-level API allows applications to do custom
unpacking of EXR data, such as on the GPU, while still benefiting from
efficient I/O, file validation, and other semantics. It provides
efficient direct access to EXR files in texturing applications. This C
library also introduces an easier path to implementing OpenEXR
bindings in other languages, such as Rust.

The 3.1 release represents a technology preview for upcoming
releases. The initial release is incremental; the existing API and
underlying behavior has not changed. The new API is available now for
performance validation testing, and then in future OpenEXR releases,
the C++ API will migrate to use the new core in stages. It is not the
intention to entirely deprecate the C++ API, nor must all applications
re-implement EXR I/O in terms of the C library. The C API does not,
and will not, provide the rich set of utility classes that exist in
the C++ layer. The 3.1 release of the OpenEXRCore library simply
offers new functionality for specialty applications seeking the
highest possible performance. In the future, the ABI will evolve, but
the API will remain consistent, or only have additions.

See :doc:`ReadingAndWritingImageFiles` for more information.

Credits
=======

The ILM OpenEXR file format was originally designed and implemented at
Industrial Light & Magic by Florian Kainz, Wojciech Jarosz, and Rod
Bogart. The PIZ compression scheme is based on an algorithm by
Christian Rouet. Josh Pines helped extend the PIZ algorithm for 16-bit
and found optimizations for the float-to-half conversions. Drew Hess
packaged and adapted ILM's internal source code for public release and
maintains the OpenEXR software distribution. The PXR24 compression
method is based on an algorithm written by Loren Carpenter at Pixar
Animation Studios.

For a complete list of contributors see the `CONTRIBUTORS.md
<https://github.com/AcademySoftwareFoundation/openexr/blob/main/CONTRIBUTING.md>`_
file.




