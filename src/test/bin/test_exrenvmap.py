#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from do_run import do_run

print(f"testing exrenvmap: {' '.join(sys.argv)}")

exrenvmap = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

test_images = {}
test_images["latlong"] = f"{image_dir}/WavyLinesLatLong.exr"
test_images["cube"] = f"{image_dir}/WavyLinesCube.exr"

assert(os.path.isfile(exrenvmap)), "\nMissing " + exrenvmap
assert(os.path.isfile(exrinfo)), "\nMissing " + exrinfo
assert(os.path.isdir(image_dir)), "\nMissing " + image_dir
assert(os.path.isfile(test_images["latlong"])), "\nMissing " + test_images["latlong"]

# exrenvmap's output cost scales with the square of the cube-face width
# (6 faces x width^2 pixels, each resampled with an NxN point-sample
# filter), but the default width of 256 is far larger than needed to
# confirm that a given flag behaves correctly. Most of the invocations
# below only check that a file was produced, that a particular header
# attribute or compression tag is set, or that the output differs from
# a baseline -- none of which requires a full-size image -- so they
# pass "-w", SMALL_WIDTH to run at a small, fixed size instead of the
# default. This cuts the bulk of the test's running time, which matters
# most on slow/emulated CI hosts (e.g. QEMU-emulated s390x) where this
# test has been observed to time out.
#
# Tests that assert on tile counts derived from the *default* map
# width (256) -- the "-t" test and the multi-file cube-face round trip
# at the end -- are left at the default size, since their assertions
# are tied to that specific width.
SMALL_WIDTH = 16

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
atexit.register(cleanup)

# no args = usage message
result = do_run ([exrenvmap], True)
assert result.stderr.startswith ("Usage: ")

# -h = usage message
result = do_run ([exrenvmap, "-h"])
assert result.stdout.startswith ("Usage: ")

# --help = usage message
result = do_run ([exrenvmap, "--help"])
assert result.stdout.startswith ("Usage: ")

# --version
result = do_run ([exrenvmap, "--version"])
assert result.stdout.startswith ("exrenvmap")
assert version in result.stdout

# default
result = do_run ([exrenvmap, "-w", str(SMALL_WIDTH), test_images["latlong"], outimage])
assert os.path.isfile(outimage)
default_file_size = os.path.getsize(outimage)

result = do_run ([exrenvmap, "-li", "-w", str(SMALL_WIDTH), test_images["latlong"], outimage])
assert os.path.isfile(outimage)

result = do_run ([exrenvmap, "-ci", "-w", str(SMALL_WIDTH), test_images["cube"], outimage])
assert os.path.isfile(outimage)

# -o 
result = do_run ([exrenvmap, "-o", "-w", str(SMALL_WIDTH), test_images["latlong"], outimage])
assert os.path.isfile(outimage)

result = do_run ([exrinfo, "-v", outimage])
assert 'tiles: tiledesc size 64 x 64 level 0 (single image) round 0 (down)' in result.stdout
os.unlink(outimage)

# -m 
result = do_run ([exrenvmap, "-m", "-w", str(SMALL_WIDTH), test_images["latlong"], outimage])
assert os.path.isfile(outimage)

result = do_run ([exrinfo, "-v", outimage])
assert 'tiles: tiledesc size 64 x 64 level 1 (mipmap) round 0 (down)' in result.stdout
os.unlink(outimage)

# -c 
result = do_run ([exrenvmap, "-c", "-w", str(SMALL_WIDTH), test_images["latlong"], outimage])
assert os.path.isfile(outimage)

result = do_run ([exrinfo, "-v", outimage])
assert 'envmap: envmap cube' in result.stdout
os.unlink(outimage)

# -l 
result = do_run ([exrenvmap, "-l", "-w", str(SMALL_WIDTH), test_images["latlong"], outimage])
assert os.path.isfile(outimage)

result = do_run ([exrinfo, "-v", outimage])
assert 'envmap: envmap latlong' in result.stdout
os.unlink(outimage)

# -w 

result = do_run ([exrenvmap, "-w", test_images["latlong"], outimage], True)
assert not os.path.isfile(outimage)

result = do_run ([exrenvmap, "-w", "-64", test_images["latlong"], outimage], True)
assert not os.path.isfile(outimage)

result = do_run ([exrenvmap, "-w", "64", test_images["latlong"], outimage])
assert os.path.isfile(outimage)

result = do_run ([exrinfo, "-v", outimage])
assert 'x tile count: 1 (sz 64)' in result.stdout
assert 'y tile count: 6 (sz 384)' in result.stdout
os.unlink(outimage)

# -f (filter)
result = do_run ([exrenvmap, "-f", test_images["latlong"], outimage], True)
assert not os.path.isfile(outimage)

result = do_run ([exrenvmap, "-f", "1.1", test_images["latlong"], outimage], True)
assert not os.path.isfile(outimage)

result = do_run ([exrenvmap, "-f", "-1.1", "6", test_images["latlong"], outimage], True)
assert not os.path.isfile(outimage)

result = do_run ([exrenvmap, "-f", "1.1", "-6", test_images["latlong"], outimage], True)
assert not os.path.isfile(outimage)

result = do_run ([exrenvmap, "-f", "1.1", "6", "-w", str(SMALL_WIDTH), test_images["latlong"], outimage])
assert os.path.isfile(outimage)
file_size = os.path.getsize(outimage)
assert file_size != default_file_size
os.unlink(outimage)

# -b (blur)
#
# On slow/emulated CI hosts (e.g. QEMU-emulated s390x), blurImage() is
# very slow and risks timeout, so just skip it.
if not os.environ.get ("OPENEXR_SKIP_SLOW_TESTS"):
    result = do_run ([exrenvmap, "-b", "-w", str(SMALL_WIDTH), test_images["latlong"], outimage])
    assert os.path.isfile(outimage)
    file_size = os.path.getsize(outimage)
    assert file_size != default_file_size
    os.unlink(outimage)

# -t 
result = do_run ([exrenvmap, "-t", test_images["latlong"], outimage], True)
assert not os.path.isfile(outimage)

result = do_run ([exrenvmap, "-t", "32", test_images["latlong"], outimage], True)
assert not os.path.isfile(outimage)

result = do_run ([exrenvmap, "-t", "-32", "48", test_images["latlong"], outimage], True)
assert not os.path.isfile(outimage)

result = do_run ([exrenvmap, "-t", "32", "-48", test_images["latlong"], outimage], True)
assert not os.path.isfile(outimage)

result = do_run ([exrenvmap, "-t", "32", "48", test_images["latlong"], outimage])
assert os.path.isfile(outimage)

result = do_run ([exrinfo, "-v", outimage])
assert 'x tile count: 8 (sz 256)' in result.stdout
assert 'y tile count: 32 (sz 1536)' in result.stdout
os.unlink(outimage)

# -u 
result = do_run ([exrenvmap, "-u", "-w", str(SMALL_WIDTH), test_images["latlong"], outimage])
assert os.path.isfile(outimage)

result = do_run ([exrinfo, "-v", outimage])
assert 'tiles: tiledesc size 64 x 64 level 0 (single image) round 1 (up)' in result.stdout
os.unlink(outimage)

# -z 
result = do_run ([exrenvmap, "-z", test_images["latlong"], outimage], True)
assert not os.path.isfile(outimage)

result = do_run ([exrenvmap, "-z", "xxx", test_images["latlong"], outimage], True)
assert not os.path.isfile(outimage)

for z in ["none", "rle", "zip", "piz", "pxr24", "b44", "b44a", "dwaa", "dwab"]:
    result = do_run ([exrenvmap, "-z", z, "-w", str(SMALL_WIDTH), test_images["latlong"], outimage])
    assert os.path.isfile(outimage)

    result = do_run ([exrinfo, "-v", outimage])
    assert f'compression: compression \'{z}\'' in result.stdout
    os.unlink(outimage)

with tempfile.TemporaryDirectory() as tempdir:

    cube_face_image_t = f"{tempdir}/out.%.exr"
    result = do_run ([exrenvmap, test_images["latlong"], cube_face_image_t])

    for o in ["+X", "-X", "+Y", "-Y", "+Z", "-Z"]:
        
        cube_face_image = f"{tempdir}/out.{o}.exr"
        result = do_run ([exrinfo, "-v", cube_face_image])
        assert 'x tile count: 4 (sz 256)' in result.stdout
        assert 'y tile count: 4 (sz 256)' in result.stdout

    result = do_run ([exrenvmap, cube_face_image_t, outimage])

    result = do_run ([exrinfo, "-v", outimage])
    assert 'tiles: tiledesc size 64 x 64 level 0 (single image) round 0 (down)' in result.stdout

print("success")
