CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT (openexr)

SET(OPENEXR_VERSION_MAJOR "2")
SET(OPENEXR_VERSION_MINOR "2")
SET(OPENEXR_VERSION_PATCH "0")

SET(OPENEXR_VERSION ${OPENEXR_VERSION_MAJOR}.${OPENEXR_VERSION_MINOR}.${OPENEXR_VERSION_PATCH})
SET(OPENEXR_VERSION_API ${OPENEXR_VERSION_MAJOR}_${OPENEXR_VERSION_MINOR})


# enable the tests
IF (OPENEXR_BUILD_TESTS) #Smode
  ENABLE_TESTING()
ENDIF (OPENEXR_BUILD_TESTS) #Smode

# distro building 
IF (OPENEXR_INSTALL) #Smode
  SET(CPACK_PACKAGE_VERSION_MAJOR "${OPENEXR_VERSION_MAJOR}")
  SET(CPACK_PACKAGE_VERSION_MINOR "${OPENEXR_VERSION_MINOR}")
  SET(CPACK_PACKAGE_VERSION_PATCH "${OPENEXR_VERSION_PATCH}")
  SET(CPACK_SOURCE_GENERATOR "TGZ")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME
      "${CMAKE_PROJECT_NAME}-${OPENEXR_VERSION}"
    )
  set(CPACK_SOURCE_IGNORE_FILES
    "/.git*;/.cvs*;${CPACK_SOURCE_IGNORE_FILES}")
  INCLUDE ( CPack )
ENDIF (OPENEXR_INSTALL) #Smode


# Allow the developer to select if Dynamic or Static libraries are built
OPTION (BUILD_SHARED_LIBS    "Build Shared Libraries" ON)
OPTION (USE_ZLIB_WINAPI      "Use ZLib Win API"       OFF)
OPTION (NAMESPACE_VERSIONING "Use Namespace Versioning" ON)

# Setup osx rpathing
SET (CMAKE_MACOSX_RPATH 1)
SET (BUILD_WITH_INSTALL_RPATH 1)

ADD_DEFINITIONS ( -DHAVE_CONFIG_H -DILM_IMF_TEST_IMAGEDIR="${CMAKE_SOURCE_DIR}/IlmImfTest/" )

INCLUDE_DIRECTORIES (
  ${CMAKE_CURRENT_BINARY_DIR}/config
  IlmImf
  IlmImfUtil
  exrmaketiled
  exrenvmap
  exrmakepreview
  exrmultiview
  IlmImfFuzzTest
)

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 OLD)  # SMODE Silent find_package(ZLib) Warning
endif()
FIND_PACKAGE(ZLIB REQUIRED)
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})

IF (NOT WIN32)
  SET ( PTHREAD_LIB pthread )
ENDIF()

INCLUDE_DIRECTORIES ( ${ILMBASE_PACKAGE_PREFIX}/include/OpenEXR )
LINK_DIRECTORIES ( ${ILMBASE_PACKAGE_PREFIX}/lib )
MESSAGE (STATUS "ILMBASE_PACKAGE_PREFIX = " ${ILMBASE_PACKAGE_PREFIX})

SET (LIB_TYPE STATIC)
IF (BUILD_SHARED_LIBS)
  # User wants to build Dynamic Libraries, so change the LIB_TYPE variable to CMake keyword 'SHARED'
  SET (LIB_TYPE SHARED)
    IF (WIN32)
    ADD_DEFINITIONS(-DOPENEXR_DLL)
  ENDIF ()
ENDIF ()

IF (USE_ZLIB_WINAPI)
  ADD_DEFINITIONS(-DZLIB_WINAPI)
ENDIF ()

# Test for GCC-style inline asm support with AVX instructions
INCLUDE (CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES (
	"
	int main()
	{
	 #if defined(__GNUC__) && defined(__SSE2__) 
		 int n   = 0;
		 int eax = 0;
		 int edx = 0;
		 __asm__(
			 \"xgetbv     ;\"
			 \"vzeroupper  \"
			 : \"=a\"(eax), \"=d\"(edx) : \"c\"(n) : );
	 #else
		 #error No GCC style inline asm supported for AVX instructions
	 #endif
	}
	" HAVE_GCC_INLINE_ASM_AVX)

# Check if sysconf(_SC_NPROCESSORS_ONLN) can be used for CPU count
CHECK_CXX_SOURCE_COMPILES (
    "
    #include <unistd.h>
    int main()
    {
        sysconf(_SC_NPROCESSORS_ONLN);
    }
    " HAVE_SYSCONF_NPROCESSORS_ONLN)


##########################
# OpenEXRConfig.h generation
##########################
set(TMP_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/config/OpenEXRConfig.h.tmp")
set(BIN_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/config/OpenEXRConfig.h")
if(WIN32)
  file(WRITE "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_HAVE_COMPLETE_IOMANIP 1\n")
elseif(APPLE)
  file(WRITE "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_HAVE_DARWIN 1\n")
  file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_HAVE_COMPLETE_IOMANIP 1\n")
  file(APPEND "${TMP_CONFIG_FILE}" "#include <string.h>\n")
else()
  # Linux
  file(WRITE "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_HAVE_LINUX_PROCFS 1\n")
  file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_HAVE_COMPLETE_IOMANIP 1\n")
  file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_HAVE_LARGE_STACK 1\n")
endif()

if(NAMESPACE_VERSIONING)
    file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_INTERNAL_NAMESPACE_CUSTOM 1\n")
    file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_NAMESPACE Imf\n")
    file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_INTERNAL_NAMESPACE Imf_${OPENEXR_VERSION_API}\n\n")
else()
    file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_INTERNAL_NAMESPACE_CUSTOM 0\n")
    file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_NAMESPACE Imf\n")
    file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_INTERNAL_NAMESPACE Imf\n\n")
endif()

file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_VERSION_STRING \"${OPENEXR_VERSION}\"\n")
file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_PACKAGE_STRING \"OpenEXR ${OPENEXR_VERSION}\"\n")


file(APPEND "${TMP_CONFIG_FILE}" "
#define OPENEXR_VERSION_MAJOR ${OPENEXR_VERSION_MAJOR}
#define OPENEXR_VERSION_MINOR ${OPENEXR_VERSION_MINOR}
#define OPENEXR_VERSION_PATCH ${OPENEXR_VERSION_PATCH}
")


file(APPEND "${TMP_CONFIG_FILE}" "
// Version as a single hex number, e.g. 0x01000300 == 1.0.3
#define OPENEXR_VERSION_HEX ((OPENEXR_VERSION_MAJOR << 24) | \\
                             (OPENEXR_VERSION_MINOR << 16) | \\
                             (OPENEXR_VERSION_PATCH <<  8))\n
")

if (HAVE_GCC_INLINE_ASM_AVX)
  file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_HAVE_GCC_INLINE_ASM_AVX 1\n")
endif()

if(HAVE_SYSCONF_NPROCESSORS_ONLN)
  file(APPEND "${TMP_CONFIG_FILE}" "#define OPENEXR_IMF_HAVE_SYSCONF_NPROCESSORS_ONLN 1\n")
endif()
# Update destination configuration file in the build-dir only if it has changed
# to prevent unnecessary rebuild of the source file including it.
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${TMP_CONFIG_FILE}" "${BIN_CONFIG_FILE}")

SET (OPENEXR_LIBSUFFIX "")
SET (ILMBASE_LIBSUFFIX "")
IF (NAMESPACE_VERSIONING)
  SET ( OPENEXR_LIBSUFFIX "-${OPENEXR_VERSION_API}" )
  # assume same NAMESPACE_VERSION setting for IlmBase for now
  SET ( ILMBASE_LIBSUFFIX "-${OPENEXR_VERSION_API}" )
ENDIF ()

##########################
# IlmImf library
##########################
ADD_SUBDIRECTORY ( IlmImf )

SET_TARGET_PROPERTIES ( IlmImf
  PROPERTIES
  VERSION 22.0.0
  SOVERSION 22
  OUTPUT_NAME "IlmImf${OPENEXR_LIBSUFFIX}"
  )
  
##########################
# IlmImfUtil library
##########################
ADD_SUBDIRECTORY ( IlmImfUtil )

SET_TARGET_PROPERTIES ( IlmImfUtil
  PROPERTIES
  VERSION 22.0.0
  SOVERSION 22
  OUTPUT_NAME "IlmImfUtil${OPENEXR_LIBSUFFIX}"
  )
  

IF (OPENEXR_BUILD_TESTS) #Smode
  ##########################
  # Example Code
  ##########################
  ADD_SUBDIRECTORY ( IlmImfExamples )

    
  ##########################
  # Tests
  ##########################
  ADD_SUBDIRECTORY ( IlmImfTest )
  ADD_SUBDIRECTORY ( IlmImfUtilTest )
  ADD_SUBDIRECTORY ( IlmImfFuzzTest )


  ##########################
  # Binaries / Utilities
  ##########################
  ADD_SUBDIRECTORY ( exrheader )
  ADD_SUBDIRECTORY ( exrmaketiled )
  ADD_SUBDIRECTORY ( exrstdattr )
  ADD_SUBDIRECTORY ( exrmakepreview )
  ADD_SUBDIRECTORY ( exrenvmap )
  ADD_SUBDIRECTORY ( exrmultiview )
  ADD_SUBDIRECTORY ( exrmultipart )
ENDIF (OPENEXR_BUILD_TESTS) #Smode

##########################
# Installation
##########################

IF (OPENEXR_INSTALL) #Smode 
  INSTALL ( FILES
    "${TMP_CONFIG_FILE}"
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/include/OpenEXR
    )

  # Documentation
  INSTALL ( FILES
    doc/TechnicalIntroduction.pdf
    doc/ReadingAndWritingImageFiles.pdf
    doc/OpenEXRFileLayout.pdf
    doc/MultiViewOpenEXR.pdf
    doc/InterpretingDeepPixels.pdf
    doc/TheoryDeepPixels.pdf
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/share/doc/OpenEXR-${OPENEXR_VERSION}
    )

  # Examples
  INSTALL ( FILES
    IlmImfExamples/main.cpp
    IlmImfExamples/drawImage.cpp
    IlmImfExamples/rgbaInterfaceExamples.cpp
    IlmImfExamples/rgbaInterfaceTiledExamples.cpp
    IlmImfExamples/generalInterfaceExamples.cpp
    IlmImfExamples/lowLevelIoExamples.cpp
    IlmImfExamples/previewImageExamples.cpp
    IlmImfExamples/generalInterfaceTiledExamples.cpp
    IlmImfExamples/generalInterfaceTiledExamples.h
    IlmImfExamples/drawImage.h
    IlmImfExamples/rgbaInterfaceExamples.h
    IlmImfExamples/generalInterfaceExamples.h
    IlmImfExamples/rgbaInterfaceTiledExamples.h
    IlmImfExamples/lowLevelIoExamples.h
    IlmImfExamples/previewImageExamples.h
    IlmImfExamples/namespaceAlias.h
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/share/doc/OpenEXR-${OPENEXR_VERSION}/examples
    )
ENDIF(OPENEXR_INSTALL)
