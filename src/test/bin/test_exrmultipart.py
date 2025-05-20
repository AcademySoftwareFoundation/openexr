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

image = f"{image_dir}/multipart.0001.exr"

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
    os.unlink(outimage)
atexit.register(cleanup)

# combine
result = do_run ([exrmultipart, "-combine", "-i", f"{image}:0", f"{image}:1", "-o", outimage])

result = do_run ([exrinfo, outimage])

# error: can't convert multipart images
result = do_run ([exrmultipart, "-convert", "-i", image, "-o", outimage], True)

# convert
singlepart_image = f"{image_dir}/GammaChart.exr"
result = do_run ([exrmultipart, "-convert", "-i", singlepart_image, "-o", outimage])

result = do_run  ([exrinfo, outimage])

# separate

# get part names from the multipart image
result = do_run  ([exrinfo, image])
part_names = {}
for p in result.stdout.split('\n part ')[1:]:
    output = p.split('\n')
    part_number, part_name = output[0].split(': ')
    part_names[part_number] = part_name

with tempfile.TemporaryDirectory() as tempdir:

    result = do_run ([exrmultipart, "-separate", "-i", image, "-o", f"{tempdir}/separate"])

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

