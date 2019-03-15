project(PackageTest)
cmake_minimum_required(VERSION 2.8)

find_package(IlmBase ${ILMBASE_VERSION} EXACT REQUIRED)

add_executable(testPackage main.cpp)
target_link_libraries(testPackage IlmBase::Imath)
