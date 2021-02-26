# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

"""External dependencies for openexr."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def openexr_deps():
    # zlib
    maybe(
        http_archive,
        name = "zlib",
        build_file = "@openexr//:bazel/third_party/zlib.BUILD",
        sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
        strip_prefix = "zlib-1.2.11",
        urls = ["https://zlib.net/zlib-1.2.11.tar.gz"],
    )

    maybe(
        http_archive,
        name = "Imath",
        build_file = "@openexr//:bazel/third_party/Imath.BUILD",
        strip_prefix = "Imath-f21e31a85a4a0b4ddcd980a19e448fd7eca72cce",
        sha256 = "6943dce2b2e8737d7cf1ae58103195b64c51c6330237ec07c12d829745b4c9c0",
        urls = ["https://github.com/AcademySoftwareFoundation/Imath/archive/f21e31a85a4a0b4ddcd980a19e448fd7eca72cce.zip"],
    )