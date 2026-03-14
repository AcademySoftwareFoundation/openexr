#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os
from do_run import do_run

print(f"testing exrinfo: {' '.join(sys.argv)}")

exrinfo = sys.argv[1]
image_dir = sys.argv[2]
version = sys.argv[3]

test_images = {}
test_images["GrayRampsHorizontal"] = f"{image_dir}/GrayRampsHorizontal.exr"

result = do_run ([exrinfo, "-h"])
assert result.stdout.startswith ("Usage: ")

result = do_run ([exrinfo, "--help"])
assert result.stdout.startswith ("Usage: ")

# --version
result = do_run ([exrinfo, "--version"])
assert result.stdout.startswith ("exrinfo")
assert version in result.stdout

result = do_run ([exrinfo, test_images["GrayRampsHorizontal"], "-a", "-v"])
output = result.stdout.split('\n')
try:
    assert ('pxr24' in output[1])
    assert ('800 x 800' in output[2])
    assert ('800 x 800' in output[3])
    assert ('1 channels' in output[4])
except AssertionError:
    print(result.stdout)
    raise

# test image as stdio
with open(test_images["GrayRampsHorizontal"], 'rb') as f:
    image_data = f.read()
result = do_run ([exrinfo, '-', "-a", "-v"], data=image_data)
output = result.stdout.decode().split('\n')
try:
    assert ('pxr24' in output[1])
    assert ('800 x 800' in output[2])
    assert ('800 x 800' in output[3])
    assert ('1 channels' in output[4])
except AssertionError:
    print(result.stdout)
    raise

print("success")

