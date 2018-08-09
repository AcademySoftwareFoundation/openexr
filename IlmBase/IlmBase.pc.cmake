libdir=@CMAKE_INSTALL_FULL_LIBDIR@
includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@

Name: IlmBase
Description: Base math and exception libraries
Version: @ILMBASE_VERSION@

Libs: -L${libdir} -lHalf -lIex@ILMBASE_LIBSUFFIX@ -lIexMath@ILMBASE_LIBSUFFIX@ -lIlmThread@ILMBASE_LIBSUFFIX@ -lImath@ILMBASE_LIBSUFFIX@ @CMAKE_THREAD_LIBS_INIT@
Cflags: @CMAKE_THREAD_LIBS_INIT@ -I${includedir}/OpenEXR
