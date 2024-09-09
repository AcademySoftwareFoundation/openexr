#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit, json
from subprocess import PIPE, run

print(f"testing exrmetrics: {' '.join(sys.argv)}")

exrmetrics = sys.argv[1]
image_dir = sys.argv[3]
version = sys.argv[4]

assert(os.path.isfile(exrmetrics)), "\nMissing " + exrmetrics
assert(os.path.isdir(image_dir)), "\nMissing " + image_dir

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
atexit.register(cleanup)

# no args = usage message
result = run ([exrmetrics], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert(result.stderr.startswith ("Usage: ")), "\n"+result.stderr

# -h = usage message
result = run ([exrmetrics, "-h"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("Usage: ")), "\n"+result.stdout

result = run ([exrmetrics, "--help"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("Usage: ")), "\n"+result.stdout

# --version
result = run ([exrmetrics, "--version"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("exrmetrics")), "\n"+result.stdout
assert(version in result.stdout), "\n"+result.stdout

# test missing arguments, using just the -option but no value

for a in ["-p","-l","-16","-z"]:
    result = run ([exrmetrics, a], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    print(" ".join(result.args))
    print(result.stderr)
    assert(result.returncode != 0), "\n"+result.stderr

command = [exrmetrics]
image = f"{image_dir}/TestImages/GrayRampsHorizontal.exr"
command += [image, outimage]

result = run (command, stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
print(result.returncode)
print(result.stdout)
print(result.stderr)
assert(result.returncode == 0), "\n"+result.stderr
assert(os.path.isfile(outimage)), "\nMissing " + outimage

# confirm data is valid JSON (will not be true if filename contains quotes)
data = json.loads(result.stdout)
for x in ['write time','output file size','input file size']:
  assert(x in data),"\n Missing field "+x

print("success")
