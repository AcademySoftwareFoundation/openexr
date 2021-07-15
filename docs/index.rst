.. Imath documentation master file, created by
   sphinx-quickstart on Wed Apr 24 15:19:01 2019.

OpenEXR Technical Documentation
===============================

.. sidebar:: OpenEXR

     .. image:: images/windowExample1.png

OpenEXR provides the specification and reference implementation of the
EXR file format, the professional-grade image storage format of the
motion picture industry.

The purpose of EXR format is to accurately and efficiently represent
high-dynamic-range scene-linear image data and associated metadata,
with strong support for multi-part, multi-channel use cases.

OpenEXR is widely used in host application software where accuracy is
critical, such as photorealistic rendering, texture access, image
compositing, deep compositing, and DI.

OpenEXR Features

* High dynamic range and color precision.
* Support for 16-bit floating-point, 32-bit floating-point, and
  32-bit integer pixels.
* Multiple image compression algorithms, both lossless and lossy. Some of
  the included codecs can achieve 2:1 lossless compression ratios on images
  with film grain.  The lossy codecs have been tuned for visual quality and
  decoding performance.
* Extensibility. New compression codecs and image types can easily be added
  by extending the C++ classes included in the OpenEXR software distribution.
  New image attributes (strings, vectors, integers, etc.) can be added to
  OpenEXR image headers without affecting backward compatibility with
  existing OpenEXR applications. 
* Support for stereoscopic image workflows and a generalization
  to multi-views.
* Flexible support for deep data: pixels can store a variable-length list
  of samples and, thus, it is possible to store multiple values at different
  depths for each pixel. Hard surfaces and volumetric data representations
  are accommodated.
* Multipart: ability to encode separate, but related, images in one file.
  This allows for access to individual parts without the need to read other
  parts in the file.
* Versioning: OpenEXR source allows for user configurable C++
  namespaces to provide protection when using multiple versions of the
  library in the same process space.

Technical Documents
###################

.. toctree::
   :maxdepth: 1

   TechnicalIntroduction
   ReadingAndWritingImageFiles
   OpenEXRCoreAPI
   OpenEXRFileLayout
   MultiViewOpenEXR
   InterpretingDeepPixels
   StandardOptionalAttributes
              
* :ref:`genindex`

Resources
#########

- Download: https://github.com/AcademySoftwareFoundation/openexr
- Install Help: `INSTALL.md <https://github.com/AcademySoftwareFoundation/openexr/blob/master/INSTALL.md>`_
- Porting Help: `Imath/OpenEXR Version 2->3 Porting Guide <https://github.com/AcademySoftwareFoundation/Imath/blob/master/docs/PortingGuide2-3.md>`_
- License: `BSD License <https://github.com/AcademySoftwareFoundation/openexr/blob/master/LICENSE.md>`_
- Reference images: https://github.com/AcademySoftwareFoundation/openexr-images

About OpenEXR
#############

OpenEXR is a project of the `Academy Software Foundation
<https://www.aswf.io>`_.  The format and library were originally
developed by Industrial Light & Magic and first released in 2003.
Weta Digital, Walt Disney Animation Studios, Sony Pictures Imageworks,
Pixar Animation Studios, DreamWorks, and other studios, companies, and
individuals have made contributions to the code base.

OpenEXR is included in the `VFX Reference Platform <https://vfxplatform.com>`_.

                  
