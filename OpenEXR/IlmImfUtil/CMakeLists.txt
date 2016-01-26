
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

IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DILMIMF_EXPORTS)
ENDIF()

LINK_DIRECTORIES ( ${CMAKE_CURRENT_BINARY_DIR}/../IlmImf )

ADD_LIBRARY ( IlmImfUtil ${LIB_TYPE}
  ${ILMIMFUTIL_SRCS}
)

TARGET_LINK_LIBRARIES ( IlmImfUtil 
  IlmImf
  ${PTHREAD_LIB} ${ZLIB_LIBRARIES}
)
TARGET_INCLUDE_DIRECTORIES ( IlmImfUtil
  PUBLIC
  $<TARGET_PROPERTY:IlmImf,INTERFACE_INCLUDE_DIRS>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/OpenEXR>
)

# Libraries

INSTALL ( TARGETS
  IlmImfUtil
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/lib
)

# Headers
INSTALL ( FILES
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
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/include/OpenEXR
  )

