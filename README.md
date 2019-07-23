# OpenEXR

[![License](https://img.shields.io/badge/License-BSD%203%20Clause-blue.svg)](LICENSE.md)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/2799/badge)](https://bestpractices.coreinfrastructure.org/projects/2799)

![openexr](/OpenEXR/doc/images/windowExample1.big.png)

**OpenEXR** is a high dynamic-range (HDR) image file format originally
developed by Industrial Light & Magic (ILM) in 2003 for use in
computer imaging applications. It supports stereoscopic and deep
images.  Weta Digital, Walt Disney Animation Studios, Sony Pictures
Imageworks, Pixar Animation Studios, DreamWorks, and other studios,
companies, and individuals have made contributions to the code
base. OpenEXR is now maintained by the [Academy Software
Foundation](https://www.aswf.io). The file format has seen wide
adoption in a number of industries.

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
* **OpenEXR_Viewers** - Example code for image viewing programs.
* **Contrib** - Various plugins and utilities, contributed by the community.
    
A collection of OpenEXR images is available from the adjacent repository
[openexr-images](https://github.com/openexr/openexr-images).

## Supported Platforms

The OpenEXR codebase can be built with any of the following:

* Linux
* macOS
* Microsoft Visual Studio 2015 or newer

The Python bindings in PyIlmBase support Python 2 and Python 3.

## Dependencies

* OpenEXR depends on [zlib](https://zlib.net).

* PyIlmBase depends on [boost-python](https://github.com/boostorg/python) and
optionally on [numpy](http://www.numpy.org).

* In OpenEXR_Viewers:

  * **exrdisplay** depends on [fltk](http://www.fltk.org/index.php)
  * **playexr** depends on [Cg](https://developer.nvidia.com/cg-toolkit)

See [INSTALL](INSTALL.md) for more details.

## Web Resources

* Main web page: http:://www.openexr.org

* GitHub repository: http://www.github.com/openexr/openexr

* Documentation: http://www.openexr.com/documentation.html.

* Developer discussion mailing list: [openexr-dev@lists.aswf.io](https://lists.aswf.io/g/openexr-dev)

## Developer Quick Start

See [INSTALL](INSTALL.md) for instructions on downloading and building OpenEXR
from source.

## License

OpenEXR is released under the [BSD-3-Clause](LICENSE) license.
 
Developers who wish to contribute code to be considered for inclusion
in the OpenEXR distribution must first complete the Contributor
License Agreement and submit it to info@openexr.com. Commits must
include a “Signed-off-by” line indicating that the committer wrote the
code and has the right to release it under the BSD-3-Clause
license. See http://developercertificate.org/ for more information.

## Contributing

See [CONTRIBUTING](CONTRIBUTING.md) for more information about
contributing to OpenEXR.

## Project Goverance

OpenEXR is governed by the Academy Software Foundation. See
[GOVERNANCE](GOVERNANCE.md) for more infomation.

## Documentation

Technical documentation is available at
http://www.openexr.com/documentation.html, or:

* [Technical Introduction](/OpenEXR/doc/TechnicalIntroduction.pdf)

* [Reading and Writing Image Files](/OpenEXR/doc/ReadingAndWritingImageFiles.pdf)

* [OpenEXR File Layout](/OpenEXR/doc/OpenEXRFileLayout.pdf)

* [Multi-View OpenEXR](/OpenEXR/doc/MultiViewOpenEXR.pdf)

* [Interpreting OpenEXR Deep Pixels](/OpenEXR/doc/InterpretingDeepPixels.pdf)

* [The Theory Of OpenEXR Deep Samples](/OpenEXR/doc/InterpretingDeepPixels.pdf)

## Frequently Asked Questions

* "``pip install openexr`` doesn't work."

  The OpenEXR project provides python bindings for the Imath
  vector/matrix classes, but it does *not* provide python bindings for
  reading, writing, or editing .exr files.  The
  [openexrpython](https://github.com/jamesbowman/openexrpython) module
  is not affiliated with the OpenEXR project or the ASWF. Please
  direct questions there.

  Alternatively,
  [OpenImageIO](https://sites.google.com/site/openimageio/home) also
  includes python bindings for OpenEXR.

---

![aswf](/ASWF/images/aswf.png)
