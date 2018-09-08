## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = \
	Half HalfTest \
	Iex IexMath IexTest \
	Imath ImathTest \
	IlmThread \
	config

DIST_SUBDIRS = \
	$(SUBDIRS) 


EXTRA_DIST = \
	AUTHORS ChangeLog LICENSE NEWS \
	README.md \
	bootstrap \
	config.windows/IlmBaseConfig.h \
	CMakeLists.txt

dist-hook:
	cp -fpR $(abspath $(srcdir)/..)/cmake $(distdir)
	find $(distdir)/cmake -type d ! -perm -700 -exec chmod u+rwx {} \;

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = IlmBase.pc

