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

    # Generate arrays for R, G, and B channels with random values
    height, width = (20, 10)
    R = np.random.rand(height, width).astype('f')
    G = np.random.rand(height, width).astype('f')
    B = np.random.rand(height, width).astype('f')
    channels = { "R" : R, "G" : G, "B" : B }
    header = { "compression" : OpenEXR.ZIP_COMPRESSION,
               "type" : OpenEXR.scanlineimage }

    with OpenEXR.File(header, channels) as outfile:
        outfile.write("readme.exr")

    print("ok")

def test_write_RGB():

    # Generate a 3D NumPy array for RGB channels with random values
    height, width = (20, 10)
    RGB = np.random.rand(height, width, 3).astype('f')

    channels = { "RGB" : RGB }
    header = { "compression" : OpenEXR.ZIP_COMPRESSION,
               "type" : OpenEXR.scanlineimage }

    with OpenEXR.File(header, channels) as outfile:
        outfile.write("readme.exr")

    print("ok")

def test_read_separate_channels():

    with OpenEXR.File("readme.exr", separate_channels=True) as infile:

        header = infile.header()
        print(f"type={header['type']}")
        print(f"compression={header['compression']}")

        R = infile.channels()["R"].pixels
        G = infile.channels()["G"].pixels
        B = infile.channels()["B"].pixels
        height, width = R.shape
        for y in range(height):
            for x in range(width):
                pixel = (R[y, x], G[y, x], B[y, x])
                print(f"pixel[{y}][{x}]={pixel}")

    print("ok")

def test_read_RGB():

    with OpenEXR.File("readme.exr") as infile:

        print(f"readme.exr: {infile.channels()}")
        
        RGB = infile.channels()["RGB"].pixels
        height, width = RGB.shape[0:2]
        for y in range(height):
            for x in range(width):
                pixel = tuple(RGB[y, x])
                print(f"pixel[{y}][{x}]={pixel}")

    print("ok")

def test_modify():

    with OpenEXR.File("readme.exr") as f:
        
        f.header()["displayWindow"] = ((3,4),(5,6))
        f.header()["screenWindowCenter"] = np.array([1.0,2.0],'float32')
        f.header()["comments"] = "test image"
        f.header()["longitude"] = -122.5
        f.write("readme_modified.exr")

        with OpenEXR.File("readme_modified.exr") as o:
            dw = o.header()["displayWindow"]
            assert (tuple(dw[0]), tuple(dw[1])) == ((3,4),(5,6))
            swc = o.header()["screenWindowCenter"]
            assert tuple(swc) == (1.0, 2.0)
            assert o.header()["comments"] == "test image"
            assert o.header()["longitude"] == -122.5

    print("ok")

def test_multipart_write():

    height, width = (20, 10)
    Z0 = np.zeros((height, width), dtype='f')
    Z1 = np.ones((height, width), dtype='f')

    P0 = OpenEXR.Part({}, {"Z" : Z0 })
    P1 = OpenEXR.Part({}, {"Z" : Z1 })

    f = OpenEXR.File([P0, P1])
    f.write("readme_2part.exr")

    with OpenEXR.File("readme_2part.exr") as o:
        assert o.parts[0].name() == "Part0"
        assert o.parts[0].width() == 10
        assert o.parts[0].height() == 20
        assert o.parts[1].name() == "Part1"
        assert o.parts[1].width() == 10
        assert o.parts[1].height() == 20

    print("ok")

def test_multipart_write():

    height, width = (20, 10)

    Z0 = np.zeros((height, width), dtype='f')
    P0 = OpenEXR.Part(header={"type" : OpenEXR.scanlineimage },
                      channels={"Z" : Z0 })

    Z1 = np.ones((height, width), dtype='f')
    P1 = OpenEXR.Part(header={"type" : OpenEXR.scanlineimage },
                      channels={"Z" : Z1 })

    f = OpenEXR.File(parts=[P0, P1])
    f.write("readme_2part.exr")

    with OpenEXR.File("readme_2part.exr") as o:
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

    height, width = (20, 10)

    Z = np.zeros((height, width), dtype='f')
    P = OpenEXR.Part({"type" : OpenEXR.tiledimage,
                      "tiles" : OpenEXR.TileDescription() },
                     {"Z" : Z })

    with OpenEXR.File([P]) as f:
        f.write("readme_tiled.exr")

    with OpenEXR.File("readme_tiled.exr") as o:
        assert o.parts[0].name() == "Part0"
        assert o.parts[0].type() == OpenEXR.tiledimage

def test_write_deep():
    
    height, width = (20, 10)

    Z = np.empty((height, width), dtype=object)
    for y in range(height):
        for x in range(width):
            Z[y, x] = np.array([y*width+x], dtype='uint32')

    channels = { "Z" : Z }
    header = { "compression" : OpenEXR.ZIPS_COMPRESSION,
               "type" : OpenEXR.deepscanline }
    with OpenEXR.File(header, channels) as outfile:
        outfile.write("readme_test_tiled_deep.exr")

def test_read_deep():

    with OpenEXR.File("readme_test_tiled_deep.exr") as infile:

        Z = infile.channels()["Z"].pixels
        height, width = Z.shape
        for y in range(height):
            for x in range(width):
                for z in Z[y,x]:
                    print(f"deep sample at {y},{x}: {z}")
        

if __name__ == '__main__':

    test_multipart_write()
    test_write()
    test_write_RGB()
    test_read_separate_channels()
    test_read_RGB()
    test_modify()
    test_write_tiled()
    test_write_deep()
    test_read_deep()
