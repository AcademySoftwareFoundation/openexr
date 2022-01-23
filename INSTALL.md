# Building and Installation

## Download

To build the latest release of OpenEXR, begin by downloading the
source from the GitHub Releases page: 
https://github.com/AcademySoftwareFoundation/openexr/releases.

To build from the latest development version, which may not be stable,
clone the GitHub repo and build from the master branch:

    % git clone https://github.com/AcademySoftwareFoundation/openexr

You can alternatively download the repository tarball file either via
a browser, or on the Linux/macOS via the command line using ``wget``
or ``curl``:

    % curl -L https://github.com/AcademySoftwareFoundation/openexr/tarball/master | tar xv

In the instructions that follow, we will refer to the top-level
directory of the source code tree as ``$openexr_source_directory``.

## Prerequisites

Make sure these are installed on your system before building OpenEXR:

* OpenEXR requires CMake version 3.12 or newer
* C++ compiler that supports C++11
* zlib 
* Imath (auto fetched by CMake if not found)

The instructions that follow describe building OpenEXR with CMake.

Note that as of OpenEXR 3, the Gnu autoconf bootstrap/configure build
system is no longer supported.

## Linux/macOS Quick Start

To build via CMake, first choose a location for the build directory,
which we will refer to as ``$build_directory``.

    % mkdir $build_directory
    % cd $build_directory
    % cmake $openexr_source_directory
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

    % cmake .. -DCMAKE_INSTALL_PREFIX=$openexr_install_directory

## Porting Applications from OpenEXR v2 to v3

See the [porting
guide](https://github.com/AcademySoftwareFoundation/Imath/blob/master/docs/PortingGuide2-3.md)
for details about differences from previous releases and how to
address them. Also refer to the porting guide for details about
changes to Imath.

## Documentation

The OpenEXR technical documentation at
[openexr.readthedocs.io](https://openexr.readthedocs.io) is generated
via [Sphinx](https://www.sphinx-doc.org) with the
[Breathe](https://breathe.readthedocs.io) extension using information
extracted from header comments by [Doxgen](https://www.doxygen.nl).

To build the documentation locally from the source headers and
``.rst`` files, set the CMake option ``DOCS=ON``. This adds
``Doxygen`` and ``Sphinx`` CMake targets. Local documentation
generation is off by default.

Building the documentation requires that sphinx, breathe, and doxygen
are installed.

Note that the [openexr.readthedocs.io](https://openexr.readthedocs.io)
documentation takes the place of the formerly distributed .pdf
documents in the ``docs`` folder, although readthedocs supports
downloading of documentation in pdf format, for those who prefer it
that way.

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

    libOpenEXR.so -> libOpenEXR-3_1.so
    libOpenEXR-3_1.so -> libOpenEXR-3_1.so.30
    libOpenEXR-3_1.so.30 -> libOpenEXR-3_1.so.30.3.0
    libOpenEXR-3_1.so.30.3.0 (the shared object file)
    
The ``-3_1`` suffix encodes the major and minor version, which can be
configured via the ``OPENEXR_LIB_SUFFIX`` CMake setting. The "30"
corresponds to the so version, or in ``libtool`` terminology the
_current_ shared object version; the "3" denotes the ``libtool``
_revision_, and the "0" denotes the ``libtool`` _age_. See the
[``libtool``](https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html#Updating-version-info)
documentation for more details.

## Imath Dependency

OpenEXR depends on
[Imath](https://github.com/AcademySoftwareFoundation/Imath). If a
suitable installation of Imath cannot be found, CMake will
automatically download it at configuration time. To link against an
existing installation of Imath, add the Imath directory to the
``CMAKE_PREFIX_PATH`` setting:
 
    % mkdir $build_directory
    % cd $build_directory
    % cmake -DCMAKE_PREFIX_PATH=$imath_install_directory \
            -DCMAKE_INSTALL_PREFIX=$openexr_install_destination \
            $openexr_source_directory
    % cmake --build . --target install --config Release

Alternatively, you can specify the ``Imath_DIR`` variable:

    % mkdir $build_directory
    % cd $build_directory
    % cmake -DImath_DIR=$imath_config_directory \
            -DCMAKE_INSTALL_PREFIX=$openexr_install_destination \
            $openexr_source_directory
    % cmake --build . --target install --config Release

Note that ``Imath_DIR`` should point to the directory that includes
the ``ImathConfig.cmake`` file, which is typically the
``lib/cmake/Imath`` folder of the root install directory where Imath
is installed.

Please see ``cmake/OpenEXRSetup.cmake`` for other customization options.

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
tasks such as specifying a compiler set to match the [VFX reference
platform](https://vfxplatform.com), cmake provides the idea of a
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

    % cmake -LAH $openexr_source_directory

You can customize these options three ways:

1. Modify the ``.cmake`` files in place.
2. Use the UI ``cmake-gui`` or ``ccmake``.
2. Specify them as command-line arguments when you invoke cmake.

### Library Naming Options:

* ``OPENEXR_LIB_SUFFIX``

  Append the given string to the end of all the OpenEXR
  libraries. Default is ``-<major>_<minor>`` version string. Please
  see the section on library names

### Imath Dependency:

* ``CMAKE_PREFIX_PATH``

  The standard CMake path in which to
  search for dependencies, Imath in particular.  A comma-separated
  path. Add the root directory where Imath is installed.

* ``Imath_DIR``

  The config directory where Imath is installed. An alternative to
  using ``CMAKE_PREFIX_PATH``.  Note that ``Imath_DIR`` should
  be set to the directory that includes the ``ImathConfig.cmake``
  file, which is typically the ``lib/cmake/Imath`` folder of the root
  install directory.
  
### Namespace Options:

* ``OPENEXR_IMF_NAMESPACE``

  Public namespace alias for OpenEXR. Default is ``Imf``.

* ``OPENEXR_INTERNAL_IMF_NAMESPACE``

  Real namespace for OpenEXR that will end up in compiled
  symbols. Default is ``Imf_<major>_<minor>``.

* ``OPENEXR_NAMESPACE_CUSTOM``

  Whether the namespace has been customized (so external users know)

* ``IEX_NAMESPACE``

  Public namespace alias for Iex. Default is ``Iex``.

* ``IEX_INTERNAL_NAMESPACE``

  Real namespace for Iex that will end up in compiled symbols. Default
  is ``Iex_<major>_<minor>``.

* ``IEX_NAMESPACE_CUSTOM``

  Whether the namespace has been customized (so external users know)

* ``ILMTHREAD_NAMESPACE``

  Public namespace alias for IlmThread. Default is ``IlmThread``.

* ``ILMTHREAD_INTERNAL_NAMESPACE``

  Real namespace for IlmThread that will end up in compiled
  symbols. Default is ``IlmThread_<major>_<minor>``.

* ``ILMTHREAD_NAMESPACE_CUSTOM``

  Whether the namespace has been customized (so external users know)

### Component Options:

* ``BUILD_TESTING``

  Build the testing tree. Default is ``ON``.  Note that
  this causes the test suite to be compiled, but it is not
  executed. To execute the suite, run "make test".

* ``OPENEXR_RUN_FUZZ_TESTS``

  Controls whether to include the fuzz tests (very slow). Default is ``OFF``.

* ``OPENEXR_BUILD_TOOLS``

  Build and install the binary programs (exrheader, exrinfo,
  exrmakepreview, etc). Default is ``ON``.
  
* ``OPENEXR_INSTALL_EXAMPLES``

  Build and install the example code. Default is ``ON``.

### Additional CMake Options:

See the cmake documentation for more information
(https://cmake.org/cmake/help/v3.12/)

* ``CMAKE_BUILD_TYPE``

  For builds when not using a multi-configuration generator. Available
  values: ``Debug``, ``Release``, ``RelWithDebInfo``, ``MinSizeRel``

* ``BUILD_SHARED_LIBS``

  This is the primary control whether to build static libraries or
  shared libraries / dlls (side note: technically a convention, hence
  not an official ``CMAKE_`` variable, it is defined within cmake and
  used everywhere to control this static / shared behavior)

* ``OPENEXR_CXX_STANDARD``

  C++ standard to compile against. This obeys the global
  ``CMAKE_CXX_STANDARD`` but doesn’t force the global setting to
  enable sub-project inclusion. Default is ``14``.

* ``CMAKE_CXX_COMPILER``

  The C++ compiler.        

* ``CMAKE_C_COMPILER``

  The C compiler.
  
* ``CMAKE_INSTALL_RPATH``

  For non-standard install locations where you don’t want to have to
  set ``LD_LIBRARY_PATH`` to use them

* ``CMAKE_EXPORT_COMPILE_COMMANDS``

  Enable/Disable output of compile commands during generation. Default
  is ``OFF``.

* ``CMAKE_VERBOSE_MAKEFILE``

  Echo all compile commands during make. Default is ``OFF``.

## Cmake Tips and Tricks:

If you have ninja (https://ninja-build.org/) installed, it is faster
than make. You can generate ninja files using cmake when doing the
initial generation:

    % cmake -G “Ninja” ..

If you would like to confirm compile flags, you don’t have to specify
the verbose configuration up front, you can instead run

    % make VERBOSE=1
