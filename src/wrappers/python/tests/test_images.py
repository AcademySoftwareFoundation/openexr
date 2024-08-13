#!/usr/bin/env python3

#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.
#

#
# Download images from the openexr-images repo, and for each image,
# read it as both separate channels (default) and as RGB channels,
# write it, read the written file, and confirm it's identical.
#

from __future__ import print_function
import sys
import os
import tempfile
import atexit
import unittest
import numpy as np
import math
from subprocess import PIPE, run

import OpenEXR

def equalWithRelError (x1, x2, e):
    return ((x1 - x2) if (x1 > x2) else (x2 - x1)) <= e * (x1 if (x1 > 0) else -x1)

def compare_files(lhs, rhs):

    if len(lhs.parts) != len(rhs.parts):
        raise Exception(f"#parts differs: {len(lhs.parts)} {len(rhs.parts)}")

    for Plhs, Prhs in zip(lhs.parts,rhs.parts):
        compare_parts(Plhs, Prhs)

def is_default(name, value):

    if name == "screenWindowWidth":
        return value == 1.0

    if name == "type":
        return value == OpenEXR.scanlineimage

    return True

def compare_parts(lhs, rhs):
    
    attributes = set(lhs.header.keys()).union(set(rhs.header.keys()))

    for a in attributes:
        if a in ["channels", "chunkCount"]:
            continue

        if a not in lhs.header:
            if not is_default(a, rhs.header[a]):
                raise Exception(f"attribute {a} not in lhs header")
        elif a not in rhs.header:
            if not is_default(a, lhs.header[a]):
                raise Exception(f"attribute {a} not in rhs header")
        else:
            compare_attributes(a, lhs.header[a], rhs.header[a])

    if len(lhs.channels) != len(rhs.channels):
        raise Exception(f"#channels in {lhs.name} differs: {len(lhs.channels)} {len(rhs.channels)}")

    for c in lhs.channels.keys():
        compare_channels(lhs.channels[c], rhs.channels[c])

def compare_attributes(name, lhs, rhs):

    # convert tuples to array for comparison
    
    if isinstance(lhs, tuple):
        lhs = np.array(lhs)
        
    if isinstance(rhs, tuple):
        rhs = np.array(rhs)
        
    if isinstance(lhs, np.ndarray) and isinstance(rhs, np.ndarray):
        if lhs.shape != rhs.shape:
            raise Exception(f"attribute {name}: array shapes differ: {lhs} {rhs}")
        close = np.isclose(lhs, rhs, 1e-5, equal_nan=True)
        if not np.all(close):
            raise Exception(f"attribute {name}: arrays differ: {lhs} {rhs}")
    elif isinstance(lhs, float) and isinstance(rhs, float):
        if not equalWithRelError(lhs, rhs, 1e05):
            if math.isfinite(lhs) and math.isfinite(rhs):
                raise Exception(f"attribute {name}: floats differ: {lhs} {rhs}")
    elif lhs != rhs:
        raise Exception(f"attribute {name}: values differ: {lhs} {rhs}")

def compare_channels(lhs, rhs):

    if (lhs.name != rhs.name or
        lhs.type() != rhs.type() or
        lhs.xSampling != rhs.xSampling or
        lhs.ySampling != rhs.ySampling):
        raise Exception(f"channel {lhs.name} differs: {lhs.__repr__()} {rhs.__repr__()}")
    if lhs.pixels.shape != rhs.pixels.shape:
        raise Exception(f"channel {lhs.name}: image size differs: {lhs.pixels.shape} vs. {rhs.pixels.shape}")
        
    if lhs.pixels.dtype == np.dtype('O') and rhs.pixels.dtype == np.dtype('O'):
        height, width = lhs.pixels.shape
        for y in range(height):
            for x in range(width):
                ld = lhs.pixels[y,x]
                rd = rhs.pixels[y,x]
                if ld is None and rd is None:
                    continue
                if ld.shape != rd.shape:
                    raise Exception(f"channel {lhs.name}: deep pixels {i} differ: {ld} {rd}")
                with np.errstate(invalid='ignore'):
                    close = np.isclose(ld, rd, 1e-5, equal_nan=True)
                    if not np.all(close):
                        for i in np.argwhere(close==False):
                            raise Exception(f"channel {lhs.name}: pixels[{y}][{x}] deep sample{i} differs: {ld} {rd}")
    else:
                
        with np.errstate(invalid='ignore'):
            close = np.isclose(lhs.pixels, rhs.pixels, 1e-5, equal_nan=True)

        if not np.all(close):
            for i in np.argwhere(close==False):
                y,x = i
                lp = lhs.pixels[y,x]
                rp = rhs.pixels[y,x]
                if math.isfinite(lp) and math.isfinite(rp):
                    raise Exception(f"channel {lhs.name}: pixels {i} differ: {lp} {rp}")

exr_files = [
    "TestImages/GammaChart.exr",
    "TestImages/SquaresSwirls.exr",
    "TestImages/GrayRampsDiagonal.exr",
    "TestImages/BrightRingsNanInf.exr",
    "TestImages/WideFloatRange.exr",
    "TestImages/GrayRampsHorizontal.exr",
    "TestImages/WideColorGamut.exr",
    "TestImages/BrightRings.exr",
    "TestImages/RgbRampsDiagonal.exr",
    "TestImages/AllHalfValues.exr",
    "Beachball/multipart.0007.exr",
    "Beachball/singlepart.0007.exr",
    "Beachball/singlepart.0006.exr",
    "Beachball/multipart.0006.exr",
    "Beachball/multipart.0004.exr",
    "Beachball/singlepart.0004.exr",
    "Beachball/singlepart.0005.exr",
    "Beachball/multipart.0005.exr",
    "Beachball/singlepart.0001.exr",
    "Beachball/multipart.0001.exr",
    "Beachball/singlepart.0002.exr",
    "Beachball/multipart.0002.exr",
    "Beachball/multipart.0003.exr",
    "Beachball/singlepart.0003.exr",
    "Beachball/multipart.0008.exr",
    "Beachball/singlepart.0008.exr",
    "DisplayWindow/t12.exr",
    "DisplayWindow/t06.exr",
    "DisplayWindow/t07.exr",
    "DisplayWindow/t13.exr",
    "DisplayWindow/t05.exr",
    "DisplayWindow/t11.exr",
    "DisplayWindow/t10.exr",
    "DisplayWindow/t04.exr",
    "DisplayWindow/t14.exr",
    "DisplayWindow/t15.exr",
    "DisplayWindow/t01.exr",
    "DisplayWindow/t03.exr",
    "DisplayWindow/t02.exr",
    "DisplayWindow/t16.exr",
    "DisplayWindow/t09.exr",
    "DisplayWindow/t08.exr",
    "Tiles/GoldenGate.exr",
    "Tiles/Spirals.exr",
    "Tiles/Ocean.exr",
    "v2/Stereo/composited.exr",
    "v2/Stereo/Trunks.exr",
    "v2/Stereo/Balls.exr",
    "v2/Stereo/Ground.exr",
    "v2/Stereo/Leaves.exr",
    "v2/LeftView/Trunks.exr",
    "v2/LeftView/Balls.exr",
    "v2/LeftView/Ground.exr",
    "v2/LeftView/Leaves.exr",
    "v2/LowResLeftView/composited.exr",
    "v2/LowResLeftView/Trunks.exr",
    "v2/LowResLeftView/Balls.exr",
    "v2/LowResLeftView/Ground.exr",
    "v2/LowResLeftView/Leaves.exr",
    "MultiResolution/Kapaa.exr",
    "MultiResolution/KernerEnvCube.exr",
    "MultiResolution/WavyLinesLatLong.exr",
    "MultiResolution/PeriodicPattern.exr",
    "MultiResolution/ColorCodedLevels.exr",
    "MultiResolution/MirrorPattern.exr",
    "MultiResolution/Bonita.exr",
    "MultiResolution/OrientationLatLong.exr",
    "MultiResolution/StageEnvLatLong.exr",
    "MultiResolution/WavyLinesCube.exr",
    "MultiResolution/StageEnvCube.exr",
    "MultiResolution/KernerEnvLatLong.exr",
    "MultiResolution/WavyLinesSphere.exr",
    "MultiResolution/OrientationCube.exr",
    "Chromaticities/Rec709_YC.exr",
    "Chromaticities/XYZ_YC.exr",
    "Chromaticities/XYZ.exr",
    "Chromaticities/Rec709.exr",
    "ScanLines/Desk.exr",
    "ScanLines/Blobbies.exr",
    "ScanLines/CandleGlass.exr",
    "ScanLines/PrismsLenses.exr",
    "ScanLines/Tree.exr",
    "ScanLines/Cannon.exr",
    "ScanLines/MtTamWest.exr",
    "ScanLines/StillLife.exr",
    "MultiView/Fog.exr",
    "MultiView/Adjuster.exr",
    "MultiView/Balls.exr",
    "MultiView/Impact.exr",
    "MultiView/LosPadres.exr",
]

#
# These don't work yet, so skip them.
# 

bug_files = [
    "LuminanceChroma/MtTamNorth.exr", # channel BY differs.
    "Chromaticities/Rec709_YC.exr",  # channel BY differs.
    "Chromaticities/XYZ_YC.exr",  # channel BY differs.
]

bug_files = []
exr_files = [
    "v2/Stereo/Trunks.exr",
]

class TestImages(unittest.TestCase):

    def download_file(self, url, output_file):

        try:
            result = run(['curl', '-o', output_file, url], stdout=PIPE, stderr=PIPE, universal_newlines=True)
            print(" ".join(result.args))
            if result.returncode != 0:
                print(result.stderr)
                return False
        except Exception as e:
            print(f"Download of {url} failed: {e}")
            return False
        return True

    def print_file(self, f, print_pixels = False):

        print(f"file {f.filename}")
        print(f"parts:")
        parts = f.parts
        for p in parts:
            print(f"  part: {p.name()} {p.type()} {p.compression()} height={p.height()} width={p.width()}")
            h = p.header
            for a in h:
                print(f"    header[{a}] {h[a]}")
            for n,c in p.channels.items():
                print(f"    channel[{c.name}] shape={c.pixels.shape} strides={c.pixels.strides} {c.pixels.dtype}")
                if print_pixels:
                    maxy, maxx = c.pixels.shape[0], c.pixels.shape[1]
                    maxy, maxx = 2, 800
                    for y in range(maxy):
                        for x in range(maxx):
                            print(f"{n}[{y},{x}]={c.pixels[y,x]}")
                        
    def print_channel_names(self, file):
        for p in file.parts:
            s = f"part[{p.part_index}] name='{p.name()}', channels: ["
            for c in p.channels:
                s += f" {c}"
            s += " ]"
            print(s)

    def do_test_tiled(self, url):

        #
        # Write the image as tiled, reread and confirm it's the same
        #

        if "://" not in url:
            filename = url
        else:
            filename = "test_file.exr"
            if not self.download_file(url, filename):
                return

        print(f"Reading {url} ...")
        f = OpenEXR.File(filename)

        # Set the type and tile description (default)
        for P in f.parts:
            if P.header["type"] in [OpenEXR.deepscanline,  OpenEXR.deeptiled]:
                return
            
            P.header["compression"] = OpenEXR.ZIP_COMPRESSION
            P.header["type"] = OpenEXR.tiledimage
            if "tiles" not in P.header:
                P.header["tiles"] = OpenEXR.TileDescription()
                for n,C in P.channels.items():
                    C.xSampling = 1
                    C.ySampling = 1

        f.write("tiled.exr")

        t = OpenEXR.File("tiled.exr")

        # Clear the chunkCount values before comparison, since they'll
        # differ on conversion from scanline to tiled.
        for P in f.parts and t.parts:
            if "chunkCount" in P.header: del P.header["chunkCount"]
        for P in t.parts:
            if "chunkCount" in P.header: del P.header["chunkCount"]

        print(f"Comparing original to tiled...")
        compare_files(f, t)

    def do_test_image(self, url):

        verbose = False
        verbose_pixels = False

        print(f"testing image {url}...")
        
        if "://" not in url:
            filename = url
        else:
            filename = "test_file.exr"
            if not self.download_file(url, filename):
                return

        # Read the file as separate channels, as usual...

        print(f"Reading {url} as separate channels...")
        with OpenEXR.File(filename) as separate_channels:
            if verbose:
                print("separate_channels:")
                self.print_file(separate_channels, verbose_pixels)
                self.print_channel_names(separate_channels)

            # Write it out

            print(f"Writing separate_channels.exr...")
            separate_channels.write("separate_channels.exr")

            # Read the file that was just written
            print(f"Reading {url} as separate channels...")
            with OpenEXR.File("separate_channels.exr") as separate_channels2:
                if verbose:
                    self.print_channel_names(separate_channels2)

                # Confirm that the file that was just written is identical to the original

                print(f"Comparing separate_channels to separate_channels2...")
                compare_files(separate_channels, separate_channels2)
                print(f"Comparing separate_channels to separate_channels2...done.")

                # Read the original file as RGBA channels

                print(f"Reading {url} as rgba channels...")
                with OpenEXR.File(filename, True) as rgba_channels:
                    if verbose:
                        self.print_channel_names(rgba_channels)

                    # Write it out

                    print(f"Writing rgba_channels.exr...")
                    rgba_channels.write("rgba_channels.exr")

                # Read the file that was just written (was RGBA in memory,
                # should have been written as usual)

                print(f"Reading rgba_channels as separate channels3...")
                with OpenEXR.File("rgba_channels.exr") as separate_channels3:
                    if verbose:
                        print("separate_channels3:")
                        self.print_file(separate_channels3, verbose_pixels)
                        self.print_channel_names(separate_channels3)

                    # Confirm that it, too, is the same as the original

                    print(f"Comparing separate_channels to separate_channels3...")
                    compare_files(separate_channels, separate_channels3)

        print("ok.")

    def test_images(self):

        #
        # Run the test only if the OPENEXR_TEST_IMAGE_REPO env var is set
        #

        REPO_VAR = "OPENEXR_TEST_IMAGE_REPO" 
        if REPO_VAR in os.environ:

            REPO = os.environ[REPO_VAR]

            for filename in exr_files:

                if filename in bug_files:
                    print(f"skipping bug file: {filename}")
                    continue

                url = f"{REPO}/{filename}"

                self.do_test_image(url)
#                self.do_test_tiled(url)

            print("OK")

        else:    

            print(f"{sys.argv[0]}: skipping images, no repo")

if __name__ == '__main__':


    if len(sys.argv) > 2:
        exr_files = sys.argv[2:]
        bug_files = []
        
    unittest.main()
    print("OK")


