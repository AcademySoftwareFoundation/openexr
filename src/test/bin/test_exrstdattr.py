#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from do_run import do_run

print(f"testing exrstdattr: {' '.join(sys.argv)}")

exrstdattr = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

assert(os.path.isfile(exrstdattr)), "\nMissing " + exrstdattr
assert(os.path.isfile(exrinfo)), "\nMissing " + exrinfo
assert(os.path.isdir(image_dir)), "\nMissing " + image_dir

test_images = {}
test_images["GrayRampsHorizontal"] = f"{image_dir}/GrayRampsHorizontal.exr"

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

fd, outimage2 = tempfile.mkstemp(".exr")
os.close(fd)


def cleanup():
    print(f"deleting {outimage}")
atexit.register(cleanup)

# no args = usage message
result = do_run ([exrstdattr], True)
assert result.stderr.startswith ("Usage: ")

# -h = usage message
result = do_run ([exrstdattr, "-h"])
assert result.stdout.startswith ("Usage: ")

result = do_run ([exrstdattr, "--help"])
assert result.stdout.startswith ("Usage: ")

# --version
result = do_run ([exrstdattr, "--version"])
assert result.stdout.startswith ("exrstdattr")
assert version in result.stdout

attrs = [
    ["-part", "0"],
    ["-screenWindowCenter", "42", "43"],
    ["-screenWindowWidth", "4.4"],
    ["-pixelAspectRatio", "1.7"],
    ["-wrapmodes", "clamp"],
    ["-timeCode", "12345678", "34567890"],
    ["-keyCode", "1", "2", "3", "4", "5", "6", "20"],
    ["-framesPerSecond", "48", "1"],
    ["-envmap", "LATLONG"],
    ["-isoSpeed", "2.1"],
    ["-aperture", "3.2"],
    ["-expTime", "4.3"],
    ["-focus", "5.4"],
    ["-altitude", "6.5"],
    ["-latitude", "7.6"],
    ["-longitude", "8.7"],
    ["-utcOffset", "9"],
    ["-owner", "florian"],
    ["-xDensity", "10.0"],
    ["-adoptedNeutral", "1.1", "2.2"],
    ["-whiteLuminance", "17.1"],
    ["-chromaticities", "1", "2", "3", "4", "5", "6", "7", "8"],
    ["-int", "test_int", "42"],
    ["-float", "test_float", "4.2"],
    ["-string", "test_string", "forty two"],
    ["-capDate", "1999:12:31 23:59:59"],
    ["-comments", "blah blah blah"],
]

# test missing arguments, using just the -option but no value

for a in attrs:
    result = do_run ([exrstdattr, a[0]], True)

command = [exrstdattr]
for a in attrs:
    command += a 
command += [test_images["GrayRampsHorizontal"], outimage]

result = do_run (command)
assert os.path.isfile(outimage)

result = do_run ([exrinfo, "-v", outimage])
try:
    assert('adoptedNeutral: v2f [ 1.1, 2.2 ]' in result.stdout)
    assert('altitude: float 6.5' in result.stdout)
    assert('aperture: float 3.2' in result.stdout)
    assert('capDate: string \'1999:12:31 23:59:59\'' in result.stdout)
    assert('channels: chlist 1 channels' in result.stdout)
    assert('\'Y\': half samp 1 1' in result.stdout)
    assert('chromaticities: chromaticities r[1, 2] g[3, 4] b[5, 6] w[7, 8]' in result.stdout)
    assert('comments: string \'blah blah blah\'' in result.stdout)
    assert('compression: compression \'pxr24\' (0x05)' in result.stdout)
    assert('dataWindow: box2i [ 0, 0 - 799 799 ] 800 x 800' in result.stdout)
    assert('displayWindow: box2i [ 0, 0 - 799 799 ] 800 x 800' in result.stdout)
    assert('envmap: envmap latlong' in result.stdout)
    assert('expTime: float 4.3' in result.stdout)
    assert('focus: float 5.4' in result.stdout)
    assert('framesPerSecond: rational 48 / 1 (48)' in result.stdout)
    assert('isoSpeed: float 2.1' in result.stdout)
    assert('keyCode: keycode mfgc 1 film 2 prefix 3 count 4 perf_off 5 ppf 6 ppc 20' in result.stdout)
    assert('latitude: float 7.6' in result.stdout)
    assert('lineOrder: lineOrder 0 (increasing)' in result.stdout)
    assert('longitude: float 8.7' in result.stdout)
    assert('owner: string \'florian\'' in result.stdout)
    assert('pixelAspectRatio: float 1.7' in result.stdout)
    assert('screenWindowCenter: v2f [ 42, 43 ]' in result.stdout)
    assert('screenWindowWidth: float 4.4' in result.stdout)
    assert('test_float: float 4.2' in result.stdout)
    assert('test_int: int 42' in result.stdout)
    assert('test_string: string \'forty two\'' in result.stdout)
    assert('timeCode: timecode time 305419896 user 878082192' in result.stdout)
    assert('type: string \'scanlineimage\'' in result.stdout)
    assert('utcOffset: float 9' in result.stdout)
    assert('whiteLuminance: float 17.1' in result.stdout)
    assert('wrapmodes: string \'clamp\'' in result.stdout)
    assert('xDensity: float 10' in result.stdout)
except AssertionError:
    print(result.stdout)
    raise

# test for bad command
result = do_run([exrstdattr,test_images["GrayRampsHorizontal"],outimage,"extra_argument"],True)

# test for bad erase argument
result = do_run ([exrstdattr, "-erase"], True)

# test for errors trying to delete a critical attribute
result = do_run ([exrstdattr, "-erase","dataWindow",outimage,outimage2], True)

# test deleting 'comments'
result = do_run ([exrstdattr, "-erase","comments",outimage,outimage2])
assert os.path.isfile(outimage2)

result = do_run ([exrinfo, "-v", outimage2])
assert "comments" not in result.stdout

print("success")
