#! /bin/bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

# Portable sed -i: macOS requires '' argument, GNU sed doesn't
sedi() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sed -i '' "$@"
    else
        sed -i "$@"
    fi
}

if [[ -e current_deflate_version ]]; then
    curl https://api.github.com/repos/ebiggers/libdeflate/releases/latest > /tmp/exr_latest_deflate
    latest=`cat /tmp/exr_latest_deflate | jq -r .name`
    filename=`cat /tmp/exr_latest_deflate | jq -r '.assets[] | select(.content_type == "application/gzip") | .name'`
    unpackdir=`basename $filename .tar.gz`
    dlurl=`cat /tmp/exr_latest_deflate | jq -r '.assets[] | select(.content_type == "application/gzip") | .browser_download_url'`
    current=`cat current_deflate_version`
    if [[ "$current" != "$latest" ]]; then
        echo "Upgrading to $latest (from $current) of libdeflate..."
        wget -O "${filename}" "${dlurl}" || echo "Unable to retrieve latest release"
        tar xzf "${filename}"
        rm -rf deflate
        mv "$unpackdir" deflate

        # trim down unneeded stuff
        cd deflate
        rm -rf .github
        rm -f .cirrus.yml
        rm -f CMakeLists.txt
        rm -rf programs
        rm -f libdeflate.pc.in
        rm -f libdeflate-config.cmake.in
        rm -rf scripts
        sedi -f ../patchup_deflate_lib.sed lib/lib_common.h
        sedi -f ../patchup_deflate_lib.sed common_defs.h
        sedi -f ../patchup_deflate_lib.sed lib/utils.c
        cd ..

        rm "${filename}"
        echo "$latest" > current_deflate_version
        echo "Updated deflate to version $latest"
    else
        echo "Deflate $latest is the same as $current in the tree"
    fi
else
    echo "ERROR: should be run from the external directory in the OpenEXR tree"
fi

