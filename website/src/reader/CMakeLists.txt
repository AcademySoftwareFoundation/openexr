cmake_minimum_required(VERSION 3.12)
project(exrreader)
find_package(OpenEXR REQUIRED)

add_executable(${PROJECT_NAME} reader.cpp)
target_link_libraries(${PROJECT_NAME} OpenEXR::OpenEXR)

