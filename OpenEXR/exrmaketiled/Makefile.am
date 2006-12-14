## Process this file with automake to produce Makefile.in

bin_PROGRAMS = exrmaketiled

INCLUDES = @ILMBASE_CXXFLAGS@ \
	   -I$(top_builddir) \
           -I$(top_srcdir)/IlmImf -I$(top_srcdir)/config

LDADD = @ILMBASE_LDFLAGS@ \
	$(top_builddir)/IlmImf/libIlmImf.la \
	-lz

exrmaketiled_SOURCES = main.cpp Image.h Image.cpp makeTiled.cpp makeTiled.h

noinst_HEADERS = Image.h makeTiled.h

EXTRA_DIST = main.cpp Image.h Image.cpp makeTiled.cpp makeTiled.h


