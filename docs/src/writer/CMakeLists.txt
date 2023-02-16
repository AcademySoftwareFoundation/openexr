cmake_minimum_required(VERSION 3.10)
project(exrwriter)
find_package(OpenEXR REQUIRED)

add_executable(${PROJECT_NAME} writer.cpp)
target_link_libraries(${PROJECT_NAME} OpenEXR::OpenEXR)

