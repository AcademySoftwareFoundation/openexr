# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

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

generate_header = rule(
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
