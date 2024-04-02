#!/usr/bin/env bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.
#
# Used by the website.yml workflow
#

set -x

DOXYGEN_VERSION=$1

if [[ $OSTYPE == *linux* ]]; then
    sudo apt-get install -y doxygen
elif [[ $OSTYPE == *darwin* ]]; then
    wget https://github.com/doxygen/doxygen/releases/download/Release_${DOXYGEN_VERSION//./_}/Doxygen-${DOXYGEN_VERSION}.dmg
    sudo hdiutil attach Doxygen-${DOXYGEN_VERSION}.dmg
    sudo cp /Volumes/Doxygen/Doxygen.app/Contents/MacOS/Doxywizard /usr/local/bin
    sudo cp /Volumes/Doxygen/Doxygen.app/Contents/Resources/doxygen /usr/local/bin
    sudo cp /Volumes/Doxygen/Doxygen.app/Contents/Resources/doxyindexer /usr/local/bin
    sudo hdiutil detach /Volumes/Doxygen
elif [[ $OSTYPE == *msys* ]]; then
    mkdir doxygen
    cd doxygen
    curl -kLSs https://github.com/doxygen/doxygen/releases/download/Release_${DOXYGEN_VERSION//./_}/doxygen-${DOXYGEN_VERSION}.windows.x64.bin.zip -o doxygen.zip
    unzip doxygen.zip
    cp * c:\\Windows
fi
