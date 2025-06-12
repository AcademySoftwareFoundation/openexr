#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

TAG="$1"

SUDO=$(command -v sudo >/dev/null 2>&1 && echo sudo || echo "")

git clone https://github.com/AcademySoftwareFoundation/Imath.git
cd Imath

git checkout ${TAG}

cmake -S . -B _build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
$SUDO cmake --build _build \
      --target install \
      --config Release \
      --parallel 2

cd ..
rm -rf Imath
