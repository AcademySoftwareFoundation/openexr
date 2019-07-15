# Building and Installation

## TLDR; version

### For unix-ish systems (linux, mac, freebsd, etc.)

1. Download the all-in-one OpenEXR package.
2. Unpack it, open a shell and cd to the source tree where you unpacked
3. make sure you have cmake 3.12 or newer.
4. Run something like (on mac / windos):

```console
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make install
```

*For those who prefer autoconf, a configure based system is provided.
See below for more information.*

### For windows

Under windows, if you are using a command line-based setup, such as
cygwin, you can of course follow the above. For Visual Studio, cmake
generators are "multiple configuration", so you don't even have to
set the build type, although you will most likely need to specify
the install location.

## General Setup

Download the latest release of OpenEXR from github or
http://www.openexr.com/downloads.html.

To build OpenEXR, there are two methodologies that can be employed
when using the cmake-based build setup provided. The first, and
easiest, is as one big package. This means that you make your build
tree, point cmake at the source tree from the build tree, configure
as desired, and then compile / install away.

The alternate method is as separate sub-folders. This allows one to
only use IlmBase, for example. However, you must always compile
IlmBase first, as all the other folders depend on that. Then OpenEXR
must be compile prior to building the viewers, if you choose to build
the provided viewers. Finally, there are other programs in Contrib
that may be of interest.

This latter method is also the method followed by the traditional
autoconf / configure setup that is provided (see that section below).

## CMake Configuration and installation

The cmake configuration files (CMakeLists.txt and all that it adds)
represent current patterns and recommendations for "Modern CMake".
As such, we have set the minimum required version to **_3.12_**.
This is always an arbitrary line, but 3.12 is available on many
systems by default at this point, andhad some significant improvements
to the python discovery system for the PyIlmBase extension, and
so seemed like a good line to draw.

As mentioned above, there are two ways of working with the OpenEXR
repository with cmake. If you are downloading the individual package
tarballs, this will be decided for you, as they are packaged as
separate projects. However, the configuration process will remain
mostly the same.

When working with the "all-in-one" configuration, which would be the
default if you just grab the release from github, the top level
CMakeLists.txt just behaves as a "super project" in cmake parlance,
which basically makes sure cmake doesn't actually look for the
software being built in other places, but then just includes the
relevant directories.

As such, each top level folder added is in itself a cmake project.
This unfortunately causes a small amount of duplication for some
options, with the benefit of greater flexibility.

Once you have the software downloaded and are ready to build it, cmake
prefers to apply an out-of-tree configure and build process, and it
does so with one configuration (i.e. debug vs. release) per build
folder, all referencing the one source folder. The only concession to
this is that we offer the ability to build both static libraries and
shared libraries at once.

cmake is easy to use with all the defaults from a command prompt:

```console
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF ..
$ env DESTDIR=/path/to/install make install
```

This only changes a couple of options, setting the build type and
enables static-only libraries. You will notice that in this scenario,
we are also able to use a delayed version of the install path, and a
variable name that is more standard for GNU-related build tools.
**NB:** If you are building using shared libraries, and installing
to a non-system library, you may have to set CMAKE_INSTALL_PREFIX, or
set CMAKE_INSTALL_RPATH, depending on what your system requires for
run path resolution. Again, it is best to see the rpath handling docs
from cmake to understand how that is controlled, and then double
confirm that our settings are in line with what your install needs.
This specific document is currently [here](https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling)

A couple of tools are provided as part of cmake, **cmake-gui**
and **cmake**. The former is a windowed application, the latter a
console-based version of the same functionality. When starting from
scratch, these tools run an initial configuration step that determines
the setup / O.S. you have chosen based on the existing environment
(paths to compilers, etc). This step is required, as this set of tools
may change what options are provided (for example, the mac options for
creating a framework are only displayed on a mac).

However, once cmake has performed an initial environment scan and
configuration, it can display a UI with all the options and important
variables we have defined. Once happy with the configuration,
you generate the build scripts and compile the software (or load a
project up in an IDE). If you are against all UI programs and would
like to see this list, you can also run the following

```console
$ mkdir build
$ cd build
$ cmake ..
$ cmake -LAH
```

which will list all options and "advanced" variables (values that are
cached), at which point you could set those name on the command line
when re-configuring from scratch using the -DOPTION_NAME=VALUE
mechanism (NB: you may have to specify the type in special cases)

### Configuration options

For the OpenEXR sub projects, we are either
using the cmake accepted / generated names (i.e. BUILD_TESTING
from ctest), or have them centralized per sub project as much as
possible. Those files are:

[Top Level Choices of Modules](CMakeLists.txt)
[IlmBase Settings](IlmBase/config/IlmBaseSetup.cmake)
[OpenEXR Settings](OpenEXR/config/OpenEXRSetup.cmake)
[PyIlmBase Settings](PyIlmBase/config/PyIlmBaseSetup.cmake)

If you are interested in controlling custom namespace declarations
or similar options, you are encouraged to look at these files and
read the comments in addition to the tooltip string provided for
the gui application. 
As per usual, these settings can also be
seen and/or edited using any of the various gui editors for working with
cmake such as **ccmake**, **cmake-gui**, as well as some of the IDEs in
common use.

### Side note on continuous integration

The command line cmake tool is the driver behind all of the above,
and has a rich set of integration capabilities with continuous
integration systems, including the ability to make cross-platform
scripts instead of a mish-mash of bash and cmd batch scripts. If you
are integrating OpenEXR into an internal continuous integration
system, please see [this file](cmake/SampleCTestScript.cmake)
as a sample script that uses ctest to build a configuration and
test it.

### A note on cross-compiling

Some limited attempts have been made to test cross compiling with
OpenEXR. For cmake, this is usually done using a toolchain file.
A sample file is provided in the cmake folder at the top level of
OpenEXR which demonstrates a toolchain file for using mingw under
linux to cross compile for windows. This toolchain file will
probably need to be customized for your particular O.S.
distribution and desired output platform. Additionally, you may
have extra steps to get the code compiling if you do not have
an emulator to run the binaries.

[Sample Toolchain file](cmake/Toolchain-mingw.cmake)

## Traditional autoconf configuration (unix platforms)

The autoconf configuration assumes that each sub-element of
OpenEXR is it's own project. As such, it needs to be compiled
in stages, as was described as the second method to install
OpenEXR in the general setup above.

Like most autoconf / configure based setups, there is a set of
options that can be set via various flags to the ```configure```
script. ```configure --help``` will show you these prior to
configuring anything.

Unlike the very basic directions below, you can also do the
out-of-tree builds using the configure mechanism - just pre-make
and cd to that build folder, and add the path to the configure
script. If you are using the git repo, you will have to have
pre-run the bootstrap command to prepare the configure script.

**Important:** If you are checking out the git repository and
attempting to build and install, you need to insert a call to
```bootstrap``` in the steps below. This requires you to have
autoconf and all the autotools installed. A distribution
package does not have this requirement.

1. Compile IlmBase:

```console
$ cd <source root>/IlmBase
$ ./configure
$ make
$ make install
```

2. Compile OpenEXR (if desired)

```console
$ cd <source root>/OpenEXR
$ ./configure
$ make
$ make install
```

From here, you are free to compile and install the extra modules
included as desired - PyIlmBase and OpenEXR_Viewers using the
same pattern as above.

## Building with CMake

Alternatively, you can build with **cmake**, version 3.12 or newer. 
