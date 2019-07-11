
add_executable(exrmultiview
  makeMultiView.cpp
  main.cpp
  Image.cpp
)
target_link_libraries(exrmultiview OpenEXR::IlmImf)
install(TARGETS exrmultiview DESTINATION ${CMAKE_INSTALL_BINDIR})
