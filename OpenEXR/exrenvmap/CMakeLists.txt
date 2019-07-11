
add_executable( exrenvmap
  makeLatLongMap.cpp
  readInputImage.cpp
  resizeImage.cpp
  makeCubeMap.cpp
  main.cpp
  blurImage.cpp
  EnvmapImage.cpp
)

target_link_libraries(exrenvmap OpenEXR::IlmImf)
install(TARGETS exrenvmap DESTINATION ${CMAKE_INSTALL_BINDIR})
