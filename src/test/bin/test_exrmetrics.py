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

for a in ["-p","-l","-16","-z","-t","-i","--passes","-o","--pixelmode","--time"]:
    result = run ([exrmetrics, a], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    print(" ".join(result.args))
    print(result.stderr)
    assert(result.returncode != 0), "\n"+result.stderr
    assert("Missing" in result.stderr),"expected 'Missing argument' error"

for image in [f"{image_dir}/TestImages/GrayRampsHorizontal.exr",f"{image_dir}/Beachball/multipart.0001.exr",f"{image_dir}/LuminanceChroma/Flowers.exr"]:
    for time in ["none","read","write","reread","read,write","read,reread","read,write,reread"]:
        for passes in ["1","2"]:
            for nosize in range(0,2):
              command = [exrmetrics]
              command += ["-i",image, "--passes",passes,"--time",time,"-o",outimage]
              if nosize:
                  command += ['--no-size']
              result = run (command, stdout=PIPE, stderr=PIPE, universal_newlines=True)
              print(" ".join(result.args))
              print(result.returncode)
              print(result.stderr)
              assert(result.returncode == 0), "\n"+result.stderr
              assert(os.path.isfile(outimage)), "\nMissing " + outimage
              if len(result.stdout):
                # confirm data is valid JSON (will not be true if filename contains quotes)
                data = json.loads(result.stdout)
                assert(len(data)==1),"\n Unexpected list size in JSON object"
                if not nosize:
                  for x in ['file','pixels','compression','part type','total raw size']:
                     assert(x in data[0]),"\n Missing field "+x

print("success")
