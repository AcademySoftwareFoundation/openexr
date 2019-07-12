add_executable(IexTest
  main.cpp
  testBaseExc.cpp
)

target_link_libraries(IexTest IlmBase::Iex)
add_test(NAME IlmBase.Iex COMMAND $<TARGET_FILE:IexTest>)
