..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _The OpenEXR API:

The OpenEXR API
###############

There are two separate, independent APIs for reading and writing EXR
image files: the traditional C++ API, and the newer C API, referred to
as ``OpenEXRCore``. The C++ API is the original, widely-used interface
first released in 2003. It consists of the ``OpenEXR``, ``Iex``, and
``IlmThread`` libraries, with functions and classes in the ``Imf::``
namespace (or "image format", the original the name of the library at
ILM prior to its public release).

The ``OpenEXRCore`` library, written primarily by Kimball Thurston at
Weta Digital, was introduced in July, 2021. This C-language
implementation of the file format is the result of a significant
re-thinking of image file I/O and access to image data. The
``OpenEXRCore`` library provides thread-safe, non-blocking access to
files, which was not possible with the older C++ API, where the
framebuffer management is separate from read requests.  This new
low-level API allows applications to do custom unpacking of EXR data,
such as on the GPU, while still benefiting from efficient I/O, file
validation, and other semantics. It provides efficient direct access
to EXR files in texturing applications. This C library also introduces
an easier path to implementing OpenEXR bindings in other languages,
such as Rust.

Currently, the two libraries and APIs sit alongside each other,
although in future OpenEXR releases, the C++ API will migrate to use
the new core in stages. It is not the intention to entirely deprecate
the C++ API, nor must all applications re-implement EXR I/O in terms
of the C library. The C API does not, and will not, provide the rich
set of utility classes that exist in the C++ layer.  The
``OpenEXRCore`` library simply offers new functionality for specialty
applications seeking the highest possible performance.

.. toctree::
   :caption: API
   :maxdepth: 2
              
   HelloWorld
   ReadingAndWritingImageFiles
   OpenEXRCoreAPI
              
* :ref:`genindex`

