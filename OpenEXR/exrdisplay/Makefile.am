## Process this file with automake to produce Makefile.in

if HAVE_FLTK

bin_PROGRAMS = exrdisplay

INCLUDES = @FLTK_CXXFLAGS@ -I$(top_builddir) -I$(top_srcdir)/Iex \
           -I$(top_srcdir)/Half -I$(top_srcdir)/Imath \
           -I$(top_srcdir)/IlmImf

LDADD = @FLTK_LDFLAGS@ \
	-L$(top_builddir)/Iex -L$(top_builddir)/Half \
	-L$(top_builddir)/Imath -L$(top_builddir)/IlmImf \
	-lIlmImf -lImath -lHalf -lIex -lz

install-exec-hook:
	@FLTK_CONFIG@ --post $(DESTDIR)$(bindir)/exrdisplay

endif

exrdisplay_SOURCES = main.cpp ImageView.cpp ImageView.h
noinst_HEADERS = ImageView.h
EXTRA_DIST = $(noinst_HEADERS)
