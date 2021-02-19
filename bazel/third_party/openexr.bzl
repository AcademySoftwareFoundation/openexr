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
        strip_prefix = "Imath-03b50286eb2114120a119f7f1c1f50aa4a4073f4",
        sha256 = "a43e25b1ad80f8c2a0a7b3e2f87052ae85d5e9e84beaaebc7dd79e7988d0a80d",
        urls = ["https://github.com/AcademySoftwareFoundation/Imath/archive/03b50286eb2114120a119f7f1c1f50aa4a4073f4.zip"],
    )

