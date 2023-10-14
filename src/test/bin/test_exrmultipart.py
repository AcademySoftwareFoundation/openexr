#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from subprocess import PIPE, run

print(f"testing exrmultipart: {' '.join(sys.argv)}")

exrmultipart = sys.argv[1]
exrinfo = sys.argv[2]
image_dir = sys.argv[3]
version = sys.argv[4]

result = run ([exrmultipart], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode != 0), "\n"+result.stderr
assert("Usage:" in result.stderr), "\n"+result.stderr

# -h = usage message
result = run ([exrmultipart, "-h"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("Usage: ")), "\n"+result.stdout

result = run ([exrmultipart, "--help"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("Usage: ")), "\n"+result.stdout

# --version
result = run ([exrmultipart, "--version"], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
assert(result.stdout.startswith ("exrmultipart")), "\n"+result.stdout
assert(version in result.stdout), "\n"+result.stdout

image = f"{image_dir}/Beachball/multipart.0001.exr"

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
    os.unlink(outimage)
atexit.register(cleanup)

# combine
command = [exrmultipart, "-combine", "-i", f"{image}:0", f"{image}:1", "-o", outimage]
result = run (command, stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr

result = run ([exrinfo, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr

# error: can't convert multipart images
command = [exrmultipart, "-convert", "-i", image, "-o", outimage]
result = run (command, stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert (result.returncode != 0)

# convert
singlepart_image = f"{image_dir}/Beachball/singlepart.0001.exr"
command = [exrmultipart, "-convert", "-i", singlepart_image, "-o", outimage]
result = run (command, stdout=PIPE, stderr=PIPE, universal_newlines=True)
assert(result.returncode == 0), "\n"+result.stderr

result = run ([exrinfo, outimage], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr

# separate

# get part names from the multipart image
result = run ([exrinfo, image], stdout=PIPE, stderr=PIPE, universal_newlines=True)
print(" ".join(result.args))
assert(result.returncode == 0), "\n"+result.stderr
part_names = {}
for p in result.stdout.split('\n part ')[1:]:
    output = p.split('\n')
    part_number, part_name = output[0].split(': ')
    part_names[part_number] = part_name

with tempfile.TemporaryDirectory() as tempdir:

    command = [exrmultipart, "-separate", "-i", image, "-o", f"{tempdir}/separate"]
    result = run (command, stdout=PIPE, stderr=PIPE, universal_newlines=True)
    print(" ".join(result.args))
    assert(result.returncode == 0), "\n"+result.stderr

    for i in range(1, 10):
        s = f"{tempdir}/separate.{i}.exr"
        result = run ([exrinfo, "-v", s], stdout=PIPE, stderr=PIPE, universal_newlines=True)
        print(" ".join(result.args))
        assert(result.returncode == 0), "\n"+result.stderr
        output = result.stdout.split('\n')
        assert(output[1].startswith(' parts: 1')), "\n"+result.stdout
        output[2].startswith(' part 1:')
        part_name = output[2][9:]
        part_number = str(i)
        assert(part_names[part_number] == part_name), "\n"+result.stdout

print("success")

