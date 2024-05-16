#!/usr/bin/env python3

#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.
#

# This is the example code from src/wrappers/python/README.md

import OpenEXR
import numpy as np
import random

def test_write():

    width = 10
    height = 20
    R = np.ndarray((height, width), dtype='f')
    G = np.ndarray((height, width), dtype='f')
    B = np.ndarray((height, width), dtype='f')
    for y in range(0, height):
        for x in range(0, width):
            R[y][x] = random.random()
            G[y][x] = random.random()
            B[y][x] = random.random()

    channels = { "R" : OpenEXR.Channel(R),
                 "G" : OpenEXR.Channel(G),
                 "B" : OpenEXR.Channel(B) }

    header = { "compression" : OpenEXR.ZIP_COMPRESSION,
               "type" : OpenEXR.scanlineimage }

    outfile = OpenEXR.File(header, channels)
    outfile.write("readme.exr")

def test_write_RGB():

    width = 10
    height = 20
    RGB = np.ndarray((height, width, 3), dtype='f')
    for y in range(0, height):
        for x in range(0, width):
           for i in range(0,3):
               RGB[y][x][i] = random.random()

    channels = { "RGB" : OpenEXR.Channel(RGB) }
    header = { "compression" : OpenEXR.ZIP_COMPRESSION,
               "type" : OpenEXR.scanlineimage }

    outfile = OpenEXR.File(header, channels)
    outfile.write("readme.exr")

def test_read():

    infile = OpenEXR.File("readme.exr")

    header = infile.header()
    print(f"type={header['type']}")
    print(f"compression={header['compression']}")

    R = infile.channels()["R"].pixels
    G = infile.channels()["G"].pixels
    B = infile.channels()["B"].pixels
    width = R.shape[1]
    height = R.shape[0]
    for y in range(0, height):
        for x in range(0, width):
            print(f"pixel[{y}][{x}]=({R[y][x]}, {G[y][x]}, {B[y][x]})")

def test_read_RGB():

    infile = OpenEXR.File("readme.exr", rgba=True)

    RGB = infile.channels()["RGB"].pixels
    width = RGB.shape[1]
    height = RGB.shape[0]
    for y in range(0, height):
        for x in range(0, width):
            print(f"pixel[{y}][{x}]=({RGB[y][x][0]}, {RGB[y][x][1]}, {RGB[y][x][2]})")

def test_modify():

    f = OpenEXR.File("readme.exr")
    f.header()["displayWindow"] = OpenEXR.Box2i(OpenEXR.V2i(3,4),
                                                OpenEXR.V2i(5,6))
    f.header()["comments"] = "test image"
    f.header()["longitude"] = -122.5
    f.write("readme_modified.exr")

    o = OpenEXR.File("readme_modified.exr")
    assert o.header()["displayWindow"] == OpenEXR.Box2i(OpenEXR.V2i(3,4),
                                                        OpenEXR.V2i(5,6))
    assert o.header()["comments"] == "test image"
    assert o.header()["longitude"] == -122.5

    print("ok")

def test_multipart_write():

    height = 20
    width = 10
    Z0 = np.zeros((height, width), dtype='f')
    Z1 = np.ones((height, width), dtype='f')

    P0 = OpenEXR.Part({}, {"Z" : OpenEXR.Channel(Z0) })
    P1 = OpenEXR.Part({}, {"Z" : OpenEXR.Channel(Z1) })

    f = OpenEXR.File([P0, P1])
    f.write("readme_2part.exr")

    o = OpenEXR.File("readme_2part.exr")
    assert o.parts[0].name() == "Part0"
    assert o.parts[0].width() == 10
    assert o.parts[0].height() == 20
    assert o.parts[1].name() == "Part1"
    assert o.parts[1].width() == 10
    assert o.parts[1].height() == 20
    print("ok")

def test_multipart_write():

    height = 20
    width = 10

    Z0 = np.zeros((height, width), dtype='f')
    P0 = OpenEXR.Part(header={"type" : OpenEXR.scanlineimage },
                      channels={"Z" : OpenEXR.Channel(Z0) })

    Z1 = np.ones((height, width), dtype='f')
    P1 = OpenEXR.Part(header={"type" : OpenEXR.scanlineimage },
                      channels={"Z" : OpenEXR.Channel(Z1) })

    f = OpenEXR.File(parts=[P0, P1])
    f.write("readme_2part.exr")

    o = OpenEXR.File("readme_2part.exr")
    assert o.parts[0].name() == "Part0"
    assert o.parts[0].type() == OpenEXR.scanlineimage
    assert o.parts[0].width() == 10
    assert o.parts[0].height() == 20
    assert np.array_equal(o.parts[0].channels["Z"].pixels, Z0)
    assert o.parts[1].name() == "Part1"
    assert o.parts[1].type() == OpenEXR.scanlineimage
    assert o.parts[1].width() == 10
    assert o.parts[1].height() == 20
    assert np.array_equal(o.parts[1].channels["Z"].pixels, Z1)
    print("ok")

def test_write_tiled():

    height = 20
    width = 10
    Z = np.zeros((height, width), dtype='f')

    P = OpenEXR.Part({"type" : OpenEXR.tiledimage,
                      "tiles" : OpenEXR.TileDescription() },
                     {"Z" : OpenEXR.Channel(Z) })

    f = OpenEXR.File([P])
    f.write("readme_tiled.exr")

    o = OpenEXR.File("readme_tiled.exr")
    assert o.parts[0].name() == "Part0"
    assert o.parts[0].type() == OpenEXR.tiledimage

if __name__ == '__main__':

    test_multipart_write()
    test_write()
    test_write_RGB()
    test_read()
    test_read_RGB()
    test_modify()
    test_write_tiled()

