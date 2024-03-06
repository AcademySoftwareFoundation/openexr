#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from subprocess import PIPE, run

print(f"testing exrmanifest: {sys.argv}")

src_dir = os.path.dirname (sys.argv[0])
exrmanifest = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

# no args = usage message, error
result = run ([exrmanifest], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(result.stderr.startswith ("Usage: ")), "\n"+result.stderr

# -h = usage message
result = run ([exrmanifest, "-h"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("Usage: ")), "\n"+result.stdout

result = run ([exrmanifest, "--help"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("Usage: ")), "\n"+result.stdout

# --version
result = run ([exrmanifest, "--version"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("exrmanifest")), "\n"+result.stdout
assert(version in result.stdout), "\n"+result.stdout

# invalid arguments
result = run ([exrmanifest, "foo.exr", "bar.exr"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr

for test_image in ["11.deep.exr", "42.deep.exr", "64.deep.exr", "multivariate.deep.exr", "objectid.deep.exr"]:
    test_file = src_dir + "/test_images/" + test_image
    result = run ([exrmanifest, test_file], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    print(" ".join(result.args))
    assert(result.returncode == 0), "\n"+result.stderr
    stdout_is = result.stdout
    with open (test_file + ".txt", 'r') as file:
        stdout_should_be = file.read()
        assert stdout_is == stdout_should_be

print("success")









