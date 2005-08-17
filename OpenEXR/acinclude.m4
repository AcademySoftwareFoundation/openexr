dnl
dnl Nvidia SDK
dnl

AC_DEFUN([AM_PATH_NVSDK],
[dnl
dnl Get the cflags
dnl
AC_ARG_WITH(nvsdk-prefix,[  --with-nvsdk-prefix=PFX  Prefix where Nvidia SDK is installed (optional)],
	    nvsdk_prefix="$withval", nvsdk_prefix="")

  if test x$nvsdk_prefix != x ; then
    NVSDK_CXXFLAGS="-DUNIX -I$nvsdk_prefix/inc"
  else
    NVSDK_CXXFLAGS=""
  fi

  AC_MSG_CHECKING(for Nvidia SDK)
  no_nvsdk=""

  ac_save_CXXFLAGS="$CXXFLAGS"
  ac_save_LDFLAGS="$LDFLAGS"
  CXXFLAGS="$CXXFLAGS $NVSDK_CXXFLAGS"
  LDFLAGS="-lGL"

  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_LINK([
#include <GL/gl.h>
#include <GL/glu.h>
#define GLH_EXT_SINGLE_FILE
#include <glh/glh_extensions.h>],
[
    glh_init_extensions ("GL_ARB_multitexture " "GL_NV_vertex_program " "GL_NV_fragment_program ");
],, no_nvsdk=yes)
  AC_LANG_RESTORE
  CXXFLAGS="$ac_save_CXXFLAGS"
  LDFLAGS="$ac_save_LDFLAGS"

  if test "x$no_nvsdk" = "x" ; then
    AC_MSG_RESULT(yes)
      ifelse([$1], , :, [$1])
  else
    AC_MSG_RESULT(no)
    echo "*** The Nvidia SDK test program could not be compiled."
    echo "*** Possible reasons:"
    echo "***     - The Nvidia SDK is not installed."
    echo "***     - configure cannot find the SDK (use the"
    echo "***       --with-nvsdk-prefix option to tell configure where"
    echo "***       to find it)."
    echo "***     - Your Nvidia SDK is out of date.  Please update it"
    echo "***       to the latest version."
    echo "***"
    echo "*** The exrdisplay program will not be built with fragment shader"
    echo "*** support because the fragment shader support depends on the"
    echo "*** Nvidia SDK."
    NVSDK_CXXFLAGS=""
    ifelse([$2], , :, [$2])
  fi
  AC_SUBST(NVSDK_CXXFLAGS)
])
  

dnl
dnl FLTK with GL support
dnl

AC_DEFUN([AM_PATH_FLTK],
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_VAR(FLTK_CONFIG, Path to fltk-config command)
AC_PATH_PROG(FLTK_CONFIG, fltk-config, no)
AC_ARG_WITH(fltk-config,[  --with-fltk-config=PATH Specify which fltk-config to use (optional)], FLTK_CONFIG="$withval",)

  if test x$FLTK_CONFIG != xno ; then
    FLTK_CXXFLAGS="`$FLTK_CONFIG --use-gl --cxxflags`"
    FLTK_LDFLAGS="`$FLTK_CONFIG --use-gl --ldflags`"
  else
    FLTK_CXXFLAGS=""
    FLTK_LDFLAGS=""
  fi

  AC_MSG_CHECKING(for FLTK with GL support)
  no_fltk=""

  ac_save_CXXFLAGS="$CXXFLAGS"
  ac_save_LDFLAGS="$LDFLAGS"
  CXXFLAGS="$CXXFLAGS $FLTK_CXXFLAGS"
  LDFLAGS="$LDFLAGS $FLTK_LDFLAGS"

dnl
dnl Now check if the installed FLTK has GL support
dnl
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_LINK([
#include <stdlib.h>
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>],
[
    Fl_Gl_Window foo (); 
],, no_fltk=yes)
  AC_LANG_RESTORE
  CXXFLAGS="$ac_save_CXXFLAGS"
  LDFLAGS="$ac_save_LDFLAGS"
  
  if test "x$no_fltk" = "x" ; then
    AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
    AC_MSG_RESULT(no)
    echo "*** The fltk test program could not be compiled.  Possible reasons:"
    echo "***"
    echo "***     - FLTK is not installed."
    echo "***     - Your version of FLTK does not support OpenGL."
    echo "***     - configure cannot find your 'fltk-config' program (use"
    echo "***       the --with-fltk-config option to tell configure where"
    echo "***       to find it)."
    echo "***     - Your version of FLTK is too old.  The exrdisplay"
    echo "***       program requires FLTK 1.1 or higher."
    echo "***     - Your FLTK library was compiled with a different C++"
    echo "***       compiler than the one you're using to compile OpenEXR."
    echo "***"
    echo "*** The exrdisplay program will not be built because it depends on"
    echo "*** a working FLTK install with OpenGL support."
    FLTK_CXXFLAGS=""
    FLTK_LDFLAGS=""
    ifelse([$2], , :, [$2])
  fi
  AC_SUBST(FLTK_CXXFLAGS)
  AC_SUBST(FLTK_LDFLAGS)
])

