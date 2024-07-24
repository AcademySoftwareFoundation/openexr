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

The OpenEXR python module provides rudimentary support for reading and
writing basic scanline image data. Many features of the file format
are not yet supported, including:

- Writing of tiled images
- Multiresoltion images
- Deep image data
- Some attribute types
- Nonunity channel sampling frequencies
- No support for interleaved channel data

## Project Governance

OpenEXR is a project of the [Academy Software
Foundation](https://www.aswf.io). See the project's [governance
policies](https://github.com/AcademySoftwareFoundation/openexr/blob/main/GOVERNANCE.md), [contribution guidelines](https://github.com/AcademySoftwareFoundation/openexr/blob/main/CONTRIBUTING.md), and [code of conduct](https://github.com/AcademySoftwareFoundation/openexr/blob/main/CODE_OF_CONDUCT.md)
for more information.

# Quick Start

<!-- this code is replicated as a test in -->
<!-- src/wrappers/python/test/test_readme.py -->

The "hello, world" image writer:

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


