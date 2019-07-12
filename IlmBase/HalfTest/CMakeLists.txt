
add_executable(HalfTest
  main.cpp
  testArithmetic.cpp
  testBitPatterns.cpp
  testClassification.cpp
  testError.cpp
  testFunction.cpp
  testLimits.cpp
  testSize.cpp
)

target_link_libraries(HalfTest IlmBase::Half)
add_test(NAME IlmBase.Half COMMAND $<TARGET_FILE:HalfTest>)
