## Process this file with automake to produce Makefile.in

# tell autoconf to include the m4 macros in the /m4 directory 
# (an alternative to the acinclude.m4 mechanism)
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = config IlmImf IlmImfUtil IlmImfTest IlmImfUtilTest \
	  IlmImfFuzzTest exrheader exrmaketiled IlmImfExamples doc \
	  exrstdattr exrmakepreview exrenvmap exrmultiview exrmultipart

DIST_SUBDIRS = \
	$(SUBDIRS) 

EXTRA_DIST = \
	AUTHORS COPYING ChangeLog INSTALL LICENSE NEWS PATENTS \
        README README.CVS README.OSX \
	README.git README.namespacing README.cmake.txt \
	bootstrap openexr.m4 \
	config.windows/OpenEXRConfig.h \
        CMakeLists.txt

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = OpenEXR.pc

m4datadir = $(datadir)/aclocal
m4data_DATA = openexr.m4

