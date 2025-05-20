#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from do_run import do_run

print(f"testing exr2aces: {' '.join(sys.argv)}")

exr2aces = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

if not os.path.isfile(exr2aces) or not os.access(exr2aces, os.X_OK):
    print(f"error: no such file: {exr2aces}")
    sys.exit(1)

# no args = usage message, error
result = do_run ([exr2aces], True)
assert result.stderr.startswith ("Usage: ")

# -h = usage message
result = do_run ([exr2aces, "-h"])
assert result.stdout.startswith ("Usage: ")

result = do_run ([exr2aces, "--help"])
assert result.stdout.startswith ("Usage: ")

# --version
result = do_run ([exr2aces, "--version"])
assert result.stdout.startswith ("exr2aces")
assert version in result.stdout

# invalid arguments
result = do_run ([exr2aces, "foo.exr", "bar.exr"], True)

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

image = f"{image_dir}/GrayRampsHorizontal.exr"
result = do_run ([exr2aces, "-v", image, outimage])

result = do_run ([exrinfo, "-v", outimage])

# confirm the output has the proper chromaticities
assert "chromaticities: chromaticities r[0.7347, 0.2653] g[0, 1] b[0.0001, -0.077] w[0.32168, 0.33767]" in result.stdout

print("success")









