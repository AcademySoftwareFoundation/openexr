
add_executable(exr2aces main.cpp)
target_link_libraries(exr2aces OpenEXR::IlmImf)
install(TARGETS exr2aces DESTINATION ${CMAKE_INSTALL_BINDIR})
