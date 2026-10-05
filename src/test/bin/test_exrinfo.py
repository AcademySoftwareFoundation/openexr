#!/usr/bin/env python

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import sys, os, tempfile, atexit
from do_run import do_run

print(f"testing exrinfo: {' '.join(sys.argv)}")

exrinfo = sys.argv[1]
image_dir = sys.argv[2]
version = sys.argv[3]
exrstdattr = sys.argv[4]

assert(os.path.isfile(exrinfo)), "\nMissing " + exrinfo
assert(os.path.isdir(image_dir)), "\nMissing " + image_dir
assert(os.path.isfile(exrstdattr)), "\nMissing " + exrstdattr

test_images = {}
test_images["GrayRampsHorizontal"] = f"{image_dir}/GrayRampsHorizontal.exr"

result = do_run ([exrinfo, "-h"])
assert result.stdout.startswith ("Usage: ")

result = do_run ([exrinfo, "--help"])
assert result.stdout.startswith ("Usage: ")

# --version
result = do_run ([exrinfo, "--version"])
assert result.stdout.startswith ("exrinfo")
assert version in result.stdout

result = do_run ([exrinfo, test_images["GrayRampsHorizontal"], "-a", "-v"])
output = result.stdout.split('\n')
try:
    assert ('pxr24' in output[1])
    assert ('800 x 800' in output[2])
    assert ('800 x 800' in output[3])
    assert ('1 channels' in output[4])
except AssertionError:
    print(result.stdout)
    raise

# test image as stdio
with open(test_images["GrayRampsHorizontal"], 'rb') as f:
    image_data = f.read()
result = do_run ([exrinfo, '-', "-a", "-v"], data=image_data)
output = result.stdout.decode().split('\n')
try:
    assert ('pxr24' in output[1])
    assert ('800 x 800' in output[2])
    assert ('800 x 800' in output[3])
    assert ('1 channels' in output[4])
except AssertionError:
    print(result.stdout)
    raise

#
# Inconsistent color space metadata is reported as a warning on stderr.
# These files are not malformed, so exrinfo still succeeds: do_run would
# exit if the return code were non-zero.
#

fd, outimage = tempfile.mkstemp(".exr")
os.close(fd)

def cleanup():
    print(f"deleting {outimage}")
    os.remove(outimage)
atexit.register(cleanup)

rec709 = ["0.64", "0.33", "0.30", "0.60", "0.15", "0.06", "0.3127", "0.3290"]
ap0 = ["0.73470", "0.26530", "0.00000", "1.00000", "0.00010", "-0.07700",
       "0.32168", "0.33767"]

def check_warnings(attrs, expected):
    do_run ([exrstdattr] + attrs + [test_images["GrayRampsHorizontal"], outimage])
    result = do_run ([exrinfo, outimage])
    for e in expected:
        assert e in result.stderr, f"expected '{e}' in stderr, got '{result.stderr}'"
    if not expected:
        assert "WARNING" not in result.stderr, f"unexpected warning: '{result.stderr}'"

# an ID that agrees with the chromaticities is clean
check_warnings (["-colorInteropID", "lin_rec709_scene",
                 "-chromaticities"] + rec709, [])

# an ID that disagrees with the chromaticities names both color spaces
check_warnings (["-colorInteropID", "lin_ap1_scene",
                 "-chromaticities"] + rec709,
                ["colorInteropID is 'lin_ap1_scene' but the chromaticities are those of 'lin_rec709_scene'"])

# chromaticities matching no known color space are the same finding, but
# there is no other color space to name in the message
check_warnings (["-colorInteropID", "lin_ap1_scene",
                 "-chromaticities", "1", "2", "3", "4", "5", "6", "7", "8"],
                ["the chromaticities are not those denoted by the colorInteropID"])

# a part tagged "data" should carry no color-related attributes
check_warnings (["-colorInteropID", "data",
                 "-chromaticities"] + rec709 + ["-whiteLuminance", "100"],
                ["colorInteropID is 'data' but the part has a chromaticities attribute",
                 "colorInteropID is 'data' but the part has a whiteLuminance attribute"])

# an empty ID says nothing and should be omitted instead
check_warnings (["-colorInteropID", ""], ["colorInteropID is empty"])

#
# acesImageContainerFlag asserts SMPTE ST 2065-4 compliance, which requires the
# color space be ACES2065-1. exrstdattr has no dedicated option for it, so
# set it as a generic int attribute.
#

# a consistent ACES container is clean, with or without the ID
check_warnings (["-int", "acesImageContainerFlag", "1",
                 "-colorInteropID", "lin_ap0_scene",
                 "-chromaticities"] + ap0, [])
check_warnings (["-int", "acesImageContainerFlag", "1",
                 "-chromaticities"] + ap0, [])

# ST 2065-4 requires the chromaticities
check_warnings (["-int", "acesImageContainerFlag", "1",
                 "-colorInteropID", "lin_ap0_scene"],
                ["acesImageContainerFlag is present but the part has no chromaticities attribute"])

# a present ID has to agree, and so do the chromaticities
check_warnings (["-int", "acesImageContainerFlag", "1",
                 "-colorInteropID", "lin_rec709_scene",
                 "-chromaticities"] + rec709,
                ["acesImageContainerFlag is present but colorInteropID is 'lin_rec709_scene', not 'lin_ap0_scene'",
                 "acesImageContainerFlag is present but the chromaticities are those of 'lin_rec709_scene', not 'lin_ap0_scene'"])

# 1 is the only defined value of the flag
check_warnings (["-int", "acesImageContainerFlag", "0",
                 "-colorInteropID", "lin_ap0_scene",
                 "-chromaticities"] + ap0,
                ["acesImageContainerFlag is present but is not an int of value 1"])

# as is an attribute of the wrong type
check_warnings (["-string", "acesImageContainerFlag", "yes",
                 "-colorInteropID", "lin_ap0_scene",
                 "-chromaticities"] + ap0,
                ["acesImageContainerFlag is present but is not an int of value 1"])

print("success")

