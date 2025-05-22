#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from do_run import do_run

print(f"testing exrmanifest: {sys.argv}")

src_dir = os.path.dirname (sys.argv[0])
exrmanifest = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

test_images = {}
test_images["11"] = f"{image_dir}/11.deep.exr"
test_images["42"] = f"{image_dir}/42.deep.exr"
test_images["64"] = f"{image_dir}/64.deep.exr"
test_images["multivariate"] = f"{image_dir}/multivariate.deep.exr"
test_images["objectid"] = f"{image_dir}/objectid.deep.exr"

# no args = usage message, error
result = do_run ([exrmanifest], True)
assert result.stderr.startswith ("Usage: ")

# -h = usage message
result = do_run ([exrmanifest, "-h"])
assert result.stdout.startswith ("Usage: ")

result = do_run ([exrmanifest, "--help"])
assert result.stdout.startswith ("Usage: ")

# --version
result = do_run ([exrmanifest, "--version"])
assert result.stdout.startswith ("exrmanifest")
assert version in result.stdout

# invalid arguments
result = do_run ([exrmanifest, "foo.exr", "bar.exr"], True)

for image in test_images:
    result = do_run ([exrmanifest, test_images[image]])
    stdout_is = result.stdout
    with open (test_images[image] + ".txt", 'r') as file:
        stdout_should_be = file.read()
        assert stdout_is == stdout_should_be

print("success")









