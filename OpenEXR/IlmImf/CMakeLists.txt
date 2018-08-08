# yue.nicholas@gmail.com

SET(CMAKE_INCLUDE_CURRENT_DIR 1)

IF (WIN32)
  SET(RUNTIME_DIR ${OPENEXR_PACKAGE_PREFIX}/bin)
  SET(WORKING_DIR ${RUNTIME_DIR})
ELSE ()
  SET(RUNTIME_DIR ${OPENEXR_PACKAGE_PREFIX}/lib)
  SET(WORKING_DIR .)
ENDIF ()

SET(BUILD_B44EXPLOGTABLE OFF)
IF (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/b44ExpLogTable.h")
  SET(BUILD_B44EXPLOGTABLE ON)
ENDIF ()

IF (BUILD_B44EXPLOGTABLE)
  ADD_EXECUTABLE ( b44ExpLogTable
    b44ExpLogTable.cpp
  )

  IF (WIN32)
    TARGET_LINK_LIBRARIES ( b44ExpLogTable
      IlmBase::Half_static
      IlmBase::IlmThread_static
      IlmBase::Iex_static)
  ELSE()
    TARGET_LINK_LIBRARIES ( b44ExpLogTable
      IlmBase::Half${OPENEXR_TARGET_SUFFIX}
      IlmBase::IlmThread${OPENEXR_TARGET_SUFFIX}
      IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
      ${PTHREAD_LIB})
  ENDIF()

  TARGET_INCLUDE_DIRECTORIES ( b44ExpLogTable
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../IlmBase/Half
  )

  ADD_CUSTOM_COMMAND (
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/b44ExpLogTable.h
    COMMAND $<TARGET_FILE:b44ExpLogTable> > ${CMAKE_CURRENT_BINARY_DIR}/b44ExpLogTable.h
    DEPENDS b44ExpLogTable
  )

ELSE ()
  message (STATUS "Skipping build of b44ExpLogTable.h")
ENDIF()

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/dwaLookups.h")
  ADD_EXECUTABLE ( dwaLookups
    dwaLookups.cpp
  )

  SET(BUILD_DWALOOKUPS ON)

  SET_ILMBASE_INCLUDE_DIRS(dwaLookups)

  IF (WIN32)
    TARGET_LINK_LIBRARIES ( dwaLookups
      IlmBase::Half_static
      IlmBase::IlmThread_static
      IlmBase::Iex_static)
  ELSE()
    TARGET_LINK_LIBRARIES ( dwaLookups
      IlmBase::Half${OPENEXR_TARGET_SUFFIX}
      IlmBase::IlmThread${OPENEXR_TARGET_SUFFIX}
      IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
      ${PTHREAD_LIB})
  ENDIF()

  ADD_CUSTOM_COMMAND (
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dwaLookups.h
    COMMAND $<TARGET_FILE:dwaLookups> > ${CMAKE_CURRENT_BINARY_DIR}/dwaLookups.h
    DEPENDS dwaLookups
  )
ELSE ()
  message (STATUS "Skipping build of dwaLookups.h")
ENDIF()

SET ( ILMIMF_SRCS
  ImfAttribute.cpp
  ImfBoxAttribute.cpp
  ImfCRgbaFile.cpp
  ImfChannelList.cpp
  ImfChannelListAttribute.cpp
  ImfFloatAttribute.cpp
  ImfFrameBuffer.cpp
  ImfHeader.cpp
  ImfIO.cpp
  ImfInputFile.cpp
  ImfIntAttribute.cpp
  ImfLineOrderAttribute.cpp
  ImfMatrixAttribute.cpp
  ImfOpaqueAttribute.cpp
  ImfOutputFile.cpp
  ImfRgbaFile.cpp
  ImfStringAttribute.cpp
  ImfVecAttribute.cpp
  ImfHuf.cpp
  ImfThreading.cpp
  ImfWav.cpp
  ImfLut.cpp
  ImfCompressor.cpp
  ImfRleCompressor.cpp
  ImfZipCompressor.cpp
  ImfPizCompressor.cpp
  ImfB44Compressor.cpp
  ImfDwaCompressor.cpp
  ImfMisc.cpp
  ImfCompressionAttribute.cpp
  ImfDoubleAttribute.cpp
  ImfConvert.cpp
  ImfPreviewImage.cpp
  ImfPreviewImageAttribute.cpp
  ImfVersion.cpp
  ImfChromaticities.cpp
  ImfChromaticitiesAttribute.cpp
  ImfKeyCode.cpp
  ImfKeyCodeAttribute.cpp
  ImfTimeCode.cpp
  ImfTimeCodeAttribute.cpp
  ImfRational.cpp
  ImfRationalAttribute.cpp
  ImfFramesPerSecond.cpp
  ImfStandardAttributes.cpp
  ImfStdIO.cpp
  ImfEnvmap.cpp
  ImfEnvmapAttribute.cpp
  ImfScanLineInputFile.cpp
  ImfTiledInputFile.cpp
  ImfTiledMisc.cpp
  ImfTiledOutputFile.cpp
  ImfTiledRgbaFile.cpp
  ImfTileDescriptionAttribute.cpp
  ImfTileOffsets.cpp
  ImfRgbaYca.cpp
  ImfPxr24Compressor.cpp
  ImfTestFile.cpp
  ImfStringVectorAttribute.cpp
  ImfMultiView.cpp
  ImfAcesFile.cpp
  ImfMultiPartOutputFile.cpp
  ImfGenericOutputFile.cpp
  ImfOutputPartData.cpp
  ImfMultiPartInputFile.cpp
  ImfGenericInputFile.cpp
  ImfPartType.cpp
  ImfInputPartData.cpp
  ImfOutputPart.cpp
  ImfTiledOutputPart.cpp
  ImfInputPart.cpp
  ImfTiledInputPart.cpp
  ImfDeepScanLineInputPart.cpp
  ImfDeepScanLineOutputPart.cpp
  ImfDeepScanLineInputFile.cpp
  ImfDeepScanLineOutputFile.cpp
  ImfDeepTiledInputPart.cpp
  ImfDeepTiledOutputPart.cpp
  ImfDeepTiledInputFile.cpp
  ImfDeepTiledOutputFile.cpp
  ImfDeepFrameBuffer.cpp
  ImfDeepCompositing.cpp
  ImfCompositeDeepScanLine.cpp
  ImfDeepImageStateAttribute.cpp
  ImfFastHuf.cpp
  ImfFloatVectorAttribute.cpp
  ImfRle.cpp
  ImfSystemSpecific.cpp
  ImfZip.cpp
)

IF (BUILD_B44EXPLOGTABLE)
  SET_SOURCE_FILES_PROPERTIES (
    ImfB44Compressor.cpp
    PROPERTIES
    OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/b44ExpLogTable.h
  )
ENDIF ()

IF (BUILD_DWALOOKUPS)
  SET_SOURCE_FILES_PROPERTIES (
    ImfDwaCompressor.cpp
    PROPERTIES
    OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dwaLookups.h
  )
ENDIF ()

SET (OPENEXR_LIB_TARGETS "")

IF ( OPENEXR_BUILD_SHARED )
  LIST ( APPEND OPENEXR_LIB_TARGETS IlmImf )

  ADD_LIBRARY ( IlmImf SHARED ${ILMIMF_SRCS} )

  TARGET_COMPILE_DEFINITIONS ( IlmImf PRIVATE ILMIMF_EXPORTS )
  IF (WIN32)
    TARGET_COMPILE_DEFINITIONS ( IlmImf PUBLIC OPENEXR_DLL )
    TARGET_COMPILE_DEFINITIONS ( IlmImf PUBLIC _CRT_SECURE_NO_WARNINGS )
  ENDIF ()

  TARGET_LINK_LIBRARIES ( IlmImf
    PUBLIC  IlmBase::Iex
    PUBLIC  IlmBase::Half
    PUBLIC  IlmBase::Imath
    PUBLIC  IlmBase::IlmThread
    PRIVATE ${PTHREAD_LIB}
    PRIVATE ${ZLIB_LIBRARIES}
  )
  SET_ILMBASE_INCLUDE_DIRS(IlmImf)

  SET_TARGET_PROPERTIES ( IlmImf
    PROPERTIES
    VERSION ${OPENEXR_VERSION_MAJOR}.${OPENEXR_VERSION_MINOR}.${OPENEXR_VERSION_PATCH}
    SOVERSION ${OPENEXR_SOVERSION}
    OUTPUT_NAME "IlmImf${OPENEXR_LIBSUFFIX}"
    )

  IF (BUILD_DWALOOKUPS)
    ADD_DEPENDENCIES ( IlmImf dwaLookups )
  ENDIF ()

  IF (BUILD_B44EXPLOGTABLE)
    ADD_DEPENDENCIES ( IlmImf b44ExpLogTable )
  ENDIF()

ENDIF ()

IF (OPENEXR_BUILD_STATIC)
  LIST ( APPEND OPENEXR_LIB_TARGETS IlmImf_static )

  ADD_LIBRARY ( IlmImf_static STATIC ${ILMIMF_SRCS} )

  SET_TARGET_PROPERTIES ( IlmImf_static
    PROPERTIES
    VERSION ${OPENEXR_VERSION_MAJOR}.${OPENEXR_VERSION_MINOR}.${OPENEXR_VERSION_PATCH}
    OUTPUT_NAME "IlmImf${OPENEXR_LIBSUFFIX}_s"
    )
    SET_ILMBASE_INCLUDE_DIRS(IlmImf_static)

  IF (WIN32)
    TARGET_COMPILE_DEFINITIONS ( IlmImf_static PUBLIC _CRT_SECURE_NO_WARNINGS )
  ENDIF ()

  iF (BUILD_DWALOOKUPS)
    ADD_DEPENDENCIES ( IlmImf_static b44ExpLogTable )
  ENDIF ()

  IF (BUILD_B44EXPLOGTABLE)
    ADD_DEPENDENCIES ( IlmImf_static dwaLookups )
  ENDIF()

ENDIF ()

IF (OPENEXR_BUILD_SHARED OR OPENEXR_BUILD_STATIC)
  INSTALL ( TARGETS ${OPENEXR_LIB_TARGETS}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION ${RUNTIME_DIR}
  )
ENDIF ()

INSTALL (
  FILES
    ImfForward.h
    ImfExport.h
    ImfAttribute.h
    ImfBoxAttribute.h
    ImfCRgbaFile.h
    ImfChannelList.h
    ImfChannelListAttribute.h
    ImfCompressionAttribute.h
    ImfDoubleAttribute.h
    ImfFloatAttribute.h
    ImfFrameBuffer.h
    ImfHeader.h
    ImfIO.h
    ImfInputFile.h
    ImfIntAttribute.h
    ImfLineOrderAttribute.h
    ImfMatrixAttribute.h
    ImfOpaqueAttribute.h
    ImfOutputFile.h
    ImfRgbaFile.h
    ImfStringAttribute.h
    ImfVecAttribute.h
    ImfHuf.h
    ImfWav.h
    ImfLut.h
    ImfArray.h
    ImfCompression.h
    ImfLineOrder.h
    ImfName.h
    ImfPixelType.h
    ImfVersion.h
    ImfXdr.h
    ImfConvert.h
    ImfPreviewImage.h
    ImfPreviewImageAttribute.h
    ImfChromaticities.h
    ImfChromaticitiesAttribute.h
    ImfKeyCode.h
    ImfKeyCodeAttribute.h
    ImfTimeCode.h
    ImfTimeCodeAttribute.h
    ImfRational.h
    ImfRationalAttribute.h
    ImfFramesPerSecond.h
    ImfStandardAttributes.h
    ImfStdIO.h
    ImfEnvmap.h
    ImfEnvmapAttribute.h
    ImfInt64.h
    ImfRgba.h
    ImfTileDescription.h
    ImfTileDescriptionAttribute.h
    ImfTiledInputFile.h
    ImfTiledOutputFile.h
    ImfTiledRgbaFile.h
    ImfRgbaYca.h
    ImfTestFile.h
    ImfThreading.h
    ImfB44Compressor.h
    ImfStringVectorAttribute.h
    ImfMultiView.h
    ImfAcesFile.h
    ImfMultiPartOutputFile.h
    ImfGenericOutputFile.h
    ImfMultiPartInputFile.h
    ImfGenericInputFile.h
    ImfPartType.h
    ImfPartHelper.h
    ImfOutputPart.h
    ImfTiledOutputPart.h
    ImfInputPart.h
    ImfTiledInputPart.h
    ImfDeepScanLineOutputFile.h
    ImfDeepScanLineOutputPart.h
    ImfDeepScanLineInputFile.h
    ImfDeepScanLineInputPart.h
    ImfDeepTiledInputFile.h
    ImfDeepTiledInputPart.h
    ImfDeepTiledOutputFile.h
    ImfDeepTiledOutputPart.h
    ImfDeepFrameBuffer.h
    ImfDeepCompositing.h
    ImfCompositeDeepScanLine.h
    ImfNamespace.h
    ImfMisc.h
    ImfDeepImageState.h
    ImfDeepImageStateAttribute.h
    ImfFloatVectorAttribute.h

  DESTINATION
    include/OpenEXR
)

if (OPENEXR_BUILD_SHARED)
    add_library(OpenEXR::IlmImf ALIAS IlmImf)
endif()
if (OPENEXR_BUILD_STATIC)
    add_library(OpenEXR::IlmImf_static ALIAS IlmImf_static)
endif()
