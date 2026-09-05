#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# This script is used by the ci_workflow.yml/ci_steps.yml CI to
# install zstd. This is a part of the process that validates that
# OpenEXR's cmake properly locates the zstd dependency, either
# finding this installation or using the library vendored in the
# source tree.

set -ex

TAG="$1"

# The sudo is nececessary since the installation goes to /usr/local.
SUDO=$(command -v sudo >/dev/null 2>&1 && echo sudo || echo "")

git clone https://github.com/facebook/zstd
pushd zstd

git checkout ${TAG}

# zstd's CMake project lives in build/cmake; build out-of-source.
cmake -S build/cmake -B _build -DCMAKE_BUILD_TYPE=Release
$SUDO cmake --build _build \
      --target install \
      --config Release \
      --parallel 2

popd
rm -rf zstd
