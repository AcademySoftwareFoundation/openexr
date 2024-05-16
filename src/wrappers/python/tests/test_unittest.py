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

def required_attribute(name):
    return (name == "channels" or
            name == "compression" or
            name == "dataWindow" or
            name == "displayWindow" or
            name == "lineOrder" or 
            name == "pixelAspectRatio" or
            name == "screenWindowCenter" or
            name == "screenWindowWidth" or
            name == "tiles" or
            name == "type" or
            name == "name" or
            name == "version" or
            name == "chunkCount")

def compare_files(A, B):

    if len(A.parts) != len(B.parts):
        print(f"#parts differs: {len(A.parts)} {len(B.parts)}")
        return False

    for PA, PB in zip(A.parts,B.parts):
        if compare_parts(PA, PB):
            return False

    return True

def compare_parts(A, B):

    akeys = set(A.header.keys())
    bkeys = set(B.header.keys())

    for k in akeys-bkeys:
        if not required_attribute(k):
            print("Attribute {k} is not in both headers")
            return False

    for k in bkeys-akeys:
        if not required_attribute(k):
            print("Attribute {k} is not in both headers")
            return False

    for k in akeys.intersection(bkeys):
        if k == "preview" or k == "float":
            continue
        if A.header[k] != B.header[k]:
            print(f"attribute {k} {type(A.header[k])} differs: {A.header[k]} {B.header[k]}")
            return False

    if len(A.channels) != len(B.channels):
        print(f"#channels in {A.name} differs: {len(A.channels)} {len(B.channels)}")
        return False

    for c in A.channels.keys():
        if compare_channels(A.channels[c], B.channels[c]):
            return False

    return True

def compare_channels(A, B):

    if (A.name != B.name or
        A.type() != B.type() or
        A.xSampling != B.xSampling or
        A.ySampling != B.ySampling):
        print(f"channel {A.name} differs: {A.__repr__()} {B.__repr__()}")
        return False

    return True

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

class TestOpenEXR(unittest.TestCase):

    def test_read_write(self):

        #
        # Read a file and write it back out, then read the freshly-written
        # file to validate it's the same.
        #

        infilename = f"{test_dir}/test.exr"
        infile = OpenEXR.File(infilename)

        outfilename = mktemp_outfilename()
        infile.write(outfilename)

        outfile = OpenEXR.File(outfilename)

        assert outfile == infile

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

    def test_rational(self):

        r = OpenEXR.Rational(1,2)

        self.assertEqual(r.n, 1)
        self.assertEqual(r.d, 2)

    def test_empty_header(self):

        # Construct a file from scratch and write it.

        width = 10
        height = 20
        size = width * height
        Z = np.array([i for i in range(0,size)], dtype='uint32').reshape((height, width))
        channels = { "Z" : OpenEXR.Channel(Z, 1, 1) }

        header = {}

        outfile = OpenEXR.File(header, channels)

        outfilename = mktemp_outfilename()
        outfile.write(outfilename)

        infile = OpenEXR.File(outfilename)

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

        outfile = OpenEXR.File(header, channels)

        # confirm that the write assigned names to the channels
        self.assertEqual(outfile.channels()['A'].name, "A")

        outfilename = mktemp_outfilename()
        outfile.write(outfilename)

        # Verify reading it back gives the same data
        infile = OpenEXR.File(outfilename)

        compare_files(infile, outfile)

        assert infile == outfile

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

        outfile = OpenEXR.File(header, channels)
        outfilename = mktemp_outfilename()
        outfile.write(outfilename)

        # Verify reading it back gives the same data
        infile = OpenEXR.File(outfilename)

        compare_files(infile, outfile)

        assert infile == outfile

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

        outfile = OpenEXR.File(header, channels)
        outfilename = mktemp_outfilename()
        outfile.write(outfilename)

        # Verify reading it back gives the same data
        infile = OpenEXR.File(outfilename)

        compare_files(infile, outfile)

        assert infile == outfile

    def test_modify_in_place(self):

        #
        # Test modifying header attributes in place
        #

        infilename = f"{test_dir}/test.exr"
        f = OpenEXR.File(infilename)

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
        m = OpenEXR.File(outfilename)

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

        outfile = OpenEXR.File(header, channels)

        outfilename = mktemp_outfilename()
        outfile.write(outfilename)

        infile = OpenEXR.File(outfilename)

        Q = infile.header()["preview"].pixels

        assert preview_pixels_equal(P, Q)

        assert infile == outfile

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
        header["chromaticities"] = OpenEXR.Chromaticities(OpenEXR.V2f(1.0,2.0),
                                                          OpenEXR.V2f(3.0,4.0),
                                                          OpenEXR.V2f(5.0,6.0),
                                                          OpenEXR.V2f(7.0,8.0))
        header["box2i"] = OpenEXR.Box2i(OpenEXR.V2i(0,1),OpenEXR.V2i(2,3))
        header["box2f"] = OpenEXR.Box2f(OpenEXR.V2f(0,1),OpenEXR.V2f(2,3))
        header["compression"] = OpenEXR.ZIPS_COMPRESSION
        header["double"] = OpenEXR.Double(42000)
        header["float"] = 4.2
        header["int"] = 42
        header["keycode"] = OpenEXR.KeyCode(0,0,0,0,0,4,64)
        header["lineorder"] = OpenEXR.INCREASING_Y
        header["m33f"] = OpenEXR.M33f(1,0,0,0,1,0,0,0,1)
        header["m33d"] = OpenEXR.M33d(1,0,0,0,1,0,0,0,1)
        header["m44f"] = OpenEXR.M44f(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)
        header["m44d"] = OpenEXR.M44d(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)
        header["rational"] = OpenEXR.Rational(1,3)
        header["string"] = "stringy"
        header["timecode"] = OpenEXR.TimeCode(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18)
        header["v2i"] = OpenEXR.V2i(1,2)
        header["v2f"] = OpenEXR.V2f(1.2,3.4)
        header["v2d"] = OpenEXR.V2d(1.2,3.4)
        header["v3i"] = OpenEXR.V3i(1,2,3)
        header["v3f"] = OpenEXR.V3f(1.2,3.4,5.6)
        header["v3d"] = OpenEXR.V3d(1.2,3.4,5.6)

        outfile = OpenEXR.File(header, channels)

        outfilename = mktemp_outfilename()
        outfile.write(outfilename)

        # Verify reading it back gives the same data

        infile = OpenEXR.File(outfilename)

        compare_files(infile, outfile)

        assert infile == outfile

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
            header["chromaticities"] = OpenEXR.Chromaticities(OpenEXR.V2f(1.0,2.0),
                                                              OpenEXR.V2f(3.0,4.0),
                                                              OpenEXR.V2f(5.0,6.0),
                                                              OpenEXR.V2f(7.0,8.0))
            header["box2i"] = OpenEXR.Box2i(OpenEXR.V2i(0,1),OpenEXR.V2i(2,3))
            header["box2f"] = OpenEXR.Box2f(OpenEXR.V2f(0,1),OpenEXR.V2f(2,3))
            header["compression"] = OpenEXR.ZIPS_COMPRESSION
            header["double"] = OpenEXR.Double(42000)
            header["float"] = 4.2
            header["int"] = 42
            header["keycode"] = OpenEXR.KeyCode(0,0,0,0,0,4,64)
            header["lineorder"] = OpenEXR.INCREASING_Y
            header["m33f"] = OpenEXR.M33f(1,0,0,0,1,0,0,0,1)
            header["m33d"] = OpenEXR.M33d(1,0,0,0,1,0,0,0,1)
            header["m44f"] = OpenEXR.M44f(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)
            header["m44d"] = OpenEXR.M44d(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)
            header["preview"] = OpenEXR.PreviewImage(P)
            header["rational"] = OpenEXR.Rational(1,3)
            header["string"] = "stringy"
            header["timecode"] = OpenEXR.TimeCode(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18)
            header["v2i"] = OpenEXR.V2i(1,2)
            header["v2f"] = OpenEXR.V2f(1.2,3.4)
            header["v2d"] = OpenEXR.V2d(1.2,3.4)
            header["v3i"] = OpenEXR.V3i(1,2,3)
            header["v3f"] = OpenEXR.V3f(1.2,3.4,5.6)
            header["v3d"] = OpenEXR.V3d(1.2,3.4,5.6)
            return header

        header1 = make_header()
        header2 = make_header()

        P1 = OpenEXR.Part(header1, channels, "Part1")
        P2 = OpenEXR.Part(header2, channels, "Part2")

        parts = [P1, P2]
        outfile2 = OpenEXR.File(parts)

        outfilename = mktemp_outfilename()
        outfile2.write(outfilename)

        # Verify reading it back gives the same data
        i = OpenEXR.File(outfilename)
        assert i == outfile2

if __name__ == '__main__':
    unittest.main()
