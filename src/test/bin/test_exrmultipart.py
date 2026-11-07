#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from do_run import do_run

print(f"testing exrmultipart: {' '.join(sys.argv)}")

exrmultipart = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

test_images = {}
test_images["multipart"] = f"{image_dir}/multipart.0001.exr"
test_images["singlepart"] = f"{image_dir}/GammaChart.exr"

result = do_run  ([exrmultipart], True)
assert "Usage:" in result.stderr

# -h = usage message
result = do_run  ([exrmultipart, "-h"])
assert result.stdout.startswith ("Usage: ")

result = do_run  ([exrmultipart, "--help"])
assert result.stdout.startswith ("Usage: ")

# --version
result = do_run  ([exrmultipart, "--version"])
assert result.stdout.startswith ("exrmultipart")
assert version in result.stdout

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
    os.unlink(outimage)
atexit.register(cleanup)

# combine
result = do_run ([exrmultipart, "-combine", "-i", f"{test_images['multipart']}:0", f"{test_images['multipart']}:1", "-o", outimage])

result = do_run ([exrinfo, outimage])

# error: can't convert multipart images
result = do_run ([exrmultipart, "-convert", "-i", test_images["multipart"], "-o", outimage], True)

# convert
result = do_run ([exrmultipart, "-convert", "-i", test_images["singlepart"], "-o", outimage])

result = do_run  ([exrinfo, outimage])

# convert single-part tiled and deep files: -convert must
# preserve the pixel type instead of failing with a type-mismatch error.
for image_name, image_type in [("WavyLinesLatLong.exr", "tiledimage"),
                               ("42.deep.exr", "deepscanline")]:
    result = do_run ([exrmultipart, "-convert", "-i", f"{image_dir}/{image_name}", "-o", outimage])
    result = do_run ([exrinfo, outimage])
    assert f"type: '{image_type}'" in result.stdout

# separate

# get part names from the multipart image
result = do_run  ([exrinfo, test_images["multipart"]])
part_names = {}
for p in result.stdout.split('\n part ')[1:]:
    output = p.split('\n')
    part_number, part_name = output[0].split(': ')
    part_names[part_number] = part_name

with tempfile.TemporaryDirectory() as tempdir:

    result = do_run ([exrmultipart, "-separate", "-i", test_images["multipart"], "-o", f"{tempdir}/separate"])

    for i in range(1, 10):
        s = f"{tempdir}/separate.{i}.exr"
        result = do_run ([exrinfo, "-v", s])
        output = result.stdout.split('\n')
        assert output[1].startswith(' parts: 1')
        output[2].startswith(' part 1:')
        part_name = output[2][9:]
        part_number = str(i)
        assert part_names[part_number] == part_name

print("success")

