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

OpenEXR, including all contributions, is released under the modified
BSD license. Please see the LICENSE file for the legal fine print.
      
## Sub-modules
The OpenEXR distribution consists of the following sub-modules:

* IlmBase - Utility libraries from Industrial Light & Magic: Half, Imath, Iex, IlmThread.
* PyIlmBase - Python bindings for the IlmBase libraries.
* OpenEXR - The core image library.
* OpenEXR_Viewers - Standard image viewing programs
* Contrib - Various plugins and utilities, contributed by the community.
    
Please see the README files of each of the individual directories for more information.

A collection of OpenEXR images is available from the adjacent repository
[openexr-images](https://github.com/openexr/openexr-images).

## Web Resources

Main web page: http:://www.openexr.org

GitHub page: http://www.github.com/openexr

Mail lists:

http://lists.nongnu.org/mailman/listinfo/openexr-announce - OpenEXR-related announcements.

http://lists.nongnu.org/mailman/listinfo/openexr-user - for discussion about OpenEXR applications or general questions.

http://lists.nongnu.org/mailman/listinfo/openexr-devel - for developers using OpenEXR in their applications.

## Building and Installation

Download the latest release of OpenEXR from http://www.openexr.com/downloads.html.

Alternatively, you can download the latest release or the lastest development branch directly from http://github.com/openexr.

To build the OpenEXR binaries from source, compile and install the individual sub-models, according to the instructions in the respective README files.

Build and install the IlmBase module first, then build and install the OpenEXR module. Optionally, then build and install PyIlmBase, OpenEXR_Viewers, and Contrib.

The build system supports building via autoconf or cmake. See in README files in the IlmBase, PyIlmBase, OpenEXR, and OpenEXR_Viewers directories for specific instructions.

## Building with CMake

To build with CMake, OpenEXR has a few prerequisites.

* CMake 3.11 or newer
* zlib
* boost-python (if the python bindings are to be built)
* fltk (if the openexr viewer is to be built)
* Cg (if playexr is to be built)

When these prerequisites are fulfulled, prepare the build environment.

In the root CMakeLists.txt file, or using a tools such as ccmake or cmake-gui,
configure the OpenEXR build. The options are detailed below.

Create a build directory, cd into it, and run cmake to configure the build.
Select an appropriate generator, such as "Unix Makefiles", or "Visual Studio 15 2017 Win64".

````
cmake -DCMAKE_INSTALL_PREFIX=<install location> <OpenEXR source root> -G "selected generator" -DCMAKE_PREFIX_PATH=<paths to dependencies - zlib etc>
````


The available options are:

* OPENEXR_BUILD_ILMBASE (ON)
By default, IlmBase is always built.

* OPENEXR_BUILD_OPENEXR (ON)
By default, OpenEXR is always built.

* OPENEXR_BUILD_PYTHON_LIBS (ON)
By default, the Python bindings will be built.

* OPENEXR_BUILD_VIEWERS (OFF)
By default, the viewers are not built, as they have not been updated for
modern OpenGL.

* OPENEXR_BUILD_SHARED (ON)
* OPENEXR_BUILD_STATIC (OFF)
The build can be configured to create either shared libraries, or static 
libraries, or both.

* OPENEXR_NAMESPACE_VERSIONING (ON)
OpenEXR symbols will be contained within a namespace

* OPENEXR_FORCE_CXX03 (OFF)
C++03 compatibility is possible as an option

* OPENEXR_ENABLE_TESTS (ON)
By default, the tests will be built.

* OPENEXR_PYTHON_MAJOR, OPENEXR_PYTHON_MINOR "2", "7"
By default, OpenEXR is built against Python 2.7.x.

## Documentation

Documentation is available at http://www.openexr.com/documentation.html.

