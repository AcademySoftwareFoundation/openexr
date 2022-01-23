# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

"""External dependencies for openexr."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def openexr_deps():
    """Fetches dependencies (zlib and Imath) of OpenEXR."""

    maybe(
        http_archive,
        name = "net_zlib_zlib",
        build_file = "@openexr//:bazel/third_party/zlib.BUILD",
        sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
        strip_prefix = "zlib-1.2.11",
        urls = [
            "https://zlib.net/zlib-1.2.11.tar.gz",
            "https://mirror.bazel.build/zlib.net/zlib-1.2.11.tar.gz",
        ],
    )

    # sha256 was determined using:
    # curl -sL https://github.com/AcademySoftwareFoundation/Imath/archive/refs/tags/v3.1.4.tar.gz --output Imath-3.1.4.tar.gz
    # sha256sum Imath-3.1.4.tar.gz
    # If the hash is incorrect Bazel will report an error and show the actual hash of the file.
    maybe(
        http_archive,
        name = "Imath",
        build_file = "@openexr//:bazel/third_party/Imath.BUILD",
        strip_prefix = "Imath-3.1.4",
        sha256 = "fcca5fbb37d375a252bacd8a29935569bdc28b888f01ef1d9299ca0c9e87c17a",
        urls = ["https://github.com/AcademySoftwareFoundation/Imath/archive/refs/tags/v3.1.4.tar.gz"],
    )
