dnl Process this file with autoconf to produce a configure script.
AC_INIT(ImathTest/main.cpp)

dnl Checks for target
AC_CANONICAL_TARGET

OPENEXR_MAJOR_VERSION=1
OPENEXR_MINOR_VERSION=2
OPENEXR_MICRO_VERSION=2

OPENEXR_VERSION=$OPENEXR_MAJOR_VERSION.$OPENEXR_MINOR_VERSION.${OPENEXR_MICRO_VERSION}
AC_SUBST(OPENEXR_VERSION)

LIBTOOL_CURRENT=2
LIBTOOL_REVISION=2
LIBTOOL_AGE=0
LIBTOOL_VERSION=$LIBTOOL_CURRENT:$LIBTOOL_REVISION:$LIBTOOL_AGE
AC_SUBST(LIBTOOL_VERSION)

dnl GNU automake support.
AM_INIT_AUTOMAKE(OpenEXR, ${OPENEXR_VERSION})
AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_LN_S
AM_PROG_LIBTOOL
AC_PROG_MAKE_SET

dnl
dnl PKGCONFIG preparations
dnl

if test -z $PKG_CONFIG_PATH; then
	PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib/pkgconfig
fi

export PKG_CONFIG_PATH

dnl
dnl check for FLTK
dnl
AM_PATH_FLTK(have_fltk=yes, have_fltk=no)

dnl
dnl check for Nvidia SDK
dnl
if test $have_fltk = "yes"; then
  AM_PATH_NVSDK(have_nvsdk=yes, have_nvsdk=no)
  if test $have_nvsdk = "yes"; then
    AC_DEFINE(HAVE_FRAGMENT_SHADERS)
  fi
else have_nvsdk = "no"
fi

dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(limits.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_CHECK_FUNCS(strerror)

dnl Checks for zlib
AC_CHECK_LIB(z, compress,
             [:],
             [AC_MSG_ERROR([
*** OpenEXR requires a recent version of zlib, which you don't appear to
*** have.
***
*** This could be because the run-time linker is not finding zlib, or it
*** is finding the wrong version.  In this case, you'll need to set your
*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point
*** to the proper version.  Also, make sure you have run ldconfig if
*** that is required on your system.
			   ])]
)

dnl Checks for ios_base
AC_MSG_CHECKING(for ios_base support in C++ standard library)
ios_base="no"
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_TRY_COMPILE([#include <iostream>],[

    std::ios_base::fmtflags flags;
],
AC_DEFINE(HAVE_IOS_BASE) ios_base=yes)
AC_MSG_RESULT($ios_base)
AC_LANG_RESTORE


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


dnl Checks for <limits>
AC_MSG_CHECKING(for <limits> support in C++ standard library)
stl_limits="no"
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_TRY_COMPILE([#include <limits>],[
],
AC_DEFINE(HAVE_STL_LIMITS) stl_limits=yes)
AC_MSG_RESULT($stl_limits)
AC_LANG_RESTORE


dnl gcc 2.95 requires increased template depth
if test x"$GCC" = xyes; then
  AC_MSG_CHECKING(for template depth required to build OpenEXR)
  case `$CC -dumpversion` in
    2.95*)
      CXXFLAGS="$CXXFLAGS -ftemplate-depth-99"
      AC_MSG_RESULT(99)
      ;;
    *)
      AC_MSG_RESULT(standard)
      ;;
  esac
fi

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
*irix*) 
  AC_DEFINE(PLATFORM_IRIX) 
  ;;
*solaris*) 
  AC_DEFINE(PLATFORM_SUNOS5) 
  ;;
*osf*) 
  AC_DEFINE(PLATFORM_OSF1) 
  ;;
*darwin*) 
  AC_DEFINE(PLATFORM_DARWIN_PPC) 
  CXXFLAGS="$CXXFLAGS -Wno-long-double"
  ;;
esac

dnl Processor-specific optimizations
AC_MSG_CHECKING(for processor-specific optimizations)
case $target_cpu in
i686) EXTRA_CPU_CFLAGS=$(if $CC -march=i686 -S -o /dev/null -xc /dev/null > /dev/null; then echo "-march=i686"; fi) ;;
*athlon*)  EXTRA_CPU_CFLAGS=$(if $CC -march=i686 -S -o /dev/null -xc /dev/null > /dev/null; then echo "-march=i686"; fi) ;;
esac
AC_MSG_RESULT([$EXTRA_CPU_CFLAGS])

AM_CFLAGS="$EXTRA_OPT_CFLAGS $EXTRA_CPU_CFLAGS"
AM_CXXFLAGS="$EXTRA_OPT_CFLAGS $EXTRA_CPU_CFLAGS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CXXFLAGS)

dnl OS X istream::read bug workaround
AC_ARG_ENABLE(osx-istream-hack, [  --enable-osx-istream-hack  Workaround for OS X istream::read bug], AC_DEFINE(BROKEN_ISTREAM_HACK),)

dnl build imfexamples example program?
build_imfexamples="no"
AC_ARG_ENABLE(imfexamples,      [  --enable-imfexamples       Build IlmImf example program], build_imfexamples="yes",)

AM_CONDITIONAL(HAVE_FLTK, test "x$have_fltk" = xyes)
AM_CONDITIONAL(HAVE_NVSDK, test "x$have_nvsdk" = xyes)
AM_CONDITIONAL(BUILD_IMFEXAMPLES, test "x$build_imfexamples" = xyes)

AC_OUTPUT([
Makefile
OpenEXR.pc
openexr.spec
Half/Makefile
HalfTest/Makefile
Iex/Makefile
IexTest/Makefile
IlmImf/Makefile
IlmImfTest/Makefile
Imath/Makefile
ImathTest/Makefile
exrdisplay/Makefile
exrheader/Makefile
exrmaketiled/Makefile
IlmImfExamples/Makefile
doc/Makefile
vc/Makefile
exrstdattr/Makefile
exrmakepreview/Makefile
exrenvmap/Makefile
])

AC_MSG_RESULT([
---------------------------------------------
Summary for OpenEXR features:

build exrdisplay FLTK application		$have_fltk
    use Nvidia SDK				$have_nvsdk
build IlmImf example program                    $build_imfexamples
---------------------------------------------
])
