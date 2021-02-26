# Building and Installation

## Download

To build the latest release of OpenEXR, begin by downloading the
source from the Releases page
https://github.com/AcademySoftwareFoundation/openexr/tarball/v2.5.1.

To build from the latest development version, which may not be stable,
download the master branch via
https://github.com/AcademySoftwareFoundation/openexr/tarball/master, and extract the
contents via ``tar``.

You can download the repository tarball file either via a browser, or
on the Linux/macOS via the command line using ``wget`` or ``curl``:

    % curl -L https://github.com/AcademySoftwareFoundation/openexr/tarball/master | tar xv

This will produce a source directory named
``openexr-openexr-<abbreviated-SHA-1-checksum>``.

Alternatively, clone the GitHub repo directly via:

    % git clone https://github.com/AcademySoftwareFoundation/openexr.git

In the instructions that follow, we will refer to the top-level
directory of the source code tree as ``$source_directory``.

## Prerequisites

Make sure these are installed on your system before building OpenEXR:

* OpenEXR requires CMake version 3.10 or newer
  - NB: CMake 3.12 is required for configuring to build static and
    shared libraries in the same config (object library support)
* C++ compiler that supports C++11
* Zlib (auto compiled if not found)
* Imath (auto compiled if not found)

The instructions that follow describe building OpenEXR with CMake, but
you can also build and install OpenEXR via the autoconf
bootstrap/configure utilities, described below.

## Linux/macOS Quick Start

To build via CMake, first choose a location for the build directory,
which we will refer to as ``$build_directory``.

    % mkdir $build_directory
    % cd $build_directory
    % cmake $source_directory
    % make
    % make install

Note that the CMake configuration prefers to apply an out-of-tree
build process, since there may be multiple build configurations
(i.e. debug and release), one per folder, all pointing at once source
tree, hence the ``$build_directory`` noted above, referred to in CMake
parlance as the *build directory*. You can place this directory
wherever you like.

See the CMake Configuration Options section below for the most common
configuration options especially the install directory. Note that with
no arguments, as above, ``make install`` installs the header files in
``/usr/local/include``, the object libraries in ``/usr/local/lib``, and the
executable programs in ``/usr/local/bin``.

## Windows Quick Start

Under Windows, if you are using a command line-based setup, such as
cygwin, you can of course follow the above. For Visual Studio, cmake
generators are "multiple configuration", so you don't even have to set
the build type, although you will most likely need to specify the
install location.  Install Directory By default, ``make install``
installs the headers, libraries, and programs into ``/usr/local``, but you
can specify a local install directory to cmake via the
``CMAKE_INSTALL_PREFIX`` variable:

    % cmake .. -DCMAKE_INSTALL_PREFIX=$install_directory

## Library Names

By default the installed libraries follow a pattern for how they are
named. This is done to enable multiple versions of the library to be
installed and targeted by different builds depending on the needs of
the project. A simple example of this would be to have different
versions of the library installed to allow for applications targeting
different VFX Platform years to co-exist.

If you are building dynamic libraries, once you have configured, built,
and installed the libraries, you should see the following pattern of
symlinks and files in the install lib folder:

    libHalf.so -> libHalf-$LIB_SUFFIX.so
    libHalf-$LIB_SUFFIX.so -> libHalf-$LIB_SUFFIX.so.$SO_MAJOR_VERSION
    libHalf-$LIB_SUFFIX.so.$SO_MAJOR_VERSION -> libHalf-$LIB_SUFFIX.so.$SO_FULL_VERSION
    libHalf-$LIB_SUFFIX.so.$SO_FULL_VERSION (actual file)

You can configure the LIB_SUFFIX, although it defaults to the library
major and minor version, so in the case of a 2.3 library, it would default
to 2_3. You would then link your programs against this versioned library
to have maximum safety (i.e. `-lHalf-2_3`), and the pkg-config and cmake
configuration files included with find_package should set this up.

## Sub-Libraries

OpenEXR consists of a number of libraries - OpenEXR, OpenEXRUtil,
IlmThread, Iex, and IexMath. To build and install OpenEXR, take care
to set the ``CMAKE_SYSTEM_PREFIX`` to the directory in which you
installed any custom Imath and/or zlib and ``CMAKE_INSTALL_PREFIX`` to
the directory in which to install OpenEXR:
 
    % mkdir $build_directory
    % cd $build_directory
    % cmake -DCMAKE_SYSTEM_PREFIX=$install_directory \ 
            -DCMAKE_INSTALL_PREFIX=$install_directory \ 
            $source_directory/OpenEXR
    % cmake --build . --target install --config Release

The libraries in OpenEXR follow the standard cmake setting of
``BUILD_SHARED_LIBS`` to control whether to build static or shared
libraries.

Please see cmake/OpenEXRSetup.cmake for other customization options.

## Custom Namespaces

If you are interested in controlling custom namespace declarations or
similar options, you are encouraged to look at the ``CMakeLists.txt``
infrastructure. The settings can be found in
``cmake/OpenEXRSetup.cmake``. As per usual, these settings can also be
seen and/or edited using any of the various gui editors for working
with cmake such as ``ccmake``, ``cmake-gui``, as well as some of the
IDEs in common use.

## Cross Compiling / Specifying Specific Compilers

When trying to either cross-compile for a different platform, or for
tasks such as specifying a compiler set to match the VFX reference
platform (https://vfxplatform.com/), cmake provides the idea of a
toolchain which may be useful instead of having to remember a chain of
configuration options. It also means that platform-specific compiler
names and options are out of the main cmake file, providing better
isolation.

A toolchain file is simply just a cmake script that sets all the
compiler and related flags and is run very early in the configuration
step to be able to set all the compiler options and such for the
discovery that cmake performs automatically. These options can be set
on the command line still if that is clearer, but a theoretical
toolchain file for compiling for VFX Platform 2015 is provided in the
source tree at cmake/Toolchain-Linux-VFX_Platform15.cmake which will
hopefully provide a guide how this might work.

For cross-compiling for additional platforms, there is also an
included sample script in cmake/Toolchain-mingw.cmake which shows how
cross compiling from Linux for Windows may work. The compiler names
and paths may need to be changed for your environment.

More documentation:

* Toolchains: https://cmake.org/cmake/help/v3.12/manual/cmake-toolchains.7.html
* Cross compiling: https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/

## CMake Configuration Options

The default CMake configuration options are stored in
``cmake/OpenEXRSetup.cmake``. To see a complete set of option
variables, run:

    % cmake -LAH $source_directory

You can customize these options three ways:

1. Modify the ``.cmake`` files in place.
2. Use the UI ``cmake-gui`` or ``ccmake``.
2. Specify them as command-line arguments when you invoke cmake.

### Verbose Output Options:

* **CMAKE\_EXPORT\_COMPILE\_COMMANDS**

  Enable/Disable output of compile commands during generation. Default is OFF.

* **CMAKE\_VERBOSE\_MAKEFILE**

  Echo all compile commands during make. Default is OFF.

### Compiler Options:

* **OPENEXR\_CXX\_STANDARD**

  C++ standard to compile against. This obeys the global ``CMAKE_CXX_STANDARD`` but doesn’t force the global setting to enable sub-project inclusion. Default is ``14``.

### Library Naming Options:

* **OPENEXR\_LIB\_SUFFIX**

  Append the given string to the end of all the OpenEXR libraries. Default is ``-<major>_<minor>`` version string. Please see the section on library names

### Namespace Options:

* **OPENEXR\_IMF\_NAMESPACE**
 
  Public namespace alias for OpenEXR. Default is ``Imf``.

* **OPENEXR\_INTERNAL\_IMF\_NAMESPACE**
 
  Real namespace for OpenEXR that will end up in compiled symbols. Default is ``Imf\_<major>\_<minor>``.

* **OPENEXR\_NAMESPACE\_CUSTOM**
 
  Whether the namespace has been customized (so external users know)


* **IEX\_NAMESPACE**

  Public namespace alias for Iex. Default is ``Iex``.

* **IEX\_INTERNAL\_NAMESPACE**
 
  Real namespace for Iex that will end up in compiled symbols. Default is ``Iex\_<major>\_<minor>``.

* **IEX\_NAMESPACE\_CUSTOM**
 
  Whether the namespace has been customized (so external users know)


* **ILMTHREAD\_NAMESPACE**

  Public namespace alias for IlmThread. Default is ``IlmThread``.

* **ILMTHREAD\_INTERNAL\_NAMESPACE**
 
  Real namespace for IlmThread that will end up in compiled symbols. Default is ``IlmThread\_<major>\_<minor>``.

* **ILMTHREAD\_NAMESPACE\_CUSTOM**
 
  Whether the namespace has been customized (so external users know)

* **BUILD\_TESTING**
 
  Build the testing tree. Default is ON.  Note that this causes the test suite to be compiled, but it is not executed.

* **OPENEXR\_RUN\_FUZZ\_TESTS**
 
  Controls whether to include the fuzz tests (very slow). Default is OFF.

### Additional CMake Options:

See the cmake documentation for more information (https://cmake.org/cmake/help/v3.12/)

* **CMAKE\_BUILD\_TYPE**

  For builds when not using a multi-configuration generator. Available values: ``Debug``, ``Release``, ``RelWithDebInfo``, ``MinSizeRel``

* **BUILD\_SHARED\_LIBS**

  This is the primary control whether to build static libraries or
  shared libraries / dlls (side note: technically a convention, hence
  not an official ``CMAKE\_`` variable, it is defined within cmake and
  used everywhere to control this static / shared behavior)

* For forcing particular compilers to match VFX platform requirements

  ** CMAKE\_CXX\_COMPILER**

  ** CMAKE\_C\_COMPILER**

  ** CMAKE\_LINKER**

     All the related cmake compiler flags (i.e. CMAKE\_CXX_FLAGS, CMAKE_CXX_FLAGS_DEBUG)

  ** CMAKE\_INSTALL\_RPATH**

     For non-standard install locations where you don’t want to have to set ``LD_LIBRARY_PATH`` to use them

## Cmake Tips and Tricks:

If you have ninja (https://ninja-build.org/) installed, it is faster than make. You can generate ninja files using cmake when doing the initial generation:

    % cmake -G “Ninja” ..

If you would like to confirm compile flags, you don’t have to specify the verbose configuration up front, you can instead run

    % make VERBOSE=1
