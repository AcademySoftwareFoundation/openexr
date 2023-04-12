#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os
from subprocess import PIPE, run

print(f"testing exrinfo: {' '.join(sys.argv)}")

exrinfo = sys.argv[1]
image_dir = sys.argv[2]
version = sys.argv[3]

result = run ([exrinfo, "-h"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(result.stdout.startswith ("Usage: "))

result = run ([exrinfo, "--help"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(result.stdout.startswith ("Usage: "))

# --version
result = run ([exrinfo, "--version"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
print(result.stdout)
assert(result.returncode == 0)
assert(result.stdout.startswith ("exrinfo"))
assert(version in result.stdout)

image = f"{image_dir}/TestImages/GrayRampsHorizontal.exr"
result = run ([exrinfo, image, "-a", "-v"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
output = result.stdout.split('\n')
assert ('pxr24' in output[1])
assert ('800 x 800' in output[2])
assert ('800 x 800' in output[3])
assert ('1 channels' in output[4])

# test image as stdio
with open(image, 'rb') as f:
    data = f.read()
result = run ([exrinfo, '-', "-a", "-v"], input=data, stdout=PIPE, stderr=PIPE)
print(" ".join(result.args))
assert(result.returncode == 0)
output = result.stdout.decode().split('\n')
assert ('pxr24' in output[1])
assert ('800 x 800' in output[2])
assert ('800 x 800' in output[3])
assert ('1 channels' in output[4])

print("success")

