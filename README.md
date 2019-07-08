# OpenEXR

[![License](https://img.shields.io/badge/License-BSD%203%20Clause-blue.svg)](LICENSE.md)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/2799/badge)](https://bestpractices.coreinfrastructure.org/projects/2799)

**OpenEXR** is a high dynamic-range (HDR) image file format developed
by Industrial Light & Magic (ILM) for use in computer imaging
applications. It supports stereoscopic and deep images.  Weta Digital,
Walt Disney Animation Studios, Sony Pictures Imageworks, Pixar
Animation Studios, DreamWorks, and other studios, companies, and
individuals have made contributions to the code base. The file format
has seen wide adoption in a number of industries.

OpenEXR's features include:

* Higher dynamic range and color precision than existing 8- and 10-bit
  image file formats.
* Support for 16-bit floating-point, 32-bit floating-point, and
  32-bit integer pixels. The 16-bit floating-point format, called "half",
  is compatible with the half data type in NVIDIA's Cg graphics language
  and is supported natively on their GPUs.
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

## OpenEXR Sub-modules

The OpenEXR distribution consists of the following sub-modules:

* **IlmBase** - Utility libraries from Industrial Light & Magic: Half, Imath, Iex, IlmThread.
* **PyIlmBase** - Python bindings for the IlmBase libraries.
* **OpenEXR** - The core image library.
* **OpenEXR_Viewers** - Standard image viewing programs
* **Contrib** - Various plugins and utilities, contributed by the community.
    
Please see the README.md files of each of the individual directories for more information.

A collection of OpenEXR images is available from the adjacent repository
[openexr-images](https://github.com/openexr/openexr-images).

## Supported Platforms

The OpenEXR codebase can be built with any of the following:

* Linux: GCC 4.8 or newer
* Microsoft Visual Studio 2015 or newer
* Mac OS

The Python bindings in PyIlmBase support Python 2.6 and 2.7; they have
not been tested for Python 3.

## Dependencies

OpenEXR depends on [zlib](https://zlib.net).

PyIlmBase depends on [boost-python](https://github.com/boostorg/python) and
optionally on [numpy](http://www.numpy.org).

In OpenEXR_Viewers:

* **exrdisplay** depends on [fltk](http://www.fltk.org/index.php)
* **playexr** depends on [Cg](https://developer.nvidia.com/cg-toolkit)

## Web Resources

* Main web page: http:://www.openexr.org

* GitHub repository: http://www.github.com/openexr/openexr

* Documentation: http://www.openexr.com/documentation.html.

* Developer discussion mailing list: [openexr-dev@lists.aswf.io](https://lists.aswf.io/g/openexr-dev)

## Developer Quick Start

Download the latest release of OpenEXR from the GitHub Releases page:
https://github.com/openexr/openexr/releases.

For the basic installation on Linux:

    cd <source root>/IlmBase
    ./configure
    make
    make install

    cd <source root>/OpenEXR
    ./configure
    make 
    make install

See the module README files for options to ``configure``.

See the [build documentation](INSTALL.md) documentation for help with
installation on other platforms.

## Contributing

Developers who wish to contribute code to be considered for inclusion
in the OpenEXR distribution must first complete the Contributor
License Agreement and submit it to info@openexr.com. We prefer code
submissions in the form of pull requests to this repository. Every
commit must be signed off. That is, every commit log message must
include a “Signed-off-by” line (generated, for example, with “git
commit --signoff”), indicating that the committer wrote the code and
has the right to release it under the BSD-3-Clause license. See
http://developercertificate.org/ for more information on this
requirement.

See [CONTRIBUTING.md](CONTRIBUTING.md) for more information about
contributing to OpenEXR.

## Project Goverance

OpenEXR is governed by the Academy Software Foundation. See
[GOVERNANCE.md](GOVERNANCE.md) for more infomation.

## Documentation

Documentation is available at http://www.openexr.com/documentation.html.

