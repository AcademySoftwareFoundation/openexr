#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from subprocess import PIPE, run

print(f"testing exrstdattr: {sys.argv}")

exrstdattr = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]

assert(os.path.isfile(exrstdattr))
assert(os.path.isfile(exrinfo))
assert(os.path.isdir(image_dir))

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
atexit.register(cleanup)

# no args = usage message
result = run ([exrstdattr], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 1)
assert(result.stderr.startswith ("Usage: "))

# -h = usage message
result = run ([exrstdattr, "-h"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 1)
assert(result.stderr.startswith ("Usage: "))

command = [exrstdattr]
command += ["-part", "0"]
command += ["-screenWindowCenter", "42", "43"]
command += ["-screenWindowWidth", "4.4"]
command += ["-pixelAspectRatio", "1.7"]
command += ["-wrapmodes", "clamp"]
command += ["-timeCode", "12345678", "34567890"]
command += ["-keyCode", "1", "2", "3", "4", "5", "6", "20"]
command += ["-framesPerSecond", "48", "1"]
command += ["-envmap", "LATLONG"]
command += ["-isoSpeed", "2.1"]
command += ["-aperture", "3.2"]
command += ["-expTime", "4.3"]
command += ["-focus", "5.4"]
command += ["-altitude", "6.5"]
command += ["-latitude", "7.6"]
command += ["-longitude", "8.7"]
command += ["-utcOffset", "9"]
command += ["-owner", "florian"]
command += ["-xDensity", "10.0"]
command += ["-lookModTransform", "lmt"]
command += ["-renderingTransform", "rt"]
command += ["-adoptedNeutral", "1.1", "2.2"]
command += ["-whiteLuminance", "17.1"]
command += ["-chromaticities", "1", "2", "3", "4", "5", "6", "7", "8"]
command += ["-int", "test_int", "42"]
command += ["-float", "test_float", "4.2"]
command += ["-string", "test_string", "forty two"]
command += ["-capDate", "1999:12:31 23:59:59"]
command += ["-comments", "blah blah blah"]
image = f"{image_dir}/TestImages/GrayRampsHorizontal.exr"
command += [image, outimage]

result = run (command, stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
assert(os.path.isfile(outimage))

result = run ([exrinfo, "-v", outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0)
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
assert('lookModTransform: string \'lmt\'' in result.stdout)
assert('owner: string \'florian\'' in result.stdout)
assert('pixelAspectRatio: float 1.7' in result.stdout)
assert('renderingTransform: string \'rt\'' in result.stdout)
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

print("success")
