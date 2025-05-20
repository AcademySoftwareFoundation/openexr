#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from do_run import do_run

print(f"testing exrmakepreview: {' '.join(sys.argv)}")

exrmakepreview = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

assert(os.path.isfile(exrmakepreview)), "\nMissing " + exrmakepreview
assert(os.path.isfile(exrinfo)), "\nMissing " + exrinfo
assert(os.path.isdir(image_dir)), "\nMissing " + image_dir

# no args = usage message
result = do_run ([exrmakepreview], True)
assert result.stderr.startswith ("Usage: ")

# -h = usage message
result = do_run ([exrmakepreview, "-h"])
assert result.stdout.startswith ("Usage: ")

result = do_run ([exrmakepreview, "--help"])
assert result.stdout.startswith ("Usage: ")

# --version
result = do_run ([exrmakepreview, "--version"])
assert result.stdout.startswith ("exrmakepreview")
assert version in result.stdout


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
result = do_run ([exrmakepreview, "-w", "50", "-e", "1", "-v", image, outimage])

result = do_run ([exrinfo, "-v", outimage])
output = result.stdout.split('\n')
assert "preview 50 x 50" in find_line("  preview", output)

print("success")









