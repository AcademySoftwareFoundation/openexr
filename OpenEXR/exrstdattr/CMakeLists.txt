
add_executable(exrstdattr main.cpp)
target_link_libraries(exrstdattr OpenEXR::IlmImf)
install(TARGETS exrstdattr DESTINATION ${CMAKE_INSTALL_BINDIR})
