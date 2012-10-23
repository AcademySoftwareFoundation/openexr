#
# Makefile for the OpenEXR Shake plugin on GNU/Linux.
#
# Set the NR_SHAKE_LOCATION environment variable to the root of
# your Shake installation and type "make".  This will produce
# a plugin named 'exrFormat.so'.
#
# You'll need to compile both the OpenEXR libraries and this
# plugin with the same version of gcc that Shake uses.  As
# of Shake 3.5, this is gcc 3.2.
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
	mkdir OpenEXR_Shake-1.0.1
	mkdir OpenEXR_Shake-1.0.1/mac
	mkdir OpenEXR_Shake-1.0.1/mac/English.lproj
	mkdir OpenEXR_Shake-1.0.1/mac/exrFormat.pbproj
	cp -Rp  Makefile README exrFormat.C exrFormat.h \
		OpenEXR_Shake-1.0.1
	cp -Rp  mac/English.lproj/InfoPlist.strings \
		mac/English.lproj/locversion.plist \
		OpenEXR_Shake-1.0.1/mac/English.lproj/
	cp -Rp	mac/exrFormat.pbproj/project.pbxproj \
		OpenEXR_Shake-1.0.1/mac/exrFormat.pbproj/
	cp -Rp  mac/Info.plist mac/version.plist \
		OpenEXR_Shake-1.0.1/mac
	tar -zcvf OpenEXR_Shake-1.0.1.tar.gz OpenEXR_Shake-1.0.1 && \
		rm -rf OpenEXR_Shake-1.0.1


