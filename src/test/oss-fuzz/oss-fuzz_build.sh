#!/bin/bash -eux

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# This script is invoked by
# https://github.com/google/oss-fuzz/blob/master/projects/openexr/build.sh
# to build the OpenEXR fuzzers.
#

set -x

cmake -S $SRC/openexr -B $BUILD_DIR --preset oss_fuzz
cmake --build $BUILD_DIR --target oss_fuzz -j"$(nproc)"
cmake --install $BUILD_DIR --component oss_fuzz
