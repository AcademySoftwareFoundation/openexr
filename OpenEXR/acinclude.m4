dnl
dnl FLTK with GL support
dnl

AC_DEFUN(AM_PATH_FLTK,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(fltk-prefix,[  --with-fltk-prefix=PFX   Prefix where libfltk is installed (optional)], fltk_prefix="$withval", fltk_prefix="/usr")
AC_ARG_ENABLE(fltktest, [  --disable-fltktest       Do not try to compile and run a test Fltk program],, enable_fltktest=yes)

  if test "x$fltk_prefix" != "xNONE" ; then
    fltk_args="$fltk_args --prefix=$fltk_prefix"
    FLTK_INCLUDES="-I$fltk_prefix/include"
    FLTK_LIBS="-L$fltk_prefix/lib"
  elif test "$prefix" != ""; then
    fltk_args="$fltk_args --prefix=$prefix"
    FLTK_INCLUDES="-I$prefix/include"
    FLTK_LIBS="-L$prefix/lib"
  fi

  FLTK_LIBS="$X_LIBS $FLTK_LIBS -lfltk_gl -lfltk -lXext -lX11 -lGL -lm"

  AC_MSG_CHECKING(for FLTK with GL support)
  no_fltk=""


  if test "x$enable_fltktest" = "xyes" ; then
    ac_save_CXXFLAGS="$CXXFLAGS"
    ac_save_LIBS="$LIBS"
    CXXFLAGS="$CXXFLAGS $FLTK_INCLUDES"
    LIBS="$LIBS $FLTK_LIBS"
dnl
dnl Now check if the installed FLTK has GL support
dnl
      rm -f conf.fltktest
      AC_LANG_SAVE
      AC_LANG_CPLUSPLUS
      AC_TRY_RUN([
#include <stdlib.h>
#include <Fl/Fl.h>

int main ()
{
  Fl::gl_visual(FL_RGB);
  system("touch conf.fltktest");
  return 0;
}

],, no_fltk=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       AC_LANG_RESTORE
       CXXFLAGS="$ac_save_CXXFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_fltk" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.fltktest ; then
       :
     else
       echo "*** Could not run FLTK test program, checking why..."
       CXXFLAGS="$CXXFLAGS $FLTK_INCLUDES"
       LIBS="$LIBS $FLTK_LIBS"
       AC_LANG_SAVE
       AC_LANG_CPLUSPLUS
       AC_TRY_LINK([
#include <stdio.h>
#include <Fl/Fl.h>
],     [ Fl::gl_visual(FL_RGB);return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding FLTK or finding the wrong"
       echo "*** version of FLTK. If it is not finding FLTK, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"])
       AC_LANG_RESTORE
       CXXFLAGS="$ac_save_CXXFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     FLTK_INCLUDES=""
     FLTK_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(FLTK_INCLUDES)
  AC_SUBST(FLTK_LIBS)
  rm -f conf.fltktest
])
