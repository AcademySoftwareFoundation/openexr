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
from subprocess import PIPE, run

import OpenEXR


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
    "v2/Stereo/Trunks.exr", # deepscanlne, RuntimeError: Invalid base pointer, please set a proper sample count slice.
    "v2/Stereo/Balls.exr", # RuntimeError: Invalid base pointer, please set a proper sample count slice.
    "v2/Stereo/Ground.exr", # RuntimeError: Invalid base pointer, please set a proper sample count slice.
    "v2/Stereo/Leaves.exr", # RuntimeError: Invalid base pointer, please set a proper sample count slice.
    "v2/LeftView/Trunks.exr",
    "v2/LeftView/Balls.exr",
    "v2/LeftView/Ground.exr",
    "v2/LeftView/Leaves.exr",
    "v2/LowResLeftView/Trunks.exr",
    "v2/LowResLeftView/Balls.exr",
    "v2/LowResLeftView/Ground.exr",
    "v2/LowResLeftView/Leaves.exr",
    "Chromaticities/Rec709_YC.exr",  # channel BY differs.
    "Chromaticities/XYZ_YC.exr",  # channel BY differs.
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

        filename = "test_file.exr"
        if not self.download_file(url, filename):
            return

        print(f"Reading {url} ...")
        f = OpenEXR.File(filename)

        # Set the type and tile description (default)
        for P in f.parts:
            P.header["compression"] = OpenEXR.NO_COMPRESSION
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
        self.assertEqual(f, t)

    def do_test_image(self, url):

        verbose = False

        filename = "test_file.exr"
        if not self.download_file(url, filename):
            return

        # Read the file as separate channels, as usual...

        print(f"Reading {url} as separate channels...")
        separate_channels = OpenEXR.File(filename)
        if verbose:
            print_channel_names(separate_channels)

        # Write it out

        print(f"Writing separate_channels.exr...")
        for P in separate_channels.parts:
            P.header["compression"] = OpenEXR.ZIP_COMPRESSION
        separate_channels.write("separate_channels.exr")

        # Read the file that was just written
        print(f"Reading {url} as separate channels...")
        separate_channels2 = OpenEXR.File("separate_channels.exr")
        if verbose:
            print_channel_names(separate_channels2)

        # Confirm that the file that was just written is identical to the original

        # Clear the chunkCount values before comparison, since they might differ
        for P in separate_channels.parts:
            if "chunkCount" in P.header: del P.header["chunkCount"]
        for P in separate_channels2.parts:
            if "chunkCount" in P.header: del P.header["chunkCount"]

        print(f"Comparing separate_channels to separate_channels2...")
        self.assertEqual(separate_channels, separate_channels2)

        # Read the original file as RGBA channels

        print(f"Reading {url} as rgba channels...")
        rgba_channels = OpenEXR.File(filename, True)
        for P in rgba_channels.parts:
            P.header["compression"] = OpenEXR.ZIP_COMPRESSION
        if verbose:
            print_channel_names(rgba_channels)

        # Write it out

        print(f"Writing rgba_channels.exr...")
        rgba_channels.write("rgba_channels.exr")

        # Read the file that was just written (was RGBA in memory,
        # should have been written as usual)

        print(f"Reading rgba_channels as separate channels...")
        separate_channels2  = OpenEXR.File("rgba_channels.exr")
        if verbose:
            print_channel_names(separate_channels2)

        # Confirm that it, too, is the same as the original

        print(f"Comparing separate_channels to separate_channels2...")
        self.assertEqual(separate_channels, separate_channels2)
        print("good.")

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
                self.do_test_tiled(url)

            print("OK")

        else:    

            print(f"{sys.argv[0]}: skipping images, no repo")

if __name__ == '__main__':
    unittest.main()
    print("OK")


