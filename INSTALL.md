# Building and Installation

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

## Building from Git

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

## Building with CMake

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

