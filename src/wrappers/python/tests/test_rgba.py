#!/usr/bin/env python3

#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.
#

from __future__ import print_function
import sys
import os
import tempfile
import atexit
import unittest
import numpy as np

import OpenEXR

class TestRGBA(unittest.TestCase):

    def do_rgb(self, array_dtype):

        # Construct an RGB channel

        height = 5
        width = 4
        nrgba = 3
        size = width * height * nrgba
        RGB = np.array([i for i in range(0,size)], dtype=array_dtype).reshape((height, width, nrgba))
        channels = { "RGB" : OpenEXR.Channel(RGB) }

        header = {}
        outfile = OpenEXR.File(header, channels)

        outfile.write("out.exr")

        # 
        # Read as separate channels
        #

        infile = OpenEXR.File("out.exr", separate_channels=True)

        R = infile.channels()["R"].pixels
        G = infile.channels()["G"].pixels
        B = infile.channels()["B"].pixels

        shape = R.shape
        width = shape[1]
        height = shape[0]
        for y in range(0,height):
            for x in range(0,width):
                r = R[y][x]
                g = G[y][x]
                b = B[y][x]
                self.assertEqual(r, RGB[y][x][0])
                self.assertEqual(g, RGB[y][x][1])
                self.assertEqual(b, RGB[y][x][2])

        #
        # Read as RGB channel
        #

        infile = OpenEXR.File("out.exr")

        inRGB = infile.channels()["RGB"].pixels
        shape = inRGB.shape
        width = shape[1]
        height = shape[0]
        self.assertEqual(shape[2], 3)

        self.assertTrue(np.array_equal(inRGB, RGB))

    def do_rgba(self, array_dtype):

        # Construct an RGB channel

        height = 6
        width = 5
        nrgba = 4
        size = width * height * nrgba
        RGBA = np.array([i for i in range(0,size)], dtype=array_dtype).reshape((height, width, nrgba))
        channels = { "RGBA" : OpenEXR.Channel(RGBA) }

        header = {}
        outfile = OpenEXR.File(header, channels)

        outfile.write("out.exr")

        # 
        # Read as separate channels
        #

        infile = OpenEXR.File("out.exr", separate_channels=True)

        R = infile.channels()["R"].pixels
        G = infile.channels()["G"].pixels
        B = infile.channels()["B"].pixels
        A = infile.channels()["A"].pixels

        shape = R.shape
        width = shape[1]
        height = shape[0]
        for y in range(0,height):
            for x in range(0,width):
                r = R[y][x]
                g = G[y][x]
                b = B[y][x]
                a = A[y][x]
                self.assertEqual(r, RGBA[y][x][0])
                self.assertEqual(g, RGBA[y][x][1])
                self.assertEqual(b, RGBA[y][x][2])
                self.assertEqual(a, RGBA[y][x][3])

        #
        # Read as RGBA channel
        #

        infile = OpenEXR.File("out.exr")

        inRGBA = infile.channels()["RGBA"].pixels
        shape = inRGBA.shape
        width = shape[1]
        height = shape[0]
        self.assertEqual(shape[2], 4)

        self.assertTrue(np.array_equal(inRGBA, RGBA))

    def do_rgba_prefix(self, array_dtype):

        # Construct an RGB channel

        height = 6
        width = 5
        nrgba = 4
        size = width * height
        RGBA = np.array([i for i in range(0,size*nrgba)], dtype=array_dtype).reshape((height, width, nrgba))
        Z = np.array([i for i in range(0,size)], dtype=array_dtype).reshape((height, width))
        channels = { "left" : OpenEXR.Channel(RGBA), "left.Z" : OpenEXR.Channel(Z) }

        header = {}
        outfile = OpenEXR.File(header, channels)

        print(f"write out.exr")
        outfile.write("out.exr")

        # 
        # Read as separate channels
        #

        print(f"read out.exr as single channels")
        infile = OpenEXR.File("out.exr", separate_channels=True)

        R = infile.channels()["left.R"].pixels
        G = infile.channels()["left.G"].pixels
        B = infile.channels()["left.B"].pixels
        A = infile.channels()["left.A"].pixels
        Z = infile.channels()["left.Z"].pixels

        shape = R.shape
        width = shape[1]
        height = shape[0]
        for y in range(0,height):
            for x in range(0,width):
                r = R[y][x]
                g = G[y][x]
                b = B[y][x]
                a = A[y][x]
                self.assertEqual(r, RGBA[y][x][0])
                self.assertEqual(g, RGBA[y][x][1])
                self.assertEqual(b, RGBA[y][x][2])
                self.assertEqual(a, RGBA[y][x][3])

        #
        # Read as RGBA channel
        #

        print(f"read out.exr as rgba channels")
        infile = OpenEXR.File("out.exr")

        inRGBA = infile.channels()["left"].pixels
        shape = inRGBA.shape
        width = shape[1]
        height = shape[0]
        self.assertEqual(shape[2], 4)
        inZ = infile.channels()["left.Z"].pixels

        self.assertTrue(np.array_equal(inRGBA, RGBA))

    def test_rgb_uint32(self):
        self.do_rgb('uint32')

    def test_rgb_f(self):
        self.do_rgb('f')

    def test_rgba_uint32(self):
        self.do_rgba('uint32')

    def test_rgba_prefix_uint32(self):
        self.do_rgba_prefix('uint32')

    def test_rgba_f(self):
        self.do_rgba('f')

if __name__ == '__main__':
    unittest.main()
    print("OK")


