## Process this file with automake to produce Makefile.in

if HAVE_CG

bin_PROGRAMS = playexr

playexr_CPPFLAGS = $(AM_CPPFLAGS) 

INCLUDES = \
	@OPENEXR_CTL_CXXFLAGS@ \
	@GL_CXXFLAGS@ \
	@CG_CXXFLAGS@

LDADD =	\
	@OPENEXR_CTL_LDFLAGS@ \
	@CG_LDFLAGS@ \
	@OPENEXR_CTL_LIBS@ \
	-lglut




playexr_SOURCES = \
	main.cpp \
	playExr.cpp \
	playExr.h \
	fileNameForFrame.cpp \
	fileNameForFrame.h \
	FileReadingThread.cpp \
	FileReadingThread.h \
	ImageBuffers.cpp \
	ImageBuffers.h \
	Timer.cpp \
	Timer.h \
	ctlToLut.cpp \
	ctlToLut.h

else
install-exec-hook:
endif

noinst_HEADERS =  \
	playExr.h \
	fileNameForFrame.h \
	FileReadingThread.h \
	ImageBuffers.h \
	Timer.h \
	ctlToLut.cpp \
	ctlToLut.h

EXTRA_DIST = \
	$(playexr_SOURCES) \
	transform_RRT.ctl \
	transform_display_video.ctl

# this produces a data directory for playexr to find default 
# ctl files you will need to set the environment variable 
# CTL_MODULE_PATH to point to this directory 
# (by default /usr/local/lib/CTL) for playexr  to work properly
# note exrdisplay also uses these .ctl files
ctldatadir = $(libdir)/CTL
ctldata_DATA = \
	transform_RRT.ctl \
	transform_display_video.ctl

# since ctl files appear above, tell the makefile to do nothing
# with them (except install them)
%.ctl:
	@true
