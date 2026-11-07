#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

#
# Validate that the OpenEXR configure found the dependencies (Imath,
# libdeflate, zstd, OpenJPH) that the CI job installed, instead of
# silently falling back to a vendored copy or picking up another
# installed version of one.
#
# cmake/OpenEXRSetup.cmake falls back to the vendored copy of a
# dependency when it doesn't find a compatible installed one, and
# find_package() may find a copy that's already on the machine, e.g. in
# the CI container, instead of the one CI installed (see issue #2806).
# Neither is an error, so this reads what CMake decided from the config
# headers it generates in the build directory, and compares it with
# what the job asked for.
#
# Run by .github/workflows/ci_steps.yml, after the configure step.
#

import os
import re
import sys
import argparse

def read_defines(path):
    """Parse "#define VAR VALUE" and "#undef VAR" lines out of a C/C++
    header, returning a dict mapping VAR to VALUE (None for #undef)."""

    defines = {}
    define_re = re.compile(r"^\s*#\s*define\s+(\w+)(?:\s+(\S+))?")
    undef_re = re.compile(r"^\s*/\*\s*#\s*undef\s+(\w+)\s*\*/")
    with open(path, "r") as f:
        for line in f:
            m = define_re.match(line)
            if m:
                defines[m.group(1)] = m.group(2)
                continue
            m = undef_re.match(line)
            if m:
                defines[m.group(1)] = None
    return defines

def check_internal_flag(name, defines, expect_internal):
    flag = f"OPENEXR_USE_INTERNAL_{name}"
    if flag not in defines:
        print(f"Error: {flag} not found in OpenEXRConfigInternal.h")
        return False

    is_internal = defines[flag] is not None
    if is_internal != expect_internal:
        print(
            f"Error: expected {flag} to be "
            f"{'defined (vendored)' if expect_internal else 'undefined (external)'}, "
            f"but it was {'defined (vendored)' if is_internal else 'undefined (external)'}."
        )
        return False

    print(f"{flag}: {'vendored' if is_internal else 'external'} (as expected)")
    return True

def check_version(name, defines, expected):
    """Compare MAJOR.MINOR(.PATCH) macros against an expected "X.Y.Z" or
    "X.Y" string. Not checked when expected is None (no version
    requested); an empty string means the job installed the dependency
    but its version couldn't be determined, which is an error."""

    if expected is None:
        return True

    if not expected:
        print(
            f"Error: {name} was installed for this job, but its version is "
            f"unknown: the install script couldn't derive it from the pinned "
            f"SHA with 'git describe --tags --exact-match', i.e. the SHA isn't "
            f"a release tag."
        )
        return False

    major = defines.get(f"OPENEXR_{name}_VERSION_MAJOR")
    minor = defines.get(f"OPENEXR_{name}_VERSION_MINOR")
    patch = defines.get(f"OPENEXR_{name}_VERSION_PATCH")
    parts = [p for p in (major, minor, patch) if p is not None]
    actual = ".".join(parts)

    expected_parts = expected.split(".")[:len(parts)]
    expected_trimmed = ".".join(expected_parts)

    if actual != expected_trimmed:
        print(
            f"Error: expected {name} version {expected} "
            f"(OPENEXR_{name}_VERSION_* = {expected_trimmed}), "
            f"but found {actual}."
        )
        return False

    print(f"OPENEXR_{name}_VERSION_*: {actual} (matches expected {expected})")
    return True

def str_to_bool(s):
    return str(s).strip().upper() in ("ON", "TRUE", "1", "YES")

if __name__ == "__main__":

    print(f"validate_dependencies: {sys.argv}")

    parser = argparse.ArgumentParser(
        description="Validate that the build used the expected dependencies."
    )
    parser.add_argument("build_dir", help="Path to the CMake build directory")
    parser.add_argument("--force-internal-imath", required=True)
    parser.add_argument("--force-internal-deflate", required=True)
    parser.add_argument("--force-internal-zstd", required=True)
    parser.add_argument("--force-internal-openjph", required=True)
    parser.add_argument(
        "--imath-version",
        default=None,
        help="Exact Imath version CI requested (ignored when Imath is forced internal; omit to skip the check)",
    )
    parser.add_argument(
        "--deflate-version",
        default=None,
        help="Exact libdeflate version CI requested (ignored when libdeflate is forced internal; omit to skip the check)",
    )
    parser.add_argument(
        "--zstd-version",
        default=None,
        help="Exact zstd version CI requested (ignored when zstd is forced internal; omit to skip the check)",
    )
    parser.add_argument(
        "--openjph-version",
        default=None,
        help="Exact OpenJPH version CI requested (ignored when OpenJPH is forced internal; omit to skip the check)",
    )

    args = parser.parse_args()

    internal_header = os.path.join(args.build_dir, "cmake", "OpenEXRConfigInternal.h")
    public_header = os.path.join(args.build_dir, "cmake", "OpenEXRConfig.h")

    for path in (internal_header, public_header):
        if not os.path.exists(path):
            print(f"Error: config header does not exist: {path}")
            sys.exit(1)

    internal_defines = read_defines(internal_header)
    public_defines = read_defines(public_header)

    ok = True
    ok &= check_internal_flag("IMATH", internal_defines, str_to_bool(args.force_internal_imath))
    ok &= check_internal_flag("DEFLATE", internal_defines, str_to_bool(args.force_internal_deflate))
    ok &= check_internal_flag("ZSTD", internal_defines, str_to_bool(args.force_internal_zstd))
    ok &= check_internal_flag("OPENJPH", internal_defines, str_to_bool(args.force_internal_openjph))

    # CI pins each dependency to a git SHA, and its install script
    # records the release version that SHA is tagged with. The version
    # found must be that one, not merely some compatible version, or
    # the build may be using a copy that was already on the machine.
    # The workflow omits the version options where CI doesn't control
    # the installed version, e.g. the MSYS2 pacman packages.
    if not str_to_bool(args.force_internal_imath):
        ok &= check_version("IMATH", public_defines, args.imath_version)
    if not str_to_bool(args.force_internal_deflate):
        ok &= check_version("DEFLATE", public_defines, args.deflate_version)
    if not str_to_bool(args.force_internal_zstd):
        ok &= check_version("ZSTD", public_defines, args.zstd_version)
    if not str_to_bool(args.force_internal_openjph):
        ok &= check_version("OPENJPH", public_defines, args.openjph_version)

    if not ok:
        print("Dependency validation FAILED.")
        sys.exit(1)

    print("Dependency validation passed.")
    sys.exit(0)
