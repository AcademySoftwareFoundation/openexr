#
# Makefile for the OpenEXR Shake plugin on GNU/Linux.
#
# Set the NR_SHAKE_LOCATION environment variable to the root of
# your Shake installation and type "make".  This will produce
# a plugin named 'exrFormat.so'.
#
# You'll need to compile both the OpenEXR libraries and this
# plugin with the same version of gcc that Shake uses.  As
# of Shake 3.0, this is gcc 2.95.  Please ensure that your
# version of gcc 2.95 is 2.95.3 or better, as previous
# versions may have some problems with OpenEXR (type 
# 'gcc --version' to find out which version you're using).
#

CXXFLAGS = -O2 -fPIC \
	   -I$(NR_SHAKE_LOCATION)/sdk/include/nrcc \
	   -I$(NR_SHAKE_LOCATION)/sdk/include/nrfx \
	   -I$(NR_SHAKE_LOCATION)/sdk/include/nrgl \
	   `pkg-config --cflags OpenEXR`

LDFLAGS = -L$(NR_SHAKE_LOCATION)/lib \
	   -lnrui_lx -lnrgl_lx -lnrcc_lx -lnrzl_lx -lm \
	  `pkg-config --libs OpenEXR`

all: exrFormat.so

clean:
	rm -f *.o *.so

exrFormat.so: exrFormat.C exrFormat.h
	      @echo "Using $(NR_SHAKE_LOCATION)"
	      $(CXX) $(CXXFLAGS) exrFormat.C -c -o exrFormat.o
	      $(CXX) -shared -o exrFormat.so exrFormat.o $(LDFLAGS)

dist:
	tar -czvf OpenEXR_Shake-1.0.tar.gz Makefile README exrFormat.C \
		exrFormat.h mac/English.lproj/InfoPlist.strings \
		mac/English.lproj/locversion.plist \
		mac/exrFormat.pbproj/project.pbxproj \
		mac/Info.plist mac/version.plist
