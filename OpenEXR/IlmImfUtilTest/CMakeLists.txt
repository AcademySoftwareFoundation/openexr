
# IlmImfUtilTest

LINK_DIRECTORIES ( ${CMAKE_CURRENT_BINARY_DIR}/../IlmImf )


ADD_EXECUTABLE ( IlmImfUtilTest
  main.cpp
  testFlatImage.cpp
  testDeepImage.cpp
  testIO.cpp
 )


ADD_TEST ( TestIlmImfUtil IlmImfUtilTest )
TARGET_LINK_LIBRARIES ( IlmImfUtilTest 
        IlmImfUtil
        IlmImf  
        Half 
        Iex-2_2
        Imath-2_2 
        IlmThread-2_2 
        ${PTHREAD_LIB} ${ZLIB_LIBRARIES}
        )

