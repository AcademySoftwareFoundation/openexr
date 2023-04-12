#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from subprocess import PIPE, run

print(f"testing exrmultiview: {' '.join(sys.argv)}")

exrmultiview = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

result = run ([exrmultiview], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0)
assert("Usage:" in result.stderr)

# -h = usage message
result = run ([exrmultiview, "-h"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(result.stdout.startswith ("Usage: "))

result = run ([exrmultiview, "--help"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(result.stdout.startswith ("Usage: "))

# --version
result = run ([exrmultiview, "--version"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(result.stdout.startswith ("exrmultiview"))
assert(version in result.stdout)

left_image = f"{image_dir}/TestImages/GammaChart.exr"
right_image = f"{image_dir}/TestImages/GrayRampsHorizontal.exr"

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
    os.unlink(outimage)
atexit.register(cleanup)

command = [exrmultiview, "left", left_image, "right", right_image, outimage]
result = run (command, stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)

result = run ([exrinfo, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert('\'B\': half samp 1 1' in result.stdout)
assert('\'G\': half samp 1 1' in result.stdout)
assert('\'R\': half samp 1 1' in result.stdout)
assert('\'right.Y\': half samp 1 1' in result.stdout)

print("success")

