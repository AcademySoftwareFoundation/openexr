#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

#
# This script compares an install_manifest.txt from a CI build and
# compares it against a known valid manifest to verify that the proper
# files have been installed. Both missing files and extra files not
# expected to be installed are reported as errors.
#
# This script is invoked by the .github/workflows/ci_steps.yml workflow.
#
# The first argument is the candidate manifest from the build in
# question.  The second argument is the reference manifest from
# share/ci/install_manifest, known to be correct.
#
# The reference manifest may contain variables for the library version:
#   $MAJOR = OPENEXR_VERSION_MAJOR
#   $MINOR = OPENEXR_VERSION_MINOR
#   $PATCH = OPENEXR_VERSION_PATCH
#   $SOVERSION = OPENEXR_LIB_SOVERSION
# The values of these variables are extracted from the raw source
# files where they are maintained. This is so the reference manifests
# don't have be updated on every release, sinced these values appear
# in the installed library names.
#
# The manifests *must* be updated whenever files are added to what is
# installed: new headers, libraries, tools, cmake, etc, or a change to
# file names. Note that this validates just the file *names*, not the
# contents.
#
# The CI uploads the install manifests as job artifacts, attached to
# the GitHub status page.
#
# NOTE: If you add a new installed header, you must add it to each of
# the reference manifest in share/ci/install_manifest.
#
# The first line of the manifest file is the cmake command for the
# build that generated it, just to help identify the contents.
#

import os
import sys
import re
import argparse
from pathlib import Path

def openexr_soversion():

    with open("CMakeLists.txt", "r") as f:
        for line in f:
            m = re.search(r"set\(OPENEXR_LIB_SOVERSION (\d+)\)", line)
            if m:
                return m.group(1)

    print("Can't find soversion")
    sys.exit(1)

def openexr_version():

    major = ""
    minor = ""
    patch = ""

    with open("src/lib/OpenEXRCore/openexr_version.h", "r") as f:
        for line in f:
            pattern = r"#\s*define\s+OPENEXR_VERSION_MAJOR\s+(\d+)"
            m = re.search(pattern, line)
            if m:
                major = m.group(1)
                continue
            pattern = r"#\s*define\s+OPENEXR_VERSION_MINOR\s+(\d+)"
            m = re.search(pattern, line)
            if m:
                minor = m.group(1)
                continue
            pattern = r"#\s*define\s+OPENEXR_VERSION_PATCH\s+(\d+)"
            m = re.search(pattern, line)
            if m:
                patch = m.group(1)
                continue

    if not major or not minor or not patch:
        print("Can't find openexr version!")
        sys.exit(1)

    return major, minor, patch

def process_line(line, major, minor, patch, so):
    return line.strip().split("/_install/", 1)[-1].replace("$MAJOR", major).replace("$MINOR", minor).replace("$PATCH", patch).replace("$SOVERSION", so)

def load_manifest(path):
    """Load and return the list of files from the install manifest."""
    major, minor, patch = openexr_version()
    so = openexr_soversion()
    with open(path, 'r') as file:
        return sorted(process_line(line, major, minor, patch, so) for line in file if line[0]!='#')

def validate_install(candidate_manifest_path, reference_manifest_path, options):
    """Main function to verify the installed files."""

    candidate_manifest = load_manifest(candidate_manifest_path)
    reference_manifest = load_manifest(reference_manifest_path)

    print("reference_manifest:")
    for l in reference_manifest:
        print(f"  {l}")
    print("candidate_manifest:")
    for l in candidate_manifest:
        print(f"  {l}")

    # Compare manifests
    missing_files = sorted(set(reference_manifest) - set(candidate_manifest))
    extra_files = sorted(set(candidate_manifest) - set(reference_manifest))

    # Output results
    if missing_files:
        print("Error: The following files should have been installed but weren't:\n  " + '\n  '.join(missing_files))
    if extra_files:
        print("Error: The following files were installed but were not expected:\n  " + '\n  '.join(extra_files))

    if missing_files or extra_files:
        return 1

    print("install manifest is valid.")

    return 0

if __name__ == "__main__":

    print(f"validate_install: {sys.argv}")

    parser = argparse.ArgumentParser(description="Validate installed files against reference install manifest.")
    parser.add_argument("candidate_manifest", help="Path to the candidate install_manifest.txt")
    parser.add_argument("reference_manifest", help="Path to the reference install_manifest.txt")

    args = parser.parse_args()

    if not os.path.exists(args.candidate_manifest):
        print(f"candidate manifest does not exist: {args.candidate_manifest}")
        sys.exit(1)

    if not os.path.exists(args.reference_manifest):
        print(f"reference manifest does not exist: {args.reference_manifest}")
        sys.exit(1)

    print(f"candidate_manifest={args.candidate_manifest}")
    print(f"reference_manifest={args.reference_manifest}")

    status = validate_install(args.candidate_manifest, args.reference_manifest, args)

    sys.exit(status)

