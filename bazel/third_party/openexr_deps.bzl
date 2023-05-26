# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

"""External dependencies for openexr."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def openexr_deps():
    """Fetches dependencies (libdeflate and Imath) of OpenEXR and Skylib for header generation."""

    maybe(
        http_archive,
        name = "libdeflate",
        build_file = "@com_openexr//:bazel/third_party/libdeflate.BUILD",
        sha256 = "225d982bcaf553221c76726358d2ea139bb34913180b20823c782cede060affd",
        strip_prefix = "libdeflate-1.18",
        urls = ["https://github.com/ebiggers/libdeflate/archive/refs/tags/v1.18.tar.gz"],
    )

    maybe(
        http_archive,
        name = "Imath",
        build_file = "@com_openexr//:bazel/third_party/Imath.BUILD",
        strip_prefix = "Imath-3.1.8",
        sha256 = "a23a4e2160ca8ff68607a4e129e484edd1d0d13f707394d32af7aed659020803",
        urls = ["https://github.com/AcademySoftwareFoundation/Imath/archive/refs/tags/v3.1.8.tar.gz"],
    )

    http_archive(
        name = "bazel_skylib",
        sha256 = "b8a1527901774180afc798aeb28c4634bdccf19c4d98e7bdd1ce79d1fe9aaad7",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.4.1/bazel-skylib-1.4.1.tar.gz",
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.4.1/bazel-skylib-1.4.1.tar.gz",
        ],
    )
