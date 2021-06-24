#!/usr/bin/env bash

#
# Validate the libary symlinks:
#   * The actual elf binary is, e.g. libIlmThread-3_1.so.29.0.0
#   * The symlinks are:
#       libIlmThread.so -> libIlmThread-3_1.so
#       libIlmThread-3_1.so -> libIlmThread-3_1.so.29
#       libIlmThread-3_1.so.29 -> libIlmThread-3_1.so.29.0.0
#
# Extract the version by compiling a program that prints the
# OPENEXR_VERSION_STRING. This also validates that the program
# compiles and executes with the info from pkg-config.
# 

if [[ $# == "0" ]]; then
    echo "usage: $0 BUILD_ROOT [SRC_ROOT]"
    exit -1
fi

BUILD_ROOT=$1
SRC_ROOT=$2

# Locate OpenEXR.pc and set PKG_CONFIG_PATH accordingly

pkgconfig=$(find $BUILD_ROOT -name OpenEXR.pc)
if [[ "$pkgconfig" == "" ]]; then
    echo "Can't find OpenEXR.pc"
    exit -1
fi    
export PKG_CONFIG_PATH=$(dirname $pkgconfig)

# Build the validation program

CXX_FLAGS=$(pkg-config OpenEXR --cflags)
LD_FLAGS=$(pkg-config OpenEXR --libs --static)

VALIDATE_CPP=$(mktemp --tmpdir "validate_XXX.cpp")
VALIDATE_BIN=$(mktemp --tmpdir "validate_XXX")
trap "rm -rf $VALIDATE_CPP $VALIDATE_BIN" exit

echo -e '#include <ImfHeader.h>\n#include <OpenEXRConfig.h>\n#include <stdio.h>\nint main() { puts(OPENEXR_PACKAGE_STRING); Imf::Header h; return 0; }' > $VALIDATE_CPP

g++ $CXX_FLAGS $VALIDATE_CPP -o $VALIDATE_BIN $LD_FLAGS

# Execute the program

LIB_DIR=$(pkg-config OpenEXR --variable=libdir)
export LD_LIBRARY_PATH=$LIB_DIR

validate=`$VALIDATE_BIN`
status=$?

echo $validate

if [[ "$status" != "0" ]]; then
   echo "validate failed: $validate"
   exit -1
fi

# Get the suffix, e.g. -2_5_d, and determine if there's also a _d
libsuffix=$(pkg-config OpenEXR --variable=libsuffix)
if [[ $libsuffix != $(basename ./$libsuffix _d) ]]; then
    _d="_d"
else
    _d=""
fi

# Validate each of the libs
libs=$(pkg-config OpenEXR --libs-only-l | sed -e s/-l//g)
for lib in $libs; do

    base=$(echo $lib | cut -d- -f1)
    suffix=$(echo $lib | cut -d- -f2)

    if [[ -f $LIB_DIR/lib$base$_d.so ]]; then 
        libbase=$(readlink $LIB_DIR/lib$base$_d.so)
        libcurrent=$(readlink $LIB_DIR/$libbase)
        libversion=$(readlink $LIB_DIR/$libcurrent)
        file $LIB_DIR/$libversion | grep -q "ELF"

        if [[ "$?" != 0 ]]; then
            echo "Broken libs: lib$base.so -> $libbase -> $libcurrent -> $libversion"
            exit -1
        fi

        echo "lib$base.so -> $libbase -> $libcurrent -> $libversion"

    elif [[ ! -f $LIB_DIR/lib$lib.a ]]; then
        echo "No static lib: $LIB_DIR/lib$lib.a"
    else
        echo "Static lib lib$lib.a"
    fi

done

# Confirm no broken .so symlinks 
file $LIB_DIR/lib* | grep -q broken 
if [[ "$?" == "0" ]]; then
  echo "Broken symbolic link."
  exit -1
fi

if [[ "$SRC_ROOT" != "" ]]; then
    version=$(pkg-config OpenEXR --modversion)
    notes=$(grep "\* \[Version $version\]" $SRC_ROOT/CHANGES.md | head -1)
    if [[ "$notes" == "" ]]; then
        echo "No release notes."
    else
        echo "Release notes: $notes"
    fi
fi
   
echo "ok."
