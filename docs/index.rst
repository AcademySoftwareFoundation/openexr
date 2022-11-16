..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. toctree::
   :hidden:
      
   about
   technical
   contributing
   governance
   download
   charter
   license
   faq
      
+------------------------------+------------------------------------------------+------------------------------------------------------------------+----------------------------------+--------------------------------+----------------------------+
| :ref:`About <About OpenEXR>` | :ref:`Documentation <Technical Documentation>` | `GitHub <https://github.com/AcademySoftwareFoundation/openexr>`_ | :ref:`Contribute <contributing>` | :ref:`Governance <governance>` | :ref:`Download <Download>` |
+------------------------------+------------------------------------------------+------------------------------------------------------------------+----------------------------------+--------------------------------+----------------------------+

Overview
========

.. sidebar:: OpenEXR

     .. image:: technical/images/windowExample1.png

OpenEXR provides the specification and reference implementation of the
EXR file format, the professional-grade image storage format of the
motion picture industry.

The purpose of EXR format is to accurately and efficiently represent
high-dynamic-range scene-linear image data and associated metadata,
with strong support for multi-part, multi-channel use cases.

OpenEXR is widely used in host application software where accuracy is
critical, such as photorealistic rendering, texture access, image
compositing, deep compositing, and DI.

OpenEXR is a project of the `Academy Software Foundation
<https://www.aswf.io>`_.


Imath
-----

The OpenEXR project includes `Imath <https://imath.readthedocs.io>`_,
a basic, light-weight, and efficient C++ representation of 2D and 3D
vectors and matrices and other simple but useful mathematical objects,
functions, and data types common in computer graphics applications,
including the “half” 16-bit floating-point type.

Imath also includes optional python bindings for all types and
functions, including optimized implementations of vector and matrix
arrays.

OpenEXR Project Mission
-----------------------

The goal of the OpenEXR project is to keep the EXR format reliable and
modern and to maintain its place as the preferred image format for
entertainment content creation. 

Major revisions are infrequent, and new features will be carefully
weighed against increased complexity.  The principal priorities of the
project are:

* Robustness, reliability, security
* Backwards compatibility, data longevity
* Performance - read/write/compression/decompression time
* Simplicity, ease of use, maintainability
* Wide adoption, multi-platform support - Linux, Windows, macOS, and others

OpenEXR is intended solely for 2D data. It is not appropriate for
storage of volumetric data, cached or lit 3D scenes, or more complex
3D data such as light fields.

The goals of the Imath project are simplicity, ease of use,
correctness and verifiability, and breadth of adoption. Imath is not
intended to be a comprehensive linear algebra or numerical analysis
package.

Community
=========

* **Ask for help:**

  - Email: `openexr-dev@lists.aswf.io <https://lists.aswf.io/g/openexr-dev>`_

  - Slack: `academysoftwarefdn#openexr <https://academysoftwarefdn.slack.com/archives/CMLRW4N73>`_

* **Attend a meeting:**

  - Technical Steering Committee meetings are open to the
    public, fortnightly on Thursdays, 1:30pm Pacific Time.

  - Calendar: https://lists.aswf.io/g/openexr-dev/calendar

* **Report a bug:**

  - GitHub: https://github.com/AcademySoftwareFoundation/openexr/issues

* **Make a contribution:**

  - Read the :ref:`contribution guidelines <contributing>`

  - Submit a PR: https://github.com/AcademySoftwareFoundation/openexr/pulls

* **Report a security vulnerability:**

  - Send email to security@openexr.com

Resources
=========

- Porting help: :ref:`OpenEXR/Imath Version 2.x to 3.x Porting Guide <porting>`
- Reference images: https://github.com/AcademySoftwareFoundation/openexr-images
- Security Policy: `SECURITY.md <https://github.com/AcademySoftwareFoundation/openexr/blob/main/SECURITY.md>`_

License
=======

OpenEXR is licensed under the :ref:`BSD-3-Clause license <license>`. 

  
