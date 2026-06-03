#! /bin/bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.
#
# Download a given release of zstd and vendor it into the source tree.
# This removes the existing lib content under external/zstd and runs
# "git add" on the new content. The vendored zstd sources are compiled
# directly into OpenEXRCore (see src/lib/OpenEXRCore/CMakeLists.txt); there
# is no separate CMake project under external/zstd.
#
# Run this from the project root:
#
# ./share/util/vendor_zstd.sh 1.5.7
#

set -xeuo pipefail

do_git=true
if [[ "${1:-}" == "-git" ]]; then
    do_git=false
    shift
fi

if [[ $# -eq 0 ]]; then
  echo "Usage: $0 [-git] <version>"
  echo "Example: $0 1.5.7"
  exit 1
fi

version="$1"
tag="v${version}"
zip="${tag}.zip"
url="https://github.com/facebook/zstd/archive/refs/tags/${zip}"

if ! curl --head --silent --fail --location "$url" > /dev/null; then
  echo "No such zstd tag: ${tag}"
  exit 1
fi

zstd_dir="zstd-${tag}"

files=(
    "$zstd_dir/lib/common/*"
    "$zstd_dir/lib/compress/*"
    "$zstd_dir/lib/decompress/*"
    "$zstd_dir/lib/zstd.h"
    "$zstd_dir/lib/zstd_errors.h"
    "$zstd_dir/LICENSE"
    "$zstd_dir/README.md"
)

cd external

trap 'rm -rf "$zip" "$zstd_dir"' EXIT
wget "$url"
unzip -o "$zip" "${files[@]}"

if $do_git; then
    git rm -rf --ignore-unmatch zstd/lib zstd/LICENSE zstd/README.md
fi

mkdir -p zstd/lib
rm -rf zstd/lib/common zstd/lib/compress zstd/lib/decompress
mv "$zstd_dir/lib/common" "$zstd_dir/lib/compress" "$zstd_dir/lib/decompress" zstd/lib/
mv "$zstd_dir/lib/zstd.h" "$zstd_dir/lib/zstd_errors.h" zstd/lib/
mv "$zstd_dir/LICENSE" "$zstd_dir/README.md" zstd/

echo "v${version}" > current_zstd_version

if $do_git; then
  git add zstd/lib zstd/LICENSE zstd/README.md current_zstd_version
fi

echo "Vendored zstd ${version} into external/zstd"
