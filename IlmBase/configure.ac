dnl Process this file with autoconf to produce a configure script.
AC_INIT(IlmBase, 1.0.1)
AC_SUBST(ILMBASE_VERSION, 1.0.1)
AC_CANONICAL_HOST
AC_CONFIG_SRCDIR(ImathTest/main.cpp)
AM_CONFIG_HEADER(config/IlmBaseConfig.h)
AM_INIT_AUTOMAKE(1.6.3)  dnl Require automake 1.6.3 or better
AM_MAINTAINER_MODE


LIBTOOL_CURRENT=6
LIBTOOL_REVISION=0
LIBTOOL_AGE=0
LIBTOOL_VERSION=$LIBTOOL_CURRENT:$LIBTOOL_REVISION:$LIBTOOL_AGE
AC_SUBST(LIBTOOL_VERSION)

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

dnl
dnl PKGCONFIG preparations
dnl

if test -z "${PKG_CONFIG_PATH}"; then
	PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig
fi

export PKG_CONFIG_PATH


dnl Checks for libraries.
dnl --enable-threading
AC_ARG_ENABLE(threading,
              AC_HELP_STRING([--enable-threading],
                             [enable multi-threading [[default=yes]]]),
              [multithread="${enableval}"], [multithread=yes])

if test "${multithread}" != no ; then
    ACX_PTHREAD(
    [
	AC_DEFINE(HAVE_PTHREAD)
	LIBS="$PTHREAD_LIBS $LIBS"
	CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
	CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
	CC="$PTHREAD_CC"
	
	AM_POSIX_SEM()
    ],
    [AC_MSG_ERROR([POSIX thread support required])])
    AC_MSG_NOTICE([multithread true, LIBS = $LIBS, CC = $CC, CXXFLAGS = $CXXFLAGS])
fi

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(limits.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_CHECK_FUNCS(strerror)


dnl Checks for std::right etc. in iomanip
AC_MSG_CHECKING(for complete iomanip support in C++ standard library)
complete_iomanip="no"
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_TRY_COMPILE([#include <iomanip>],[

	std::right;
],
AC_DEFINE(HAVE_COMPLETE_IOMANIP) complete_iomanip=yes)
AC_MSG_RESULT($complete_iomanip)
AC_LANG_RESTORE


AC_MSG_CHECKING(for gcc optimization flags)
old_cflags=$CFLAGS
CFLAGS="$CFLAGS -pipe"
AC_TRY_COMPILE([#include <stdio.h>],
[ printf ("hello, world"); ],
[ EXTRA_OPT_CFLAGS="-pipe"],[ EXTRA_OPT_CFLAGS=""])
CFLAGS=$old_cflags
AC_MSG_RESULT([$EXTRA_OPT_CFLAGS])

dnl Platform-specific stuff
case "$host" in
*darwin*) 
  dnl OS X universal binary support, requires --disable-dependency-tracking
  AC_ARG_ENABLE(osx-universal-binaries,
	      AC_HELP_STRING([--enable-osx-universal-binaries],
			     [build universal binaries on OS X [[default=no]]]),
	      [build_osxuniversal="${enableval}"], [build_osxuniversal=no])

  if test "${build_osxuniversal}" != no ; then
    if test "$enable_dependency_tracking" != no ; then
      AC_MSG_ERROR([--enable-osx-universal-binary requires --disable-dependency-tracking.
Please re-run configure with these options:
  --disable-dependency-tracking --enable-osx-universal-binary
  		  ])
    fi
    CXXFLAGS="$CXXFLAGS -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch ppc -arch i386"
    dnl LDFLAGS="$LDFLAGS -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -arch ppc -arch i386"
  fi

  CXXFLAGS="$CXXFLAGS -Wno-long-double"
  ;;
esac

AM_CFLAGS="$EXTRA_OPT_CFLAGS"
AM_CXXFLAGS="$EXTRA_OPT_CFLAGS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CXXFLAGS)


AC_OUTPUT([
Makefile
IlmBase.pc
config/Makefile
Half/Makefile
HalfTest/Makefile
Iex/Makefile
IexTest/Makefile
IlmThread/Makefile
Imath/Makefile
ImathTest/Makefile
vc/Makefile
])

if test "x$build_osxuniversal" = xyes; then
  AC_MSG_RESULT([
---------------------------------------------
Summary for IlmBase features:

build OS X universal binaries                   $build_osxuniversal
---------------------------------------------
  ])
else
  AC_MSG_RESULT([
---------------------------------------------
Summary for IlmBase features:

No variable options in this package.  Configure completed successfully.
---------------------------------------------
  ])
fi

