#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit, json
from do_run import do_run

print(f"testing exrmultipart: {' '.join(sys.argv)}")

exrmultipart = sys.argv[1]
exrinfo = sys.argv[2]
exrmultiview = os.path.join(os.path.dirname(exrmultipart), "exrmultiview")
exrmaketiled = os.path.join(os.path.dirname(exrmultipart), "exrmaketiled")
exrmetrics = os.path.join(os.path.dirname(exrmultipart), "exrmetrics")
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

# Convert single-level, mipmapped, and ripmapped multiview images into one
# tiled part per view. exrmetrics reads, rewrites, and rereads every level, so
# this also checks that the output pixel data is complete and decodable.
with tempfile.TemporaryDirectory() as tempdir:
    multiview = f"{tempdir}/multiview.exr"
    do_run ([exrmultiview,
             "left", f"{image_dir}/GammaChart.exr",
             "right", f"{image_dir}/GrayRampsHorizontal.exr",
             multiview])
    for option, level_mode in [("-o", "single image"),
                               ("-m", "mipmap"),
                               ("-r", "ripmap")]:
        tiled = f"{tempdir}/multiview-{level_mode}.exr"
        split = f"{tempdir}/split-{level_mode}.exr"
        do_run ([exrmaketiled, option, multiview, tiled])
        do_run ([exrmultipart, "-convert", "-i", tiled, "-o", split])
        result = do_run ([exrinfo, split])
        assert result.stdout.count("\n part ") == 2
        assert result.stdout.count("type: 'tiledimage'") == 2
        assert result.stdout.count(f"({level_mode})") == 2
        assert "'right.Y'" not in result.stdout
        assert "'Y': half samp 1 1" in result.stdout

        result = do_run ([exrmetrics, split])
        metrics = json.loads(result.stdout)
        assert len(metrics) == 1
        assert len(metrics[0]["parts"]) == 2
        assert all(part["pixels"] > 0 for part in metrics[0]["parts"])
        assert all(part["total raw size"] > 0
                   for part in metrics[0]["parts"])

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
