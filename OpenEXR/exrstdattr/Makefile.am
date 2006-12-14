## Process this file with automake to produce Makefile.in

bin_PROGRAMS = exrstdattr

INCLUDES = @ILMBASE_CXXFLAGS@ \
	   -I$(top_builddir) \
           -I$(top_srcdir)/IlmImf -I$(top_srcdir)/config

LDADD = @ILMBASE_LDFLAGS@ \
	$(top_builddir)/IlmImf/libIlmImf.la \
	-lz

exrstdattr_SOURCES = main.cpp

