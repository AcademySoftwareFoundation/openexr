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

if [[ $# -eq 0 || "$1" == "master" ]]; then
  # no version specified: use master
  version="master"
  zip="master.zip"
  url="https://github.com/aous72/OpenJPH/archive/refs/heads/$zip"
  # Get the SHA of the master branch HEAD
  master_sha=$(git ls-remote https://github.com/aous72/OpenJPH.git refs/heads/master | cut -f1)
else
  version="$1"
  zip="$version.zip"
  url="https://github.com/aous72/OpenJPH/archive/refs/tags/$zip"
  if ! curl --head --silent --fail --location "$url" > /dev/null; then
    echo "No such OpenJPH tag: $version"
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

if $do_git; then
    git rm -rf --ignore-unmatch OpenJPH
fi
rm -rf OpenJPH
mv $OpenJPH OpenJPH

if [[ "$OSTYPE" == "darwin"* ]]; then
  sed_cmd=(sed -i '')
else
  sed_cmd=(sed -i)
fi

# Force a static build
"${sed_cmd[@]}" '/^option(BUILD_SHARED_LIBS "Shared Libraries" ON)/d' OpenJPH/CMakeLists.txt
"${sed_cmd[@]}" 's/^add_library(openjph \${SOURCES})/add_library(openjph STATIC ${SOURCES})/' OpenJPH/src/core/CMakeLists.txt

# Headers live under "common" in older OpenJPH releases but are included via
# "openjph/ojph_arch.h". Rename the directory if needed.
if [ -d OpenJPH/src/core/common ]; then
    mv OpenJPH/src/core/common OpenJPH/src/core/openjph
    "${sed_cmd[@]}" 's,/common/,/openjph/,' OpenJPH/ojph_version.cmake
    "${sed_cmd[@]}" 's,/common,/openjph,' OpenJPH/src/core/CMakeLists.txt
    "${sed_cmd[@]}" 's,common/,openjph/,' OpenJPH/src/core/CMakeLists.txt
fi

if [[ -n "${master_sha:-}" ]]; then
  echo "#define OPENJPH_VERSION_SHA ${master_sha}" >> OpenJPH/src/core/openjph/ojph_version.h
fi

if $do_git; then
  git add OpenJPH
fi



