The PyIlmBase Libraries
=======================

The PyIlmBase libraries provides python bindings for the IlmBase
libraries:

* **PyIex** - bindings for Iex
* **PyImath** - bindings for Imath
* **PyImathNumpy** - bindings that convert between numpy and Imath arrays


If you have questions about using the PyIlmBase libraries, you may want
to join our developer mailing list.  See http://www.openexr.com for
details.


License
-------

IlmBase, including all contributions, is released under a modified BSD
license. Please see the ``LICENSE`` file accompanying the distribution
for the legal fine print.

Dependencies
------------

PyIlmBase requires **numpy** to be available to the builder. Install with
your favorite package manager or use a Python virtualenv:

    virtualenv numpy
    soure numpy/bin/activate
    pip install numpy

PyIlmBase also requires [boost-python](https://github.com/boostorg/python).

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

* ``--with-boost-include-dir``, ``--with-boost-lib-dir`` - location of
  the boost installation.

With recent versions of macOS, System Integrity Protection (SIP) is enabled by
default.  This restricts where third party libraries can be loaded from and inhibits
the use of DYLD_LIBRRARY_PATH to include other directories in the search path.
If you are encountering an issue where boost_python is not being found during the
configure process even after specifying the correct --with-boost-\* directories and
boost is installed to a directory other than /usr/local, reinstall boost and IlmBase
to /usr/local and use them from that location.  install_name_tool -change may also
be used to rewrite library references to use absolute paths if installation in a
different directory is required, however this is not currently performed by the
build system and must be done manually to all the libraries and python modules.

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

Tests
-----

Type:

    make check

to run the PyIlmBase confidence tests (PyIexTest, PyImathTest,
PyImathNumpyTest).  They should all pass; if you find a test that does
not pass on your system, please let us know.


