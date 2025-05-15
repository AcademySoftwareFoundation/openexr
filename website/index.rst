..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _OpenEXR:

OpenEXR
#######

.. toctree::
   :caption: OpenEXR
   :maxdepth: 1

.. sidebar:: 

     .. image:: images/windowExample1.small.png

OpenEXR provides the specification and reference implementation of the
EXR file format, the professional-grade image storage format of the
motion picture industry.

The purpose of EXR format is to accurately and efficiently represent
high-dynamic-range scene-linear image data. This is a significant difference to
most image formats, which store images that are ready for display. Software that
handles OpenEXR images may need to process them differently to images in other
formats such as JPEG (see :doc:`SceneLinear` for more details). OpenEXR files
have strong support for multi-part, multi-channel use cases, and extensive
representation of associated metadata.

OpenEXR is widely used in host application software where accuracy is
critical, such as photorealistic rendering, texture access, image
compositing, deep compositing, and DI.

OpenEXR is a project of the `Academy Software Foundation
<https://www.aswf.io>`_.

.. include:: latest_news_title.rst

Latest News
===========

.. image:: images/news.png
   :width: 50
   :height: 50
   :align: left

|latest-news-title|

.. highlights::

   .. include:: news.rst
      :start-after: .. _LatestNewsStart:
      :end-before: .. _LatestNewsEnd:

Imath
=====

The OpenEXR project includes `Imath <https://imath.readthedocs.io>`_,
a basic, light-weight, and efficient C++ representation of 2D and 3D
vectors and matrices and other simple but useful mathematical objects,
functions, and data types common in computer graphics applications,
including the `half
<https://imath.readthedocs.io/en/latest/classes/half.html>`_ 16-bit
floating-point type.

Imath also includes optional python bindings for all types and
functions, including optimized implementations of vector and matrix
arrays.

Quick Start
===========

For a simple program that uses the C++ API to read and write a ``.exr`` file, see the
:doc:`HelloWorld` examples.

Community
=========

* **Ask a question:**

  - Email: `openexr-dev@lists.aswf.io <https://lists.aswf.io/g/openexr-dev>`_

  - Slack: `academysoftwarefdn#openexr <https://academysoftwarefdn.slack.com/archives/CMLRW4N73>`_

* **Attend a meeting:**

  - Technical Steering Committee meetings are open to the
    public, fortnightly on Thursdays, 1:30pm Pacific Time.

  - Calendar: https://zoom-lfx.platform.linuxfoundation.org/meetings/openexr

  - Meeting Notes: https://wiki.aswf.io/display/OEXR/TSC+Meetings

* **Report a bug:**

  - Submit an Issue: https://github.com/AcademySoftwareFoundation/openexr/issues

* **Report a security vulnerability:**

  - Email security@openexr.com

* **Contribute a Fix, Feature, or Improvement:**

  - Read the `Contribution Guidelines
    <https://github.com/AcademySoftwareFoundation/openexr/blob/main/CONTRIBUTING.md>`_
    and `Code of Conduct
    <https://github.com/AcademySoftwareFoundation/openexr/blob/main/CODE_OF_CONDUCT.md>`_

  - Sign the `Contributor License Agreement
    <https://contributor.easycla.lfx.linuxfoundation.org/#/cla/project/2e8710cb-e379-4116-a9ba-964f83618cc5/user/564e571e-12d7-4857-abd4-898939accdd7>`_
  
  - Submit a Pull Request: https://github.com/AcademySoftwareFoundation/openexr/pulls

Resources
=========

- Reference images: https://github.com/AcademySoftwareFoundation/openexr-images
- Security policy: `SECURITY.md <https://github.com/AcademySoftwareFoundation/openexr/blob/main/SECURITY.md>`_
- Release notes: `CHANGES.md
  <https://github.com/AcademySoftwareFoundation/openexr/blob/main/CHANGES.md>`_
- Contributors: `CONTRIBUTORS.md <https://github.com/AcademySoftwareFoundation/openexr/blob/main/CONTRIBUTORS.md>`_
- Porting Guide: :doc:`PortingGuide`
  
.. include:: toc_redirect.rst

