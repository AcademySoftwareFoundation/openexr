## Process this file with automake to produce Makefile.in

if HAVE_FLTK

bin_PROGRAMS = exrdisplay

INCLUDES = @NVSDK_CXXFLAGS@ @FLTK_CXXFLAGS@ -I$(top_builddir) \
	   -I$(top_srcdir)/Iex -I$(top_srcdir)/Half -I$(top_srcdir)/Imath \
           -I$(top_srcdir)/IlmImf

LDADD = @FLTK_LDFLAGS@ \
	-L$(top_builddir)/Iex -L$(top_builddir)/Half \
	-L$(top_builddir)/Imath -L$(top_builddir)/IlmImf \
	-lIlmImf -lImath -lHalf -lIex -lz

install-exec-hook:
	@FLTK_CONFIG@ --post $(DESTDIR)$(bindir)/exrdisplay

else
install-exec-hook:

endif

if HAVE_NVSDK
fragshader_sources = ImageViewFragShader.cpp ImageViewFragShader.h
else
fragshader_sources =
endif

exrdisplay_SOURCES = main.cpp ImageView.cpp ImageView.h $(fragshader_sources) \
		     loadImage.cpp loadImage.h scaleImage.cpp scaleImage.h

noinst_HEADERS = ImageView.h ImageViewFragShader.h loadImage.h scaleImage.h

EXTRA_DIST = main.cpp ImageView.cpp ImageView.h ImageViewFragShader.cpp \
	     ImageViewFragShader.h exrdisplay.fp30 exrdisplay.cg README \
	     loadImage.h loadImage.cpp scaleImage.cpp scaleImage.h
