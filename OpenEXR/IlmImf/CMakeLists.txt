# yue.nicholas@gmail.com


ADD_EXECUTABLE ( b44ExpLogTable
  b44ExpLogTable.cpp
  )
TARGET_LINK_LIBRARIES ( b44ExpLogTable
  Half
  Iex-2_1
  IlmThread-2_1
  ${PTHREAD_LIB}
)

SET_TARGET_PROPERTIES(b44ExpLogTable 
  PROPERTIES 
  EXCLUDE_FROM_DEFAULT_BUILD 1 EXCLUDE_FROM_ALL 1
 )

ADD_CUSTOM_COMMAND (
  TARGET b44ExpLogTable 
  POST_BUILD
  COMMAND b44ExpLogTable > b44ExpLogTable.h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/IlmImf
)
SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_SOURCE_DIR}/b44ExpLogTable.h
  PROPERTIES 
  HEADER_FILE_ONLY TRUE
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
)

IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DILMIMF_EXPORTS)
ENDIF()

ADD_LIBRARY ( IlmImf ${LIB_TYPE}
  ${ILMIMF_SRCS}
)

TARGET_LINK_LIBRARIES ( IlmImf 
  Half 
  Iex-2_1 
  Imath-2_1 
  IlmThread-2_1 
  ${PTHREAD_LIB} ${ZLIB_LIBRARIES}
)


ADD_DEPENDENCIES ( IlmImf b44ExpLogTable )

SET_SOURCE_FILES_PROPERTIES (
  ImfB44Compressor.cpp
  PROPERTIES
  OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/b44ExpLogTable.h
)

# Libraries

INSTALL ( TARGETS
  IlmImf
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/lib
)

# Headers
INSTALL ( FILES
  ${CMAKE_SOURCE_DIR}/config/OpenEXRConfig.h
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
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/include/OpenEXR
  )

