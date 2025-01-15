..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _Python:

The OpenEXR Python Module
#########################

.. toctree::
   :caption: Python
             
The OpenEXR python module provides basic access to data in EXR image
files. The read and write methods use python dictionaries for header
metadata and numpy arrays for pixel data.

To install the OpenEXR module:

.. code-block::

   % pip install OpenEXR

The module is specifically designed for maximum simplicity and ease of
use, not for high performance. If your application deals with
especially large data files, is particular about memory management, or
needs low level operations like reading specific scanlines or tiles,
this may not be the module for you. But if your application is
comfortable reading entire files into memory and can deal with pixel
data in numpy arrays, the OpenEXR module is a suitable option.

A Note about Backwards Compatibility
====================================

The v3.3 release of the OpenEXR module provides an entirely new API in
the form of the ``OpenEXR.File`` object. This API is full-featured and
fully supported going forward.

The original implementation of the OpenEXR python bindings prior to
the v3.3 release used the ``InputFile`` and ``OutputFile``
objects. This API is limited in scope, and is now deprecated. It is
still distributed as is for backwards compatibility, but usage is
discouraged.

Example Images
==============

See :ref:`Test Images` for example images to experiment with.  

Reading and Writing in a Nutshell
=================================

Generate random RGB data and write it to an EXR file:

.. code-block::

    import OpenEXR
    
    height, width = (1080, 1920)
    RGB = np.random.rand(height, width, 3).astype('float32')
    channels = { "RGB" : RGB }
    header = { "compression" : OpenEXR.ZIP_COMPRESSION,
               "type" : OpenEXR.scanlineimage }

    with OpenEXR.File(header, channels) as outfile:
        outfile.write("image.exr")

This creates a scanline image of size 10x20 pixels with R, G, and B
channels of type float, initialized to random values, and writes it to
the file ``test.exr``, compressed with ZIP compression.

Correspondingly, to read an image and print its pixel data:

.. code-block::

    import OpenEXR

    with OpenEXR.File("image.exr") as infile:

        header = infile.header()
        print(f"type={header['type']}")
        print(f"compression={header['compression']}")

        RGB = infile.channels()["RGB"].pixels
        height, width = RGB.shape[0:2]
        for y in range(height):
            for x in range(width):
                pixel = (RGB[y, x, 0], RGB[y, x, 1], RGB[y, x, 2])
                print(f"pixel[{y}][{x}]={pixel}")

Reading EXR Files with OpenEXR.File
===================================

The basic construct of the OpenEXR module is the ``File`` object.
Construct a ``File`` object with a filename as the parameter and it
reads the image data into the object:

.. code-block::

    >>> exrfile = OpenEXR.File("StillLife.exr")

OpenEXR.Part
------------

An EXR file consists of a list called ``parts`` of one or more parts,
which the OpenEXR python module represents with the ``Part`` object. A
part consists of a dictionary called ``header`` that holds the
attribute metadata, and a dictionary called ``channels`` that hold the
pixel data.

.. code-block::

    >>> exrfile = OpenEXR.File("StillLife.exr")
    >>> part = exrfile.parts[0]
    >>> part.height()
    846
    >>> part.width()
    1240

    >>> for name,value in part.header.items():
    ...   print(name, value)
    ... 
    capDate 2002:06:23 21:30:10
    channels [Channel("A", xSampling=1, ySampling=1), Channel("B", xSampling=1, ySampling=1), Channel("G", xSampling=1, ySampling=1), Channel("R", xSampling=1, ySampling=1)]
    compression Compression.PIZ_COMPRESSION
    dataWindow (array([0, 0], dtype=int32), array([1239,  845], dtype=int32))
    displayWindow (array([0, 0], dtype=int32), array([1239,  845], dtype=int32))
    lineOrder LineOrder.INCREASING_Y
    owner Copyright 2002 Industrial Light & Magic
    pixelAspectRatio 1.0
    preview PreviewImage(100, 68)
    screenWindowCenter [0. 0.]
    screenWindowWidth 0.44999998807907104
    type Storage.scanlineimage
    utcOffset 25200.0

    >>> for name,channel in part.channels.items():
    ...   print(name, channel, channel.pixels.shape, channel.pixels.dtype)
    ... 
    RGBA Channel("RGBA", xSampling=1, ySampling=1) (846, 1240, 4) float16

Since many common EXR files have only a single part, for convenience,
the ``File`` object has ``header()`` and ``channels()`` methods that

Header Metadata
---------------

The ``File`` object's ``header()`` method returns a dictionary holding
the file's metadata.  The dictionary key is the metadata attribute
name, and the dictionary value is an object holding the attribute value.

An EXR file header can store metadata attributes with any name, but see
:ref:`Standard Attributes` for a complete description of the standard
attributes in an EXR file, both required and optional, which have
strictly enforced types.

Supported types of metadata are:

* string
* list of strings
* integer
* float
* list of floats  
* V2i, V2f, V2d, V3i, V3f, V3d - 2D and 3D vectors, represented as 2x1
  or 3x1 numpy arrays with a ``dtype`` of ``int32``, ``float32``, or ``float64``.
* M33f, M33d, M44f, M44d - 3x3 or 4x4 matrices, represented as 3x3 or
  4x4 numpy arrays with a ``dtype`` of ``float32`` or ``float64``.
* Box2i, Box2f - bounding boxes, represented as tuples of numpy arrays (``min`` and
  ``max``) with a ``dtype`` of ``int32`` or ``float32``.

The OpenEXR module has enumerated types for certain attributes:

.. raw:: html

   <embed>
   <table>
   
   <tr>
     <td style="vertical-align: top; width:150px; font-weight:bold"> attribute name</td>
     <td style="vertical-align: top; width:100px; font-weight:bold"> type </td>
     <td style="vertical-align: top; width:500px; font-weight:bold"> values </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> type </tt> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> OpenEXR.Storage </tt> </td>
     <td style="vertical-align: top; width:500px">
         <ul>
           <li> <tt> OpenEXR.scanlineimage </tt>
           <li> <tt> OpenEXR.tiledimage </tt>
           <li> <tt> OpenEXR.deepscanline </tt>
           <li> <tt> OpenEXR.deeptile </tt>
           <li> <tt> OpenEXR.NUM_STORAGETYPES </tt>
         </ul>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> lineOrder </tt> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> OpenEXR.LineOrder </tt> </td>
     <td style="vertical-align: top; width:500px">
         <ul>
           <li> <tt> OpenEXR.INCREASING_Y </tt>
           <li> <tt> OpenEXR.DECREASING_Y </tt>
           <li> <tt> OpenEXR.RANDOM_Y </tt>
           <li> <tt> OpenEXR.NUM_LINE_ORDERS </tt>
         </ul>
     </td>
   </tr>

   <tr>
     <td style="vertical-align: top; width:150px">
       <p> <tt> compression </tt> </p>
     </td>
     <td style="vertical-align: top; width:100px"> <tt> OpenEXR.Compression </tt> </td>
     <td style="vertical-align: top; width:500px">
         <ul>
           <li> <tt> OpenEXR.NO_COMPRESSION </tt> 
           <li> <tt> OpenEXR.RLE_COMPRESSION </tt>
           <li> <tt> OpenEXR.ZIPS_COMPRESSION </tt>
           <li> <tt> OpenEXR.ZIP_COMPRESSION </tt> 
           <li> <tt> OpenEXR.PIZ_COMPRESSION </tt> 
           <li> <tt> OpenEXR.PXR24_COMPRESSION </tt>
           <li> <tt> OpenEXR.B44_COMPRESSION </tt>
           <li> <tt> OpenEXR.B44A_COMPRESSION </tt>
           <li> <tt> OpenEXR.DWAA_COMPRESSION </tt>
           <li> <tt> OpenEXR.DWAB_COMPRESSION </tt> 
           <li> <tt> OpenEXR.NUM_COMPRESSION_METHODS </tt> 
         </ul>
     </td>
   </tr>

   </table>
   </embed>


The ``dataWindow`` Attribute and Image Size
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``dataWindow`` attribute is especially important. Its size matches
the shape of the channel pixel arrays. The ``min`` of the
``dataWindow`` attribute specifies the row/column coordinate of the
pixel at the origin of the image. However, the numpy arrays holding
the pixel data are *not* offset by this value.

.. code-block::
              
    >>> min,max = exrfile.header()["dataWindow"]
    >>> height = max[1] - min[1] + 1
    >>> width = max[0] - min[0] + 1
    >>> height,width
    (846, 1240)

OpenEXR.Channel
---------------

The ``channels()`` method of the ``File`` object returns a dictionary
holding pixel data. The key is the channel name and the value is a
``Channel`` object.

The ``Channel`` object has a ``pixels`` field that is a 2D numpy array
holding the pixel data. Supported types are ``uint32``, ``float16``,
and ``float32``.

For parts that contain RGB data, where the file contains separate
``R``, ``G``, ``B``, and optionally ``A`` channels, the channels
dictionary holds a single channel named ``RGB`` and with a numpy array
of shape ``(height, width, 3)``, or ``RGBA`` and a numpy array of
shape ``(height, width, 4)`` if there is alpha.

All channels within a part have the same width and height, and thus
the same pixel array shape. 

For single-part files, ``channels()`` returns the image channels for the
file. For multi-part files, ``channels()`` takes a part number as
argument and returns the channels for that part:

.. code-block::

    >>> for p in range(len(exrfile.parts)):
    ...   for name,channel in exrfile.channels(p).items():
    ...     print(name, channel.pixels.shape, channel.pixels.dtype)
    ... 
    RGBA (846, 1240, 4) float16
       

The channel object also has ``xSampling``, ``ySampling``, and
``pLinear`` fields that hold the channel’s subsampling values and
plinear setting used for DWA compression. The default sampling values
are 1 and are only used for luminance subsampling.

Pixel Arrays
~~~~~~~~~~~~

The first dimension of a channel's ``pixels`` array is the image
height. The second dimension is the image width.  All channels of a
part must have the same width and height. It's an error to create or
write a File object with channels of different shapes. The ``Part``
object has ``height()`` and ``width()`` methods that return the image
dimension. You can, of course, query the dimension of a channel via
the pixel array itself.

.. code-block::

    >>> part = exrfile.parts[0]
    >>> dw = part.header['dataWindow']
    >>> height = part.height()
    >>> width = part.width()
    >>> for name,channel in part.channels.items():
    ...   print(name, channel.pixels.shape, height, width, dw)
    ... 
    (846, 1240, 4) 846 1240 (array([0, 0], dtype=int32), array([1239,  845], dtype=int32))
       
The ``File`` object allocates space for the pixel arrays upon
read. There is no mechanism to provide memory addresses for the pixel
arrays.

Pixel Array Data Layout
~~~~~~~~~~~~~~~~~~~~~~~

Although the OpenEXR file format supports channels of arbitrary name
and number of type ``uint32``, ``float16``, and ``float32``, most
programs working with OpenEXR files expect this data to represent
pixels, so it's more convenient to group ``R``, ``G``, ``B``, and
``A`` channels together. By default, the ``File`` object does this and
returns a channel with name ``RGB`` and a numpy array of shape
``(height, width, 3)``, or ``RGBA`` and a numpy array of shape
``(height, width, 4)`` if there is alpha.

The ``File()`` object constructor takes an optional
``separate_channels`` argument, ``False`` by default, but if ``True``,
it skips the channel grouping and returns each channel as a separate
2D numpy array.

Tiled Images
------------

The ``File`` object reads tiled EXR images into pixel arrays just the
same as scanline images.

Although the EXR format supports multiple tile levels, currently, the
API provides no access to these levels.

Deep Images
-----------

Deep EXR files store an arbitrary number of data values per pixel. For
deep parts, the ``Channel`` object's ``pixels`` array has a ``dtype``
of ``object``, which is in turn a 1D numpy array holding the deep samples
for that pixel. Supported types for the deep sample array are
``uint32``, ``float16``, and ``float32``.

If the deep sample array object for a given pixel is ``None``, there
are no samples for that pixel.

.. code-block::
   
    RGB = infile.channels()["RGB"].pixels
    height, width = R.shape
    for y in range(height):
        for x in range(width):
            if R[y,x].dtype == None:
                print(f"No samples for pixel {y},{x}")
            else:
                for i in range(RGB[y,x].shape(0)):
                    print(f"pixel {y},{x} sample[{i}]: {RGB[y,x]}") 
                    
All channel within a given deep part must have the same number of
samples, so the deep sample arrays for all channels have the same size
and shape.

Writing EXR Files with OpenEXR.File
===================================

To write an EXR file, construct a ``File`` object and call the
``write()`` method.

For single-part files, the ``File`` object constructor takes a
dictionary for the header and a dictionary for the channels.

Construct the channels dict with values that are either numpy arrays
or ``Channel`` objects if you need to specify the ``xSampling``,
``ySampling``, or ``pLinear`` values.

The channel pixel arrays must have a ``dtype`` of ``uint32``,
``float16``, or ``float32``.

All channel pixel arrays within a given part must have the same
dimensions. The ``write`` method will throw an exception if they are
not.

.. code-block::

    height, width = (20, 10)
    RGB = np.random.rand(height, width, 3).astype('f')
    channels = { "RGB" : RGB }
    header = { "compression" : OpenEXR.ZIP_COMPRESSION,
               "type" : OpenEXR.scanlineimage }

    with OpenEXR.File(header, channels) as outfile:
        outfile.write("test.exr")

Writing Multi-Part EXR Files
----------------------------

For multi-part images, pass the ``File`` constructor a list of
``Part`` objects, each of which holds the header and channels dicts.

.. code-block::

    height, width = (20, 10)
    Z0 = np.zeros((height, width), dtype='float32')
    Z1 = np.ones((height, width), dtype='ffloat32')

    P0 = OpenEXR.Part({}, {"Z" : Z0 })
    P1 = OpenEXR.Part({}, {"Z" : Z1 })

    f = OpenEXR.File([P0, P1])
    f.write("readme_2part.exr")

    with OpenEXR.File("multipart.exr") as o:
        assert o.parts[0].name() == "Part0"
        assert o.parts[0].width() == 10
        assert o.parts[0].height() == 20
        assert o.parts[1].name() == "Part1"
        assert o.parts[1].width() == 10
        assert o.parts[1].height() == 20

Writing Tiled EXR Files
-----------------------

To write a tiled image, set the ``type`` header attribute to
``OpenEXR.tiledimage`` and the ``tiles`` header attribute to an object
of type ``OpenEXR.TileDescription`` with the appropriate settings.

.. code-block::

    height, width = (20, 10)

    Z = np.zeros((height, width), dtype='f')
    channels = { "Z" : Z }
    header = {        "type" : OpenEXR.tiledimage,
                     "tiles" : OpenEXR.TileDescription(),
               "compression" : OpenEXR.ZIPSCOMPRESSION }

    with OpenEXR.File(channels, header) as exrfile:
        exrfile.write("tiled.exr")


Writing Deep EXR Files
----------------------

For deep images, the channel pixel arrays must have a ``dtype`` of
``object``, or ``None`` for pixels with no samples.  The object must
be a numpy array with a ``dtype`` of ``uint32``, ``float16``, or
``float32``.

.. code-block::

    height, width = (20, 10)

    Z = np.empty((height, width), dtype=object)
    for y in range(height):
        for x in range(width):
            Z[y, x] = np.array([y*width+x], dtype='float32')

    channels = { "Z" : Z }
    header = { "compression" : OpenEXR.ZIPS_COMPRESSION,
               "type" : OpenEXR.deepscanline }
    with OpenEXR.File(header, channels) as outfile:
        outfile.write("deep.exr")

All deep pixel arrays within a given part must have the same number of
samples, so the pixel arrays must have the same size and shape.  The
``write`` method will throw an exception if they are not.

