## Process this file with automake to produce Makefile.in

bin_PROGRAMS = exrheader

INCLUDES = @ILMBASE_CXXFLAGS@ \
	   -I$(top_builddir) \
           -I$(top_srcdir)/IlmImf -I$(top_srcdir)/config

LDADD = @ILMBASE_LDFLAGS@ @ILMBASE_LIBS@ \
	$(top_builddir)/IlmImf/libIlmImf.la \
	-lz

exrheader_SOURCES = main.cpp

