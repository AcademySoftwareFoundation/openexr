#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os
from do_run import do_run

print(f"testing exrheader: {sys.argv}")

exrheader = sys.argv[1]
image_dir = sys.argv[2]
version = sys.argv[3]

# no args = usage message
result = do_run ([exrheader], True)
assert result.stderr.startswith ("Usage: ")

# -h = usage message
result = do_run ([exrheader, "-h"])
assert result.stdout.startswith ("Usage: ")

# --help = usage message
result = do_run ([exrheader, "--help"])
assert result.stdout.startswith ("Usage: ")

# --version
result = do_run ([exrheader, "--version"])
assert result.stdout.startswith ("exrheader")
assert version in result.stdout

# nonexistent.exr, error
result = do_run ([exrheader, "nonexistent.exr"], True)

def find_line(keyword, lines):
    for line in lines:
        if line.startswith(keyword):
            return line
    return None

# attributes
image = f"{image_dir}/GrayRampsHorizontal.exr"
result = do_run ([exrheader, image])

output = result.stdout.split('\n')
try:
    assert "2, flags 0x0" in find_line("file format version:", output)
    assert "pxr24" in find_line ("compression", output)
    assert "(0 0) - (799 799)" in find_line ("dataWindow", output)
    assert "(0 0) - (799 799)" in find_line ("displayWindow", output)
    assert "increasing y" in find_line ("lineOrder", output)
    assert "1" in find_line ("pixelAspectRatio", output)
    assert "(0 0)" in find_line ("screenWindowCenter", output)
    assert "1" in find_line ("screenWindowWidth", output)
    assert "scanlineimage" in find_line ("type (type string)", output)
except AssertionError:
    print(result.stdout)
    raise
print("success")









