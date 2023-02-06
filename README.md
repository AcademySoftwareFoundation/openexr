[![License](https://img.shields.io/github/license/AcademySoftwareFoundation/openexr)](LICENSE.md)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/2799/badge)](https://bestpractices.coreinfrastructure.org/projects/2799)
[![Build Status](https://github.com/AcademySoftwareFoundation/openexr/workflows/CI/badge.svg)](https://github.com/AcademySoftwareFoundation/openexr/actions?query=workflow%3ACI)
[![Analysis Status](https://github.com/AcademySoftwareFoundation/openexr/workflows/Analysis/badge.svg)](https://github.com/AcademySoftwareFoundation/openexr/actions?query=workflow%3AAnalysis)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=AcademySoftwareFoundation_openexr&metric=alert_status)](https://sonarcloud.io/dashboard?id=AcademySoftwareFoundation_openexr)

# OpenEXR

<img align="right" src="docs/technical/images/windowExample1.png">

OpenEXR provides the specification and reference implementation of the
EXR file format, the professional-grade image storage format of the
motion picture industry.

The purpose of EXR format is to accurately and efficiently represent
high-dynamic-range scene-linear image data and associated metadata,
with strong support for multi-part, multi-channel use cases.

OpenEXR is widely used in host application software where accuracy is
critical, such as photorealistic rendering, texture access, image
compositing, deep compositing, and DI.

OpenEXR is a project of the [Academy Software
Foundation](https://www.aswf.io). See the project's [governance policies](https://openexr.readthedocs.io/en/latest/goverance) and [code of conduct](https://openexr.readthedocs.io/en/latest/code_of_conduct)
for more information.

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

# Quick Start

OpenEXR builds on Linux, macOS, Microsoft Windows. The instructions
below are the basics. See the
[build](https://openexr.readthedocs.io/en/latest/build) for more
details and options.

## Prerequisites

Building OpenEXR from C++/C source requires:

* CMake version 3.12 or newer
* C++ compiler that supports C++11 or later
* zlib 
* Imath (auto-fetched by CMake if not found)

## Linux/macOS

To build via CMake, you need to first identify three directories:

1. The source directory, i.e. the top-level directory of the
   downloaded source archive or cloned repo, referred to below as ``$srcdir``
2. A temporary directory to hold the build artifacts, referred to below as
   ``$builddir``
3. A destination directory into which to install the
   libraries and headers, referred to below as ``$installdir``.  

To build:

    % cd $builddir
    % cmake $srcdir --install-prefix $installdir
    % cmake --build $builddir --target install --config Release

See the CMake Configuration Options section below for the most common
configuration options especially the install directory. Note that with
no arguments, as above, ``make install`` installs the header files in
``/usr/local/include``, the object libraries in ``/usr/local/lib``, and the
executable programs in ``/usr/local/bin``.

See the [build](https://openexr.readthedocs.io/en/latest/build)
documentation for more configuation options.

## Windows Quick Start

Under Windows, if you are using a command line-based setup, such as
cygwin, you can of course follow the above. For Visual Studio, cmake
generators are "multiple configuration", so you don't even have to set
the build type, although you will most likely need to specify the
install location.  Install Directory By default, ``make install``
installs the headers, libraries, and programs into ``/usr/local``, but you
can specify a local install directory to cmake via the
``CMAKE_INSTALL_PREFIX`` variable:

    % cmake .. -DCMAKE_INSTALL_PREFIX=$openexr_install_directory

See the [build](https://openexr.readthedocs.io/en/latest/build)
documentation for more configuation options.

## A Simple Program

A simple program to print the size of an exr image:

    #include <ImfInputFile.h>
    #include <ImfHeader.h>
    #include <ImathBox.h>
    
    #include <iostream>
    
    int
    main(int argc, char *argv[])
    {
        if (argc < 2)
        {
            std::cerr << "Usage: exrexample <file.exr>" << std::endl;
            return -1;
        }
        
        try {
            
            Imf::InputFile file(argv[1]);
            Imath::Box2i dw = file.header().dataWindow();
            int width = dw.max.x - dw.min.x + 1;
            int height = dw.max.y - dw.min.y + 1;
    
            std::cout << argv[1] << ": " << width << "x" << height << std::endl;
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    
        return 0;
    }

The CMakeLists.txt file:

    cmake_minimum_required(VERSION 3.10)
    project(exrexample)
    find_package(OpenEXR REQUIRED)
    add_executable(${PROJECT_NAME} main.cpp)
    target_link_libraries(${PROJECT_NAME} OpenEXR::OpenEXR)

To build:

    #/bin/bash
    export CMAKE_PREFIX_PATH=<openexr install directory>
    mkdir _build
    cmake -S . -B _build
    cmake --build _build

# Community

* **Ask a question:**

  - Email: openexr-dev@lists.aswf.io

  - Slack: [academysoftwarefdn#openexr](https://academysoftwarefdn.slack.com/archives/CMLRW4N73)

* **Report a bug:**

  - GitHub: https://github.com/AcademySoftwareFoundation/openexr/issues

* **Report a security vulnerability:**

  - Send email to security@openexr.com

* **Get involved:**

  - Technical Steering Committee meetings are open to the public,
    fortnightly on Thursdays, 1:30pm Pacific Time.  See the
    [ASWF](https://lists.aswf.io/g/openexr-dev/calendar) calendar for
    the Zoom link.

  - Read the [contribution guidelines](https://openexr.readthedocs.io/en/latest/contributing)

  - Submit a PR: https://github.com/AcademySoftwareFoundation/openexr/pulls

# Resources

- Website: http://www.openexr.com
- Technical Documentation: https://openexr.readthedocs.io
- Porting help: [OpenEXR/Imath Version 2.x to 3.x Porting Guide](https://openexr.readthedocs.io/en/latest/porting)
- Reference images: https://github.com/AcademySoftwareFoundation/openexr-images
- Security Policy: [SECURITY.md](SECURITY.md)

# License

OpenEXR is licensed under the [BSD-3-Clause license](LICENSE.md).


---

![aswf](/ASWF/images/aswf.png)
