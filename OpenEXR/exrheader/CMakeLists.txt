
add_executable(exrheader main.cpp)
target_link_libraries(exrheader OpenEXR::IlmImf)
install(TARGETS exrheader DESTINATION ${CMAKE_INSTALL_BINDIR})
