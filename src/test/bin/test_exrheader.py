#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os
from subprocess import PIPE, run

print(f"testing exrheader: {sys.argv}")

exrheader = sys.argv[1]
image_dir = sys.argv[2]
version = sys.argv[3]

# no args = usage message
result = run ([exrheader], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(result.stderr.startswith ("Usage: ")), "\n"+result.stderr

# -h = usage message
result = run ([exrheader, "-h"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("Usage: ")), "\n"+result.stdout

# --help = usage message
result = run ([exrheader, "--help"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("Usage: ")), "\n"+result.stdout

# --version
result = run ([exrheader, "--version"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("exrheader")), "\n"+result.stdout
assert(version in result.stdout), "\n"+result.stdout

# nonexistent.exr, error
result = run ([exrheader, "nonexistent.exr"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr

def find_line(keyword, lines):
    for line in lines:
        if line.startswith(keyword):
            return line
    return None

# attributes
image = f"{image_dir}/TestImages/GrayRampsHorizontal.exr"
result = run ([exrheader, image], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr

output = result.stdout.split('\n')
try:
    assert ("2, flags 0x0" in find_line("file format version:", output))
    assert ("pxr24" in find_line ("compression", output))
    assert ("(0 0) - (799 799)" in find_line ("dataWindow", output))
    assert ("(0 0) - (799 799)" in find_line ("displayWindow", output))
    assert ("increasing y" in find_line ("lineOrder", output))
    assert ("1" in find_line ("pixelAspectRatio", output))
    assert ("(0 0)" in find_line ("screenWindowCenter", output))
    assert ("1" in find_line ("screenWindowWidth", output))
    assert ("scanlineimage" in find_line ("type (type string)", output))
except AssertionError:
    print(result.stdout)
    raise
print("success")









