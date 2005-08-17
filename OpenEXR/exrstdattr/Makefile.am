## Process this file with automake to produce Makefile.in

bin_PROGRAMS = exrstdattr

INCLUDES = -I$(top_builddir) -I$(top_srcdir)/Iex \
           -I$(top_srcdir)/Half -I$(top_srcdir)/Imath \
           -I$(top_srcdir)/IlmImf

LDADD = -L$(top_builddir)/Iex -L$(top_builddir)/Half \
	-L$(top_builddir)/Imath -L$(top_builddir)/IlmImf \
	-lIlmImf -lImath -lHalf -lIex -lz

exrstdattr_SOURCES = main.cpp

