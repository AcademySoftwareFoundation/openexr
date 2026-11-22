# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

import argparse
import re
from pathlib import Path


VERSION_FIELDS = ("MAJOR", "MINOR", "PATCH")
VERSION_RELEASE_TYPE = "-dev"


def read_version(path):
    content = Path(path).read_text(encoding="utf-8")
    values = {}
    for field in VERSION_FIELDS:
        name = f"OPENEXR_VERSION_{field}"
        matches = re.findall(
            rf"^\s*#\s*define\s+{name}\s+([0-9]+)\s*$",
            content,
            flags=re.MULTILINE,
        )
        if len(matches) != 1:
            raise ValueError(
                f"expected exactly one numeric definition of {name} in {path}"
            )
        values[field.lower()] = matches[0]
    return values


def substitutions(config, version):
    major = version["major"]
    minor = version["minor"]
    patch = version["patch"]
    version_string = f"{major}.{minor}.{patch}"

    if config == "iex":
        return {"@IEX_INTERNAL_NAMESPACE@": f"Iex_{major}_{minor}"}
    if config == "ilmthread":
        return {"@ILMTHREAD_INTERNAL_NAMESPACE@": f"IlmThread_{major}_{minor}"}
    return {
        "@OPENEXR_INTERNAL_IMF_NAMESPACE@": f"Imf_{major}_{minor}",
        "@OPENEXR_LIB_VERSION@": f"99.{version_string}",
        "@OPENEXR_PACKAGE_NAME@": (
            f"OpenEXR {version_string}{VERSION_RELEASE_TYPE}"
        ),
        "@OPENEXR_VERSION@": version_string,
        "@OPENEXR_VERSION_MAJOR@": major,
        "@OPENEXR_VERSION_MINOR@": minor,
        "@OPENEXR_VERSION_PATCH@": patch,
        "@OPENEXR_VERSION_RELEASE_TYPE@": VERSION_RELEASE_TYPE,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--config", choices=("iex", "ilmthread", "openexr"), required=True
    )
    parser.add_argument("--version-header", required=True)
    parser.add_argument("--template", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    version = read_version(args.version_header)
    content = Path(args.template).read_text(encoding="utf-8")
    for placeholder, value in substitutions(args.config, version).items():
        if placeholder not in content:
            raise ValueError(
                f"expected placeholder {placeholder} in {args.template}"
            )
        content = content.replace(placeholder, value)

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(content, encoding="utf-8")


if __name__ == "__main__":
    main()
