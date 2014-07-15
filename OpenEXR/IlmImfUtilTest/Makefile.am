## Process this file with automake to produce Makefile.in

check_PROGRAMS = IlmImfUtilTest

IlmImfUtilTest_SOURCES = main.cpp \
	testFlatImage.h testFlatImage.cpp \
	testDeepImage.h testDeepImage.cpp \
	testIO.h testIO.cpp tmpDir.h

INCLUDES = -I$(top_builddir)  \
	   -I$(top_srcdir)/IlmImf \
	   -I$(top_srcdir)/IlmImfUtil \
	   -I$(top_srcdir)/config \
	   @ILMBASE_CXXFLAGS@

LDADD = -L$(top_builddir)/IlmImf \
	-L$(top_builddir)/IlmImfUtil \
	@ILMBASE_LDFLAGS@ @ILMBASE_LIBS@ \
	-lIlmImfUtil -lIlmImf -lz

TESTS = IlmImfUtilTest

EXTRA_DIST = CMakeLists.txt
