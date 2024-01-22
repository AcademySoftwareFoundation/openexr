# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# ------------------------------------------------------------------------------
# AUTOMATIC COMPRESSOR INTEGRATION
# 
# This sections reads all "Imf*Compressor.h" files in this directory and grabs
# some metadata embeded as comments:
# 
# // Mandatory section to register this compression method in OpenEXR
# // ----------------------------------------------------------------
# // RLE_COMPRESSION = 1
# // RLE_COMPRESSION name rle
# // RLE_COMPRESSION desc run-length encoding.
# // RLE_COMPRESSION scanlines 1
# // RLE_COMPRESSION lossy false
# // RLE_COMPRESSION deep true
# // RLE_COMPRESSION newscan RleCompressor (hdr, maxScanLineSize)
# // RLE_COMPRESSION newtile RleCompressor (hdr, uiMult (tileLineSize, numTileLines))
# 
# Each new compressor needs to add that metadata to seamlessly integrate into 
# the build. The collected metadata will be used to update:
#   - ImfCompression.h
#   - ImfCompression.cpp
#   - ImfCompressor.cpp
#   - ImfCRgbaFile.h
# ------------------------------------------------------------------------------

message(STATUS "Parsing compression headers")
set(tab "    ")
set(comp_enum_list "${tab}NO_COMPRESSION = 0,")
set(NO_COMPRESSION_name "none")
set(NO_COMPRESSION_desc "no compression.")
set(NO_COMPRESSION_scanlines "1")
set(NO_COMPRESSION_lossy "false")
set(NO_COMPRESSION_deep "true")

# get list of headers
file(GLOB compressor_headers Imf*Compressor.h)
list(REMOVE_ITEM compressor_headers "ImfCompressor.h")
set(id_list)    # id list for sanity checks
foreach(header ${compressor_headers})
  # extract registration metadata
  file(STRINGS ${header} meta REGEX "// [A-Z0-9]+_COMPRESSION *.+")
  # process each field
  foreach(field ${meta})
    string(REGEX MATCH "[A-Z0-9]+_COMPRESSION" enum_name ${field})
    if(field MATCHES "// [A-Z0-9]+_COMPRESSION *=.+")
      # extract enum ids
      string(REGEX MATCH "[0-9]+$" id ${field})
      # sanity check: no duplicates
      list(FIND id_list ${id} check)
      if(check GREATER -1)
        message(FATAL_ERROR "Found duplicate compressor id in ${field} !")
      else()
        list(APPEND id_list ${id})
      endif()
      # end of sanity-checking
      string(SUBSTRING ${field} 3 -1 enum)
      string(PREPEND enum ${tab} )
      string(PREPEND enum ${id} ) # prepend id for sorting
      string(APPEND enum "," )
      list(APPEND comp_enum_list ${enum})
    else()
      # store other fields in named variables, i.e. ${RLE_COMPRESION_name}
      string(REGEX MATCH "${enum_name} *([a-z]+) (.+)" tmp ${field})
      set(data_name ${CMAKE_MATCH_1})
      set(data_value ${CMAKE_MATCH_2})
      set(${enum_name}_${data_name} ${data_value})
    endif()
  endforeach()
endforeach()

# sanity check: no gaps in enum ids
list(SORT id_list COMPARE NATURAL)
set(previous_id 0)
foreach(id ${id_list})
  math(EXPR predicted_id "${previous_id} + 1")
  if(NOT id EQUAL predicted_id)
    message(FATAL_ERROR "Found a gap in compressor id list, between ${previous_id} and ${id}. id_list=${id_list} !")
  endif()
  set(previous_id ${predicted_id})
endforeach()

# sort enum list and remove prefixed id
list(SORT comp_enum_list COMPARE NATURAL)
list(TRANSFORM comp_enum_list REPLACE "^[0-9]+" "")
# final enum fields formatting
list(APPEND comp_enum_list "${tab}NUM_COMPRESSION_METHODS")
string(JOIN "\n" EXR_CODEC_ENUM_DEFINITION ${comp_enum_list})

# build codec declaration string
foreach(enum_field ${comp_enum_list})
  string(REGEX MATCH "[A-Z0-9]+_COMPRESSION" enum_name ${enum_field})
  if(NOT enum_name STREQUAL "NUM_COMPRESSION")
    list(APPEND decls "    CompressionDesc (")
    list(APPEND decls "        \"${${enum_name}_name}\",")
    list(APPEND decls "        \"${${enum_name}_desc}\",")
    list(APPEND decls "        ${${enum_name}_scanlines},")
    list(APPEND decls "        ${${enum_name}_lossy},")
    list(APPEND decls "        ${${enum_name}_deep}),")
  endif()
endforeach()
string(JOIN "\n" EXR_CODEC_DECLARATIONS ${decls})

# build codec name to id map
set(name_to_id "${tab}{\"no\", Compression::NO_COMPRESSION},")
foreach(enum_field ${comp_enum_list})
  string(REGEX MATCH "[A-Z0-9]+_COMPRESSION" enum_name ${enum_field})
  if(NOT enum_name STREQUAL "NUM_COMPRESSION")
    list(APPEND name_to_id "${tab}{\"${${enum_name}_name}\", Compression::${enum_name}},")
  endif()
endforeach()
string(JOIN "\n" EXR_CODEC_NAME_TO_ID ${name_to_id})

# build include string
foreach(header ${compressor_headers})
  cmake_path(GET header FILENAME filename)
  list(APPEND includes "#include \"${filename}\"")
endforeach()
string(JOIN "\n" EXR_COMPRESSION_INCLUDES ${includes})

# build string to instance both compressor types (scanline and tile)
foreach(enum_field ${comp_enum_list})
  string(REGEX MATCH "[A-Z0-9]+_COMPRESSION" enum_name ${enum_field})
  if(NOT enum_name STREQUAL "NUM_COMPRESSION" AND NOT enum_name STREQUAL "NO_COMPRESSION")
    list(APPEND new_scan_comp "${tab}${tab}case ${enum_name}:")
    list(APPEND new_scan_comp "${tab}${tab}${tab}return new ${${enum_name}_newscan}\;")
    list(APPEND new_tile_comp "${tab}${tab}case ${enum_name}:")
    list(APPEND new_tile_comp "${tab}${tab}${tab}return new ${${enum_name}_newtile}\;")
    endif()
endforeach()
string(JOIN "\n\n" EXR_NEW_COMPRESSOR ${new_scan_comp})
string(JOIN "\n\n" EXR_NEW_TILE_COMPRESSOR ${new_tile_comp})

# build defines string for ImfCRgbaFile.h
foreach(enum ${comp_enum_list})
  string(STRIP "${enum}" enum)
  list(APPEND crgba_defines "#define IMF_${enum}")
endforeach()
string(JOIN "\n" EXR_CRGBA_DEFINES ${crgba_defines})

# create the files
message(STATUS "Configuring compression methods")
configure_file(ImfCompression.h.in ${CMAKE_CURRENT_SOURCE_DIR}/ImfCompression.h)
configure_file(ImfCompression.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/ImfCompression.cpp)
configure_file(ImfCompressor.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/ImfCompressor.cpp)
configure_file(ImfCRgbaFile.h.in ${CMAKE_CURRENT_SOURCE_DIR}/ImfCRgbaFile.h)

# ------------------------------------------------------------------------------
# END OF AUTOMATIC COMPRESSOR INTEGRATION
# ------------------------------------------------------------------------------

openexr_define_library(OpenEXR
  PRIV_EXPORT OPENEXR_EXPORTS
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    ImfAutoArray.h
    ImfB44Compressor.h
    ImfCheckedArithmetic.h
    ImfCompression.h
    ImfCompressor.h
    ImfDwaCompressor.h
    ImfDwaCompressorSimd.h
    ImfFastHuf.h
    ImfInputPartData.h
    ImfInputStreamMutex.h
    ImfMisc.h
    ImfOptimizedPixelReading.h
    ImfOutputPartData.h
    ImfOutputStreamMutex.h
    ImfPizCompressor.h
    ImfPxr24Compressor.h
    ImfRle.h
    ImfRleCompressor.h
    ImfScanLineInputFile.h
    ImfSimd.h
    ImfSystemSpecific.h
    ImfTileOffsets.h
    ImfTiledMisc.h
    ImfZip.h
    ImfZipCompressor.h
    b44ExpLogTable.h
    dwaLookups.h
    ImfAcesFile.cpp
    ImfAttribute.cpp
    ImfB44Compressor.cpp
    ImfBoxAttribute.cpp
    ImfChannelList.cpp
    ImfChannelListAttribute.cpp
    ImfChromaticities.cpp
    ImfChromaticitiesAttribute.cpp
    ImfCompositeDeepScanLine.cpp
    ImfCompressionAttribute.cpp
    ImfCompressor.cpp
    ImfCompression.cpp
    ImfConvert.cpp
    ImfCRgbaFile.cpp
    ImfDeepCompositing.cpp
    ImfDeepFrameBuffer.cpp
    ImfDeepImageStateAttribute.cpp
    ImfDeepScanLineInputFile.cpp
    ImfDeepScanLineInputPart.cpp
    ImfDeepScanLineOutputFile.cpp
    ImfDeepScanLineOutputPart.cpp
    ImfDeepTiledInputFile.cpp
    ImfDeepTiledInputPart.cpp
    ImfDeepTiledOutputFile.cpp
    ImfDeepTiledOutputPart.cpp
    ImfDoubleAttribute.cpp
    ImfDwaCompressor.cpp
    ImfEnvmap.cpp
    ImfEnvmapAttribute.cpp
    ImfFastHuf.cpp
    ImfFloatAttribute.cpp
    ImfFloatVectorAttribute.cpp
    ImfFrameBuffer.cpp
    ImfFramesPerSecond.cpp
    ImfGenericInputFile.cpp
    ImfGenericOutputFile.cpp
    ImfHeader.cpp
    ImfHuf.cpp
    ImfIDManifest.cpp
    ImfIDManifestAttribute.cpp
    ImfInputFile.cpp
    ImfInputPart.cpp
    ImfInputPartData.cpp
    ImfIntAttribute.cpp
    ImfIO.cpp
    ImfKeyCode.cpp
    ImfKeyCodeAttribute.cpp
    ImfLineOrderAttribute.cpp
    ImfLut.cpp
    ImfMatrixAttribute.cpp
    ImfMisc.cpp
    ImfMultiPartInputFile.cpp
    ImfMultiPartOutputFile.cpp
    ImfMultiView.cpp
    ImfOpaqueAttribute.cpp
    ImfOutputFile.cpp
    ImfOutputPart.cpp
    ImfOutputPartData.cpp
    ImfPartType.cpp
    ImfPizCompressor.cpp
    ImfPreviewImage.cpp
    ImfPreviewImageAttribute.cpp
    ImfPxr24Compressor.cpp
    ImfRational.cpp
    ImfRationalAttribute.cpp
    ImfRgbaFile.cpp
    ImfRgbaYca.cpp
    ImfRle.cpp
    ImfRleCompressor.cpp
    ImfScanLineInputFile.cpp
    ImfStandardAttributes.cpp
    ImfStdIO.cpp
    ImfStringAttribute.cpp
    ImfStringVectorAttribute.cpp
    ImfSystemSpecific.cpp
    ImfTestFile.cpp
    ImfThreading.cpp
    ImfTileDescriptionAttribute.cpp
    ImfTiledInputFile.cpp
    ImfTiledInputPart.cpp
    ImfTiledMisc.cpp
    ImfTiledOutputFile.cpp
    ImfTiledOutputPart.cpp
    ImfTiledRgbaFile.cpp
    ImfTileOffsets.cpp
    ImfTimeCode.cpp
    ImfTimeCodeAttribute.cpp
    ImfVecAttribute.cpp
    ImfVersion.cpp
    ImfWav.cpp
    ImfZip.cpp
    ImfZipCompressor.cpp
  HEADERS
    ImfAcesFile.h
    ImfArray.h
    ImfAttribute.h
    ImfBoxAttribute.h
    ImfChannelList.h
    ImfChannelListAttribute.h
    ImfChromaticities.h
    ImfChromaticitiesAttribute.h
    ImfCompositeDeepScanLine.h
    ImfCompression.h
    ImfCompressionAttribute.h
    ImfCompressor.h
    ImfConvert.h
    ImfCRgbaFile.h
    ImfDeepCompositing.h
    ImfDeepFrameBuffer.h
    ImfDeepImageState.h
    ImfDeepImageStateAttribute.h
    ImfDeepScanLineInputFile.h
    ImfDeepScanLineInputPart.h
    ImfDeepScanLineOutputFile.h
    ImfDeepScanLineOutputPart.h
    ImfDeepTiledInputFile.h
    ImfDeepTiledInputPart.h
    ImfDeepTiledOutputFile.h
    ImfDeepTiledOutputPart.h
    ImfDoubleAttribute.h
    ImfEnvmap.h
    ImfEnvmapAttribute.h
    ImfExport.h
    ImfFloatAttribute.h
    ImfFloatVectorAttribute.h
    ImfForward.h
    ImfFrameBuffer.h
    ImfFramesPerSecond.h
    ImfGenericInputFile.h
    ImfGenericOutputFile.h
    ImfHeader.h
    ImfHuf.h
    ImfIDManifest.h
    ImfIDManifestAttribute.h
    ImfInputFile.h
    ImfInputPart.h
    ImfInt64.h
    ImfIntAttribute.h
    ImfIO.h
    ImfKeyCode.h
    ImfKeyCodeAttribute.h
    ImfLineOrder.h
    ImfLineOrderAttribute.h
    ImfLut.h
    ImfMatrixAttribute.h
    ImfMisc.h
    ImfMultiPartInputFile.h
    ImfMultiPartOutputFile.h
    ImfMultiView.h
    ImfName.h
    ImfNamespace.h
    ImfOpaqueAttribute.h
    ImfOutputFile.h
    ImfOutputPart.h
    ImfPartHelper.h
    ImfPartType.h
    ImfPixelType.h
    ImfPreviewImage.h
    ImfPreviewImageAttribute.h
    ImfRational.h
    ImfRationalAttribute.h
    ImfRgba.h
    ImfRgbaFile.h
    ImfRgbaYca.h
    ImfStandardAttributes.h
    ImfStdIO.h
    ImfStringAttribute.h
    ImfStringVectorAttribute.h
    ImfTestFile.h
    ImfThreading.h
    ImfTileDescription.h
    ImfTileDescriptionAttribute.h
    ImfTiledInputFile.h
    ImfTiledInputPart.h
    ImfTiledOutputFile.h
    ImfTiledOutputPart.h
    ImfTiledRgbaFile.h
    ImfTimeCode.h
    ImfTimeCodeAttribute.h
    ImfVecAttribute.h
    ImfVersion.h
    ImfWav.h
    ImfXdr.h
  DEPENDENCIES
    Imath::Imath
    OpenEXR::Config
    OpenEXR::Iex
    OpenEXR::IlmThread
    OpenEXR::OpenEXRCore
  )
