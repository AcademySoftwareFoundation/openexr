dnl Process this file with autoconf to produce a configure script.
AC_INIT(IlmBase, 1.0.3)
AC_SUBST(ILMBASE_VERSION, 1.0.3)
AC_CANONICAL_HOST
AC_CONFIG_SRCDIR(ImathTest/main.cpp)
AM_CONFIG_HEADER(config/IlmBaseConfig.h)
AM_INIT_AUTOMAKE(1.6.3)  dnl Require automake 1.6.3 or better
AM_MAINTAINER_MODE


LIBTOOL_CURRENT=7
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
AC_CHECK_HEADERS(limits.h unistd.h ucontext.h)

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

dnl --enable-large-stack
case "$host" in
*linux*)
  AC_ARG_ENABLE(large-stack,
		AC_HELP_STRING([--enable-large-stack],
			       [enable optimizations for systems that support
			       large stack sizes [[default=yes]]]),
		[large_stack="${enableval}"],
		[large_stack=yes])
  ;;
*)
  AC_ARG_ENABLE(large-stack,
		AC_HELP_STRING([--enable-large-stack],
			       [enable optimizations for systems that support
			       large stack sizes [[default=no]]]),
		[large_stack="${enableval}"],
		[large_stack=no])
  ;;
esac

if test "x${large_stack}" != xno ; then
    AC_DEFINE(ILMBASE_HAVE_LARGE_STACK)
fi

dnl Define the version string
AC_DEFINE_UNQUOTED(ILMBASE_VERSION_STRING, "${VERSION}")
AC_DEFINE_UNQUOTED(ILMBASE_PACKAGE_STRING, "${PACKAGE_STRING}")


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
  dnl Compiler flags for all OS X builds

  dnl OSX architectures specification
  dnl NOTE: multiple architectures requires --disable-dependency-tracking
  dnl For more information: http://developer.apple.com/documentation/Porting/Conceptual/PortingUNIX/compiling/chapter_4_section_3.html
  AC_ARG_ENABLE(osx-arch,
	      AC_HELP_STRING([--enable-osx-arch],
			     [specify the architecture(s) for OS X builds. Ex: --enable-osx-arch="ppc i386"]),
	      [osx_arch="${enableval}"], [osx_arch=default])

  if test "${osx_arch}" != default ; then
     dnl Replace "a b c" with "-arch a -arch b -arch c". Should probably use an all-whitespace regexp rather than a single character for the search string.
     osx_arch_flags="-arch ${osx_arch// / -arch }"
     CXXFLAGS="$CXXFLAGS $osx_arch_flags"

     dnl If multiple archs specified, make sure that --disable-dependency-tracking was also specified
     if test "$enable_dependency_tracking" != no ; then
        osx_arch_array=( ${osx_arch} )
        if test ${#osx_arch_array[@]} -gt 1 ; then
          AC_MSG_ERROR([building multiple OS X architectures requires --disable-dependency-tracking.
Please re-run configure with these options:
  --enable-osx-arch="${osx_arch}" --disable-dependency-tracking
  		  ])
        fi
     fi
  fi

  dnl OSX SDK specification
  AC_ARG_ENABLE(osx-sdk,
	      AC_HELP_STRING([--enable-osx-sdk],
			     [specify the sdk (sysroot) for use with OS X. Ex: --enable-osx-sdk=MacOSX10.5]),
	      [osx_sdk="${enableval}"], [osx_sdk=default])

  if test "${osx_sdk}" != default ; then
    CXXFLAGS="$CXXFLAGS -isysroot /Developer/SDKs/${osx_sdk}"
  fi

  dnl OS X universal binary support, requires --disable-dependency-tracking
  dnl NOTE: this argument is obsolete, and has been replaced by --enable-osx-sdk and --enable-osx-arch
  dnl This is equivalent to --enable-osx-sdk=MacOSX10.4u.sdk --enable-osx-arch="ppc i386"
  AC_ARG_ENABLE(osx-universal-binaries,
	      AC_HELP_STRING([--enable-osx-universal-binaries],
			     [build two-way universal (ppc, i386) binaries on OS X. OBSOLETE: Consider using --enable-osx-sdk and --enable-osx-arch instead. [[default=no]]]),
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
  ;;
esac

AM_CFLAGS="$EXTRA_OPT_CFLAGS"
AM_CXXFLAGS="$EXTRA_OPT_CFLAGS"

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST(HAVE_UCONTEXT_H)


AC_OUTPUT([
Makefile
IlmBase.pc
config/Makefile
Half/Makefile
HalfTest/Makefile
Iex/Makefile
IexMath/Makefile
IexTest/Makefile
IlmThread/Makefile
Imath/Makefile
ImathTest/Makefile
vc/Makefile
])

if test "$osx_arch"; then
  AC_MSG_RESULT([
---------------------------------------------
Summary for IlmBase features:

OS X architectures                              $osx_arch
OS X SDK                                        $osx_sdk
OS X two-way universal build (deprecated)       $build_osxuniversal
enable large stack optimizations                $large_stack
---------------------------------------------
  ])
else
  AC_MSG_RESULT([
---------------------------------------------
Summary for IlmBase features:

enable large stack optimizations                $large_stack
---------------------------------------------
  ])
fi

