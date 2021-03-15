# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

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
        strip_prefix = "Imath-f03e14b80f4bde30126f36b3d58dd5ff2e0f11fd",
        sha256 = "943ddd370ba3de6637b66d8c86d2eaacca6472624b762db0bf5cdfc9c42e9eab",
        urls = ["https://github.com/AcademySoftwareFoundation/Imath/archive/f03e14b80f4bde30126f36b3d58dd5ff2e0f11fd.zip"],
    )
