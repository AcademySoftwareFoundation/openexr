"""
:mod:`Imath` --- Support types for OpenEXR library
==================================================
"""

class chromaticity(object):
    """Store chromaticity coordinates in *x* and *y*."""
    def __init__(self, x, y):
        self.x = x
        self.y = y
    def __repr__(self):
        return repr((self.x, self.y))
    def __eq__(self, other):
        return (self.x, self.y) == (other.x, other.y)

class point(object):
    """Point is a 2D point, with members *x* and *y*."""
    def __init__(self, x, y):
        self.x = x;
        self.y = y;
    def __repr__(self):
        return repr((self.x, self.y))
    def __eq__(self, other):
        return (self.x, self.y) == (other.x, other.y)

class V2i(point):
    """V2i is a 2D point, with members *x* and *y*."""
    pass

class V2f(point):
    """V2f is a 2D point, with members *x* and *y*."""
    pass

class Box:
    """Box is a 2D box, specified by its two corners *min* and *max*, both of which are :class:`point` """
    def __init__(self, min = None, max = None):
        self.min = min
        self.max = max
    def __repr__(self):
        return repr(self.min) + " - " + repr(self.max)
    def __eq__(self, other):
        return (self.min, self.max) == (other.min, other.max)

class Box2i(Box):
    """Box2i is a 2D box, specified by its two corners *min* and *max*."""
    pass

class Box2f(Box):
    """Box2f is a 2D box, specified by its two corners *min* and *max*."""
    pass

class Chromaticities:
    """
    Chromaticities holds the set of chromaticity coordinates for *red*, *green*, *blue*, and *white*.
    Each primary is a :class:`chromaticity`.
    """
    def __init__(self, red = None, green = None, blue = None, white = None):
        self.red   = red
        self.green = green
        self.blue  = blue
        self.white = white
    def __repr__(self):
        return repr(self.red) + " " + repr(self.green) + " " + repr(self.blue) + " " + repr(self.white)

class Enumerated(object):
    def __init__(self, v):
        if v in self.names:
            self.v = eval("self." + v)
        else:
            self.v = v
    def __repr__(self):
        return self.names[self.v]
    def __cmp__(self, other):
        return self.v - other.v
    def __eq__(self, other):
        return self.v == other.v

class LineOrder(Enumerated):
    """
    .. index:: INCREASING_Y, DECREASING_Y, RANDOM_Y

    LineOrder can have three possible values:
    ``INCREASING_Y``,
    ``DECREASING_Y``,
    ``RANDOM_Y``.

    .. doctest::
    
       >>> import Imath
       >>> print Imath.LineOrder(Imath.LineOrder.DECREASING_Y)
       DECREASING_Y
    """
    INCREASING_Y = 0
    DECREASING_Y = 1
    RANDOM_Y	 = 2
    names = ["INCREASING_Y", "DECREASING_Y", "RANDOM_Y"]

class Compression(Enumerated):
    """
    .. index:: NO_COMPRESSION, RLE_COMPRESSION, ZIPS_COMPRESSION, ZIP_COMPRESSION, PIZ_COMPRESSION, PXR24_COMPRESSION, B44_COMPRESSION, B44A_COMPRESSION, DWAA_COMPRESSION, DWAB_COMPRESSION,

    Compression can have possible values:
    ``NO_COMPRESSION``,
    ``RLE_COMPRESSION``,
    ``ZIPS_COMPRESSION``,
    ``ZIP_COMPRESSION``,
    ``PIZ_COMPRESSION``,
    ``PXR24_COMPRESSION``,
    ``B44_COMPRESSION``,
    ``B44A_COMPRESSION``,
    ``DWAA_COMPRESSION``,
    ``DWAB_COMPRESSION``.

    .. doctest::
    
       >>> import Imath
       >>> print Imath.Compression(Imath.Compression.RLE_COMPRESSION)
       RLE_COMPRESSION
    """
    NO_COMPRESSION  = 0
    RLE_COMPRESSION = 1
    ZIPS_COMPRESSION = 2
    ZIP_COMPRESSION = 3
    PIZ_COMPRESSION = 4
    PXR24_COMPRESSION = 5
    B44_COMPRESSION = 6
    B44A_COMPRESSION = 7
    DWAA_COMPRESSION = 8
    DWAB_COMPRESSION = 9
    names = [
        "NO_COMPRESSION", "RLE_COMPRESSION", "ZIPS_COMPRESSION", "ZIP_COMPRESSION", "PIZ_COMPRESSION", "PXR24_COMPRESSION",
        "B44_COMPRESSION", "B44A_COMPRESSION", "DWAA_COMPRESSION", "DWAB_COMPRESSION"
    ]

class PixelType(Enumerated):
    """
    .. index:: UINT, HALF, FLOAT

    PixelType can have possible values ``UINT``, ``HALF``, ``FLOAT``.

    .. doctest::
    
       >>> import Imath
       >>> print Imath.PixelType(Imath.PixelType.HALF)
       HALF
    """
    UINT  = 0
    HALF  = 1
    FLOAT = 2
    names = ["UINT", "HALF", "FLOAT"]

class Channel:
    """
    Channel defines the type and spatial layout of a channel.
    *type* is a :class:`PixelType`.
    *xSampling* is the number of X-axis pixels between samples.
    *ySampling* is the number of Y-axis pixels between samples.

    .. doctest::
    
       >>> import Imath
       >>> print Imath.Channel(Imath.PixelType(Imath.PixelType.FLOAT), 4, 4)
       FLOAT (4, 4)
    """

    def __init__(self, type = PixelType(PixelType.HALF), xSampling = 1, ySampling = 1):
        self.type = type
        self.xSampling = xSampling
        self.ySampling = ySampling
    def __repr__(self):
        return repr(self.type) + " " + repr((self.xSampling, self.ySampling))
    def __eq__(self, other):
        return (self.type, self.xSampling, self.ySampling) == (other.type, other.xSampling, other.ySampling)

class Rational(object):
    def __init__(self, n, d):
        self.n = n
        self.d = d
    def __repr__(self):
        return repr("%s/%s (%.3f)" % (self.n, self.d, self.n/float(self.d)))
    def __eq__(self, other):
        return self.n == other.n and self.d == other.d


class TimeCode:
    def __init__(self, hours, minutes, seconds, frame, dropFrame=False, colorFrame=False, fieldPhase=False, bgf0=False, bgf1=False, bgf2=False, binaryGroup1=0, binaryGroup2=0, binaryGroup3=0, binaryGroup4=0, binaryGroup5=0, binaryGroup6=0, binaryGroup7=0, binaryGroup8=0):
        self.hours = hours
        self.minutes = minutes
        self.seconds = seconds
        self.frame = frame
        self.dropFrame = dropFrame
        self.colorFrame = colorFrame
        self.fieldPhase = fieldPhase
        self.bgf0 = bgf0
        self.bgf1 = bgf1
        self.bgf2 = bgf2
        self.binaryGroup1 = binaryGroup1
        self.binaryGroup2 = binaryGroup2
        self.binaryGroup3 = binaryGroup3
        self.binaryGroup4 = binaryGroup4
        self.binaryGroup5 = binaryGroup5
        self.binaryGroup6 = binaryGroup6
        self.binaryGroup7 = binaryGroup7
        self.binaryGroup8 = binaryGroup8

    def __repr__(self):
        # ignoring binaryGroups for now
        return "<Imath.TimeCode instance { time: %s:%s:%s:%s, dropFrame: %s, colorFrame: %s, fieldPhase: %s, bgf0: %s, bgf1: %s, bgf2: %s" % (self.hours, self.minutes, self.seconds, self.frame, self.dropFrame, self.colorFrame, self.fieldPhase, self.bgf0, self.bgf1, self.bgf2)

    def __eq__(self, other): 
        return self.__dict__ == other.__dict__

class KeyCode:
    def __init__(self, filmMfcCode=0, filmType=0, prefix=0, count=0, perfOffset=0, perfsPerFrame=4, perfsPerCount=64):
        self.filmMfcCode = filmMfcCode
        self.filmType = filmType
        self.prefix = prefix
        self.count = count
        self.perfOffset = perfOffset
        self.perfsPerFrame = perfsPerFrame
        self.perfsPerCount = perfsPerCount
    def __repr__(self):
        return "<Imath.KeyCode instance { filmMfcCode: %s, filmType: %s, prefix: %s, count: %s, perfOffset: %s, perfsPerFrame: %s, perfsPerCount: %s }" % (self.filmMfcCode, self.filmType, self.prefix, self.count, self.perfOffset, self.perfsPerFrame, self.perfsPerCount)

    def __eq__(self, other): 
        return self.__dict__ == other.__dict__

class PreviewImage:
    """
    .. index:: RGBA, thumbnail, preview, JPEG, PIL, Python Imaging Library

    PreviewImage is a small preview image, intended as a thumbnail version of the full image.
    The image has size (*width*, *height*) and 8-bit pixel values are
    given by string *pixels* in RGBA order from top-left to bottom-right.

    For example, to create a preview image from a JPEG file using the popular 
    `Python Imaging Library <http://www.pythonware.com/library/pil/handbook/index.htm>`_:

    .. doctest::
    
       >>> import Image
       >>> import Imath
       >>> im = Image.open("lena.jpg").resize((100, 100)).convert("RGBA")
       >>> print Imath.PreviewImage(im.size[0], im.size[1], im.tostring())
       <Imath.PreviewImage instance 100x100>
    """
    def __init__(self, width, height, pixels):
        self.width = width
        self.height = height
        self.pixels = pixels
    def __repr__(self):
        return "<Imath.PreviewImage instance %dx%d>" % (self.width, self.height)

class LevelMode(Enumerated):
    ONE_LEVEL = 0
    MIPMAP_LEVELS = 1
    RIPMAP_LEVELS = 2
    names = ["ONE_LEVEL", "MIPMAP_LEVELS", "RIPMAP_LEVELS"]

class LevelRoundingMode(Enumerated):
    ROUND_DOWN = 0
    ROUND_UP = 1
    names = ["ROUND_DOWN", "ROUND_UP"]

class TileDescription:
    def __init__(self, xs = 32, ys = 32, m = LevelMode(LevelMode.ONE_LEVEL), r =LevelRoundingMode(LevelRoundingMode.ROUND_DOWN)):
        self.xSize = xs
        self.ySize = ys
        self.mode = m
        self.roundingMode = r
    def __repr__(self):
        return "<Imath.TileDescription instance %dx%d %s %s>" % (self.xSize, self.ySize, repr(self.mode), repr(self.roundingMode))
