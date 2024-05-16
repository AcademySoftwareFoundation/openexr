<!-- SPDX-License-Identifier: BSD-3-Clause -->
<!-- Copyright (c) Contributors to the OpenEXR Project -->

[![License](https://img.shields.io/github/license/AcademySoftwareFoundation/openexr)](https://github.com/AcademySoftwareFoundation/openexr/blob/main/LICENSE.md)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/2799/badge)](https://bestpractices.coreinfrastructure.org/projects/2799)
[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/AcademySoftwareFoundation/openexr/badge)](https://securityscorecards.dev/viewer/?uri=github.com/AcademySoftwareFoundation/openexr)
[![Build Status](https://github.com/AcademySoftwareFoundation/openexr/workflows/CI/badge.svg)](https://github.com/AcademySoftwareFoundation/openexr/actions?query=workflow%3ACI)
[![Analysis Status](https://github.com/AcademySoftwareFoundation/openexr/workflows/Analysis/badge.svg)](https://github.com/AcademySoftwareFoundation/openexr/actions?query=workflow%3AAnalysis)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=AcademySoftwareFoundation_openexr&metric=alert_status)](https://sonarcloud.io/dashboard?id=AcademySoftwareFoundation_openexr)

# OpenEXR

OpenEXR provides the specification and reference implementation of the
EXR file format, the professional-grade image storage format of the
motion picture industry.

The purpose of EXR format is to accurately and efficiently represent
high-dynamic-range scene-linear image data and associated metadata,
with strong support for multi-part, multi-channel use cases.

OpenEXR is widely used in host application software where accuracy is
critical, such as photorealistic rendering, texture access, image
compositing, deep compositing, and DI.

## OpenEXR Project Mission

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

## Python Module

The OpenEXR python module provides full support for reading and
writing all types of ``.exr`` image files, including scanline, tiled,
deep, mult-part, multi-view, and multi-resolution images with pixel
types of unsigned 32-bit integers and 16- and 32-bit floats. It
provides access to pixel data through numpy arrays, as either one
array per channel or with R, G, B, and A interleaved into a single
array RGBA array.

## Project Governance

OpenEXR is a project of the [Academy Software
Foundation](https://www.aswf.io). See the project's [governance
policies](https://github.com/AcademySoftwareFoundation/openexr/blob/main/GOVERNANCE.md), [contribution guidelines](https://github.com/AcademySoftwareFoundation/openexr/blob/main/CONTRIBUTING.md), and [code of conduct](https://github.com/AcademySoftwareFoundation/openexr/blob/main/CODE_OF_CONDUCT.md)
for more information.

# Quick Start

The "Hello, World" image writer:

    import OpenEXR
    import numpy as np
    import random

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

Or alternatively, construct the same output file via a single RGB pixel array:

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

The corresponding example of reading an image is:

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

Or alternatively, read the data as a single RGBA array:

    infile = OpenEXR.File("readme.exr", rgba=True)

    RGB = infile.channels()["RGB"].pixels
    width = RGB.shape[1]
    height = RGB.shape[0]
    for y in range(0, height):
        for x in range(0, width):
            print(f"pixel[{y}][{x}]=({RGB[y][x][0]}, {RGB[y][x][1]}, {RGB[y][x][2]})")

To modify the header metadata in a file:

    f = OpenEXR.File("readme.exr")
    f.header()["displayWindow"] = OpenEXR.Box2i(OpenEXR.V2i(3,4),
                                                OpenEXR.V2i(5,6))
    f.header()["comments"] = "test image"
    f.header()["longitude"] = -122.5
    f.write("readme_modified.exr")

To read and write a multi-part file, use a list of ``Part`` objects:

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
    assert o.parts[0].width() == 10
    assert o.parts[0].height() == 20
    assert np.array_equal(o.parts[0].channels["Z"].pixels, Z0)
    assert o.parts[1].name() == "Part1"
    assert o.parts[1].width() == 10
    assert o.parts[1].height() == 20
    assert np.array_equal(o.parts[1].channels["Z"].pixels, Z1)

# Community

* **Ask a question:**

  - Email: openexr-dev@lists.aswf.io

  - Slack: [academysoftwarefdn#openexr](https://academysoftwarefdn.slack.com/archives/CMLRW4N73)

* **Attend a meeting:**

  - Technical Steering Committee meetings are open to the
    public, fortnightly on Thursdays, 1:30pm Pacific Time.

  - Calendar: https://lists.aswf.io/g/openexr-dev/calendar

  - Meeting notes: https://wiki.aswf.io/display/OEXR/TSC+Meetings

* **Report a bug:**

  - Submit an Issue: https://github.com/AcademySoftwareFoundation/openexr/issues

* **Report a security vulnerability:**

  - Email to security@openexr.com

* **Contribute a Fix, Feature, or Improvement:**

  - Read the [Contribution Guidelines](https://github.com/AcademySoftwareFoundation/openexr/blob/main/CONTRIBUTING.md) and [Code of Conduct](https://github.com/AcademySoftwareFoundation/openexr/blob/main/CODE_OF_CONDUCT.md)

  - Sign the [Contributor License
    Agreement](https://contributor.easycla.lfx.linuxfoundation.org/#/cla/project/2e8710cb-e379-4116-a9ba-964f83618cc5/user/564e571e-12d7-4857-abd4-898939accdd7)

  - Submit a Pull Request: https://github.com/AcademySoftwareFoundation/openexr/pulls

# Resources

- Website: http://www.openexr.com
- Technical documentation: https://openexr.readthedocs.io
- Porting help: [OpenEXR/Imath Version 2.x to 3.x Porting Guide](https://openexr.readthedocs.io/en/latest/PortingGuide.html)
- Reference images: https://github.com/AcademySoftwareFoundation/openexr-images
- Security policy: [SECURITY.md](https://github.com/AcademySoftwareFoundation/openexr/blob/main/SECURITY.md)
- Release notes: [CHANGES.md](https://github.com/AcademySoftwareFoundation/openexr/blob/main/CHANGES.md)
- Contributors: [CONTRIBUTORS.md](https://github.com/AcademySoftwareFoundation/openexr/blob/main/CONTRIBUTORS.md)  

# License

OpenEXR is licensed under the [BSD-3-Clause license](https://github.com/AcademySoftwareFoundation/openexr/blob/main/LICENSE.md).


