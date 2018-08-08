The OpenEXR Libraries
=====================

The OpenEXR module contains the following:

* **IlmImf** - the core library that implements the "EXR" file format.
* **IlmImfUtil** - related utilities.
* **IlmImfExamples** - code that demonstrates how
to use the IlmImf library to read and write OpenEXR files.  
* **doc** - high-level documentation and history about the OpenEXR
format.
* **exr2aces** - 
* **exrbuild**
* **exrenvmap**
* **exrmakepreview**
* **exrmaketiled**
* **exrmultipart**
* **exrmultiview**
* **exrstdattr**

In addition, the distribution also includes confidence tests:

* **IlmImfTest** - basic confidence test.
* **IlmImfUtilTest** - huge input resilience test.
* **IlmImfFuzzTest** - damaged input resilience test.
  
If you have questions about using the OpenEXR libraries, you may want
to join our developer mailing list.  See http://www.openexr.com for
details.

License
-------

IlmBase, including all contributions, is released under a modified BSD
license. Please see the ``LICENSE`` file accompanying the distribution
for the legal fine print.

Dependencies
------------

Building OpenEXR requires IlmBase and the [zlib](https://zlib.net) library.

Building and Installation
-------------------------

To configure the Makefiles, run the ``configure`` script:

    ./configure

For help with useful build options:

    ./configure --help

In particular, arguments of note include:

* ``--prefix=<install directory>`` - a directory into which
  to install the headers and libraries. By default, headers and
  libraries are installed into ``/usr/local``.

* ``--with-ilmbase-prefix=<IlmBase install directory>`` - the direction
  into which the IlmBase headers and libraries have been installed, if
  not ``/usr/local``.

* ``--enable-imfexamples`` - build IlmImf example program.

See the Test section below for options to enable time-consuming
confidence tests.

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

#### Building on **macOS**

**macOS** supports multiple architectures. By default, IlmBase will be
built for the system doing the building. For example, if you build
IlmBase on an Intel system, the libraries will be built for Intel.

See the ``README`` file in the IlmBase library for details.

#### Header Installation Directory

All include files needed to use the OpenEXR libraries are installed in the 
``OpenEXR`` subdirectory of the install prefix, e.g. ``/usr/local/include/OpenEXR``.

Namespaces
----------

The IlmBase and OpenEXR libraries implement user-configurable
namespaces, which makes it possible to deal with multiple versions of
these libraries loaded at runtime.

See the ``README`` file in the IlmBase library for details.

Tests
-----

Type:

    make check

to run the IlmBase confidence tests (HalfTest, IexTest, and
ImathTest).  They should all pass; if you find a test that does not
pass on your system, please let us know.

Two of the confidence tests are not configured by default, because
they can take hours to run, but can be enabled via options to the
``configure`` script:

* ``--enable-imffuzztest`` - build damaged input resilience test
* ``--enable-imfhugetest`` - build huge input resilience test

Using IlmBase in Your Applications
----------------------------------

On systems with support for **pkg-config**, use ``pkg-config --cflags
OpenEXR`` for the C++ flags required to compile against OpenEXR
headers; and ``pkg-config --libs OpenEXR`` for the linker flags
required to link against OpenEXR libraries.



