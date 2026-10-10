#!/usr/bin/env python3

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

"""Sphinx configuration for the Bazel build (see //website:docs).

It evaluates the unmodified conf.py of the CMake build. That file locates the
project version via a CWD-relative "../CMakeLists.txt", so //website:docs stages
it in _upstream/ with CMakeLists.txt in the Sphinx source root next to this file.
"""

import os

_source_root = os.path.dirname(os.path.abspath(__file__))
_upstream_conf = os.path.join(_source_root, "_upstream", "conf.py")

os.chdir(os.path.dirname(_upstream_conf))
try:
    with open(_upstream_conf) as _f:
        exec(compile(_f.read(), _upstream_conf, "exec"))
finally:
    os.chdir(_source_root)
