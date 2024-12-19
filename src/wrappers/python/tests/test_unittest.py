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
import fractions

import OpenEXR

test_dir = os.path.dirname(__file__)

outfilenames = []
def mktemp_outfilename():
    fd, outfilename = tempfile.mkstemp(".exr")
    os.close(fd)
    global outfilenames
    outfilenames += outfilename
    return outfilename

def cleanup():
    for outfilename in outfilenames:
        if os.path.isfile(outfilename):
            print(f"deleting {outfilename}")
            os.unlink(outfilename)
atexit.register(cleanup)


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
        if a in ["channels"]:
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
        close = np.isclose(lhs, rhs, 1e-5)
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
        
    close = np.isclose(lhs.pixels, rhs.pixels, 1e-5)
    if not np.all(close):
        for i in np.argwhere(close==False):
            y,x = i
            if math.isfinite(lhs.pixels[y,x]) and math.isfinite(rhs.pixels[y,x]):
                raise Exception(f"channel {lhs.name}: pixels {i} differ: {lhs.pixels[y,x]} {rhs.pixels[y,x]}")


def print_file(f, print_pixels = False):

    print(f"file {f.filename}")
    print(f"parts:")
    parts = f.parts
    for p in parts:
        print(f"  part: {p.name()} {p.type()} {p.compression()} height={p.height()} width={p.width()}")
        h = p.header
        for a in h:
            print(f"    header[{a}] {h[a]}")
        for n,c in p.channels.items():
            print(f"    channel[{c.name}] shape={c.pixels.shape} strides={c.pixels.strides} {c.type()} {c.pixels.dtype}")
            if print_pixels:
                for y in range(0,c.pixels.shape[0]):
                    s = f"      {c.name}[{y}]:"
                    for x in range(0,c.pixels.shape[1]):
                        s += f" {c.pixels[y][x]}"
                    print(s)

def preview_pixels_equal(a, b):

    if a.shape != b.shape:
        return False

    for y in range(0,a.shape[0]):
        for x in range(0,a.shape[1]):
            if len(a[y][x]) != len(b[y][x]):
                for i in range(0,len(a[y][x])):
                    if a[y][x][i] != b[y][x][i]:
                        return False

    return True

class TestUnittest(unittest.TestCase):

    def setUp(self):
        # Print the name of the current test method
        print(f"Running test {self.id().split('.')[-1]}")

    def test_tuple(self):

        width = 5
        height = 10
        size = width * height
        Z = np.array([i for i in range(0,size)], dtype='uint32').reshape((height, width))
        channels = { "Z" : Z }

        header = {}
        header["t2i"] = (0,1)
        header["t2f"] = (2.3,4.5)
        header["t3i"] = (0,1,2)
        header["t3f"] = (3.4,5.6,7.8)

        header["a2f"] = np.array((1.2, 3.4), 'float32')
        header["a2d"] = np.array((1.2, 3.4), 'float64')
        header["a3f"] = np.array((1.2, 3.4, 5.6), 'float32')
        header["a3d"] = np.array((1.2, 3.4, 5.6), 'float64')

        header["a33f"] = np.identity(3, 'float32') 
        header["a33d"] = np.identity(3, 'float64') 
        header["a44f"] = np.identity(4, 'float32') 
        header["a44d"] = np.identity(4, 'float64') 

        outfilename = mktemp_outfilename()
        with OpenEXR.File(header, channels) as outfile:

            outfile.write(outfilename)

            with OpenEXR.File(outfilename) as infile:
                compare_files (infile, outfile)
                
        with self.assertRaises(Exception):
            header["v"] = (0,"x")
            with OpenEXR.File(header, channels) as outfile:
                outfile.write(outfilename)

        # tuple must be either all int or all float
        with self.assertRaises(Exception):
            header["v"] = (1,2.3)
            with OpenEXR.File(header, channels) as outfile:
                outfile.write(outfilename)

    def test_read_write(self):

        #
        # Read a file and write it back out, then read the freshly-written
        # file to validate it's the same.
        #

        infilename = f"{test_dir}/test.exr"
        with OpenEXR.File(infilename) as infile:

            outfilename = mktemp_outfilename()
            infile.write(outfilename)

            with OpenEXR.File(outfilename) as outfile:
                compare_files(outfile, infile)

    def test_keycode(self):

        filmMfcCode = 1
        filmType = 2
        prefix = 3
        count = 4
        perfOffset = 5
        perfsPerFrame = 6
        perfsPerCount = 20

        k = OpenEXR.KeyCode(filmMfcCode, filmType, prefix, count, perfOffset, perfsPerFrame, perfsPerCount)

        assert (k.filmMfcCode == filmMfcCode and
                k.filmType == filmType and
                k.prefix == prefix and
                k.count == count and
                k.perfOffset == perfOffset and
                k.perfsPerFrame == perfsPerFrame and
                k.perfsPerCount == perfsPerCount)

    def test_empty_header(self):

        # Construct a file from scratch and write it.

        width = 10
        height = 20
        size = width * height
        Z = np.array([i for i in range(0,size)], dtype='uint32').reshape((height, width))
        channels = { "Z" : OpenEXR.Channel(Z, 1, 1) }

        header = {}

        with OpenEXR.File(header, channels) as outfile:

            outfilename = mktemp_outfilename()
            outfile.write(outfilename)

            with OpenEXR.File(outfilename) as infile:
                compare_files (infile, outfile)
                
    def test_write_uint(self):

        # Construct a file from scratch and write it.

        width = 5
        height = 10
        size = width * height
        R = np.array([i for i in range(0,size)], dtype='uint32').reshape((height, width))
        G = np.array([i*2 for i in range(0,size)], dtype='uint32').reshape((height, width))
        B = np.array([i*3 for i in range(0,size)], dtype='uint32').reshape((height, width))
        A = np.array([i*5 for i in range(0,size)], dtype='uint32').reshape((height, width))
        channels = {
            "R" : OpenEXR.Channel(R, 1, 1),
            "G" : OpenEXR.Channel(G, 1, 1),
            "B" : OpenEXR.Channel(B, 1, 1),
            "A" : OpenEXR.Channel(A, 1, 1), 
        }

        header = {}

        with OpenEXR.File(header, channels) as outfile:

            # confirm that the write assigned names to the channels
            self.assertEqual(outfile.channels()['A'].name, "A")

            outfilename = mktemp_outfilename()
            outfile.write(outfilename)

            # Verify reading it back gives the same data
            with OpenEXR.File(outfilename, separate_channels=True) as infile:

                compare_files(infile, outfile)

    def test_write_half(self):

        # Construct a file from scratch and write it.

        width = 10
        height = 20
        size = width * height
        R = np.array([i for i in range(0,size)], dtype='e').reshape((height, width))
        G = np.array([i*10 for i in range(0,size)], dtype='e').reshape((height, width))
        B = np.array([i*100 for i in range(0,size)], dtype='e').reshape((height, width))
        A = np.array([i/size for i in range(0,size)], dtype='e').reshape((height, width))
        channels = {
            "A" : OpenEXR.Channel("A", A, 1, 1), 
            "B" : OpenEXR.Channel("B", B, 1, 1),
            "G" : OpenEXR.Channel("G", G, 1, 1),
            "R" : OpenEXR.Channel("R", R, 1, 1)
        }

        header = {}

        with OpenEXR.File(header, channels) as outfile:

            outfilename = mktemp_outfilename()
            outfile.write(outfilename)

            # Verify reading it back gives the same data
            with OpenEXR.File(outfilename, separate_channels=True) as infile:
                compare_files (infile, outfile)

    def test_write_tiles(self):

        # Construct a file from scratch and write it.

        width = 10
        height = 20
        size = width * height
        R = np.array([i for i in range(0,size)], dtype='e').reshape((height, width))
        G = np.array([i*10 for i in range(0,size)], dtype='e').reshape((height, width))
        B = np.array([i*100 for i in range(0,size)], dtype='e').reshape((height, width))
        A = np.array([i/size for i in range(0,size)], dtype='e').reshape((height, width))
        channels = {
            "A" : OpenEXR.Channel("A", A, 1, 1), 
            "B" : OpenEXR.Channel("B", B, 1, 1),
            "G" : OpenEXR.Channel("G", G, 1, 1),
            "R" : OpenEXR.Channel("R", R, 1, 1)
        }

        header = { "type" : OpenEXR.tiledimage,
                   "tiles" : OpenEXR.TileDescription() }

        with OpenEXR.File(header, channels) as outfile:
            outfilename = mktemp_outfilename()
            outfile.write(outfilename)

            # Verify reading it back gives the same data
            with OpenEXR.File(outfilename, separate_channels=True) as infile:
                compare_files(infile, outfile)

    def test_modify_in_place(self):

        #
        # Test modifying header attributes in place
        #

        infilename = f"{test_dir}/test.exr"
        with OpenEXR.File(infilename, separate_channels=True) as f:

            # set the value of an existing attribute
            par = 2.3
            f.parts[0].header["pixelAspectRatio"] = par

            # add a new attribute
            f.parts[0].header["foo"] = "bar"

            dt = np.dtype({
                "names": ["r", "g", "b", "a"],
                "formats": ["u4", "u4", "u4", "u4"],
                "offsets": [0, 4, 8, 12],
            })
            pwidth = 3
            pheight = 3
            psize = pwidth * pheight
            P = np.array([ [(0,0,0,0), (1,1,1,1), (2,2,2,2) ],
                           [(3,3,3,3), (4,4,4,4), (5,5,5,5) ],
                           [(6,6,6,6), (7,7,7,7), (8,8,8,8) ] ], dtype=dt).reshape((pwidth,pheight))
            f.parts[0].header["preview"] = OpenEXR.PreviewImage(P)

            # Modify a pixel value
            f.parts[0].channels["R"].pixels[0][1] = 42.0
            f.channels()["G"].pixels[2][3] = 666.0

            # write to a new file
            outfilename = mktemp_outfilename()
            f.write(outfilename)

        # read the new file
        with OpenEXR.File(outfilename, separate_channels=True) as m:

            # validate the values are the same
            eps = 1e-5
            mpar = m.parts[0].header["pixelAspectRatio"]
            assert equalWithRelError(m.parts[0].header["pixelAspectRatio"], par, eps)
            assert m.parts[0].header["foo"] == "bar"

            assert preview_pixels_equal(m.parts[0].header["preview"].pixels, P)

            assert equalWithRelError(m.parts[0].channels["R"].pixels[0][1], 42.0, eps)
            assert equalWithRelError(m.parts[0].channels["G"].pixels[2][3], 666.0, eps)

    def test_preview_image(self):

        width = 5
        height = 10
        size = width * height
        Z = np.array([i*5 for i in range(0,size)], dtype='uint32').reshape((height, width))
        channels = { "Z" : OpenEXR.Channel("Z", Z, 1, 1) }

        dt = np.dtype({
            "names": ["r", "g", "b", "a"],
            "formats": ["u4", "u4", "u4", "u4"],
            "offsets": [0, 4, 8, 12],
        })
        pwidth = 3
        pheight = 3
        psize = pwidth * pheight
        P = np.array([(i,i,i,i) for i in range(0,psize)], dtype=dt).reshape((pwidth,pheight))

        header = {}
        header["preview"] = OpenEXR.PreviewImage(P)

        with OpenEXR.File(header, channels) as outfile:

            outfilename = mktemp_outfilename()
            outfile.write(outfilename)

            with OpenEXR.File(outfilename) as infile:

                Q = infile.header()["preview"].pixels

                assert preview_pixels_equal(P, Q)

                compare_files (infile, outfile)

    def test_write_float(self):

        # Construct a file from scratch and write it.

        width = 50
        height = 1
        size = width * height
        R = np.array([i for i in range(0,size)], dtype='f').reshape((height, width))
        G = np.array([i*10 for i in range(0,size)], dtype='f').reshape((height, width))
        B = np.array([i*100 for i in range(0,size)], dtype='f').reshape((height, width))
        A = np.array([i*1000 for i in range(0,size)], dtype='f').reshape((height, width))
        channels = {
            "R" : OpenEXR.Channel("R", R, 1, 1),
            "G" : OpenEXR.Channel("G", G, 1, 1),
            "B" : OpenEXR.Channel("B", B, 1, 1),
            "A" : OpenEXR.Channel("A", A, 1, 1)
        }

        header = {}
        header["floatvector"] = [1.0, 2.0, 3.0]
        header["stringvector"] = ["do", "re", "me"]
        header["chromaticities"] = (1.0,2.0, 3.0,4.0, 5.0,6.0,7.0,8.0)
        header["box2i"] = ((0,1), (2,3))
        header["box2f"] = ((0.0,1.0), (2.0,3.0))
        header["compression"] = OpenEXR.ZIPS_COMPRESSION
        header["double"] = np.array([42000.0], 'float64')
        header["float"] = 4.2
        header["int"] = 42
        header["keycode"] = OpenEXR.KeyCode(0,0,0,0,0,4,64)
        header["lineorder"] = OpenEXR.INCREASING_Y
        header["m33f"] = np.identity(3, 'float32') 
        header["m33d"] = np.identity(3, 'float64') 
        header["m44f"] = np.identity(4, 'float32') 
        header["m44d"] = np.identity(4, 'float64') 
        header["rational"] = fractions.Fraction(1,3)
        header["string"] = "stringy"
        header["timecode"] = OpenEXR.TimeCode(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18)
        header["v2i"] = (1,2)
        header["v2f"] = (1.2,3.4)
        header["v2d"] = (1.2,3.4)
        header["v3i"] = (1,2,3)
        header["v3f"] = (1.2,3.4,5.6)
        header["v3d"] = (1.2,3.4,5.6)

        with OpenEXR.File(header, channels) as outfile:

            outfilename = mktemp_outfilename()
            outfile.write(outfilename)
            print("write done.")
                  
            # Verify reading it back gives the same data
            with OpenEXR.File(outfilename, separate_channels=True) as infile:
                compare_files (infile, outfile)
                
    def test_write_2part(self):

        #
        # Construct a 2-part file by replicating the header and channels
        #

        width = 10
        height = 20
        size = width * height
        R = np.array([i for i in range(0,size)], dtype='f').reshape((height, width))
        G = np.array([i*10 for i in range(0,size)], dtype='f').reshape((height, width))
        B = np.array([i*100 for i in range(0,size)], dtype='f').reshape((height, width))
        A = np.array([i*1000 for i in range(0,size)], dtype='f').reshape((height, width))
        channels = {
            "R" : OpenEXR.Channel("R", R, 1, 1),
            "G" : OpenEXR.Channel("G", G, 1, 1),
            "B" : OpenEXR.Channel("B", B, 1, 1),
            "A" : OpenEXR.Channel("A", A, 1, 1)
        }

        pwidth = 3
        pheight = 3
        psize = pwidth * pheight

        dt = np.dtype({
            "names": ["r", "g", "b", "a"],
            "formats": ["u4", "u4", "u4", "u4"],
            "offsets": [0, 4, 8, 12],
        })
        P = np.array([(i,i,i,i) for i in range(0,psize)], dtype=dt).reshape((pwidth,pheight))

        def make_header():
            header = {}
            header["floatvector"] = [1.0, 2.0, 3.0]
            return header
            header["stringvector"] = ["do", "re", "me"]
            header["chromaticities"] = (1.0,2.0, 3.0,4.0, 5.0,6.0,7.0,8.0)
            header["box2i"] = ((0,1),(2,3))
            header["box2f"] = ((0.0,1.0),(2.0,3.0))
            header["compression"] = OpenEXR.ZIPS_COMPRESSION
            header["double"] = np.array([42000.0], 'float64')
            header["float"] = 4.2
            header["int"] = 42
            header["keycode"] = OpenEXR.KeyCode(0,0,0,0,0,4,64)
            header["lineorder"] = OpenEXR.INCREASING_Y
            header["m33f"] = np.identity(3, 'float32')
            header["m33d"] = np.identity(3, 'float64')
            header["m44f"] = np.identity(4, 'float32')
            header["m44d"] = np.identity(4, 'float64')
            header["preview"] = OpenEXR.PreviewImage(P)
            header["rational"] = fractions.Fraction(1,3)
            header["string"] = "stringy"
            header["timecode"] = OpenEXR.TimeCode(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18)
            header["v2i"] = (1,2)
            header["v2f"] = (1.2,3.4)
            header["v2d"] = (1.2,3.4)
            header["v3i"] = (1,2,3)
            header["v3f"] = (1.2,3.4,5.6)
            header["v3d"] = (1.2,3.4,5.6)
            return header

        header1 = make_header()
        header2 = make_header()

        P1 = OpenEXR.Part(header1, channels, "Part1")
        P2 = OpenEXR.Part(header2, channels, "Part2")

        parts = [P1, P2]
        with OpenEXR.File(parts) as outfile2:

            outfilename = mktemp_outfilename()
            outfile2.write(outfilename)

            # Verify reading it back gives the same data
            with OpenEXR.File(outfilename, separate_channels=True) as i:
                compare_files (i, outfile2)

if __name__ == '__main__':
    unittest.main()
