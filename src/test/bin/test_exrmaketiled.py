#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from do_run import do_run

print(f"testing exrmaketiled: {' '.join(sys.argv)}")

exrmaketiled = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

test_images = {}
test_images["GammaChart"] = f"{image_dir}/GammaChart.exr"

assert(os.path.isfile(exrmaketiled)), "\nMissing " + exrmaketiled
assert(os.path.isfile(exrinfo)), "\nMissing " + exrinfo
assert(os.path.isdir(image_dir)), "\nMissing " + image_dir
assert(os.path.isfile(test_images["GammaChart"])), "\nMissing " + image

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
atexit.register(cleanup)

# no args = usage message
result = do_run ([exrmaketiled], True)
assert result.stderr.startswith ("Usage: ")

# -h = usage message
result = do_run ([exrmaketiled, "-h"])
assert result.stdout.startswith ("Usage: ")

result = do_run ([exrmaketiled, "--help"])
assert result.stdout.startswith ("Usage: ")

# --version
result = do_run ([exrmaketiled, "--version"])
assert result.stdout.startswith ("exrmaketiled")
assert version in result.stdout

result = do_run ([exrmaketiled, test_images["GammaChart"], outimage])
assert os.path.isfile(outimage)

result = do_run ([exrinfo, "-v", outimage])
assert 'tiled image has levels: x 1 y 1' in result.stdout

print("success")
