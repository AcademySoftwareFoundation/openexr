## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = libHalf.la

libHalf_la_SOURCES = half.cpp half.h halfFunction.h halfLimits.h eLut.h \
		     toFloat.h

libHalfincludedir = $(includedir)/OpenEXR

libHalfinclude_HEADERS = half.h halfFunction.h halfLimits.h

noinst_HEADERS = eLut.h toFloat.h

# these are used to build eLut.h and toFloat.h dynamically
EXTRA_DIST = eLut.cpp toFloat.cpp

CLEANFILES = eLut eLut.h toFloat toFloat.h

BUILT_SOURCES = eLut.h toFloat.h

eLut.h:	eLut
	./eLut > eLut.h

toFloat.h: toFloat
	   ./toFloat > toFloat.h
