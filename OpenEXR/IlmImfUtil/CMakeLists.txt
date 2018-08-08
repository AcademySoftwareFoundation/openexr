
SET ( ILMIMFUTIL_SRCS
  ImfImageChannel.cpp
  ImfFlatImageChannel.cpp
  ImfDeepImageChannel.cpp
  ImfSampleCountChannel.cpp
  ImfImageLevel.cpp
  ImfFlatImageLevel.cpp
  ImfDeepImageLevel.cpp
  ImfImage.cpp
  ImfFlatImage.cpp
  ImfDeepImage.cpp
  ImfImageIO.cpp
  ImfFlatImageIO.cpp
  ImfDeepImageIO.cpp
  ImfImageDataWindow.cpp
)

SET (OPENEXR_LIB_TARGETS "")

LINK_DIRECTORIES ( ${CMAKE_CURRENT_BINARY_DIR}/../IlmImf )

IF(OPENEXR_BUILD_SHARED)
  LIST ( APPEND OPENEXR_LIB_TARGETS IlmImfUtil )

  ADD_LIBRARY ( IlmImfUtil SHARED ${ILMIMFUTIL_SRCS} )
  SET_ILMBASE_INCLUDE_DIRS( IlmImfUtil )

  TARGET_COMPILE_DEFINITIONS ( IlmImfUtil PRIVATE ILMIMFUTIL_EXPORTS )
  IF (WIN32)
    TARGET_COMPILE_DEFINITIONS ( IlmImfUtil PUBLIC OPENEXR_DLL )
  ENDIF ()

  TARGET_LINK_LIBRARIES ( IlmImfUtil
    PUBLIC  IlmBase::Iex
    PUBLIC  IlmBase::Half
    PUBLIC  IlmBase::Imath
    PUBLIC  IlmBase::IlmThread
    PUBLIC  OpenEXR::IlmImf
    PRIVATE ${PTHREAD_LIB}
    PRIVATE ${ZLIB_LIBRARIES}
  )

  SET_TARGET_PROPERTIES ( IlmImfUtil
    PROPERTIES
    VERSION ${OPENEXR_VERSION_MAJOR}.${OPENEXR_VERSION_MINOR}.${OPENEXR_VERSION_PATCH}
    SOVERSION ${OPENEXR_SOVERSION}
    OUTPUT_NAME "IlmImfUtil${OPENEXR_LIBSUFFIX}"
    )
ENDIF()


IF ( OPENEXR_BUILD_STATIC )
  LIST ( APPEND OPENEXR_LIB_TARGETS IlmImfUtil_static )

  ADD_LIBRARY ( IlmImfUtil_static STATIC ${ILMIMFUTIL_SRCS} )
  SET_ILMBASE_INCLUDE_DIRS( IlmImfUtil_static )

  SET_TARGET_PROPERTIES ( IlmImfUtil_static
    PROPERTIES
    VERSION ${OPENEXR_VERSION_MAJOR}.${OPENEXR_VERSION_MINOR}.${OPENEXR_VERSION_PATCH}
    SOVERSION ${OPENEXR_SOVERSION}
    OUTPUT_NAME "IlmImfUtil${OPENEXR_LIBSUFFIX}_s"
    )
ENDIF()


IF (OPENEXR_BUILD_SHARED OR OPENEXR_BUILD_STATIC)
  INSTALL ( TARGETS
    ${OPENEXR_LIB_TARGETS}
      ARCHIVE DESTINATION lib
      LIBRARY DESTINATION lib
      RUNTIME DESTINATION ${RUNTIME_DIR}
  )
ENDIF ()

# Headers
INSTALL (
  FILES
    ImfImageChannel.h
    ImfFlatImageChannel.h
    ImfDeepImageChannel.h
    ImfSampleCountChannel.h
    ImfImageLevel.h
    ImfFlatImageLevel.h
    ImfDeepImageLevel.h
    ImfImage.h
    ImfFlatImage.h
    ImfDeepImage.h
    ImfImageIO.h
    ImfFlatImageIO.h
    ImfDeepImageIO.h
    ImfImageDataWindow.h
    ImfImageChannelRenaming.h
    ImfUtilExport.h

  DESTINATION
    ${CMAKE_INSTALL_PREFIX}/include/OpenEXR
  )

if (OPENEXR_BUILD_SHARED)
    add_library(OpenEXR::IlmImfUtil ALIAS IlmImfUtil)
endif()
if (OPENEXR_BUILD_STATIC)
    add_library(OpenEXR::IlmImfUtil_static ALIAS IlmImfUtil_static)
endif()
