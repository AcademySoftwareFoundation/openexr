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

    # Generate a 3D NumPy array for RGB channels with random values
    height, width = (20, 10)
    RGB = np.random.rand(height, width, 3).astype('f')

    channels = { "RGB" : RGB }
    header = { "compression" : OpenEXR.ZIP_COMPRESSION,
               "type" : OpenEXR.scanlineimage }

    with OpenEXR.File(header, channels) as outfile:
        outfile.write("readme.exr")

Or alternatively, construct the same output file via separate pixel arrays
for each channel:

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

The corresponding example of reading an image is:

    with OpenEXR.File("readme.exr") as infile:

        RGB = infile.channels()["RGB"].pixels
        height, width, _ = RGB.shape
        for y in range(height):
            for x in range(width):
                pixel = tuple(RGB[y, x])
                print(f"pixel[{y}][{x}]={pixel}")

Or alternatively, read the data as separate arrays for each channel:

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

To modify the header metadata in a file:

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

Note that OpenEXR's Imath-based vector and matrix attribute values
appear in the header dictionary as 2-element, 3-element, 3x3, 4x4
numpy arrays, and bounding boxes appear as tuples of 2-element arrays,
or tuples for convenience.

To read and write a multi-part file, use a list of ``Part`` objects:

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

Deep data is stored in a numpy array whose entries are numpy
arrays. Construct a numpy array with a ``dtype`` of ``object``, and
assign each entry a numpy array holding the samples. Each pixel can
have a different number of samples, including ``None`` for no data,
but all channels in a given part must have the same number of samples.

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

To read a deep file:

    with OpenEXR.File("readme_test_tiled_deep.exr") as infile:

        Z = infile.channels()["Z"].pixels
        height, width = Z.shape
        for y in range(height):
            for x in range(width):
                for z in Z[y,x]:
                    print(f"deep sample at {y},{x}: {z}")


# Community

* **Ask a question:**

  - Email: openexr-dev@lists.aswf.io

  - Slack: [academysoftwarefdn#openexr](https://academysoftwarefdn.slack.com/archives/CMLRW4N73)

* **Attend a meeting:**

  - Technical Steering Committee meetings are open to the
    public, fortnightly on Thursdays, 1:30pm Pacific Time.

  - Calendar: https://zoom-lfx.platform.linuxfoundation.org/meetings/openexr

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


