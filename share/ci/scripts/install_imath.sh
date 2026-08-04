#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# This script is used by the ci_workflow.yml/ci_steps.yml CI to
# install Imath. This is a part of the process that validates that
# OpenEXR's cmake properly locates the Imath dependency, either
# finding this installation or fetching the library from github to
# build internally.

set -ex

TAG="$1"

# The sudo is nececessary since the installation goes to /usr/local.
SUDO=$(command -v sudo >/dev/null 2>&1 && echo sudo || echo "")

# In an MSYS2 mingw shell, install into $MINGW_PREFIX (e.g. /mingw32)
# instead of the default /usr/local, so find_package() locates it the
# same way it would find a pacboy-installed package.
PREFIX_ARG=()
if [ -n "$MINGW_PREFIX" ]; then
  PREFIX_ARG=("-DCMAKE_INSTALL_PREFIX=$MINGW_PREFIX")
fi

git clone https://github.com/AcademySoftwareFoundation/Imath.git
cd Imath

git checkout ${TAG}

cmake -S . -B _build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF "${PREFIX_ARG[@]}"
$SUDO cmake --build _build \
      --target install \
      --config Release \
      --parallel 2

cd ..
rm -rf Imath
