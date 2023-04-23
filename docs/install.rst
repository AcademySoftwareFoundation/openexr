..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _Install:

Install
========

.. toctree::
   :caption: Install
             
The OpenEXR library is available for download and installation in
binary form via package managers on many Linux distributions. See
`https://pkgs.org/download/openexr
<https://pkgs.org/download/openexr>`_ for a complete list. 

RHEL/CentOS:

.. code-block::

    $ sudo yum makecache
    $ sudo yum install OpenEXR

Ubuntu:

.. code-block::

    $ sudo apt-get update
    $ sudo apt-get install openexr

Beware that some distributions are out of date and only provide
distributions of outdated releases OpenEXR. We recommend against using
OpenEXR v2, and we *strongly* recommend against using OpenEXR v1.

On macOS, install via `Homebrew <https://formulae.brew.sh/formula/openexr>`_:

.. code-block::

   $ brew install openexr

We do not recommend installation via
`Macports <https://ports.macports.org/port/openexr>`_ because the
distribution is out of date.

Also note that the official OpenEXR project does not provide supported
python bindings. ``pip install openexr`` installs the `openexrpython
<https://github.com/jamesbowman/openexrpython>`_ module, which is not
affiliated with the OpenEXR project or the ASWF. Please direct
questions there.

Build from Source
-----------------

OpenEXR builds on Linux, macOS, Microsoft Windows via CMake, and is
cross-compilable on other systems.

Download the source from the `GitHub releases page
<https://github.com/AcademySoftwareFoundation/openexr/releases>`_
page, or clone the `repo <https://github.com/AcademySoftwareFoundation/openexr>`_.

The ``release`` branch of the repo always points to the most advanced
release.


Prerequisites
~~~~~~~~~~~~~

Make sure these are installed on your system before building OpenEXR:

* OpenEXR requires CMake version 3.12 or newer
* C++ compiler that supports C++11
* Imath (auto fetched by CMake if not found) (https://github.com/AcademySoftwareFoundation/openexr)
* libdeflate source code (auto fetched by CMake if not found) (https://github.com/ebiggers/libdeflate)

The instructions that follow describe building OpenEXR with CMake.

Note that as of OpenEXR 3, the Gnu autoconf bootstrap/configure build
system is no longer supported.

Linux/macOS
~~~~~~~~~~~

To build via CMake, you need to first identify three directories:

1. The source directory, i.e. the top-level directory of the
   downloaded source archive or cloned repo, referred to below as ``$srcdir``
2. A temporary directory to hold the build artifacts, referred to below as
   ``$builddir``
3. A destination directory into which to install the
   libraries and headers, referred to below as ``$installdir``.  

To build:
.. code-block::

    $ cd $builddir
    $ cmake $srcdir --install-prefix $installdir
    $ cmake --build $builddir --target install --config Release

Note that the CMake configuration prefers to apply an out-of-tree
build process, since there may be multiple build configurations
(i.e. debug and release), one per folder, all pointing at once source
tree, hence the ``$builddir`` noted above, referred to in CMake
parlance as the *build directory*. You can place this directory
wherever you like.

See the CMake Configuration Options section below for the most common
configuration options especially the install directory. Note that with
no arguments, as above, ``make install`` installs the header files in
``/usr/local/include``, the object libraries in ``/usr/local/lib``, and the
executable programs in ``/usr/local/bin``.

Windows
~~~~~~~

Under Windows, if you are using a command line-based setup, such as
cygwin, you can of course follow the above. For Visual Studio, cmake
generators are "multiple configuration", so you don't even have to set
the build type, although you will most likely need to specify the
install location.  Install Directory By default, ``make install``
installs the headers, libraries, and programs into ``/usr/local``, but you
can specify a local install directory to cmake via the
``CMAKE_INSTALL_PREFIX`` variable:

.. code-block::

    $ cmake .. -DCMAKE_INSTALL_PREFIX=$openexr_install_directory

Library Names
-------------

By default the installed libraries follow a pattern for how they are
named. This is done to enable multiple versions of the library to be
installed and targeted by different builds depending on the needs of
the project. A simple example of this would be to have different
versions of the library installed to allow for applications targeting
different VFX Platform years to co-exist.

If you are building dynamic libraries, once you have configured, built,
and installed the libraries, you should see the following pattern of
symlinks and files in the install lib folder:

.. code-block::

    libOpenEXR.so -> libOpenEXR-3_1.so
    libOpenEXR-3_1.so -> libOpenEXR-3_1.so.30
    libOpenEXR-3_1.so.30 -> libOpenEXR-3_1.so.30.3.0
    libOpenEXR-3_1.so.30.3.0 (the shared object file)
    
The ``-3_1`` suffix encodes the major and minor version, which can be
configured via the ``OPENEXR_LIB_SUFFIX`` CMake setting. The ``30``
corresponds to the so version, or in ``libtool`` terminology the
``current`` shared object version; the `3` denotes the ``libtool``
``revision``, and the ``0`` denotes the ``libtool`` ``age``. See the
`libtool
<https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html#Updating-version-info>`_
documentation for more details.

Imath Dependency
----------------

OpenEXR depends on `Imath
<https://github.com/AcademySoftwareFoundation/Imath>`_. If a suitable
installation of Imath cannot be found, CMake will automatically
download it at configuration time. To link against an existing
installation of Imath, add the Imath directory to the
``CMAKE_PREFIX_PATH`` setting:
 
.. code-block::

    $ mkdir $build_directory
    $ cd $build_directory
    $ cmake -DCMAKE_PREFIX_PATH=$imath_install_directory \
            -DCMAKE_INSTALL_PREFIX=$openexr_install_destination \
            $openexr_source_directory
    $ cmake --build . --target install --config Release

Alternatively, you can specify the ``Imath_DIR`` variable:

.. code-block::

    $ mkdir $build_directory
    $ cd $build_directory
    $ cmake -DImath_DIR=$imath_config_directory \
            -DCMAKE_INSTALL_PREFIX=$openexr_install_destination \
            $openexr_source_directory
    $ cmake --build . --target install --config Release

Note that ``Imath_DIR`` should point to the directory that includes
the ``ImathConfig.cmake`` file, which is typically the
``lib/cmake/Imath`` folder of the root install directory where Imath
is installed.

See below for other customization options.

Porting Applications from OpenEXR v2 to v3
------------------------------------------

See the :doc:`PortingGuide` for details about differences from previous
releases and how to address them. Also refer to the porting guide for
details about changes to Imath.

Building the Documentation
--------------------------

The OpenEXR technical documentation at `https://openexr.readthedocs.io
<https://openexr.readthedocs.io>`_ is generated via `Sphinx
<https://www.sphinx-doc.org>`_ with the `Breathe
<https://breathe.readthedocs.io>`_ extension using information
extracted from header comments by `Doxygen <https://www.doxygen.nl>`_.

To build the documentation locally from the source headers and
``.rst`` files, set the CMake option ``BUILD_DOCS=ON``. This adds
``Doxygen`` and ``Sphinx`` CMake targets and enables building the docs
by default.  generation is off by default.

Building the documentation requires that ``sphinx``, ``breathe``, and
``doxygen`` are installed. It further requires the `sphinx-press-theme
<https://pypi.org/project/sphinx-press-theme>`_, as indicated in the
`requirements.txt
<https://github.com/AcademySoftwareFoundation/openexr/blob/main/docs/requirements.txt>`_
file.

See the `doxygen downloads page
<https://www.doxygen.nl/download.html>`_ for how to install it. Binary
distributions are available for many systems, so you likely do not
need to build from source. On Debian/Ubuntu, for example:

.. code-block::

   $ sudo apt-get install doxygen

Similarly, see the `sphinx installation page
<https://www.sphinx-doc.org/en/master/usage/installation.html>`_ for
how to install it. On Debian/Ubuntu:

.. code-block::

   $ sudo apt-get install python3-sphinx

And to install `sphinx-press-theme
<https://pypi.org/project/sphinx-press-theme>`_:

.. code-block::

   $ pip3 install sphinx_press_theme

Note that the `https://openexr.readthedocs.io <https://openexr.readthedocs.io>`_
documentation takes the place of the formerly distributed .pdf
documents in the ``docs`` folder, although readthedocs supports
downloading of documentation in pdf format, for those who prefer it
that way.

CMake Build-time Configuration Options
--------------------------------------


The default CMake configuration options are stored in
``cmake/OpenEXRSetup.cmake``. To see a complete set of option
variables, run:

.. code-block::

    $ cmake -LAH $openexr_source_directory

You can customize these options three ways:

1. Modify the ``.cmake`` files in place.
2. Use the UI ``cmake-gui`` or ``ccmake``.
3. Specify them as command-line arguments when you invoke cmake.

Library Naming Options
~~~~~~~~~~~~~~~~~~~~~~

* ``OPENEXR_LIB_SUFFIX``

  Append the given string to the end of all the OpenEXR
  libraries. Default is ``-<major>_<minor>`` version string. Please
  see the section on library names

Imath Dependency
~~~~~~~~~~~~~~~~

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
  
Namespace Options
~~~~~~~~~~~~~~~~~

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

Component Options
~~~~~~~~~~~~~~~~~

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

Additional CMake Options
~~~~~~~~~~~~~~~~~~~~~~~~

See the CMake documentation for more information (https://cmake.org/cmake/help/v3.12/).

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

Cross Compiling / Specifying Specific Compilers
-----------------------------------------------

When trying to either cross-compile for a different platform, or for
tasks such as specifying a compiler set to match the `VFX reference
platform <https://vfxplatform.com>`_, cmake provides the idea of a
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
source tree at ``cmake/Toolchain-Linux-VFX_Platform15.cmake`` which
will hopefully provide a guide how this might work.

For cross-compiling for additional platforms, there is also an
included sample script in ``cmake/Toolchain-mingw.cmake`` which shows
how cross compiling from Linux for Windows may work. The compiler
names and paths may need to be changed for your environment.

More documentation:

* Toolchains: https://cmake.org/cmake/help/v3.12/manual/cmake-toolchains.7.html
* Cross compiling: https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/

Ninja
-----

If you have `Ninja <https://ninja-build.org>`_ installed, it is faster
than make. You can generate ninja files using cmake when doing the
initial generation:

.. code-block::

    $ cmake -G “Ninja” ..

