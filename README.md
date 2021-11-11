[![License](https://img.shields.io/github/license/AcademySoftwareFoundation/openexr)](LICENSE.md)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/2799/badge)](https://bestpractices.coreinfrastructure.org/projects/2799)
[![Build Status](https://github.com/AcademySoftwareFoundation/openexr/workflows/CI/badge.svg)](https://github.com/AcademySoftwareFoundation/openexr/actions?query=workflow%3ACI)
[![Analysis Status](https://github.com/AcademySoftwareFoundation/openexr/workflows/Analysis/badge.svg)](https://github.com/AcademySoftwareFoundation/openexr/actions?query=workflow%3AAnalysis)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=AcademySoftwareFoundation_openexr&metric=alert_status)](https://sonarcloud.io/dashboard?id=AcademySoftwareFoundation_openexr)

# OpenEXR

<img align="right" src="docs/images/windowExample1.png">

OpenEXR provides the specification and reference implementation of the
EXR file format, the professional-grade image storage format of the
motion picture industry.

The purpose of EXR format is to accurately and efficiently represent
high-dynamic-range scene-linear image data and associated metadata,
with strong support for multi-part, multi-channel use cases.

OpenEXR is widely used in host application software where accuracy is
critical, such as photorealistic rendering, texture access, image
compositing, deep compositing, and DI.

### About OpenEXR

OpenEXR is a project of the [Academy Software
Foundation](https://www.aswf.io).  The format and library were
originally developed by Industrial Light & Magic and first released
in 2003.  Weta Digital, Walt Disney Animation Studios, Sony Pictures
Imageworks, Pixar Animation Studios, DreamWorks, and other studios,
companies, and individuals have made contributions to the code base.

OpenEXR is included in the [VFX Reference
Platform](https://vfxplatform.com).

### OpenEXR Features

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

### OpenEXR and Imath Version 3

With the release of OpenEXR 3, the Imath library formerly distributed
via the IlmBase component of OpenEXR is now an independent library
dependency, available for download from https:://github.com/AcademySoftwareFoundation/Imath.
You can choose to build OpenEXR against an external installation of
Imath, or the default CMake configuration will download and build it
automatically during the OpenEXR build process.  Note that the half
16-bit floating point data type is included in Imath.

See the [porting
guide](https://github.com/AcademySoftwareFoundation/Imath/blob/master/docs/PortingGuide2-3.md)
for details about differences from previous releases and how to
address them. Also refer to the porting guide for details about
changes to Imath.

#### New Features in OpenEXR v3.1

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

See [Reading and Writing OpenEXR Image Files with the C-language
API](https://openexr.readthedocs.io/en/latest/OpenEXRCoreAPI.html)
for more information.

### Supported Platforms

OpenEXR builds on Linux, macOS, Microsoft Windows, and is
cross-compilable on other systems.

### OpenEXR Project Mission

The goal of the OpenEXR project is to keep the EXR format reliable and
modern and to maintain its place as the preferred image format for
entertainment content creation. 

Major revisions are infrequent, and new features will be carefully
weighed against increased complexity.  The principal priorities of the
project are:

* Robustness, reliability, security
* Backwards compatibility, data longevity
* Performance - read/write/compression/decompression time
* Simplicity, ease of use, maintainability
* Wide adoption, multi-platform support - Linux, Windows, macOS, and others

OpenEXR is intended solely for 2D data. It is not appropriate for
storage of volumetric data, cached or lit 3D scenes, or more complex
3D data such as light fields.

The goals of the IlmBase project are simplicity, ease of use,
correctness and verifiability, and breadth of adoption. IlmBase is not
intended to be a comprehensive linear algebra or numerical analysis
package.

### OpenEXR Project Governance

OpenEXR is hosted by the Academy Software Foundation. See
[GOVERNANCE](GOVERNANCE.md) for more information about how the project
operates.

The OpenEXR project is dedicated to promoting a harassment-free
community. Read our [code of conduct](CODE_OF_CONDUCT.md).

## Developer Quick Start

See [INSTALL](INSTALL.md) for instructions on downloading and building OpenEXR
from source.

## Resources

* Website: http://www.openexr.com

* GitHub repository: http://www.github.com/AcademySoftwareFoundation/openexr

* Technical Documentation: https://openexr.readthedocs.io

* Reference images: https://github.com/AcademySoftwareFoundation/openexr-images.

### Getting Help

There are several ways to connect with the OpenEXR project:

* The openexr-dev@lists.aswf.io mail list: This is a development
  focused mail list with a deep history of technical conversations and
  decisions that have shaped the project. Subscribe at
  [openexr-dev@lists.aswf.io](https://lists.aswf.io/g/openexr-dev).

* ASWF Slack channel: [#openexr](https://academysoftwarefdn.slack.com/archives/CMLRW4N73)
  
* GitHub Issues: GitHub issues are used both to track bugs and to
  discuss feature requests.

See [CONTRIBUTING](CONTRIBUTING.md) for more information.

### Getting Involved

OpenEXR welcomes contributions to the project. See
[CONTRIBUTING](CONTRIBUTING.md) for more information about
contributing to OpenEXR.

## License

OpenEXR is released under the [BSD-3-Clause](LICENSE) license. See
[PATENTS](PATENTS) for license information about portions of
OpenEXR that are provided under a different license.

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
