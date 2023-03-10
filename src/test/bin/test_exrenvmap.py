#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from subprocess import PIPE, run

print(f"testing exrenvmap: {sys.argv}")

exrenvmap = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]

image = f"{image_dir}/MultiResolution/WavyLinesLatLong.exr"

assert(os.path.isfile(exrenvmap))
assert(os.path.isfile(exrinfo))
assert(os.path.isdir(image_dir))
assert(os.path.isfile(image))

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
atexit.register(cleanup)

# no args = usage message
result = run ([exrenvmap], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
#print(f"stdout: {result.stdout}")
#print(f"stderr: {result.stderr}")
assert(result.returncode == 1)
assert(result.stderr.startswith ("Usage: "))

# -h = usage message
result = run ([exrenvmap, "-h"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 1)
assert(result.stderr.startswith ("Usage: "))

# default
result = run ([exrenvmap, image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))
default_file_size = os.path.getsize(outimage)

# -o 
result = run ([exrenvmap, "-o", image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert('tiles: tiledesc size 64 x 64 level 0 (single image) round 0 (down)' in result.stdout)
os.unlink(outimage)

# -m 
result = run ([exrenvmap, "-m", image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert('tiles: tiledesc size 64 x 64 level 1 (mipmap) round 0 (down)' in result.stdout)
os.unlink(outimage)

# -c 
result = run ([exrenvmap, "-c", image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert('envmap: envmap cube' in result.stdout)
os.unlink(outimage)

# -l 
result = run ([exrenvmap, "-l", image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert('envmap: envmap latlong' in result.stdout)
os.unlink(outimage)

# -w 
result = run ([exrenvmap, "-w", "64", image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert('x tile count: 1 (sz 64)' in result.stdout)
assert('y tile count: 6 (sz 384)' in result.stdout)
os.unlink(outimage)

# -f (filter)
result = run ([exrenvmap, "-f", "1.1", "6", image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))
file_size = os.path.getsize(outimage)
assert(file_size != default_file_size)
os.unlink(outimage)

# -b (blur)
result = run ([exrenvmap, "-b", image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))
file_size = os.path.getsize(outimage)
assert(file_size != default_file_size)

# -t 
result = run ([exrenvmap, "-t", "32", "48", image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert('x tile count: 8 (sz 256)' in result.stdout)
assert('y tile count: 32 (sz 1536)' in result.stdout)
os.unlink(outimage)

# -u 
result = run ([exrenvmap, "-u", image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert('tiles: tiledesc size 64 x 64 level 0 (single image) round 1 (up)' in result.stdout)
os.unlink(outimage)

# -z dwaa 
result = run ([exrenvmap, "-z", "dwaa", image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert('compression: compression \'dwaa\' (0x08)' in result.stdout)
os.unlink(outimage)

# -z dwab
result = run ([exrenvmap, "-z", "dwab", image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert('compression: compression \'dwab\' (0x09)' in result.stdout)
os.unlink(outimage)

with tempfile.TemporaryDirectory() as tempdir:

    cube_face_image_t = f"{tempdir}/out.%.exr"
    result = run ([exrenvmap, image, cube_face_image_t], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    print(" ".join(result.args))
    assert(result.returncode == 0)

    for o in ["+X", "-X", "+Y", "-Y", "+Z", "-Z"]:
        
        cube_face_image = f"{tempdir}/out.{o}.exr"
        result = run ([exrinfo, "-v", cube_face_image], stdout=PIPE, stderr=PIPE, universal_newlines=True)
        print(" ".join(result.args))
        assert(result.returncode == 0)
        assert('x tile count: 4 (sz 256)' in result.stdout)
        assert('y tile count: 4 (sz 256)' in result.stdout)

    result = run ([exrenvmap, cube_face_image_t, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    print(" ".join(result.args))
    assert(result.returncode == 0)

    result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    print(" ".join(result.args))
    assert(result.returncode == 0)
    assert('tiles: tiledesc size 64 x 64 level 0 (single image) round 0 (down)' in result.stdout)

print("success")
