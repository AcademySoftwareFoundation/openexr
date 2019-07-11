
add_executable(exrmakepreview
  main.cpp
  makePreview.cpp
)
target_link_libraries(exrmakepreview OpenEXR::IlmImf)
install(TARGETS exrmakepreview DESTINATION ${CMAKE_INSTALL_BINDIR})
