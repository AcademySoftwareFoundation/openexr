# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project

add_library(website_src OBJECT
    C_IStream_read.cpp
    C_IStream_seekg.cpp
    C_IStream_tellg.cpp
    C_IStream_clear.cpp

    compression.cpp
    envmap.cpp
    multithreading.cpp
    mergeOverlappingSamples.cpp
    readDeepScanLineFile.cpp
    readDeepTiledFile.cpp
    readGZ1.cpp
    readGZ2.cpp
    readHeader.cpp
    readComments1.cpp
    readComments2.cpp
    readRgba1.cpp
    readRgba2.cpp
    readRgbaFILE.cpp
    readTiled1.cpp
    readTiledRgba1.cpp
    readChannels1.cpp
    readChannels2.cpp
    readLayers.cpp
    splitVolumeSample.cpp
    writeDeepScanLineFile.cpp
    writeDeepTiledFile.cpp
    writeGZ1.cpp
    writeGZ2.cpp
    writeRgba1.cpp
    writeRgba2.cpp
    writeRgba3.cpp
    writeRgbaMT.cpp
    writeTiledRgbaONE1.cpp
    writeRgbaWithPreview1.cpp
    writeTiled1.cpp
    tileDescription.cpp
    validExrFile.cpp
    previewImageExamples.cpp
)

if(NOT WIN32)
    target_sources(website_src PRIVATE
        MemoryMappedIStream_isMemoryMapped.cpp
        MemoryMappedIStream_destructor.cpp
        MemoryMappedIStream_constructor.cpp
        MemoryMappedIStream_read.cpp
        MemoryMappedIStream_readMemoryMapped.cpp
    )
endif()

target_compile_options(website_src PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/wd4101>

    $<$<CXX_COMPILER_ID:GNU>:-Wno-unused-variable>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-unused-but-set-variable>

    $<$<CXX_COMPILER_ID:Clang>:-Wno-unused-variable>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-unused-but-set-variable>
)

target_link_libraries(website_src PRIVATE OpenEXR::OpenEXR)
