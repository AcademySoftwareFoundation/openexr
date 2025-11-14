#!/bin/bash -eux

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# Test script to help debug the oss-fuzz project build.sh:
# https://github.com/google/oss-fuzz/blob/master/projects/openexr/build.sh
#
# Set WORK to be the parent of the openexr repo.
# Set OUT to be an arbitrary directory (in this case gets created by CMake)
# Set SRC to be the openexr repo
#
# This assumes that https://github.com/google/oss-fuzz is cloned
# alongside the openexr repo, at the same directory level.
#

set -x

export GIT_ROOT="$(git rev-parse --show-toplevel)"
export WORK="$(dirname $GIT_ROOT)"
export SRC=$WORK
export OSS_FUZZ_HOME=$WORK/oss-fuzz/projects/openexr
export OUT=$OSS_FUZZ_HOME/_out

export CXX=$(which g++)
export CXX_FLAGS=""
export CC=$(which gcc)
export CC_FLAGS=""

# stub of a fuzzing engine, just to confirm the target builds
LIB_FUZZING_ENGINE_BASE=/tmp/lib_fuzzing_engine
export LIB_FUZZING_ENGINE=$LIB_FUZZING_ENGINE_BASE.a
export LIB_FUZZING_ENGINE_O=$LIB_FUZZING_ENGINE_BASE.o
export LIB_FUZZING_ENGINE_SRC=$LIB_FUZZING_ENGINE_BASE.cc

cat > $LIB_FUZZING_ENGINE_SRC <<'EOF'
#include <stdint.h>
#include <stdlib.h>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
int
main(int argc, char* argv[])
{
    LLVMFuzzerTestOneInput(0, 0);
    exit(0);
}
EOF

$CXX -c -o $LIB_FUZZING_ENGINE_O $LIB_FUZZING_ENGINE_SRC
ar rcs $LIB_FUZZING_ENGINE $LIB_FUZZING_ENGINE_O

rm -rf $WORK/openexr/_build.oss-fuzz
rm -rf $OUT

source $OSS_FUZZ_HOME/build.sh

ls -l $OUT
