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
        strip_prefix = "Imath-3.1.9",
        sha256 = "f1d8aacd46afed958babfced3190d2d3c8209b66da451f556abd6da94c165cf3",
        urls = ["https://github.com/AcademySoftwareFoundation/Imath/archive/refs/tags/v3.1.9.tar.gz"],
    )

    maybe(
        http_archive,
        name = "bazel_skylib",
        sha256 = "66ffd9315665bfaafc96b52278f57c7e2dd09f5ede279ea6d39b2be471e7e3aa",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.4.2/bazel-skylib-1.4.2.tar.gz",
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.4.2/bazel-skylib-1.4.2.tar.gz",
        ],
    )
