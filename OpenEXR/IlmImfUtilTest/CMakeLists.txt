add_executable(IlmImfUtilTest
  main.cpp
  testFlatImage.cpp
  testDeepImage.cpp
  testIO.cpp
 )
target_link_libraries(IlmImfUtilTest OpenEXR::IlmImfUtil)
add_test(NAME OpenEXR.IlmImfUtil COMMAND $<TARGET_FILE:IlmImfUtilTest>)
