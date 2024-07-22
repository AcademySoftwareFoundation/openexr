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

if __name__ == '__main__':
    unittest.main()
    print("OK")


