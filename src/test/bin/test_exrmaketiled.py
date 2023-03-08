#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from subprocess import PIPE, run

print(f"testing exrmaketiled: {sys.argv}")

exrmaketiled = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]

image = f"{image_dir}/TestImages/GammaChart.exr"

assert(os.path.isfile(exrmaketiled))
assert(os.path.isfile(exrinfo))
assert(os.path.isdir(image_dir))
assert(os.path.isfile(image))

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
atexit.register(cleanup)

# no args = usage message
result = run ([exrmaketiled], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 1)
assert(result.stderr.startswith ("Usage: "))

# -h = usage message
result = run ([exrmaketiled, "-h"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 1)
assert(result.stderr.startswith ("Usage: "))

result = run ([exrmaketiled, image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert('tiled image has levels: x 1 y 1' in result.stdout)

print("success")
