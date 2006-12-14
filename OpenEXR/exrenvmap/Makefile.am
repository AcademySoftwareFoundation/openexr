## Process this file with automake to produce Makefile.in

bin_PROGRAMS = exrenvmap

INCLUDES = @ILMBASE_CXXFLAGS@ \
	   -I$(top_builddir) \
           -I$(top_srcdir)/IlmImf -I$(top_srcdir)/config

LDADD = @ILMBASE_LDFLAGS@ \
	$(top_builddir)/IlmImf/libIlmImf.la \
	-lz

exrenvmap_SOURCES = main.cpp EnvmapImage.cpp EnvmapImage.h \
		    makeCubeMap.cpp makeCubeMap.h \
		    makeLatLongMap.cpp makeLatLongMap.h

noinst_HEADERS = EnvmapImage.h makeCubeMap.h makeLatLongMap.h

EXTRA_DIST = main.cpp EnvmapImage.cpp EnvmapImage.h \
	     makeCubeMap.cpp makeCubeMap.h \
	     makeLatLongMap.cpp makeLatLongMap.h
