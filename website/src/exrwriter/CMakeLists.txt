cmake_minimum_required(VERSION 3.12)
project(exrwriter)
find_package(OpenEXR REQUIRED)

add_executable(${PROJECT_NAME} exrwriter.cpp)
target_link_libraries(${PROJECT_NAME} OpenEXR::OpenEXR)

