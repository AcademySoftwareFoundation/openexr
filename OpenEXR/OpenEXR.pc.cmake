libdir=@CMAKE_INSTALL_FULL_LIBDIR@
includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@

Name: OpenEXR
Description: OpenEXR image library
Version: @OPENEXR_VERSION@

Libs: -L${libdir} -lIlmImf@OPENEXR_LIBSUFFIX@ -lIlmImfUtil@OPENEXR_LIBSUFFIX@
Cflags: -I${includedir}/OpenEXR
Requires: IlmBase
Libs.private: -lz
