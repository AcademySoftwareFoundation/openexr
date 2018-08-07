The IlmBase Libraries
=====================

The IlmBase libraries include the following:

* **Half** - a class that encapsulates our 16-bit floating-point format.

* **IlmThread** - a thread abstraction library for use with OpenEXR and
other software packages.  It currently supports pthreads and Windows
threads.

* **Imath** - 2D and 3D vectors, 3x3 and 4x4 matrices, quaternions
and other useful 2D and 3D math functions.

* **Iex** - an exception-handling library.

* **IexMath** - math exception types.

In addition, the distribution also includes confidence test libaries:
**HalfTest**, **IexTest**, and **ImathTest**.

If you have questions about using the IlmBase libraries, you may want
to join our developer mailing list.  See http://www.openexr.com for
details.

License
-------

IlmBase, including all contributions, is released under a modified BSD
license. Please see the ``LICENSE`` file accompanying the distribution
for the legal fine print.

Building and Installation
-------------------------

To configure the Makefiles, run the ``configure`` script:

    ./configure

For help with useful build options:

    ./configure --help

In particular, the ``--prefix=<install directory>`` option specifies a
directory into which to install the headers and libraries. By default,
headers and libraries are installed into ``/usr/local``.

To build the libraries after running ``configure``:

    make
    make install

#### Building from Git

If building directly from a cloned git repo, first generate the
configuration scripts by running ``bootstrap``, then ``configure`` and
``make``:

    cd <source root>/IlmBase
    ./bootstrap
    ./configure
    make
    make install

#### Building on Windows using **cmake**

To generate Visual Studio solution files and build the libraries:

1. Launch a command window, navigate to the ``IlmBase`` folder containing
   ``CMakeLists.txt``, and type the command:

       setlocal
       del /f CMakeCache.txt
       cmake -DCMAKE_INSTALL_PREFIX=<where you want to install the ilmbase builds>
           -G "Visual Studio 10 Win64" 
           ..\ilmbase

2. Navigate to ``IlmBase`` folder in Windows Explorer, open ``ILMBase.sln``
   and build the solution. When it builds successfully, right click
   ``INSTALL project`` and build. It will install the output to the path
   you set up at the previous step.

3. Go to http://www.zlib.net and download zlib.
	  
4. Launch a command window, navigate to the ``OpenEXR`` folder containing
   ``CMakeLists.txt``, and type the command:	  

       setlocal
       del /f CMakeCache.txt
       cmake 
           -DZLIB_ROOT=<zlib location>
           -DILMBASE_PACKAGE_PREFIX=<where you installed the ilmbase builds>
           -DCMAKE_INSTALL_PREFIX=<where you want to instal the openexr builds>
           -G "Visual Studio 10 Win64" ^
           ..\openexr

5. Navigate to ``OpenEXR`` folder in Windows Explorer, open
   ``OpenEXR.sln`` and build the solution. When it builds
   successfully, right click ``INSTALL project`` and build. It will
   install the output to the path you set up at the previous step.

#### Building on **macOS**

**macOS** supports multiple architectures. By default, IlmBase will be
built for the system doing the building. For example, if you build
IlmBase on an Intel system, the libraries will be built for Intel.

You can specify building for a different architecture, or multiple
architectures, by passing the ``--enable-osx-arch`` flag to
``configure``. Building for multiple architectures requires that
``--disable-dependency-tracking`` be passed as well.

For example, to build for Intel and PowerPC:

    ./configure --enable-osx-arch="i386 ppc" --disable-dependency-tracking.

To build "4-way universal" for 32-bit and 64-bit Intel and PowerPC:

    ./configure --enable-osx-arch="i386 ppc x86_64 ppc64" --disable-dependency-tracking.

For more information on universal builds, see:

    http://developer.apple.com/documentation/Porting/Conceptual/PortingUNIX/compiling/chapter_4_section_3.html

Earlier releases of **IlmBase** included an
``--enable-osx-universal-binaries`` switch, which specifies a two-way
universal build: Intel and PowerPC, 32-bit only.  This is still
available, but deprecated in favor of the more flexible
``--enable-osx-arch`` and ``--enable-osx-sdk`` switches.


#### Choosing an SDK on macOS

**macOS** allows you to specify one of several SDKs, or sysroots. This
allows you to target systems other than the system that your build
machine runs.

For example, if you are building on Mac OS X 10.4, but you need access
to features that were introduced in Mac OS X 10.5, you can build
against the Mac OS X 10.5 versions of system libraries and headers.

You can choose to build **IlmBase** with a specific SDK using the
``--enable-osx-sdk`` switch. For example:

    ./configure --enable-osx-sdk=MacOSX10.5.sdk

If you are building on Mac OS X 10.4 and want to build universal, you
will need to specify the universal version of the 10.4 SDK:
MacOSX10.4u.sdk. Otherwise, you probably don't need to specify an SDK.

For more information on sysroots, see:

    http://developer.apple.com/documentation/DeveloperTools/gcc-4.2.1/gcc/Directory-Options.html

#### Header Installation Directory

All include files needed to use the OpenEXR libraries are installed in the 
``OpenEXR`` subdirectory of the install prefix, e.g. ``/usr/local/include/OpenEXR``.

Namespaces
----------

The IlmBase libraries implement user-configurable namespaces, which
makes it possible to deal with multiple versions of these libraries
loaded at runtime.

This is helpful when, say, a base application is built with OpenEXR
v1.7, but you need to write a plugin that requires functionality from
OpenEXR v2.0. By injecting the version number into the (mangled)
symbols, via the namespacing mechanism, and changing the soname, via
the build system, you can link the plugin against the v2.0 library. At
run time, the dynamic linker can load both the 1.7 and 2.0 versions of
the library since the library soname are different and the symbols are
different.

When building IlmBase or OpenEXR the following configure script options 
are available:

    ./configure --enable-namespaceversioning

and

    ./configure --enable-customusernamespace

#### Internal Library Namespace

The ``--enable-namespaceversioning`` option controls the namespace
that is used in the library. Without an argument (see below) the
library will be built with a suffix made up of the major and minor
versions.  For example, for version 2.0.0, the internal library
namespaces will be ``Imath_2_0``, ``Iex_2_0``, ``IlmThread_2_0`` etc

For additional flexibility and control, this option can take an
additional argument in which case the internal library namespace will
be suffixed accordingly.

For example:

    ./configure --enable-namespaceversioning=ILM

will result in the namespaces of the type ``Imath_ILM``, ``Iex_ILM`` etc.

This can be useful for completely isolating your local build.

Code using the library should continue to use the namespace ``Imath``,
or for greater portability ``IMATH_NAMESPACE``, to refer to objects in
``libImath``.  In particular, the explicit use of the internal
namespace is discouraged.  This ensures that code will continue to
compile with customised or future versions of the library, which may
have a different internal namespace.

Similarily, for other namespaces in the libraries: **Iex**,
**IlmThread** and **IlmImf**.

Note that this scheme allows existing code to compile without
modifications, since the 'old' namespaces ``Imath``, ``Iex``,
``IlmThread`` and ``IlmImf`` continue to be available, albeit in a
slightly different form.

This is achieved via the following, in the Imath case:

    namespace IMATH_INTERNAL_NAMESPACE {}
    namespace IMATH_NAMESPACE 
    {
         using namespace IMATH_INTERNAL_NAMESPACE;
    }

This is included in all header files in the **Imath** library and similar ones
are present for the libraries **Iex**, **IlmThread** and **IlmImf**.

The only exception to this is where user code has forward declarations
of objects in the ``Imf`` namespace, as these will forward declare
symbols in an incorrect namespace

These forward declarations should be removed, and replaced with: 

    #include <ImfForward.h>
    
which forward-declares all types correctly.

#### Public/User Library Namespace

The ``--enable-customusernamespace`` option can override the namespace
into which will the internal namespace will be exported. This takes an
argument that sets the name of the custom user namespace.  In the
example above, ``IMATH_NAMESPACE`` could be resolved into something
other than Imath, say ``Imath_MySpecialNamespace``.

In nearly all cases, this will not be used as per the above discussion
regarding code compatibility.  Its presence is to provide a mechanism
for not prohibiting the case when the application must pass objects
from two different versions of the library.

Tests
-----

Type:

    make check

to run the IlmBase confidence tests (HalfTest, IexTest, and
ImathTest).  They should all pass; if you find a test that does not
pass on your system, please let us know.

All include files needed to use the IlmBase libraries are installed in
the ``OpenEXR`` subdirectory of the install prefix,
e.g. ``/usr/local/include/OpenEXR``.


Using IlmBase in Your Applications
----------------------------------

On systems with support for **pkg-config**, use ``pkg-config --cflags
IlmBase`` for the C++ flags required to compile against IlmBase
headers; and ``pkg-config --libs IlmBase`` for the linker flags
required to link against IlmBase libraries.

