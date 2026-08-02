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

def print_deep(outfile):
    for n,c in outfile.channels().items():
        for y in range(c.pixels.shape[0]):
            for x in range(c.pixels.shape[1]):
                d = c.pixels[y,x]
                print(f"{n}[{y},{x}]: {d}")

def compare_files(lhs, rhs):

    for Plhs, Prhs in zip(lhs.parts,rhs.parts):
        compare_parts(Plhs, Prhs)

def compare_parts(lhs, rhs):

    if len(lhs.channels) != len(rhs.channels):
        raise Exception(f"#channels in {lhs.name()} differs: {len(lhs.channels)} {len(rhs.channels)}")

    for c in lhs.channels.keys():
        compare_channels(lhs.channels[c], rhs.channels[c])

def compare_channels(lhs, rhs):

    if (lhs.name != rhs.name or
        lhs.type() != rhs.type() or
        lhs.xSampling != rhs.xSampling or
        lhs.ySampling != rhs.ySampling):
        raise Exception(f"channel {lhs.name} differs: {lhs.__repr__()} {rhs.__repr__()}")

    compare_channel_pixels(lhs, rhs)
    
def compare_channel_pixels(lhs, rhs):

    if lhs.pixels.shape != rhs.pixels.shape:
        raise Exception(f"channel {lhs.name}: image size differs: {lhs.pixels.shape} vs. {rhs.pixels.shape}")
        
    height = lhs.pixels.shape[0]
    width = lhs.pixels.shape[1]
    for y in range(height):
        for x in range(width):
            l = lhs.pixels[y,x]
            r = rhs.pixels[y,x]
            if l is None and r is None:
                continue
            close = np.isclose(l, r, 1e-5)
            if not np.all(close):
                for i in np.argwhere(close==False):
                    y,x = i
                    if math.isfinite(lhs.pixels[y,x]) and math.isfinite(rhs.pixels[y,x]):
                        raise Exception(f"channel {lhs.name}: deep pixels {i} differ: {lhs.pixels[y,x]} {rhs.pixels[y,x]}")

class TestDeep(unittest.TestCase):

    def test_deep_rgba(self):

        self.do_test_deep_rgba(False, 'uint32')
        self.do_test_deep_rgba(True, 'uint32')
        self.do_test_deep_rgba(False, 'e')
        self.do_test_deep_rgba(True, 'e')
        self.do_test_deep_rgba(False, 'float32')
        self.do_test_deep_rgba(True, 'float32')
        
    def do_test_deep_rgba(self, do_alpha, dt):

        dataWindow = ((100,100), (120,130))
        height = dataWindow[1][1] - dataWindow[0][1] + 1
        width = dataWindow[1][0] - dataWindow[0][0] + 1
        
        R = np.empty((height, width), dtype=object)
        G = np.empty((height, width), dtype=object)
        B = np.empty((height, width), dtype=object)
        if do_alpha:
            A = np.empty((height, width), dtype=object)
        Z = np.empty((height, width), dtype=object)

        for y in range(height):
            for x in range(width):
                i = y*width+x + 2
                l = i % 2 
                if l == 0:
                    R[y, x] = np.array([j for j in range(1,i)], dtype=dt)
                    G[y, x] = np.array([j*10 for j in range(1,i)], dtype=dt)
                    B[y, x] = np.array([j*100 for j in range(1,i)], dtype=dt)
                    if do_alpha:
                        A[y, x] = np.array([j*100 for j in range(1,i)], dtype=dt)
                    Z[y, x] = np.array([j*2 for j in range(1,i)], dtype=dt)
                else:
                    R[y, x] = None
                    G[y, x] = None
                    B[y, x] = None
                    if do_alpha:
                        A[y, x] = None
                    Z[y, x] = None
        
        channels = { "B" : B, "G" : G, "R" : R, "Z" : Z }
        if do_alpha:
            channels["A"] = A
        header = { "compression" : OpenEXR.ZIPS_COMPRESSION,
                   "type" : OpenEXR.deepscanline,
                   "dataWindow" : dataWindow}

        filename = "write_deep.exr"
        with OpenEXR.File(header, channels) as outfile:

            outfile.write(filename)

            with OpenEXR.File(filename) as infile:

                channel = "RGBA" if do_alpha else "RGB"
                
                C = infile.channels()[channel]
                for y in range(height):
                    for x in range(width):
                        p = C.pixels[y,x]
                        if p is None:
                            assert R[y,x] is None
                        else:
                            for i in range(p.shape[0]):
                                if do_alpha:
                                    self.assertEqual(p[i,:].tolist(), [R[y,x][i], G[y,x][i], B[y,x][i], A[y,x][i]])
                                else:
                                    self.assertEqual(p[i,:].tolist(), [R[y,x][i], G[y,x][i], B[y,x][i]])
                
                infile.write("rgb_deep.exr")

            with OpenEXR.File("rgb_deep.exr", separate_channels=True) as infile:

                compare_files(infile, outfile)

        os.remove(filename)
        os.remove("rgb_deep.exr")
                
    def test_deep(self):

        dataWindow = ((100,100), (120,130))
        height = dataWindow[1][1] - dataWindow[0][1] + 1
        width = dataWindow[1][0] - dataWindow[0][0] + 1
        
        U = np.empty((height, width), dtype=object)
        H = np.empty((height, width), dtype=object)
        F = np.empty((height, width), dtype=object)
        for y in range(height):
            for x in range(width):
                i = y*width+x
                l = i % 3 
                if l == 0:
                    U[y, x] = np.array([i], dtype='uint32')
                    H[y, x] = np.array([i], dtype='float16')
                    F[y, x] = np.array([i], dtype='float32')
                else:
                    U[y, x] = None
                    H[y, x] = None
                    F[y, x] = None
        
        channels = { "U" : U, "H" : H, "F" : F }
        header = { "compression" : OpenEXR.ZIPS_COMPRESSION,
                   "type" : OpenEXR.deepscanline,
                   "dataWindow" : dataWindow}

        filename = "write_deep.exr"
        with OpenEXR.File(header, channels) as outfile:

            outfile.write(filename)

            with OpenEXR.File(filename) as infile:

                compare_files(infile, outfile)

        os.remove(filename)

    def test_tiled_deep(self):

        dataWindow = ((100,100), (120,130))
        dataWindow = ((100,100), (103,104))
        height = dataWindow[1][1] - dataWindow[0][1] + 1
        width = dataWindow[1][0] - dataWindow[0][0] + 1
        
        U = np.empty((height, width), dtype=object)
        H = np.empty((height, width), dtype=object)
        F = np.empty((height, width), dtype=object)
        for y in range(height):
            for x in range(width):
                i = y*width+x + 2
                l = i % 3 
                if l == 0:
                    U[y, x] = np.array([j for j in range(1,i)], dtype='uint32')
                    H[y, x] = np.array([j*10 for j in range(1,i)], dtype='float16')
                    F[y, x] = np.array([j*100 for j in range(1,i)], dtype='float32')
                else:
                    U[y, x] = None
                    H[y, x] = None
                    F[y, x] = None
        
        channels = { "U" : U, "H" : H, "F" : F }
        header = { "compression" : OpenEXR.ZIPS_COMPRESSION,
                   "type" : OpenEXR.deeptile,
                   "tiles" : OpenEXR.TileDescription(),
                   "dataWindow" : dataWindow}

        filename = "test_tiled_deep.exr"
        with OpenEXR.File(header, channels) as outfile:

            outfile.write(filename)

            with OpenEXR.File(filename, False) as infile:

                compare_files(infile, outfile)

        os.remove(filename)

    def test_mixed_type_rgb_coalesce_deep_rejected(self):
        dataWindow = ((0, 0), (0, 0))
        height = width = 1

        B = np.empty((height, width), dtype=object)
        G = np.empty((height, width), dtype=object)
        R = np.empty((height, width), dtype=object)
        B[0, 0] = np.array([1.0, 2.0], dtype='float16')
        G[0, 0] = np.array([1.0, 2.0], dtype='float32')
        R[0, 0] = np.array([1.0, 2.0], dtype='float32')

        channels = {"B": B, "G": G, "R": R}
        header = {
            "compression": OpenEXR.ZIPS_COMPRESSION,
            "type": OpenEXR.deepscanline,
            "dataWindow": dataWindow,
        }

        fd, path = tempfile.mkstemp(suffix=".exr")
        os.close(fd)
        try:
            with OpenEXR.File(header, channels) as outfile:
                outfile.write(path)

            with self.assertRaises(Exception) as ctx:
                OpenEXR.File(path)
            self.assertIn("separate_channels", str(ctx.exception))

            with OpenEXR.File(path, separate_channels=True) as infile:
                self.assertIn("B", infile.channels())
                self.assertIn("G", infile.channels())
                self.assertIn("R", infile.channels())
        finally:
            if os.path.exists(path):
                os.remove(path)

    def test_mixed_type_rgb_coalesce_flat_rejected(self):
        dataWindow = ((0, 0), (0, 0))

        channels = {
            "B": np.array([[1.0]], dtype='float16'),
            "G": np.array([[2.0]], dtype='float32'),
            "R": np.array([[3.0]], dtype='float32'),
        }
        header = {
            "type": OpenEXR.scanlineimage,
            "dataWindow": dataWindow,
        }

        fd, path = tempfile.mkstemp(suffix=".exr")
        os.close(fd)
        try:
            with OpenEXR.File(header, channels) as outfile:
                outfile.write(path)

            with self.assertRaises(Exception) as ctx:
                OpenEXR.File(path)
            self.assertIn("separate_channels", str(ctx.exception))

            with OpenEXR.File(path, separate_channels=True) as infile:
                self.assertIn("B", infile.channels())
                self.assertIn("G", infile.channels())
                self.assertIn("R", infile.channels())
        finally:
            if os.path.exists(path):
                os.remove(path)

    def test_literal_rgb_key_collision_deep_rejected(self):
        dataWindow = ((0, 0), (0, 0))
        height = width = 1

        left = np.empty((height, width), dtype=object)
        left_R = np.empty((height, width), dtype=object)
        left_G = np.empty((height, width), dtype=object)
        left_B = np.empty((height, width), dtype=object)
        left[0, 0] = np.array([10.0, 11.0, 12.0, 13.0], dtype='float16')
        left_R[0, 0] = np.array([10.0, 11.0, 12.0, 13.0], dtype='float16')
        left_G[0, 0] = np.array([20.0, 21.0, 22.0, 23.0], dtype='float16')
        left_B[0, 0] = np.array([30.0, 31.0, 32.0, 33.0], dtype='float16')

        channels = {
            "left": left,
            "left.R": left_R,
            "left.G": left_G,
            "left.B": left_B,
        }
        header = {
            "compression": OpenEXR.ZIPS_COMPRESSION,
            "type": OpenEXR.deepscanline,
            "dataWindow": dataWindow,
        }

        fd, path = tempfile.mkstemp(suffix=".exr")
        os.close(fd)
        try:
            with OpenEXR.File(header, channels) as outfile:
                outfile.write(path)

            with self.assertRaises(ValueError) as ctx:
                OpenEXR.File(path)
            self.assertIn("separate_channels", str(ctx.exception))

            with OpenEXR.File(path, separate_channels=True) as infile:
                self.assertIn("left", infile.channels())
                self.assertIn("left.R", infile.channels())
                self.assertIn("left.G", infile.channels())
                self.assertIn("left.B", infile.channels())
        finally:
            if os.path.exists(path):
                os.remove(path)

if __name__ == '__main__':
    unittest.main()
    print("OK")


