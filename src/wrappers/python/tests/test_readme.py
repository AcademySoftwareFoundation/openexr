#!/usr/bin/env python3

#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.
#

# This is the example code from src/wrappers/python/README.md

def test_readme():

    import OpenEXR, Imath
    from array import array
    
    width = 10
    height = 10
    size = width * height
    
    h = OpenEXR.Header(width,height)
    h['channels'] = {'R' : Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT)),
                     'G' : Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT)),
                     'B' : Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT)),
                     'A' : Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT))} 
    o = OpenEXR.OutputFile("hello.exr", h)
    r = array('f', [n for n in range(size*0,size*1)]).tobytes()
    g = array('f', [n for n in range(size*1,size*2)]).tobytes()
    b = array('f', [n for n in range(size*2,size*3)]).tobytes()
    a = array('f', [n for n in range(size*3,size*4)]).tobytes()
    channels = {'R' : r, 'G' : g, 'B' : b, 'A' : a}
    o.writePixels(channels)
    o.close()
