..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _Install:

Install
========

.. toctree::
   :caption: Install
             
Linux
-----

The OpenEXR library is available for download and installation in
binary form via package managers on many Linux distributions. 

Beware that some distributions are out of date and only provide
distributions of outdated releases OpenEXR. We recommend against using
OpenEXR v2, and we *strongly* recommend against using OpenEXR v1.

Refer to the current version of OpenEXR on various major Linux distros at
`repology.org <https://repology.org/project/openexr/versions>`_:

.. image:: https://repology.org/badge/vertical-allrepos/openexr.svg?exclude_unsupported=1&columns=4&header=OpenEXR%20Packaging%20Status&minversion=3.0
   :target: https://repology.org/project/openexr/versions

To install via ``yum`` on RHEL/CentOS:

.. code-block::

    % sudo yum makecache
    % sudo yum install OpenEXR

To install via ``apt-get`` on Ubuntu:

.. code-block::

    % sudo apt-get update
    % sudo apt-get install openexr

macOS
-----

On macOS, install via `Homebrew <https://formulae.brew.sh/formula/openexr>`_:

.. code-block::

   % brew install openexr

Alternatively, you can install on macOS via `MacPorts
<https://ports.macports.org/port/openexr>`_:

.. code-block::

   % port install openexr

Windows
-------

Install via `vcpkg <https://vcpkg.io/en/packages>`_:

.. code-block::

   % .\vcpkg install openexr


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

* OpenEXR requires CMake version 3.14 or newer
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

    % cd $builddir
    % cmake $srcdir --install-prefix $installdir
    % cmake --build $builddir --target install --config Release

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

    % cmake .. -DCMAKE_INSTALL_PREFIX=$openexr_install_directory

Library Names
-------------

By default, libraries are installed with the following names/symlinks:

.. code-block::

    libOpenEXR.so -> libOpenEXR.so.31
    libOpenEXR.so.$SOVERSION -> libOpenEXR.so.$SOVERSION.$RELEASE
    libOpenEXR.so.$SOVERSION.$RELEASE (the shared object file)

The ``SOVERSION`` number identifies the ABI version. Each OpenEXR
release that changes the ABI in backwards-incompatible ways increases
this number. By policy, this changes only for major and minor
releases, never for patch releases. ``RELEASE`` is the
``MAJOR.MINOR.PATCH`` release name. For example, the resulting shared
library filename is ``libOpenEXR.so.31.3.2.0`` for OpenEXR release
v3.2.0. This naming scheme reinforces the correspondence between the
real filename of the ``.so`` and the release it corresponds to.

Library Suffix
~~~~~~~~~~~~~~

The ``OPENEXR_LIB_SUFFIX`` CMake option designates a suffix for the
library and appears between the library base name and the
``.so``. This defaults to encode the major and minor version, as in
``-3_1``:

.. code-block::

    libOpenEXR.so -> libOpenEXR-3_1.so
    libOpenEXR-3_1.so -> libOpenEXR-3_1.so.30
    libOpenEXR-3_1.so.30 -> libOpenEXR-3_1.so.30.3.2.0
    libOpenEXR-3_1.so.30.3.2.0 (the shared object file)
    
Imath Dependency
----------------

OpenEXR depends on `Imath
<https://github.com/AcademySoftwareFoundation/Imath>`_. If a suitable
installation of Imath cannot be found, CMake will automatically
download it at configuration time. To link against an existing
installation of Imath, add the Imath directory to the
``CMAKE_PREFIX_PATH`` setting:
 
.. code-block::

    % mkdir $build_directory
    % cd $build_directory
    % cmake -DCMAKE_PREFIX_PATH=$imath_install_directory \
            -DCMAKE_INSTALL_PREFIX=$openexr_install_destination \
            $openexr_source_directory
    % cmake --build . --target install --config Release

Alternatively, you can specify the ``Imath_DIR`` variable:

.. code-block::

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

See below for other customization options.

Porting Applications from OpenEXR v2 to v3
------------------------------------------

See the :doc:`PortingGuide` for details about differences from previous
releases and how to address them. Also refer to the porting guide for
details about changes to Imath.

Building the Website
--------------------

The `https://openexr.com <https://openexr.com>`_ website is generated
via `Sphinx <https://www.sphinx-doc.org>`_ with the `Breathe
<https://breathe.readthedocs.io>`_ extension, using the `sphinx-press-theme
<https://pypi.org/project/sphinx-press-theme>`_, and is hosted by
`readthedocs <https://readthedocs.org/projects/openexr>`_. The website
source is in `restructured text
<https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html>`_
in the ``website`` directory.  

To build the website locally from the source ``.rst`` files, set the
CMake option ``BUILD_WEBSITE=ON``. This adds the ``website`` CMake
target. Generation is off by default.

Building the website requires that ``sphinx``, ``breathe``, and
``doxygen`` are installed. It further requires the `sphinx-press-theme
<https://pypi.org/project/sphinx-press-theme>`_. Complete dependencies
are described in the `requirements.txt
<https://github.com/AcademySoftwareFoundation/imath/blob/main/website/requirements.txt>`_
file. 

On Debian/Ubuntu Linux:

.. code-block::

    % apt-get install doxygen python3-sphinx
    % pip3 install breathe
    % pip3 install sphinx_press_theme
   
    % mkdir _build
    % cd _build
    % cmake .. -DBUILD_WEBSITE=ON
    % cmake --build . --target website 

CMake Build-time Configuration Options
--------------------------------------

The default CMake configuration options are stored in
``cmake/OpenEXRSetup.cmake``. To see a complete set of option
variables, run:

.. code-block::

    % cmake -LAH $openexr_source_directory

You can customize these options three ways:

1. Modify the ``.cmake`` files in place.
2. Use the UI ``cmake-gui`` or ``ccmake``.
3. Specify them as command-line arguments when you invoke cmake.

Uninstall
~~~~~~~~~

If you did a binary install of OpenEXR via a package manager
(`apt-get`, `yum`, `port`, `brew`, etc), use the package manager to
uninstall.

If you have installed from source, *and you still have the build
tree from which you installed*, you can uninstall via: 

.. code-block::

    % cmake --build $builddir --target uninstall

or if using ``make``:

.. code-block::

    % make uninstall

The `uninstall` relies on CMake's `install_manifest.txt` for the record
of what was installed.

Library Naming Options
~~~~~~~~~~~~~~~~~~~~~~

* ``OPENEXR_LIB_SUFFIX``

  Append the given string to the end of all the OpenEXR
  libraries. Default is ``-<major>_<minor>`` version string. Please
  see the section on library names

Imath Dependency
~~~~~~~~~~~~~~~~

* ``CMAKE_PREFIX_PATH``

  The standard CMake path in which to search for dependencies, Imath
  in particular.  A comma-separated path. Add the root directory where
  Imath is installed.

* ``Imath_DIR``

  The config directory where Imath is installed. An alternative to
  using ``CMAKE_PREFIX_PATH``.  Note that ``Imath_DIR`` should
  be set to the directory that includes the ``ImathConfig.cmake``
  file, which is typically the ``lib/cmake/Imath`` folder of the root
  install directory.
  
* ``OPENEXR_IMATH_REPO`` and ``OPENEXR_IMATH_TAG``

  The github Imath repo to auto-fetch if an installed library cannot
  be found, and the tag to sync it to.  The default repo is
  ``https://github.com/AcademySoftwareFoundation/Imath.git`` and the
  tag is specific to the OpenEXR release. The internal build is
  configured as a CMake subproject.

* ``OPENEXR_FORCE_INTERNAL_IMATH``

  If set to ``ON``, force auto-fetching and internal building of Imath
  using ``OPENEXR_IMATH_REPO`` and ``OPENEXR_IMATH_TAG``. This means
  do *not* use any existing installation of Imath.

libdeflate Dependency
~~~~~~~~~~~~~~~~~~~~~

As of OpenEXR release v3.2, OpenEXR depends on 
`libdeflate <https://github.com/ebiggers/libdeflate>`_ for
DEFLATE-based compression. Previous OpenEXR releases relied on `zlib
<https://www.zlib.net>`_. Builds of OpenEXR can choose either an
``libdeflate`` installation, or CMake can auto-fetch the source and build it
internally. The internal build is linked statically, so no extra
shared object is produced.

* ``OPENEXR_DEFLATE_REPO`` and ``OPENEXR_DEFLATE_TAG``

  The GitHub ``libdeflate`` repo to auto-fetch if an installed library cannot
  be found, and the tag to sync it to. The default repo is
  ``https://github.com/ebiggers/libdeflate.git`` and the tag is
  ``v1.18``. The internal build is configured as a CMake subproject.

* ``OPENEXR_FORCE_INTERNAL_DEFLATE``

  If set to ``ON``, force auto-fetching and internal building of
  ``libdeflate`` using ``OPENEXR_DEFLATE_REPO`` and
  ``OPENEXR_DEFLATE_TAG``. This means do *not* use any existing
  installation of ``libdeflate``.

Test Images Dependency
~~~~~~~~~~~~~~~~~~~~~~

The OpenEXR test suite relies on images from the `online test image
gallery
<https://github.com/AcademySoftwareFoundation/openexr-images>`_, which CMake
automatically downloads during configuration.  You can provide an
alternate location for the test images via the ``OPENEXR_IMAGES_REPO``
and ``OPENEXR_IMAGES_TAG`` variables.

* ``OPENEXR_IMAGES_REPO`` and ``OPENEXR_IMAGES_TAG``

  The images repo to auto-fetch for the test suite, and the tag to
  sync it to.  The default repo is
  ``https://github.com/AcademySoftwareFoundation/openexr-images.git``
  and the tag is ``v1.0``. 

Note that you can void downloading images by specifying a repo on the
local filesystem via a ``file:`` url:

.. code-block::

    cmake -DOPENEXR_IMAGES_REPO=file:///my/clone/of/openexr-images -DOPENEXR_IMAGES_TAG=""

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

  Build the binary programs (exrheader, exrinfo,
  exrmakepreview, etc). Default is ``ON``.
  
* ``OPENEXR_INSTALL_TOOLS``

  Install the binary programs (exrheader, exrinfo,
  exrmakepreview, etc). Default is ``ON``.
  
* ``OPENEXR_INSTALL_DEVELOPER_TOOLS``

  Install the binary programs useful for developing
  and/or debugging OpenEXR itself (e.g. exrcheck).
  Default is ``OFF``.
  
* ``OPENEXR_BUILD_EXAMPLES``

  Build the example code. Default is ``ON``.

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

    % cmake -G “Ninja” ..

