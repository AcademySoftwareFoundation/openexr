#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit, json
from do_run import do_run

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
result = do_run  ([exrmetrics], True)
assert result.stderr.startswith ("Usage: ")

# -h = usage message
result = do_run  ([exrmetrics, "-h"])
assert result.stdout.startswith ("Usage: ")

result = do_run  ([exrmetrics, "--help"])
assert result.stdout.startswith ("Usage: ")

# --version
result = do_run  ([exrmetrics, "--version"])
assert result.stdout.startswith ("exrmetrics")
assert version in result.stdout

# test missing arguments, using just the -option but no value

for a in ["-p","-l","-16","-z","-t","-i","--passes","-o","--pixelmode","--time"]:
    result = do_run  ([exrmetrics, a], True)
    assert "Missing" in result.stderr

for image in [f"{image_dir}/GrayRampsHorizontal.exr",f"{image_dir}/multipart.0001.exr",f"{image_dir}/Flowers.exr"]:
    for time in ["none","read","write","reread","read,write","read,reread","read,write,reread"]:
        for passes in ["1","2"]:
            for nosize in range(0,2):
              command = [exrmetrics]
              command += ["-i",image, "--passes",passes,"--time",time,"-o",outimage]
              if nosize:
                  command += ['--no-size']
              result = do_run (command)
              assert os.path.isfile(outimage)
              if len(result.stdout):
                # confirm data is valid JSON (will not be true if filename contains quotes)
                data = json.loads(result.stdout)
                assert len(data) == 1
                if not nosize:
                  for x in ['file','pixels','compression','part type','total raw size']:
                     assert x in data[0]

print("success")
