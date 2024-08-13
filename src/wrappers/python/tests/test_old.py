#!/usr/bin/env python3

#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.
#

from __future__ import print_function
import sys
import os
import random
from array import array

import OpenEXR
import Imath

test_dir = os.path.dirname(__file__)

FLOAT = Imath.PixelType(Imath.PixelType.FLOAT)
UINT = Imath.PixelType(Imath.PixelType.UINT)
HALF = Imath.PixelType(Imath.PixelType.HALF)

testList = []

#
# Write a simple exr file, read it back and confirm the data is the same.
#

def test_write_read():

    width = 100
    height = 100
    size = width * height

    h = OpenEXR.Header(width,height)
    h['channels'] = {'R' : Imath.Channel(FLOAT),
                     'G' : Imath.Channel(FLOAT),
                     'B' : Imath.Channel(FLOAT),
                     'A' : Imath.Channel(FLOAT)} 
    o = OpenEXR.OutputFile(f"{test_dir}/write.exr", h)
    r = array('f', [n for n in range(size*0,size*1)]).tobytes()
    g = array('f', [n for n in range(size*1,size*2)]).tobytes()
    b = array('f', [n for n in range(size*2,size*3)]).tobytes()
    a = array('f', [n for n in range(size*3,size*4)]).tobytes()
    channels = {'R' : r, 'G' : g, 'B' : b, 'A' : a}
    o.writePixels(channels)
    o.close()

    i = OpenEXR.InputFile(f"{test_dir}/write.exr")
    h = i.header()
    assert r == i.channel('R')
    assert g == i.channel('G')
    assert b == i.channel('B')
    assert a == i.channel('A')

testList.append(("test_write_read", test_write_read))

def test_level_modes():

    assert Imath.LevelMode("ONE_LEVEL").v == Imath.LevelMode(Imath.LevelMode.ONE_LEVEL).v
    assert Imath.LevelMode("MIPMAP_LEVELS").v == Imath.LevelMode(Imath.LevelMode.MIPMAP_LEVELS).v
    assert Imath.LevelMode("RIPMAP_LEVELS").v == Imath.LevelMode(Imath.LevelMode.RIPMAP_LEVELS).v

testList.append(("test_level_modes", test_level_modes))

#
# Write an image as UINT, read as FLOAT, and the reverse.
#
def test_conversion():
    codemap = { 'f': FLOAT, 'I': UINT }
    original = [0, 1, 33, 79218]
    for frm_code,to_code in [ ('f','I'), ('I','f') ]:
        hdr = OpenEXR.Header(len(original), 1)
        hdr['channels'] = {'L': Imath.Channel(codemap[frm_code])}
        x = OpenEXR.OutputFile(f"{test_dir}/out.exr", hdr)
        x.writePixels({'L': array(frm_code, original).tobytes()})
        x.close()

        xin = OpenEXR.InputFile(f"{test_dir}/out.exr")
        assert array(to_code, xin.channel('L', codemap[to_code])).tolist() == original

testList.append(("test_conversion", test_conversion))

#
# Confirm failure on reading from non-exist location
#

def test_invalid_input():
    try:
        OpenEXR.InputFile("/bad/place")
    except:
        pass
    else:
        assert 0

testList.append(("test_invalid_input", test_invalid_input))

#
# Confirm failure on writing to invalid location
#

def test_invalid_output():

    try:
        hdr = OpenEXR.Header(640, 480)
        OpenEXR.OutputFile("/bad/place", hdr)
    except:
        pass
    else:
        assert 0

testList.append(("test_invalid_output", test_invalid_output))

def test_one():
    oexr = OpenEXR.InputFile(f"{test_dir}/write.exr")

    header = oexr.header()

    default_size = len(oexr.channel('R'))
    half_size = len(oexr.channel('R', Imath.PixelType(Imath.PixelType.HALF)))
    float_size = len(oexr.channel('R', Imath.PixelType(Imath.PixelType.FLOAT)))
    uint_size = len(oexr.channel('R', Imath.PixelType(Imath.PixelType.UINT)))

    assert default_size in [ half_size, float_size, uint_size]
    assert float_size == uint_size
    assert (float_size / 2) == half_size

    assert len(oexr.channel('R',
                            pixel_type = FLOAT,
                            scanLine1 = 10,
                            scanLine2 = 10)) == (4 * (header['dataWindow'].max.x + 1))


    data = b" " * (4 * 100 * 100)
    h = OpenEXR.Header(100,100)
    x = OpenEXR.OutputFile(f"{test_dir}/out.exr", h)
    x.writePixels({'R': data, 'G': data, 'B': data})
    x.close()

testList.append(("test_one", test_one))

#
# Check that the channel method and channels method return the same data
#

def test_channel_channels():

    aexr = OpenEXR.InputFile(f"{test_dir}/write.exr")
    acl = sorted(aexr.header()['channels'].keys())
    a = [aexr.channel(c) for c in acl]
    b = aexr.channels(acl)

    assert a == b

testList.append(("test_channel_channels", test_channel_channels))

def test_types():
    for original in [ [0,0,0], list(range(10)), list(range(100,200,3)) ]:
        for code,t in [ ('I', UINT), ('f', FLOAT) ]:
            data = array(code, original).tobytes()
            hdr = OpenEXR.Header(len(original), 1)
            hdr['channels'] = {'L': Imath.Channel(t)}

            x = OpenEXR.OutputFile(f"{test_dir}/out.exr", hdr)
            x.writePixels({'L': data})
            x.close()

            xin = OpenEXR.InputFile(f"{test_dir}/out.exr")
            # Implicit type
            assert array(code, xin.channel('L')).tolist() == original
            # Explicit typen
            assert array(code, xin.channel('L', t)).tolist() == original
            # Explicit type as kwarg
            assert array(code, xin.channel('L', pixel_type = t)).tolist() == original

testList.append(("test_types", test_types))

def test_invalid_pixeltype():
    oexr = OpenEXR.InputFile(f"{test_dir}/write.exr")
    FLOAT = Imath.PixelType.FLOAT
    try:
        f.channel('R',FLOAT)
    except:
        pass
    else:
        assert 0

testList.append(("test_invalid_pixeltype", test_invalid_pixeltype))

#
# Write arbitrarily named channels.
#

def test_write_mchannels():
    hdr = OpenEXR.Header(100, 100)
    for chans in [ set("a"), set(['foo', 'bar']), set("abcdefghijklmnopqstuvwxyz") ]:
        hdr['channels'] = dict([(nm, Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT))) for nm in chans])
        x = OpenEXR.OutputFile(f"{test_dir}/out0.exr", hdr)
        data = array('f', [0] * (100 * 100)).tobytes()
        x.writePixels(dict([(nm, data) for nm in chans]))
        x.close()
        assert set(OpenEXR.InputFile(f"{test_dir}/out0.exr").header()['channels']) == chans

testList.append(("test_write_mchannels", test_write_mchannels))

def load_red(filename):
    oexr = OpenEXR.InputFile(filename)
    return oexr.channel('R')

#
# Write the pixels to two images, first as a single call,
# then as multiple calls.  Verify that the images are identical.
#

def test_write_chunk():
    for w,h,step in [(100, 10, 1), (64,48,6), (1, 100, 2), (640, 480, 4)]:
        data = array('f', [ random.random() for x in range(w * h) ]).tobytes()

        hdr = OpenEXR.Header(w,h)
        x = OpenEXR.OutputFile(f"{test_dir}/out0.exr", hdr)
        x.writePixels({'R': data, 'G': data, 'B': data})
        x.close()

        hdr = OpenEXR.Header(w,h)
        x = OpenEXR.OutputFile(f"{test_dir}/out1.exr", hdr)
        for y in range(0, h, step):
            subdata = data[y * w * 4:(y+step) * w * 4]
            x.writePixels({'R': subdata, 'G': subdata, 'B': subdata}, step)
        x.close()

        oexr0 = load_red(f"{test_dir}/out0.exr")
        oexr1 = load_red(f"{test_dir}/out1.exr")
        assert oexr0 == oexr1

testList.append(("test_write_chunk", test_write_chunk))

for test in testList:
    funcName = test[0]
    test[1]()


