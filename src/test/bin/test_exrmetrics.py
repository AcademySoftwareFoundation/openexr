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

test_images = {}
test_images["GrayRampsHorizontal"] = f"{image_dir}/GrayRampsHorizontal.exr"
test_images["multipart"] = f"{image_dir}/multipart.0001.exr"
test_images["Flowers"] = f"{image_dir}/Flowers.exr"

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
atexit.register(cleanup)

# no args = usage message
result = do_run ([exrmetrics], True)
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

for image in test_images:
    for time in ["none","read","write","reread","read,write","read,reread","read,write,reread"]:
        for passes in ["1","2"]:
            for nosize in range(0,2):
              command = [exrmetrics]
              command += ["-i",test_images[image], "--passes",passes,"--time",time,"-o",outimage]
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

# test -p 0 with multipart file
result = do_run ([exrmetrics, test_images["multipart"], "-p", "0", "-z","zip"])
data = json.loads(result.stdout)
assert(len(data)==1),"\n Unexpected list size in JSON object"

# test --part-disk-size json output
result = do_run ([exrmetrics, test_images["multipart"], "--part-disk-size"])
data = json.loads(result.stdout)
assert(len(data)==1),"\n Unexpected list size in JSON object"

# test --passes 0 omits untested write/re-read timing from JSON and rejects -o
for time in ["none", "read", "write", "reread", "read,write,reread"]:
    result = do_run ([exrmetrics, test_images["Flowers"], "--passes", "0", "--time", time])
    if len(result.stdout):
        data = json.loads(result.stdout)
        assert len(data) == 1
        m = data[0]["metrics"][0]
        assert "write time" not in m
        assert "re-read time" not in m
        if "read" in time.split(","):
            assert "read time" in m

result = do_run ([exrmetrics, test_images["Flowers"], "--passes", "0", "-o", outimage], True)

# test --time reread --no-size without -o
result = do_run ([exrmetrics, test_images["Flowers"], "--time", "reread", "--no-size"])
data = json.loads(result.stdout)
assert "re-read time" in data[0]["metrics"][0]

print("success")
