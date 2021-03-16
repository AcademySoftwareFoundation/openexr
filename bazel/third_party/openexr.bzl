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
        strip_prefix = "Imath-3.0.0-beta",
        sha256 = "69fe9cb97bcaf155a1441aac8e12f35cfe826fb1f60feeb7afb4ceec079e5034",
        urls = ["https://github.com/AcademySoftwareFoundation/Imath/archive/v3.0.0-beta.tar.gz"],
    )

def _generate_header_impl(ctx):
    out = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.expand_template(
        output = out,
        template = ctx.file.template,
        substitutions = ctx.attr.substitutions,
    )
    return [CcInfo(
        compilation_context = cc_common.create_compilation_context(
            includes = depset([out.dirname]),
            headers = depset([out]),
        ),
    )]

openexr_generate_header = rule(
    implementation = _generate_header_impl,
    attrs = {
        "substitutions": attr.string_dict(
            mandatory = True,
        ),
        "template": attr.label(
            allow_single_file = [".h.in"],
            mandatory = True,
        ),
    },
)
