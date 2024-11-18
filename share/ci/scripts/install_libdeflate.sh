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

git clone https://github.com/ebiggers/libdeflate
cd libdeflate

git checkout ${TAG}

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
$SUDO cmake --build . \
      --target install \
      --config Release \
      --parallel 2

cd ../..
rm -rf libdeflate
