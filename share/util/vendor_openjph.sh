#! /bin/bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

#
# Download a given release of OpenJPH and vendor it into the source tree.
# This removes the existing content under the external/OpenJPH directory and
# runs "git add" on the new content.
#
# Run this from the project root:
#
# ./share/util/vendor_openjph.sh 0.24.2
#

set -xeuo pipefail

do_git=true
if [[ "${1:-}" == "-git" ]]; then
    do_git=false
    shift
fi

if [[ $# -lt 1 ]]; then
  # no version specified: use master
  version="master"
  zip="master.zip"
  url="https://github.com/aous72/OpenJPH/archive/refs/heads/$zip"
else
  version="$1"
  zip="$version.zip"
  url="https://github.com/aous72/OpenJPH/archive/refs/tags/$zip"
  if curl --head --silent --fail "$url" > /dev/null; then
    echo "Tag such OpenJPH tag: $version"
    exit 1
  fi
fi

OpenJPH=OpenJPH-$version

# Only vendor in the required files
files=(
    "$OpenJPH/CMakeLists.txt"
    "$OpenJPH/target_arch.cmake"
    "$OpenJPH/ojph_version.cmake"
    "$OpenJPH/src/core/*"
    "$OpenJPH/src/openjph-config.cmake.in"
    "$OpenJPH/src/openjph.pc.in"
    "$OpenJPH/README.md"
    "$OpenJPH/LICENSE"
)

cd external

trap 'rm -rf "$zip" "$OpenJPH"' EXIT
wget $url
unzip -o "$zip" "${files[@]}"

if $do_git && [ -d OpenJPH ]; then
    git rm -rf OpenJPH
else
    rm -rf OpenJPH
fi
mv $OpenJPH OpenJPH

# Headers live under "common" in the source but are included via
# "openjph/ojph_arch.h". Create a "openjph" symlink to the header directory.
ln -s common OpenJPH/src/core/openjph

# Force a static build
sed -i '/^option(BUILD_SHARED_LIBS "Shared Libraries" ON)/d' OpenJPH/CMakeLists.txt
sed -i 's/^add_library(openjph \${SOURCES})/add_library(openjph STATIC ${SOURCES})/' OpenJPH/src/core/CMakeLists.txt

if $do_git; then
  git add OpenJPH
fi



