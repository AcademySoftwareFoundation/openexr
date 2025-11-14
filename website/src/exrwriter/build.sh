#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.
#

$ mkdir _build
$ cmake -S . -B _build -DCMAKE_PREFIX_PATH=<path to OpenEXR libraries/includes>
$ cmake --build _build
