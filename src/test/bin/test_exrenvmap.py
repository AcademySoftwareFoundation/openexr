#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from subprocess import PIPE, run

print(f"testing exrenvmap: {sys.argv}")

exrenvmap = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

latlong_image = f"{image_dir}/MultiResolution/WavyLinesLatLong.exr"
cube_image = f"{image_dir}/MultiResolution/WavyLinesCube.exr"

assert(os.path.isfile(exrenvmap)), "\nMissing " + exrenvmap
assert(os.path.isfile(exrinfo)), "\nMissing " + exrinfo
assert(os.path.isdir(image_dir)), "\nMissing " + image_dir
assert(os.path.isfile(latlong_image)), "\nMissing " + latlong_image

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
atexit.register(cleanup)

# no args = usage message
result = run ([exrenvmap], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(result.stderr.startswith ("Usage: ")), "\n"+result.stderr

# -h = usage message
result = run ([exrenvmap, "-h"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("Usage: ")), "\n"+result.stdout

# --help = usage message
result = run ([exrenvmap, "--help"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("Usage: ")), "\n"+result.stdout

# --version
result = run ([exrenvmap, "--version"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("exrenvmap")), "\n"+result.stdout
assert(version in result.stdout), "\n"+result.stdout

# default
result = run ([exrenvmap, latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(os.path.isfile(outimage)), "\nMissing " + outimage
default_file_size = os.path.getsize(outimage)

result = run ([exrenvmap, "-li", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrenvmap, "-ci", cube_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(os.path.isfile(outimage)), "\nMissing " + outimage

# -o 
result = run ([exrenvmap, "-o", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert('tiles: tiledesc size 64 x 64 level 0 (single image) round 0 (down)' in result.stdout), "\n"+result.stdout
os.unlink(outimage)

# -m 
result = run ([exrenvmap, "-m", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert('tiles: tiledesc size 64 x 64 level 1 (mipmap) round 0 (down)' in result.stdout), "\n"+result.stdout
os.unlink(outimage)

# -c 
result = run ([exrenvmap, "-c", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert('envmap: envmap cube' in result.stdout), "\n"+result.stdout
os.unlink(outimage)

# -l 
result = run ([exrenvmap, "-l", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert('envmap: envmap latlong' in result.stdout), "\n"+result.stdout
os.unlink(outimage)

# -w 

result = run ([exrenvmap, "-w", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(not os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrenvmap, "-w", "-64", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(not os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrenvmap, "-w", "64", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert('x tile count: 1 (sz 64)' in result.stdout), "\n"+result.stdout
assert('y tile count: 6 (sz 384)' in result.stdout), "\n"+result.stdout
os.unlink(outimage)

# -f (filter)
result = run ([exrenvmap, "-f", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(not os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrenvmap, "-f", "1.1", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(not os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrenvmap, "-f", "-1.1", "6", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(not os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrenvmap, "-f", "1.1", "-6", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(not os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrenvmap, "-f", "1.1", "6", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(os.path.isfile(outimage)), "\nMissing " + outimage
file_size = os.path.getsize(outimage)
assert(file_size != default_file_size), "\n{} is the wrong size".format(outimage)
os.unlink(outimage)

# -b (blur)
result = run ([exrenvmap, "-b", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(os.path.isfile(outimage)), "\nMissing " + outimage
file_size = os.path.getsize(outimage)
assert(file_size != default_file_size), "\n{} is the wrong size".format(outimage)
os.unlink(outimage)

# -t 
result = run ([exrenvmap, "-t", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(not os.path.isfile(outimage)), "\n{} still exists".format(outimage)

result = run ([exrenvmap, "-t", "32", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(not os.path.isfile(outimage)), "\n{} still exists".format(outimage)

result = run ([exrenvmap, "-t", "-32", "48", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(not os.path.isfile(outimage)), "\n{} still exists".format(outimage)

result = run ([exrenvmap, "-t", "32", "-48", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(not os.path.isfile(outimage)), "\n{} still exists".format(outimage)

result = run ([exrenvmap, "-t", "32", "48", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert('x tile count: 8 (sz 256)' in result.stdout), "\n"+result.stdout
assert('y tile count: 32 (sz 1536)' in result.stdout), "\n"+result.stdout
os.unlink(outimage)

# -u 
result = run ([exrenvmap, "-u", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(os.path.isfile(outimage)), "\nMissing " + outimage

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert('tiles: tiledesc size 64 x 64 level 0 (single image) round 1 (up)' in result.stdout), "\n"+result.stdout
os.unlink(outimage)

# -z 
result = run ([exrenvmap, "-z", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(not os.path.isfile(outimage)), "\n{} still exists".format(outimage)

result = run ([exrenvmap, "-z", "xxx", latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(not os.path.isfile(outimage)), "\n{} still exists".format(outimage)

for z in ["none", "rle", "zip", "piz", "pxr24", "b44", "b44a", "dwaa", "dwab"]:
    result = run ([exrenvmap, "-z", z, latlong_image, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    print(" ".join(result.args))
    assert(result.returncode == 0), "\n"+result.stderr
    assert(os.path.isfile(outimage)), "\nMissing " + outimage

    result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    print(" ".join(result.args))
    assert(result.returncode == 0), "\n"+result.stderr
    assert(f'compression: compression \'{z}\'' in result.stdout), "\n"+result.stdout
    os.unlink(outimage)

with tempfile.TemporaryDirectory() as tempdir:

    cube_face_image_t = f"{tempdir}/out.%.exr"
    result = run ([exrenvmap, latlong_image, cube_face_image_t], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    print(" ".join(result.args))
    assert(result.returncode == 0), "\n"+result.stderr

    for o in ["+X", "-X", "+Y", "-Y", "+Z", "-Z"]:
        
        cube_face_image = f"{tempdir}/out.{o}.exr"
        result = run ([exrinfo, "-v", cube_face_image], stdout=PIPE, stderr=PIPE, universal_newlines=True)
        print(" ".join(result.args))
        assert(result.returncode == 0)
        assert('x tile count: 4 (sz 256)' in result.stdout), "\n"+result.stdout
        assert('y tile count: 4 (sz 256)' in result.stdout), "\n"+result.stdout

    result = run ([exrenvmap, cube_face_image_t, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    print(" ".join(result.args))
    assert(result.returncode == 0), "\n"+result.stderr

    result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    print(" ".join(result.args))
    assert(result.returncode == 0), "\n"+result.stderr
    assert('tiles: tiledesc size 64 x 64 level 0 (single image) round 0 (down)' in result.stdout), "\n"+result.stdout

print("success")
