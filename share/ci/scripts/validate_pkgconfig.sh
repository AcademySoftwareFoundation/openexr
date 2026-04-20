#!/usr/bin/env bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

#
# Validate that the given source file compiles and links with the
# given pkg-config file.
#
# First argument is the directory where Imath is installed
# Second argument is the pkg name, i.e. Imath or PyImath
# Third argument is the test program source 
#

set -x

if [ "$#" -lt 2 ]; then
    echo "Error: Expected 2 arguments, got $#." >&2
    echo "Usage: $0 <install dir> <src file>" >&2
    exit 1
fi

INSTALL=$1
SRC=$2
ARGS=$3

LIB="lib"
if [ -d "$INSTALL/lib64" ]; then
    LIB="lib64"
fi

export PKG_CONFIG_PATH="$INSTALL/$LIB/pkgconfig:/usr/local/$LIB/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"

cflags=$(pkg-config --cflags --libs $ARGS OpenEXR Imath)
bin=$SRC.bin
g++ -std=c++17 $SRC $cflags -o $bin -Wl,-rpath,$INSTALL/$LIB:/usr/local/$LIB
./$bin
