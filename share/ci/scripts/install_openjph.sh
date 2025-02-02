#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

TAG="$1"

if [[ $OSTYPE == "msys" ]]; then
    SUDO=""
else
    SUDO="sudo"
fi

git clone -b add-export https://github.com/palemieux/OpenJPH.git
cd OpenJPH

# git checkout ${TAG}

cd build
cmake -DOJPH_ENABLE_TIFF_SUPPORT=OFF -DCMAKE_BUILD_TYPE=Release .. 
$SUDO cmake --build . \
      --target install \
      --config Release \
      --parallel 2

cd ../..
rm -rf OpenJPH
