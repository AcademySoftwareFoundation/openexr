project(PackageTest)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)

find_package(IlmBase)
find_package(OpenEXR)

add_executable(testPackage main.cpp)
target_link_libraries(testPackage IlmBase::Imath OpenEXR::IlmImf)
