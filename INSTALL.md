# Building and Installation

Download the latest release of OpenEXR from
http://www.openexr.com/downloads.html.

To build the OpenEXR binaries from source, compile and install the
individual sub-models (IlmBase, PyIlmBase, OpenEXR, OpenEXR_Viewers),
according to the instructions in the respective ``README``
files. Build and install the IlmBase module first, then build and
install the OpenEXR module. Optionally, then build and install
PyIlmBase, OpenEXR_Viewers, and Contrib.

For cmake users, there is also a top-level cmake file that enables
building all the sub-modules in one pass. See the section about
cmake below.

## Traditional autoconf configuration (unix platforms)

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

After cloning the repo locally, use the cmake path outlined later,
or if you prefer the autoconf system, generate the configuration
scripts by first running the ``bootstrap`` script:

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

Alternatively, you can build with **cmake**, version 3.12 or newer. 

There are two ways of working with the OpenEXR repository with cmake.
If you are downloading the individual package tarballs, this will be
working with the folders as separate projects. The other method is if
you download a tag, or just clone the repository from github, in which
case this is called "super project" mode in cmake parlance. This
involves a cmake setup at the top level above the individual folders
which aggregates the tree into one build system, which can be used to
build all the software as one set.

Either way, the process of configuring and building is similar. You
can accept the configuration defaults we have chosen, or customize
the settings via normal cmake mechanisms. But in the case of individual
folders, you will apply the process once per folder, but in the case of
using the super project mode, you will configure and run from just the
one build folder.

Cmake prefers to apply an out-of-tree configure and build process, where
there may be multiple build configurations (i.e. debug and release), one
per folder, all pointing at once source tree. OpenEXR assumes nothing
different, so for a unix-like system, the process might be:
    - mkdir build
    - cd build
    - cmake ..
    - make
    - make test
    - env DESTDIR=/path/to/install make install

Of course, there are a number of generators and other options that can
be specified. This is beyond the scope of this document, however all of
the sub-projects within OpenEXR (IlmBase, OpenEXR, PyIlmBase, ...)
additionally have configuration options.

The libraries in IlmBase and OpenEXR follow the standard cmake setting
of BUILD_SHARED_LIBS to control whether to build static or shared
libraries. However, they each have separate controls over whether to
build both shared AND static libraries as part of one configuration,
as well as other customization options.

If you are interested in controlling custom namespace declarations
or similar options, you are encouraged to look at the CMakeLists.txt
infrastructure. In particular, there has been an attempt to centralize
the settings into a common place to more easily see all of them in a
text editor. For IlmBase, this is config/IlmBaseSetup.cmake inside the
IlmBase tree. For OpenEXR, the settings will similarly be found in
config/OpenEXRSetup.cmake. As per usual, these settings can also be
seen and/or edited using any of the various gui editors for working with
cmake such as **ccmake**, **cmake-gui**, as well as some of the IDEs in
common use.

For cross-compiling for additional platforms, there is also and included
sample script in cmake/Toolchain-mingw.cmake which may work for you by
following the comments at the top of that file, although some editing
is likely to be needed to point cmake at the correct cross-compiler
binaries for your platform.
