#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from subprocess import PIPE, run

print(f"testing exr2aces: {sys.argv}")

exr2aces = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

# no args = usage message, error
result = run ([exr2aces], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(result.stderr.startswith ("Usage: ")), "\n"+result.stderr

# -h = usage message
result = run ([exr2aces, "-h"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("Usage: ")), "\n"+result.stdout

result = run ([exr2aces, "--help"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("Usage: ")), "\n"+result.stdout

# --version
result = run ([exr2aces, "--version"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("exr2aces")), "\n"+result.stdout
assert(version in result.stdout), "\n"+result.stdout

# invalid arguments
result = run ([exr2aces, "foo.exr", "bar.exr"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr

def find_line(keyword, lines):
    for line in lines:
        if line.startswith(keyword):
            return line
    return None

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
atexit.register(cleanup)

image = f"{image_dir}/TestImages/GrayRampsHorizontal.exr"
result = run ([exr2aces, "-v", image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr

# confirm the output has the proper chromaticities
assert("chromaticities: chromaticities r[0.7347, 0.2653] g[0, 1] b[0.0001, -0.077] w[0.32168, 0.33767]" in result.stdout), "\n"+result.stdout

print("success")









