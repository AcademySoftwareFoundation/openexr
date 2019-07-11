
add_executable(exrmultipart exrmultipart.cpp)
target_link_libraries(exrmultipart OpenEXR::IlmImf)
install(TARGETS exrmultipart DESTINATION ${CMAKE_INSTALL_BINDIR})
