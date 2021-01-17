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
        strip_prefix = "Imath-8916ff558274dfa50399868481e8d0d17e15e631",
        sha256 = "decf07bbc90b5c2d3b43d631d4e1d2f24f1dd111f85e6c97c0f541c6970a7f89",
        urls = ["https://github.com/AcademySoftwareFoundation/Imath/archive/8916ff558274dfa50399868481e8d0d17e15e631.zip"],
    )
