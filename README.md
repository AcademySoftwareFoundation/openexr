OpenEXR
=======

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

License
-------

OpenEXR, including all contributions, is released under a modified BSD
license. Please see the ``LICENSE`` file accompanying the distribution
for the legal fine print.
      
OpenEXR Sub-modules
-------------------

The OpenEXR distribution consists of the following sub-modules:

* **IlmBase** - Utility libraries from Industrial Light & Magic: Half, Imath, Iex, IlmThread.
* **PyIlmBase** - Python bindings for the IlmBase libraries.
* **OpenEXR** - The core image library.
* **OpenEXR_Viewers** - Standard image viewing programs
* **Contrib** - Various plugins and utilities, contributed by the community.
    
Please see the ``README`` files of each of the individual directories for more information.

A collection of OpenEXR images is available from the adjacent repository
[openexr-images](https://github.com/openexr/openexr-images).

Dependencies
------------

OpenEXR depends on [zlib](https://zlib.net).

PyIlmBase depends on [boost-python](https://github.com/boostorg/python) and
optionally on [numpy](http://www.numpy.org).

In OpenEXR_Viewers:

* **exrdisplay** depends on [fltk](http://www.fltk.org/index.php)
* **playexr** depends on [Cg](https://developer.nvidia.com/cg-toolkit)

Web Resources
-------------

Main web page: http:://www.openexr.org

GitHub repository: http://www.github.com/openexr

Mail lists:

* **http://lists.nongnu.org/mailman/listinfo/openexr-announce** - OpenEXR-related announcements.

* **http://lists.nongnu.org/mailman/listinfo/openexr-user** - for discussion about OpenEXR applications or general questions.

* **http://lists.nongnu.org/mailman/listinfo/openexr-devel** - for developers using OpenEXR in their applications.

Building and Installation
-------------------------

Download the latest release of OpenEXR from
http://www.openexr.com/downloads.html.

To build the OpenEXR binaries from source, compile and install the
individual sub-models (IlmBase, PyIlmBase, OpenEXR, OpenEXR_Viewers),
according to the instructions in the respective ``README``
files. Build and install the IlmBase module first, then build and
install the OpenEXR module. Optionally, then build and install
PyIlmBase, OpenEXR_Viewers, and Contrib.

For the basic installation:

    cd <source root>/IlmBase
    ./configure
    make
    make install

    cd <source root>/OpenEXR
    ./configure
    make 
    make install

See the module ``README`` files for options to ``configure``.

#### Building from Git

Alternatively, you can download the latest release or the lastest
development branch directly from http://github.com/openexr.

After cloning the repo locally, generate the configuration scripts by
running the ``bootstrap`` script:

    cd <source root>/IlmBase
    ./bootstrap
    ./configure
    make
    make install

    cd <source root>/OpenExr
    ./bootstrap
    ./configure
    make
    make install

Building from git and ``bootstrap`` requires that **autoconf** is
installed.  Download and install it from
https://www.gnu.org/software/autoconf/autoconf.html.

#### Building with CMake

Alternatively, you can build with **cmake**, version 3.11 or newer. 

In the root ``CMakeLists.txt`` file, with -D options on the cmake
line, or by using a tools such as **ccmake** or **cmake-gui**,
configure the OpenEXR build. The options are detailed below.

Create a source root directory, cd into it, and run **cmake** to configure
the build.  Select an appropriate generator, such as "Unix Makefiles",
or "Visual Studio 15 2017 Win64". Then run **make** a the root
directory; this will build the appropriate submodules, according to
the settings of the **cmake** options, described below.

    cmake -DCMAKE_INSTALL_PREFIX=<install location> <OpenEXR source root> -G "selected generator" -DCMAKE_PREFIX_PATH=<paths to dependencies - zlib etc>
    make

The available options are:

* ``OPENEXR_BUILD_ILMBASE`` (ON)
By default, IlmBase is always built.

* ``OPENEXR_BUILD_OPENEXR`` (ON)
By default, OpenEXR is always built.

* ``OPENEXR_BUILD_PYTHON_LIBS`` (ON)
By default, the Python bindings will be built.

* ``OPENEXR_BUILD_VIEWERS`` (OFF)
By default, the viewers are not built, as they have not been updated for
modern OpenGL.

* ``OPENEXR_BUILD_SHARED`` (ON)
* ``OPENEXR_BUILD_STATIC`` (OFF)
The build can be configured to create either shared libraries, or static 
libraries, or both.

* ``OPENEXR_NAMESPACE_VERSIONING`` (ON)
OpenEXR symbols will be contained within a namespace

* ``OPENEXR_FORCE_CXX03`` (OFF)
C++03 compatibility is possible as an option

* ``OPENEXR_ENABLE_TESTS`` (ON)
By default, the tests will be built.

* ``OPENEXR_RUN_FUZZ_TESTS`` (OFF)
By default, the damaged input tests will NOT be run, due to their long
running time. If you wish to run them as part of "make test" (or equivalent
in your build system), then enable this. A "make fuzz" target will be
available to run the fuzz test regardless.

* ``OPENEXR_PYTHON_MAJOR``, ``OPENEXR_PYTHON_MINOR`` "2", "7"
By default, OpenEXR is built against Python 2.7.x.

## Documentation

Documentation is available at http://www.openexr.com/documentation.html.

