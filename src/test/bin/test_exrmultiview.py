#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from do_run import do_run

print(f"testing exrmultiview: {' '.join(sys.argv)}")

exrmultiview = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

result = do_run  ([exrmultiview], True)
assert "Usage:" in result.stderr

# -h = usage message
result = do_run  ([exrmultiview, "-h"])
assert result.stdout.startswith ("Usage: ")

result = do_run  ([exrmultiview, "--help"])
assert result.stdout.startswith ("Usage: ")

# --version
result = do_run  ([exrmultiview, "--version"])
assert result.stdout.startswith ("exrmultiview")
assert version in result.stdout

left_image = f"{image_dir}/GammaChart.exr"
right_image = f"{image_dir}/GrayRampsHorizontal.exr"

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
    os.unlink(outimage)
atexit.register(cleanup)

result = do_run ([exrmultiview, "left", left_image, "right", right_image, outimage])

result = do_run  ([exrinfo, outimage])
print(" ".join(result.args))
assert result.returncode == 0
try:
    assert('\'B\': half samp 1 1' in result.stdout)
    assert('\'G\': half samp 1 1' in result.stdout)
    assert('\'R\': half samp 1 1' in result.stdout)
    assert('\'right.Y\': half samp 1 1' in result.stdout)
except AssertionError:
    print(result.stdout)
    raise

print("success")

