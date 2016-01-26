# yue.nicholas@gmail.com

ADD_EXECUTABLE ( IlmImfFuzzTest
  fuzzFile.cpp
  main.cpp
  testFuzzDeepTiles.cpp
  testFuzzDeepScanLines.cpp
  testFuzzScanLines.cpp
  testFuzzTiles.cpp
  )

TARGET_LINK_LIBRARIES ( IlmImfFuzzTest 
        IlmImf
        IlmBase::Half
        IlmBase::Imath
        ${PTHREAD_LIB} ${ZLIB_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES ( IlmImfFuzzTest
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  $<TARGET_PROPERTY:IlmImf,INTERFACE_INCLUDE_DIRS>
  $<TARGET_PROPERTY:IlmBase::Half,INTERFACE_INCLUDE_DIRS>
  $<TARGET_PROPERTY:IlmBase::Imath,INTERFACE_INCLUDE_DIRS>
)

ADD_TEST ( TestIlmImfFuzz IlmImfFuzzTest )
