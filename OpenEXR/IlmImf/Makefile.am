## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = libIlmImf.la

libIlmImf_la_SOURCES = ImfAttribute.cpp ImfBoxAttribute.cpp ImfCRgbaFile.cpp \
		       ImfChannelList.cpp ImfChannelListAttribute.cpp \
		       ImfFloatAttribute.cpp ImfFrameBuffer.cpp \
		       ImfHeader.cpp ImfIO.cpp ImfInputFile.cpp \
		       ImfIntAttribute.cpp ImfLineOrderAttribute.cpp \
		       ImfMatrixAttribute.cpp ImfOpaqueAttribute.cpp \
		       ImfOutputFile.cpp ImfRgbaFile.cpp \
		       ImfStringAttribute.cpp ImfVecAttribute.cpp ImfHuf.cpp \
		       ImfWav.cpp ImfLut.cpp ImfCompressor.cpp \
		       ImfRleCompressor.cpp ImfZipCompressor.cpp \
		       ImfPizCompressor.cpp ImfMisc.cpp \
		       ImfCompressionAttribute.cpp ImfDoubleAttribute.cpp \
		       ImfAttribute.h ImfBoxAttribute.h \
		       ImfCRgbaFile.h ImfChannelList.h \
		       ImfChannelListAttribute.h \
		       ImfCompressionAttribute.h \
	               ImfDoubleAttribute.h ImfFloatAttribute.h \
	               ImfFrameBuffer.h ImfHeader.h ImfIO.h \
		       ImfInputFile.h ImfIntAttribute.h \
		       ImfLineOrderAttribute.h ImfMatrixAttribute.h \
		       ImfOpaqueAttribute.h ImfOutputFile.h \
		       ImfRgbaFile.h ImfStringAttribute.h \
		       ImfVecAttribute.h ImfHuf.h ImfWav.h ImfLut.h \
		       ImfArray.h ImfCompression.h ImfLineOrder.h \
		       ImfName.h ImfPixelType.h ImfVersion.h ImfXdr.h \
		       ImfCompressor.h ImfRleCompressor.h ImfZipCompressor.h \
		       ImfPizCompressor.h ImfMisc.h ImfAutoArray.h \
		       ImfConvert.cpp ImfConvert.h ImfPreviewImage.cpp \
		       ImfPreviewImage.h ImfPreviewImageAttribute.cpp \
		       ImfPreviewImageAttribute.h


libIlmImfincludedir = $(includedir)/OpenEXR

libIlmImfinclude_HEADERS = ImfAttribute.h ImfBoxAttribute.h \
			   ImfCRgbaFile.h ImfChannelList.h \
			   ImfChannelListAttribute.h \
			   ImfCompressionAttribute.h \
	                   ImfDoubleAttribute.h ImfFloatAttribute.h \
	                   ImfFrameBuffer.h ImfHeader.h ImfIO.h \
			   ImfInputFile.h ImfIntAttribute.h \
			   ImfLineOrderAttribute.h ImfMatrixAttribute.h \
			   ImfOpaqueAttribute.h ImfOutputFile.h \
			   ImfRgbaFile.h ImfStringAttribute.h \
			   ImfVecAttribute.h ImfHuf.h ImfWav.h ImfLut.h \
			   ImfArray.h ImfCompression.h ImfLineOrder.h \
			   ImfName.h ImfPixelType.h ImfVersion.h ImfXdr.h \
			   ImfConvert.h ImfPreviewImage.h \
			   ImfPreviewImageAttribute.h

noinst_HEADERS = ImfCompressor.h ImfRleCompressor.h ImfZipCompressor.h \
		 ImfPizCompressor.h ImfMisc.h ImfAutoArray.h

EXTRA_DIST = $(noinst_HEADERS)

INCLUDES = -I$(top_builddir) -I$(top_srcdir)/Half -I$(top_srcdir)/Iex \
	   -I$(top_srcdir)/Imath

# autoconf defines a VERSION macro which conflicts with Imf::VERSION, so 
# we have to undefine the VERSION macro when compiling IlmImf

libIlmImf_la_CXXFLAGS = $(AM_CXXFLAGS) -UVERSION

