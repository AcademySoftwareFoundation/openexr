
add_executable(exrbuild exrbuild.cpp)
target_link_libraries(exrbuild OpenEXR::IlmImf)
install(TARGETS exrbuild DESTINATION ${CMAKE_INSTALL_BINDIR})
