#! /bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

# This script can be used to generate a local test coverage report
# using the clang / llvm toolset for doing so
# It should hopefully be obvious how to adjust, but should work
# on most linux distros

buildtype=Debug
builddir=build.coverage

haveninja=`which ninja`

#imathoverride="-DOPENEXR_FORCE_INTERNAL_IMATH=ON -DIMATH_REPO=/home/user/Development/Imath"

# also turn on most of the warnings because we should look at that
# as well...
cwarns="-fstack-protector-all -Weverything -Wno-reserved-identifier -Wno-covered-switch-default -Wno-cast-align -Wno-overlength-strings -fprofile-arcs -fprofile-instr-generate -fcoverage-mapping"
cxxwarns="-fstack-protector-all -Wno-sign-conversion -Wno-float-equal -Wno-padded -Wno-zero-as-null-pointer-constant -Wno-old-style-cast -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-missing-braces -fprofile-instr-generate -fcoverage-mapping"

genargs=""
if [[ "$haveninja" != "" ]]; then
    genargs="-G Ninja"
fi

if [[ ! -e "${builddir}" ]]; then
    export CC=/usr/bin/clang
    export CXX=/usr/bin/clang++
    
    cmake -B ${builddir} -S . ${genargs} ${imathoverride} -DCMAKE_C_FLAGS="${cwarns}" -DCMAKE_CXX_FLAGS="${cxxwarns}" -DCMAKE_BUILD_TYPE=${buildtype} || exit 1
fi

if [[ "$haveninja" != "" ]]; then
    ninja -C ${builddir} || exit 1
else
    nproc=`cat /proc/cpuinfo|grep processor|wc -l`
    make -j${nproc} -C ${builddir} || exit 1
fi

# archive previous runs????
rm -rf coverage

/usr/bin/env LLVM_PROFILE_FILE=coverage/core.profraw build.coverage/bin/OpenEXRCoreTest || exit 1
/usr/bin/env LLVM_PROFILE_FILE=coverage/exr.profraw build.coverage/bin/OpenEXRTest || exit 1
/usr/bin/env LLVM_PROFILE_FILE=coverage/util.profraw build.coverage/bin/OpenEXRUtilTest || exit 1

llvm-profdata merge -sparse -o coverage/exr.profdata coverage/core.profraw coverage/exr.profraw coverage/util.profraw

llvm-cov show \
         build.coverage/bin/OpenEXRCoreTest \
         -object build.coverage/src/lib/OpenEXRCore/libOpenEXRCore-3_2_d.so \
         -instr-profile=coverage/exr.profdata \
         -show-regions \
         -show-expansions \
         --output-dir=coverage/core_only \
         --format="html" \
         -ignore-filename-regex='src/test/.*' \
         -ignore-filename-regex='build.coverage/.*' \
         -ignore-filename-regex='src/lib/OpenEXR/Imf*'

llvm-cov show \
         build.coverage/bin/OpenEXRTest \
         -object build.coverage/src/lib/OpenEXR/libOpenEXR-3_2_d.so \
         -instr-profile=coverage/exr.profdata \
         -show-regions \
         -show-expansions \
         --output-dir=coverage/exr_only \
         --format="html" \
         -ignore-filename-regex='src/test/.*' \
         -ignore-filename-regex='build.coverage/.*' \
         -ignore-filename-regex='src/lib/OpenEXRCore/*'

llvm-cov show \
         build.coverage/bin/OpenEXRUtilTest \
         -object build.coverage/src/lib/OpenEXRUtil/libOpenEXRUtil-3_2_d.so \
         -instr-profile=coverage/exr.profdata \
         -show-regions \
         -show-expansions \
         --output-dir=coverage/util_only \
         --format="html" \
         -ignore-filename-regex='src/test/.*' \
         -ignore-filename-regex='build.coverage/.*' \
         -ignore-filename-regex='src/lib/OpenEXR/Imf*' \
         -ignore-filename-regex='src/lib/OpenEXRCore/*'

llvm-cov show \
         build.coverage/bin/OpenEXRCoreTest \
         -object build.coverage/bin/OpenEXRTest \
         -object build.coverage/bin/OpenEXRUtilTest \
         -object build.coverage/src/lib/OpenEXRCore/libOpenEXRCore-3_2_d.so \
         -object build.coverage/src/lib/OpenEXR/libOpenEXR-3_2_d.so \
         -object build.coverage/src/lib/OpenEXRUtil/libOpenEXRUtil-3_2_d.so \
         -instr-profile=coverage/exr.profdata \
         -show-regions \
         -show-expansions \
         --output-dir=coverage/combined \
         --format="html" \
         -ignore-filename-regex='src/test/.*' \
         -ignore-filename-regex='build.coverage/.*'
