# OpenEXR

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

## License

OpenEXR, including all contributions, is released under a modified BSD
license. Please see the [LICENSE.md] (LICENSE.md) file accompanying
the distribution for the legal fine print.
      
## OpenEXR Sub-modules

The OpenEXR distribution consists of the following sub-modules:

* **IlmBase** - Utility libraries from Industrial Light & Magic: Half, Imath, Iex, IlmThread.
* **PyIlmBase** - Python bindings for the IlmBase libraries.
* **OpenEXR** - The core image library.
* **OpenEXR_Viewers** - Standard image viewing programs
* **Contrib** - Various plugins and utilities, contributed by the community.
    
Please see the ``README`` files of each of the individual directories for more information.

A collection of OpenEXR images is available from the adjacent repository
[openexr-images](https://github.com/openexr/openexr-images).

## Dependencies

OpenEXR depends on [zlib](https://zlib.net).

PyIlmBase depends on [boost-python](https://github.com/boostorg/python) and
optionally on [numpy](http://www.numpy.org).

In OpenEXR_Viewers:

* **exrdisplay** depends on [fltk](http://www.fltk.org/index.php)
* **playexr** depends on [Cg](https://developer.nvidia.com/cg-toolkit)

## Web Resources

Main web page: http:://www.openexr.org

GitHub repository: http://www.github.com/openexr/openexr

Deverloper discussion mailing list:

* [openexr-dev@lists.aswf.io] (https://lists.aswf.io/g/openexr-dev)

## Building and Installation

Download the latest release of OpenEXR from the GitHub Releases page:
https://github.com/openexr/openexr/releases.

For more information about building from sources, see the [INSTALL.md] (INSTALL.md) file.

## Contributing

See [CONTRIBUTING.md] (CONTRIBUTING.md) for more information about contributing to OpenEXR.

## Project Goverance

OpenEXR is owned by the Academy Software Foundation, and is maintained
by the developer community. See [GOVERNANCE.md] (GOVERNANCE.md) for
more infomation.

## Documentation

Documentation is available at http://www.openexr.com/documentation.html.

