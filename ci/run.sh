#!/usr/bin/env bash
#
# Script to install, build and run unit tests for OpenEXR
#
# MODE (install/build/test)
#
# Author: Dan Bailey

set -ex

MODE="$1"

if [ "$MODE" = "install" ]; then
    # install all the dependencies
    apt-get update
    apt-get install -y zlib1g-dev
    apt-get install -y cmake
    apt-get install -y g++
    apt-get install -y python-dev
    apt-get install -y libboost-python-dev
    apt-get install -y python-numpy
elif [ "$MODE" = "build" ]; then
    # compile IlmBase, OpenEXR and all unit tests
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++ -DCMAKE_INSTALL_PREFIX=/usr ..
    make install -j4
elif [ "$MODE" = "test" ]; then
    # run the unit tests
    build/IlmBase/HalfTest/HalfTest
    build/IlmBase/IexTest/IexTest
    build/IlmBase/ImathTest/ImathTest
    build/OpenEXR/IlmImfTest/IlmImfTest
    build/OpenEXR/IlmImfUtilTest/IlmImfUtilTest
fi
