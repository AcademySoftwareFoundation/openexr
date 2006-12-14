## Process this file with automake to produce Makefile.in

if BUILD_IMFEXAMPLES
noinst_PROGRAMS = imfexamples
endif

INCLUDES = @ILMBASE_CXXFLAGS@ \
            -I$(top_builddir)  \
            -I$(top_srcdir)/IlmImf -I$(top_srcdir)/config

LDADD = @ILMBASE_LDFLAGS@  \
	-L$(top_builddir)/IlmImf \
	-lIlmImf -lz

imfexamples_SOURCES = main.cpp drawImage.cpp rgbaInterfaceExamples.cpp \
		      rgbaInterfaceTiledExamples.cpp \
		      generalInterfaceExamples.cpp \
		      lowLevelIoExamples.cpp previewImageExamples.cpp \
		      generalInterfaceTiledExamples.cpp \
	              generalInterfaceTiledExamples.h drawImage.h \
		      rgbaInterfaceExamples.h generalInterfaceExamples.h \
		      rgbaInterfaceTiledExamples.h \
		      lowLevelIoExamples.h previewImageExamples.h

examplesdir = $(datadir)/doc/OpenEXR-@OPENEXR_VERSION@/examples
examples_DATA = $(imfexamples_SOURCES)

imfexamplesdir = $(examplesdir)

