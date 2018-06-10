# yue.nicholas@gmail.com

SET(CMAKE_INCLUDE_CURRENT_DIR 1)

IF (WIN32)
  SET(RUNTIME_DIR ${ILMBASE_PACKAGE_PREFIX}/bin)
ELSE ()
  SET(RUNTIME_DIR ${ILMBASE_PACKAGE_PREFIX}/lib)
ENDIF ()

ADD_EXECUTABLE ( b44ExpLogTable
  b44ExpLogTable.cpp
)

TARGET_LINK_LIBRARIES ( b44ExpLogTable
  Half${ILMBASE_LIBSUFFIX}
  IlmThread${ILMBASE_LIBSUFFIX}
  Iex${ILMBASE_LIBSUFFIX}
  ${PTHREAD_LIB}
)

TARGET_INCLUDE_DIRECTORIES ( b44ExpLogTable 
    PRIVATE ${CMAKE_INSTALL_PREFIX}/include/OpenEXR )

IF (BUILD_SHARED_LIBS AND WIN32)
    TARGET_COMPILE_DEFINITIONS( b44ExpLogTable PUBLIC OPENEXR_DLL )
ENDIF()



ADD_CUSTOM_COMMAND (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/b44ExpLogTable.h
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/b44ExpLogTable > ${CMAKE_CURRENT_BINARY_DIR}/b44ExpLogTable.h
  DEPENDS b44ExpLogTable
  WORKING_DIRECTORY ${RUNTIME_DIR}
)

ADD_EXECUTABLE ( dwaLookups
  dwaLookups.cpp
)

IF (BUILD_SHARED_LIBS AND WIN32)
    TARGET_COMPILE_DEFINITIONS( dwaLookups PUBLIC OPENEXR_DLL )
ENDIF()

TARGET_LINK_LIBRARIES ( dwaLookups
  Half${ILMBASE_LIBSUFFIX}
  IlmThread${ILMBASE_LIBSUFFIX}
  Iex${ILMBASE_LIBSUFFIX}
  ${PTHREAD_LIB}
)

ADD_CUSTOM_COMMAND (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dwaLookups.h
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/dwaLookups > ${CMAKE_CURRENT_BINARY_DIR}/dwaLookups.h
  DEPENDS dwaLookups
  WORKING_DIRECTORY ${RUNTIME_DIR}
)

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

SET_SOURCE_FILES_PROPERTIES (
  ImfB44Compressor.cpp
  PROPERTIES
  OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/b44ExpLogTable.h
)
SET_SOURCE_FILES_PROPERTIES (
  ImfDwaCompressor.cpp
  PROPERTIES
  OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dwaLookups.h
)


SET (OPENEXR_LIB_TARGETS "")

IF ( BUILD_SHARED_LIBS )
  LIST ( APPEND OPENEXR_LIB_TARGETS IlmImf )

  ADD_LIBRARY ( IlmImf SHARED ${ILMIMF_SRCS} )

  TARGET_COMPILE_DEFINITIONS ( IlmImf PRIVATE ILMIMF_EXPORTS )
  IF (WIN32)
    TARGET_COMPILE_DEFINITIONS ( IlmImf PUBLIC OPENEXR_DLL )
    TARGET_COMPILE_DEFINITIONS ( IlmImf PUBLIC _CRT_SECURE_NO_WARNINGS )
  ENDIF ()

  TARGET_LINK_LIBRARIES ( IlmImf
    Half${ILMBASE_LIBSUFFIX}
    Iex${ILMBASE_LIBSUFFIX}
    Imath${ILMBASE_LIBSUFFIX}
    IlmThread${ILMBASE_LIBSUFFIX}
    ${PTHREAD_LIB} ${ZLIB_LIBRARIES}
  )

  SET_TARGET_PROPERTIES ( IlmImf
    PROPERTIES
    VERSION ${OPENEXR_VERSION_MAJOR}.${OPENEXR_VERSION_MINOR}.${OPENEXR_VERSION_PATCH}
    SOVERSION ${OPENEXR_VERSION_MAJOR}
    OUTPUT_NAME "IlmImf${OPENEXR_LIBSUFFIX}"
    )

  ADD_DEPENDENCIES ( IlmImf b44ExpLogTable )
  ADD_DEPENDENCIES ( IlmImf dwaLookups )

ENDIF ()

IF (BUILD_STATIC_LIBS)
  LIST ( APPEND OPENEXR_LIB_TARGETS IlmImf_static )

  ADD_LIBRARY ( IlmImf_static STATIC ${ILMIMF_SRCS} )

  SET_TARGET_PROPERTIES ( IlmImf_static
    PROPERTIES
    VERSION ${OPENEXR_VERSION_MAJOR}.${OPENEXR_VERSION_MINOR}.${OPENEXR_VERSION_PATCH}
    OUTPUT_NAME "IlmImf${OPENEXR_LIBSUFFIX}_s"
    )

  IF (WIN32)
    TARGET_COMPILE_DEFINITIONS ( IlmImf PUBLIC _CRT_SECURE_NO_WARNINGS )
  ENDIF ()

  ADD_DEPENDENCIES ( IlmImf_static b44ExpLogTable )
  ADD_DEPENDENCIES ( IlmImf_static dwaLookups )

ENDIF ()

IF (BUILD_SHARED_LIBS OR BUILD_STATIC_LIBS)
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
