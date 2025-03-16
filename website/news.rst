..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _News:

.. _release notes: https://github.com/AcademySoftwareFoundation/openexr/blob/main/CHANGES.md
.. _imath release notes: https://github.com/AcademySoftwareFoundation/Imath/blob/main/CHANGES.md

News
####

.. toctree::
   :caption: News

.. include:: latest_news_title.rst

|latest-news-title|
===============================================================

.. _LatestNewsStart:

We have a proposal that adds support for lossless JPEG 2000 coding (as
the HT256 compressor) to OpenEXR and would welcome your feedback.

The HT256 compressor uses the High-Throughput (HT) block coder and
promises high speed and high coding efficiency, and it supports the
full range of OpenEXR features, including 32-bit floating-point image
channels.  The HT block coder, which is standardized in `Rec. ITU-T
T.814
<https://loc.gov/preservation/digital/formats/fdd/fdd000566.shtml>`_
| `ISO/IEC 15444-15 <https://www.iso.org/standard/76621.html>`_, is
relatively new, and is estimated to increase the speed of JPEG 2000 by
roughly an order of magnitude. It is royalty-free, used in cinema and
distribution servicing, and implemented in both commercial and
open-source toolkits. The proposed integration with OpenEXR currently
uses the `OpenJPH <https://github.com/aous72/OpenJPH>`_ open-source
library. For ease in managing the dependency, the OpenEXR CMake
configuration supports automatically fetching and building OpenJPH
internally, or linking against an external installation.

Support for the HT256 compressor is available now for testing and
evaluation on the htj2k-beta branch at
https://github.com/AcademySoftwareFoundation/openexr/tree/htj2k-beta. We
expect to merge this to the main branch in a few weeks and release it
officially in August, pending feedback.

Note that this branch is provided for evaluation purposes only. The
file format may change before final release, so files written with
this evaluation branch may not be readable by future OpenEXR releases.

To help evaluate performance, the branch includes a new tool,
exrmetrics, which reports statistics about read/write time and file
size/compression ratio. See
`Tools/exrmetrics <https://openexr.com/en/latest/bin/exrmetrics.html>`_.
for details.  (Note that exrmetrics is also on the main branch, but
without HTJ2K support. It will be included with the next official
release.)

Please provide comments and feedback at the project GitHub
Issues page, or on the ASWF #openexr slack. Ping @palemieux with
technical questions.

We are particularly interested in feedback regarding real-world
time/space metrics, as well as any pipeline integration or build
issues.

.. _LatestNewsEnd:


November 11, 2024 - OpenEXR v3.3.2 Released
===========================================

Patch release that fixes several bugs and build issues:

* A recent change to CMake had the unintended consequence of
  installing headers and libraries from `libdeflate` when doing an
  internal build. This is now fixed.
* Fix custom namespaces
* Add thread control to `exrmetrics` tool
* Reintroduce single cache for successive scanline reads
* Allow empty filename when providing a custom stream
* Handle non-seekable stream in python module's `InputFile` object

This release fixes:

* OSS-fuzz `372524117 <https://issues.oss-fuzz.com/issues/372524117>`_
  Null-dereference WRITE in Imf_3_4::ScanLineProcess::run_fill

October 8, 2024 - OpenEXR v3.3.1 Released
=========================================


Patch release that addresses several build and performance issues:

* Fix a performance regression 3.3.0 in huf/piz compression
* Replace ``FetchContent_Populate`` with ``FetchContent_MakeAvailable``
* Build wheels for python 3.12
* Fix a problem with python wheel sdist that caused local build to fail
* Compile source files in parallel under MSVC



September 30, 2024 - OpenEXR v3.3.0 Released
============================================


Minor release several significant changes:

- The C++ API now uses the OpenEXRCore library underneath.

  - This is a transparent change to the existing API, although the ABI
    (i.e. structure / class layout) has changed

  - Existing reading of pixel data should be more efficient due to
    fewer memory allocations / frees during the process of
    reading. Additionally, some more specialisation of unpacking
    routines may result in faster unpack times

  - All compression routines are implemented by the C Core layer
    underneath and no longer duplicated

  - Initial support for "stateless" reading of scanlines has been
    proposed, allowing multiple threads to read scanlines into
    different frame buffer objects at the same time. While well tested
    at the Core level, the C++ api should be considered experimental
    for this release

  - Thread dispatch for reading different file types has been made
    more homogeneous, so is simpler and more consistent

- New API for accessing compression types

  In anticipation of future support for new compression types, there
  is now a convenience API for mapping between compression type names
  and the associated enum:


  ```
  getCompressionDescriptionFromId(Compression, std::string&)
  getCompressionIdFromName(const std::string&, Compression&)
  getCompressionNameFromId(Compression, std::string&)
  getCompressionNamesString(const std::string&, std::string&)
  getCompressionNumScanlines(Compression)
  isValidCompression(int)
  ```

- New bin tools:

  - exrmetrics - Read an OpenEXR image from infile, write an identical
    copy to outfile reporting time taken to read/write and file
    sizes. Useful for benchmarking performance in space and time.

  - exrmanifest - Read exr files and print the contents of the
    embedded manifest. The manifest provides a mapping between integer
    object identifiers and human-readible strings. See `OpenEXR Deep
    IDs
    Specification <https://openexr.com/en/latest/DeepIDsSpecification.html>`_
    for more details.

- New python bindings.

  This version introduces a new python API, the File object, which
  provides full support for reading and writing all types of .exr
  image files, including scanline, tiled, deep, mult-part, multi-view,
  and multi-resolution images with pixel types of unsigned 32-bit
  integers and 16- and 32-bit floats. It provides access to pixel data
  through numpy arrays, as either one array per channel or with R, G,
  B, and A interleaved into a single array RGBA array.

  Previous releases of the openexr python module supported only
  scanline files. The previous API remains in place for now for
  backwards compatibility.

  See `src/wrappers/python/README.md
  <https://github.com/AcademySoftwareFoundation/openexr/blob/v3.3.0-rc1/src/wrappers/python/README.md>`_
  for a synopsis.


September 9, 2024 - Imath v3.1.12 Released
==========================================

Patch release with a small fix:

- Support for compiling half.h with hip-runtime-amd

Also, the v3.1.11 release had improper versioning in its cmake and
pkgconf configuration files. This is now fixed.

March 26, 2024 - OpenEXR v3.2.4 and OpenEXR v3.1.13 Released
============================================================

OpenEXR v3.2.4 is released and available for download from `v3.2.4
<https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v3.2.4>`_.

Patch release that fixes handling of dwa compression in OpenEXRCore library.

Other miscellaneous changes:

- Add CMake `find_dependency` for `libdeflate`, to fix a vcpkg build error
- Remove the unused CMake option ``OPENEXR_INSTALL_EXAMPLES``
- Fix some other compiler warnings.

Also, the dwa bug fix has been back-ported to v3.1 and is available for
download at `v3.1.13
<https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v3.1.13>`_.

March 6, 2024 - OpenEXR v3.2.3 Released
=======================================

OpenEXR v3.2.3 is released and available for download from `v3.2.3
<https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v3.2.3>`_.

Patch release with various build/bug/documentation fixes:

* Fix ``bswap`` on NetBSD
* Fix issue with decompressing fp32 dwa files
* Support cmake config for ``libdeflate``
* updated security policy
* miscelleneous website improvements

This release also addresses several recent fuzz/security issues, including:

* OSS-fuzz `66676
  <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=66676>`_ Null-dereference in Imf_3_3::realloc_deepdata

* OSS-fuzz `66612
  <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=66612>`_ Null-dereference in Imf_3_3::realloc_deepdata

This release also formally adopts the process of publishing openexr
python wheels to `pypi.org <http://pypi.org>`_.

This release also introduces the process of signing release artifacts
via `sigstore <https://www.sigstore.dev>`_.

February 28, 2024 - Imath v3.1.11 Released
==========================================

Imath v3.1.11 is released and available for download from `v3.1.11
<https://github.com/AcademySoftwareFoundation/Imath/releases/tag/v3.1.11>`_.

Patch release with small build fix:

* Add explicit ``std::`` namespace for ``isfinite`` in ``ImathFun.cpp``

This release also introduces the practice of signing release artifacts
via `sigstore <https://www.sigstore.dev>`_.

February 11, 2024 - OpenEXR v3.2.2 Released
===========================================

OpenEXR v3.2.2 is released and available for download from `v3.2.2
<https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v3.2.2>`_.

Patch release that addresses
`CVE-2023-5841 <https://takeonme.org/cves/CVE-2023-5841.html>`_.

February 11, 2024 - OpenEXR v3.1.12 Released
============================================

OpenEXR v3.1.12 is released and available for download from `v3.1.12
<https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v3.1.12>`_.

Patch release that addresses
`CVE-2023-5841 <https://takeonme.org/cves/CVE-2023-5841.html>`_.

December 19, 2023 - OpenEXR v2.5.10 Released
============================================

OpenEXR v2.5.10 is released and available for download from `v2.5.10
<https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.5.10>`_.

Patch release for OpenEXR v2.5 that fixes a build failure on macOS
prior to 10.6 (fallback for missing `libdispatch`).

September 23, 2024 - OpenEXR v3.2.1 Released
============================================

OpenEXR v3.2.1 is released and available for download from `v3.2.1
<https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v3.2.1>`_.

Patch release with miscellaneous build fixes:

* Fix for linking statically against an external ``libdeflate``
* Fix a compile error with ``OPENEXR_VERSION_HEX``
* Fix various compiler warnings
* Pkg-config generation is now on by default for all systems, including Windows


September 20, 2023 - ASWF Dev Days
==================================

OpenEXR is participating in the Academy Software Foundation's `Dev
Days <https://sites.google.com/view/aswfdevdays/home>`_, a great way to
learn about how to contribute to open source software. Project
maintainers will be on hand October 12-13 to help guide you through
the process of making a simple first contribution.

Read more about Dev Days on the project wiki
`here <https://github.com/AcademySoftwareFoundation/openexr/wiki/Dev-Days-2023>`_.

For Dev Days, pick a simple change you can make within one day's
work. For ideas, consider the "good first issues" on either the 
`OpenEXR Issues
<https://github.com/AcademySoftwareFoundation/openexr/issues>`_ page or
the `Imath Issues
<https://github.com/AcademySoftwareFoundation/Imath/issues>`_ page.
Feel free to choose any other issue as well, or any other contribution
that you find appealing.

Contact the project maintainers via email at
`openexr-dev@lists.aswf.io`_ or Slack at `academysoftwarefdn#openexr
<https://academysoftwarefdn.slack.com/archives/CMLRW4N73>`_. Also,
consider attending the OpenEXR Technical Steering Committee Meeting,
held every other Thursday at 1:30PM Pacific Time. These are public
discussions open to anyone with interest in the project. The times and
Zoom link are on the `project calendar
<https://lists.aswf.io/g/openexr-dev/calendar>`_.

A list of ideas for more substantial contributions is on the `OpenEXR wiki
<https://wiki.aswf.io/display/OEXR/OpenEXR+Project+Ideas>`_. Although
these are beyond the scope of the spirit of Dev Days, feel free to
discuss these as well.

August 30, 2023 - OpenEXR v3.2.0 Released
=========================================

Minor release with several additions, changes and improvements:

* Zip compression via ``libdeflate``

  As of OpenEXR release v3.2, OpenEXR depends on `libdeflate
  <https://github.com/ebiggers/libdeflate>`_ for DEFLATE-based
  compression. Previous OpenEXR releases relied on `zlib
  <https://www.zlib.net>`_. Builds of OpenEXR can choose either an
  ``libdeflate`` installation, or CMake can auto-fetch the source and
  build it internally. The internal build is linked statically, so no
  extra shared object is produced.

  See https://openexr.com/en/latest/install.html for more details.

* New camdkit/camdkit-enabled standard attributes

  These changes bring to OpenEXR new standard optional attributes that
  were discussed in the `SMPTE Rapid Industry Solutions On-Set Virtual
  Production
  Initiative <https://www.smpte.org/blog/update-on-smptes-rapid-industry-solutions-ris-on-set-virtual-production-osvp-initiative>`_. Additionally,
  some useful attributes from the SMPTE ACES Container File Layout
  standard, SMPTE ST 2065-4:2023, have been included as well. The new
  attributes are: 

  Support automated editorial workflow:

  - ``reelName``
  - ``imageCounter``
  - ``ascFramingDecisionList``

  Support forensics:

  - ``cameraMake``
  - ``cameraModel``
  - ``cameraSerialNumber``
  - ``cameraFirmware``
  - ``cameraUuid``
  - ``cameraLabel``
  - ``lensMake``
  - ``lensModel``
  - ``lensSerialNumber``
  - ``lensFirmware``
  - ``cameraColorBalance``

  Support pickup shots:

  - ``shutterAngle``
  - ``cameraCCTSetting``
  - ``cameraTintSetting``

  Support metadata-driven match move:

  - ``sensorCenterOffset``
  - ``sensorOverallDimensions``
  - ``sensorPhotositePitch``
  - ``sensorAcquisitionRectangle``
  - ``nominalFocalLength``
  - ``effectiveFocalLength``
  - ``pinholeFocalLength``
  - ``entrancePupilOffset``
  - ``tStop`` (complementing existing aperture)

  Also, ``renderingTransform`` and ``lookTransform`` have been deprecated.

  See https://openexr.com/en/latest/StandardAttributes.html and `PR
  1383
  <https://github.com/AcademySoftwareFoundation/openexr/pull/1383>`_
  for more details.

* Updated SO versioning policy

  This change adopts a policy of appending the ``MAJOR.MINOR.PATCH``
  software release name to the ``SONAME`` to form the real name of the
  shared library.

  See https://openexr.com/en/latest/install.html `PR
  1498
  <https://github.com/AcademySoftwareFoundation/openexr/pull/1498>`_
  for more details.

* Python bindings & PyPI wheel

  Support for the PyPI `OpenEXR python bindings
  <https://pypi.org/project/OpenEXR>`_ have been formally adopted by
  the OpenEXR project.
  
* Miscellaneous improvements:

  - "docs" renamed to "website" (See `PR #1504 <https://github.com/AcademySoftwareFoundation/openexr/pull/1504>`_).

  - Additional deep & multipart code examples (See `PR #1493
    <https://github.com/AcademySoftwareFoundation/openexr/pull/1493>`_
    and `PR #1502 <https://github.com/AcademySoftwareFoundation/openexr/pull/1502>`_)

  - Many small build/test fixes
  
  - bin tools man pages

  - Expanded test coverage

August 13, 2023 - OpenEXR v3.1.11 Released
==========================================

Patch release that fixes a build failure with ``-march=x86-64-v3``

August 2, 2023 - OpenEXR v3.1.10 Released
=========================================

Patch release that addresses miscellaneous build issues, test
failures, and performance regressions, as well as:

* OSS-fuzz `59457
  <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=59457>`_ 
  Heap-buffer-overflow in ``LossyDctDecoder_execute``

August 1, 2023 - 2023 ASWF OpenEXR Virtual Town Hall
====================================================

Slides from the 2023 ASWF OpenEXR Virtual Town Hall are here:
https://wiki.aswf.io/display/OEXR/2023+ASWF+OpenEXR+Virtual+Town+Hall

Agenda:

- OpenEXR v3.1 Review

  - OpenEXRCore, now with DWAA/B compression support

  - Performance optimizations (zip, neon, huf decoder, SSE4)

  - Bug/build fixes

  - New https://openexr.com website, with test images

  - Other developments:

    - MacPorts is now up to date (v3.1.9)

    - PyPI python bindings (a.k.a. “pip install openexr”)

    - OpenEXR & USDZ

    - Coalition for Content Provenance and Authenticity (C2PA)

- OpenEXR v3.2 Preview

  - Lossless compression via libdeflate (replaces zlib)

  - ABI compatibility

  - New Optional Standard Attributes:

    - Support automated editorial workflow

      ``reelName``, ``imageCounter``, ``ascFramingDecisionList``

    - Support forensics (“which other shots used that camera and lens
      before the camera firmware was updated?”)

      ``cameraMake``, ``cameraModel``, ``cameraSerialNumber``,
      ``cameraFirmware``, ``cameraUuid``, ``cameraLabel``,
      ``lensMake``, ``lensModel``, ``lensSerialNumber``,
      ``lensFirmware``, ``cameraColorBalance``

    - Support pickup shots (reproduce critical camera settings)

      ``shutterAngle``, ``cameraCCTSetting``, ``cameraTintSetting``

    - Support metadata-driven match move

      ``sensorCenterOffset``, ``sensorOverallDimensions``,
      ``sensorPhotositePitch``, ``sensorAcquisitionRectangle``
      ``nominalFocalLength``, ``effectiveFocalLength``,
      ``pinholeFocalLength``, ``entrancePupilOffset``, ``tStop
      (complementing existing 'aperture')``

    - https://www.smpte.org/blog/update-on-smptes-rapid-industry-solutions-ris-on-set-virtual-production-osvp-initiative    

    - https://cookeoptics.com/wp-content/uploads/2023/07/Cooke-Camera-Lens-Definitions-for-VFX-210723.pdf

- Discussion topics: Experiments in GPU Decompression & Real-time Streaming

July 31, 2023 - OpenEXR v2.5.9 Released
=======================================

Patch release for v2.5 that fixes a compile failure with gcc-13 gcc 13
and a problem with PyIlmBase's pkgconfig.

June 25, 2023 - OpenEXR v3.1.9 Released
=======================================

Patch release that addresses miscelleneous build, doc, test issues, in
particular:

- Build fix for older macOS versions

Also:

* OSS-fuzz `59382 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=59382>`_
  Heap-buffer-overflow in ``internal_huf_decompress``

June 2, 2023 - OpenEXR v3.1.8 Released
======================================

Patch release that addresses miscellaneous build issues, for macOS in
particular, but also includes:
 
* Support for DWA compression in OpenEXRCore
* Fix for threadpool deadlocks during shutdown on Windows  

This release also addresses:

* OSS-fuzz `59070 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=59070>`_ Stack-buffer-overflow in ``DwaCompressor_readChannelRules``

May 31, 2023 - Imath Version 3.1.9 Released
===========================================

Patch release that fixes an correct .so version number introduced in
v3.1.8. All Imath library functionality is compatible with v3.1.8.

This release also reverts
`#305 <https://github.com/AcademySoftwareFoundation/Imath/pull/305>`_,
which inadvertently introduced additional python bindings in v3.1.8
that altered the ABI of ``libPyImath``. ``libPyImath`` is now
ABI-compatible with v3.1.7 and previous releases.

May 22, 2023 - Imath Version 3.1.8 Released
===========================================

**NOTE: this version has an incorrect .so number and should not be used. Use v3.1.9 instead.**

Patch release that addresses miscellaneous minor compiler/build/doc
issues and extends test coverage.

March 28, 2023 - OpenEXR v3.1.7 Released
========================================

Patch release that fixes a build regression on ARMv7, and fixes a build
issue with zlib.

See the `release notes`_ for more details.

Download OpenEXR v3.1.7 from https://github.com/AcademySoftwareFoundation/OpenEXR/releases/tag/v3.1.7.


March 9, 2023 - OpenEXR v3.1.6 Released
=======================================

Patch release that address various bug/build issues and optimizations:

* NEON optimizations for ZIP reading
* Enable fast Huffman & Huffman zig-zag transform for Arm Neon
* Support relative and absolute libdir/incluedir in pkg-config generation
* Fix for reading memory mapped files with DWA compression
* Enable SSE4 support on Windows
* Fast huf decoder
* CMake config for generating docs is now BUILD_DOC

Also, this release includes a major update and reorganization of the
repo documentation and the https://openexr.com website.

In addition, numerous typos and misspellings in comments and doxygen
content have been fixed via
`codespell <https://github.com/codespell-project/codespell>`_.

Specific OSS-fuzz issues address:

* `OSS-fuzz 52730 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=52730>`_ Heap-buffer-overflow in fasthuf_initialize
* `OSS-fuzz 49698 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=49698>`_ Heap-buffer-overflow in fasthuf_decode
* `OSS-fuzz 47517 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=47517>`_ Integer-overflow in reconstruct_chunk_table
* `OSS-fuzz 47503 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=47503>`_ Heap-buffer-overflow in uncompress_b44_impl
* `OSS-fuzz 47483 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=47483>`_ Heap-buffer-overflow in generic_unpack

See `CHANGES.md <https://github.com/AcademySoftwareFoundation/openexr/blob/main/CHANGES.md>`_ for more details.

March 1, 2023 - Imath v3.1.7 Released
=====================================

Patch release with miscellaneous bug/doc/build fixes. In particular:

- Support for relative prefix in pkg-config
- Reorganization of documentation at https://imath.readthedocs.io
- The CMake config for building the docs is now BUILD_DOCS instead of DOCS
- Add a trace() method on Matrix types

See [CHANGES.md](https://github.com/AcademySoftwareFoundation/Imath/blob/main/CHANGES.md) for more details.

November 7, 2022 - Imath v3.1.6 Released
========================================

Patch release with miscellaneous bug/doc/build fixes.

* fix memory leak in ``V3fArrayFromBuffer``
* Add ``<cstdint>`` for ``int64_t``
* Initialize ``x`` in ``testRoots.cpp:solve()`` to suppress compiler warning
* Fix gcc compiler warning in ``testFun.cpp``
* Test return value of ``extractSHRT`` to avoid uninitialized reference
* Fix example code so it compiles as is
* Cuda safety in several headers
* Fix markdown and typos in ``README.md``
* Do not warn if half.h has already being included
* Fix compiler warnings on windows
* Remove irrelevant cvs ignore files
* Update sphinx version

April 11, 2022 - OpenEXR v3.1.5 Released
========================================

Patch release that address various bug/build/doc issues:

* Add backwards-compatibilty flags to the core library to match
  original behavior of the the c++ library. Fixes reading of certain
  files by the new core.
* Fix build failures on MSVC14 and MSVC 2022
* Fix build failure on latest 64-bit Ubuntu
* Documentation refers to primary branch as "main"
* Update the CI workflow matrix to VFX-CY2022
* Update auto-fetch Imath version to v3.1.5

Specific OSS-fuzz issues addressed:

* `OSS-fuzz 46309 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=46309>`_ Heap-buffer-overflow in ``Imf_3_1::memstream_read``
* `OSS-fuzz 46083 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=46083>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 45899 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=45899>`_ Int``eger-overflow in internal_exr_compute_chunk_offset_size``
* `OSS-fuzz 44084 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=44084>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``

March 29, 2022 - Imath v3.1.5 Released
======================================

Patch release with miscellaneous bug/doc/build fixes.

In particular, this fixes an issue that could lead to incorrect values
for numeric_limits. This also updates the CI workflow matrix to
VFX-CY2022.

* Update CI workflow matrix for VFX-CY2022
* Use ``_WIN32`` instead of ``_MSC_VER`` to fix mingw build
* Fix 32-bit x86 build failure with 16c instructions
* Move ``numeric_limits`` specializations into ``half.h``
* Change references to ``master`` branch to ``main``


January 27, 2021 - OpenEXR v3.1.4 Released
==========================================

Patch release that addresses various issues:

* Several bug fixes to properly reject invalid input upon read 
* A check to enable SSE2 when building with Visual Studio
* A check to fix building with VisualStudio on ARM64
* Update the automatically-downloaded version of Imath to v3.1.4
* Miscellaneous documentation improvements

This addresses one public security vulnerability:

* `CVE-2021-45942 <https://nvd.nist.gov/vuln/detail/CVE-2021-45942>`_ Heap-buffer-overflow in ``Imf_3_1::LineCompositeTask::execute``

Specific OSS-fuzz issues:

* `OSS-fuzz 43961 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=43961>`_ Heap-buffer-overflow in ``generic_unpack``
* `OSS-fuzz 43961 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=43961>`_ Heap-buffer-overflow in ``generic_unpack``
* `OSS-fuzz 43916 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=43916>`_ Heap-buffer-overflow in ``hufDecode``
* `OSS-fuzz 43763 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=43763>`_ Heap-buffer-overflow in ``internal_huf_decompress``
* `OSS-fuzz 43745 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=43745>`_ Floating-point-exception in ``internal_exr_compute_tile_information``
* `OSS-fuzz 43744 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=43744>`_ Divide-by-zero in ``internal_exr_compute_tile_information``
* `OSS-fuzz 42197 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=42197>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 42001 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=42001>`_ Timeout in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 41999 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=41999>`_ Heap-buffer-overflow in ``Imf_3_1::LineCompositeTask::execute``
* `OSS-fuzz 41669 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=41669>`_ Integer-overflow in ``Imf_3_1::rleUncompress``
* `OSS-fuzz 41625 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=41625>`_ Heap-buffer-overflow in ``uncompress_b44_impl``
* `OSS-fuzz 41416 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=41416>`_ Heap-buffer-overflow in ``Imf_3_1::LineCompositeTask::execute``
* `OSS-fuzz 41075 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=41075>`_ Integer-overflow in ``Imf_3_1::copyIntoDeepFrameBuffer``
* `OSS-fuzz 40704 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=40704>`_ Crash in ``Imf_3_1::DeepTiledInputFile::readPixelSampleCounts``
* `OSS-fuzz 40702 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=40702>`_ Null-dereference in bool ``Imf_3_1::readDeepTile<Imf_3_1::DeepTiledInputFile>``
* `OSS-fuzz 40701 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=40701>`_ Null-dereference in bool ``Imf_3_1::readDeepTile<Imf_3_1::DeepTiledInputPart>``
* `OSS-fuzz 40423 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=40423>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 40234 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=40234>`_ Heap-buffer-overflow in ``generic_unpack``
* `OSS-fuzz 40231 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=40231>`_ Heap-buffer-overflow in ``hufDecode``
* `OSS-fuzz 40091
  <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=40091>`_
  Heap-buffer-overflow in ``hufDecode``
  
See the `release notes`_ for more details.

Download OpenEXR v3.1.4 from https://github.com/AcademySoftwareFoundation/OpenEXR/releases/tag/v3.1.4.


January 21, 2021 - Imath v3.1.4 Released
========================================

Patch release with miscellaneous bug/doc/build fixes.

* Added missing check ``_M_IX86`` or ``_M_X64`` when using ``__lzcnt``.
* ``SolveNormalizedCubic`` fix to return proper real root
* Add docs target only if not a subproject
* Fix docs race condition and make installation optional
* Remove dead PyImath code and references to ilmbase
* Use ``equalWithAbsError`` instead of equal operator for float
* Fix sphinx warnings and man page filenames
* Adding missing stdexcept header
* Use ``.x`` instead of ``operator[]`` for better SIMD auto-vectorization
* Successor/predecessor functions use ``isnan()`` and ``isinf()``
* Fix python imath export
* Cuda safety fixes

See the `imath release notes`_ for more details.

Download Imath v3.1.4 from https://github.com/AcademySoftwareFoundation/Imath/releases/tag/v3.1.4.

October 27, 2021 - OpenEXR v3.1.3 Released
==========================================

Patch release with a change to default zip compression level:

* Default zip compression level is now 4 (instead of 6), which in
  our tests improves compression times by 2x with only a tiny drop in
  compression ratio.

* ``setDefaultZipCompression()`` and ``setDefaultDwaCompression()`` now set
  default compression levels for writing.

* The Header now has ``zipCompressionLevel()`` and
  ``dwaCompressionLevel()`` to get/set the levels used for writing.

Also, various bug fixes, build improvements, and documentation updates. In particular:

* Fixes a build failure with Imath prior to v3.1
* Fixes a bug in detecting invalid chromaticity values

See the `release notes`_ for more details.

Download OpenEXR v3.1.3 from https://github.com/AcademySoftwareFoundation/OpenEXR/releases/tag/v3.1.3.

Oct 4, 2021 - OpenEXR v3.1.2 Released
=====================================

Patch release with various bug fixes, build improvements, and
documentation updates. in particular:

* Fix a test failure on arm7 
* Proper handling of pthread with glibc 2.34+ 
* Miscellaneous fixes for handling of invalid input by the new
  OpenEXRCore library 

With this version, the OpenEXR technical documentation formerly
distributed exclusively as pdf's is now published online at
https://openexr.readthedocs.io, with the document source now
maintained as .rst files in the repo's docs subfolder. 

Specific OSS-fuzz issues:

* `OSS-fuzz 39196 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=39196>`_ Stack-buffer-overflow in ``dispatch_print_error`` 
* `OSS-fuzz 39198 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=39198>`_ Direct-leak in ``exr_attr_chlist_add_with_length`` 
* `OSS-fuzz 39206 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=39206>`_ Direct-leak in ``extract_attr_string_vector`` 
* `OSS-fuzz 39212 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=39212>`_ Heap-use-after-free in ``dispatch_print_error`` 
* `OSS-fuzz 39205 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=39205>`_ Timeout in ``openexr_exrcheck_fuzzer`` 
* `OSS-fuzz 38912 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=38912>`_ Integer-overflow in ``Imf_3_1::bytesPerDeepLineTable`` 
* `OSS-fuzz 39084 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=39084>`_ Divide-by-zero in ``Imf_3_1::RGBtoXYZ`` 
  
See the `release notes`_ for more details.

Download OpenEXR v3.1.2 from https://github.com/AcademySoftwareFoundation/OpenEXR/releases/tag/v3.1.2.

Sep 2, 2021 - Imath v3.1.3 Released
===================================

Patch release with miscellaneous minor fixes.

See the `imath release notes`_ for more details.

Download Imath v3.1.3 from https://github.com/AcademySoftwareFoundation/Imath/releases/tag/v3.1.3.

Aug 2, 2021 - OpenEXR v3.1.1 Released
=====================================

Patch release that fixes build failures on various systems (ARM64
macOS, FreeBSD), introduces CMake
``CMAKE_CROSSCOMPILING_EMULATOR`` support, and fixes a few other
minor issues.

See the `release notes`_ for more details.

Download OpenEXR v3.1.1 from https://github.com/AcademySoftwareFoundation/OpenEXR/releases/tag/v3.1.1.

July 31, 2021 - Imath v3.1.2 Released
=====================================

Patch release that fixes a potential Windows build issue

Download Imath v3.1.2 from https://github.com/AcademySoftwareFoundation/Imath/releases/tag/v3.1.2.

July 22, 2021 - OpenEXR v3.1.0 Released
=======================================

The 3.1 release of OpenEXR introduces a new library, OpenEXRCore,
which is the result of a significant re-thinking of how OpenEXR
manages file I/O and provides access to image data. It begins to
address long-standing scalability issues with multithreaded image
reading and writing.

The OpenEXRCore library provides thread-safe, non-blocking access to
files, which was not possible with the current API, where the
framebuffer management is separate from read requests. It is written
entirely in C and provides a new C-language API alongside the existing
C++ API. This new low-level API allows applications to do custom
unpacking of EXR data, such as on the GPU, while still benefiting from
efficient I/O, file validation, and other semantics. It provides
efficient direct access to EXR files in texturing applications. This C
library also introduces an easier path to implementing OpenEXR
bindings in other languages, such as Rust.

The 3.1 release represents a technology preview for upcoming
releases. The initial release is incremental; the existing API and
underlying behavior has not changed. The new API is available now for
performance validation testing, and then in future OpenEXR releases,
the C++ API will migrate to use the new core in stages.  It is not the
intention to entirely deprecate the C++ API, nor must all applications
re-implement EXR I/O in terms of the C library. The C API does not,
and will not, provide the rich set of utility classes that exist in
the C++ layer. The 3.1 release of the OpenEXRCore library simply
offers new functionality for specialty applications seeking the
highest possible performance. In the future, the ABI will evolve, but
the API will remain consistent, or only have additions. 

Technical Design
----------------

The OpenEXRCore API introduces a ``context`` object to manage file
I/O. The context provides customization for I/O, memory allocation,
and error handling.  This makes it possible to use a decode and/or
encode pipeline to customize how the chunks are written and read, and
how they are packed or unpacked.

The OpenEXRCore library is built around the concept of “chunks”, or
atomic blocks of data in a file, the smallest unit of data to be read
or written.  The contents of a chunk vary from file to file based on
compression (i.e. zip and zips) and layout (scanline
vs. tiled). Currently this is either 1, 16, or 32 scanlines, or 1 tile
(or subset of a tile on edge boundaries / small mip level).

The OpenEXRCore library is specifically designed for multipart EXR
files. It will continue to produce legacy-compatible single part files
as needed, but the API assumes you are always dealing with a
multi-part file. It also fully supports attributes, although being C,
it lacks some of the C++ layer’s abstraction.

Limitations
-----------

* No support yet for DWAA and DWAB compression during decode and
  encode pipelines. The low-level chunk I/O still works with DWAA and
  DWAB compressed files, but the encoder and decoder are not yet
  included in this release.

* For deep files, reading of deep data is functional, but the path for
  encoding deep data into chunk-level data (i.e. packing and
  compressing) is not yet complete.

* For both of these deficiencies, it is easy to define a custom
  routine to implement this, should it be needed prior to the library
  providing full support.

* No attempt to search through the file and find missing chunks is
  made when a corrupt chunk table is encountered. However, if a
  particular chunk is corrupt, this is handled such that the other
  chunks may be read without rendering the context unusable

Download OpenEXR v3.1.0 from
https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v3.1.0.

July 20, 2021 - Imath v3.1.1
----------------------------

Patch release that fixes a build failure on ARM64 macOS

Download Imath v3.1.1 from
https://github.com/AcademySoftwareFoundation/Imath/releases/tag/v3.1.1.

July 13, 2021 - Imath v3.1.0 Released
=====================================

Minor release with new features:

* Optimized half-to-float and float-to-half conversion, using F16C SSE
  instruction set if available. Non-SSE conversion eliminates the
  float-to-half exponent lookup table, and half-to-float conversion
  provides a compile-time-optional bit shifting that is slower but
  eliminates the need for the lookup table, for applications where
  memory is limited.

* Half-to-float and float-to-half conversion is also available as
  C-language functions ``imath_half_to_float()`` and
  ``imath_float_to_half()``.
  
* All new conversions produced identical results, and new options are
  off by default to ensure backwards compatibility. See
  https://imath.readthedocs.io for more info.
  
* ``NOEXCEPT`` specifier can be eliminated at compile-time via the
  ``IMATH_USE_NOEXCEPT`` CMake option.

* Python bindings:

  * FixedArray objects support a "read only" state. 
  * FixedArray objects support python buffer protocol.
      

* Optimized 4x4 matrix multiplication.

Download Imath v3.1.0 from
https://github.com/AcademySoftwareFoundation/Imath/releases/tag/v3.1.0.

July 1, 2021 - OpenEXR v3.0.5 Released
======================================

Patch release that fixes problems with library symlinks and
pkg-config, as well as miscellaneous bugs/security issues.

Download OpenEXR v3.0.5 from
https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v3.0.5.

June 16, 2021 - OpenEXR v2.5.7 Released
=======================================

Patch release for v2.5 with security and build fixes:

* `OSS-fuzz 28051 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=28051>`_ Heap-buffer-overflow in ``Imf_2_5::copyIntoFrameBuffer``
* `OSS-fuzz 28155 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=28155>`_ Crash in ``Imf_2_5::PtrIStream::read`` 
* Fix broken symlink and pkg-config lib suffix for cmake debug builds 

Download OpenEXR v2.5.7 from https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.5.7.

June 3, 2021 - OpenEXR v3.0.4 Released
======================================

Patch release that corrects a problem with the release version numbers
in v3.0.2/v3.0.3, and with the referenced Imath release.

Download OpenEXR v3.0.4 from https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v3.0.4.

June 1, 2021 - Imath v3.0.4 Released
====================================

Patch release that corrects a problem with the release version number
of v3.0.2

Download Imath v3.0.4 from https://github.com/AcademySoftwareFoundation/Imath/releases/tag/v3.0.4.

May 18, 2021 - OpenEXR v3.0.3 Released
======================================

Patch release that fixes a regression in v3.0.2 the prevented headers
from being installed properly.

Download OpenEXR v3.0.3 from https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v3.0.3.

May 17, 2021 - OpenEXR v3.0.2 Released
======================================

Patch release with miscellaneous bug/build fixes, primarily:

* Fix TimeCode.frame max value
* Don't impose C++14 on downstream projects 
* Restore fix to macOS universal 2 build lost from #854 

Specific OSS-fuzz issues:

* `OSS-fuzz 33741 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=33741>`_ Integer-overflow in ``Imf_3_0::getScanlineChunkOffsetTableSize``
* `OSS-fuzz 32620 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=32620>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
  
Download OpenEXR v3.0.2 from https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v3.0.2.

May 17, 2021 - OpenEXR v2.5.6 Released
======================================

Patch release for v2.5 that fixes a regression in
``Imath::succf()/Imath::predf()``.

Download OpenEXR v2.5.6 from https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.5.6.

<hr>

May 17, 2021 - OpenEXR v2.4.3 Released
======================================

Patch release for v2.4 that addresses the following security vulnerabilities:

* `CVE-2021-20296 <https://nvd.nist.gov/vuln/detail/CVE-2021-20296>`_ Segv on unknown address in ``Imf_2_5::hufUncompress`` - Null Pointer dereference 
* `CVE-2021-3479 <https://nvd.nist.gov/vuln/detail/CVE-2021-3479>`_ Out-of-memory in ``openexr_exrenvmap_fuzzer`` 
* `CVE-2021-3478 <https://nvd.nist.gov/vuln/detail/CVE-2021-3478>`_ Out-of-memory in ``openexr_exrcheck_fuzzer`` 
* `CVE-2021-3477 <https://nvd.nist.gov/vuln/detail/CVE-2021-3477>`_ Heap-buffer-overflow in ``Imf_2_5::DeepTiledInputFile::readPixelSampleCounts`` 
* `CVE-2021-3476 <https://nvd.nist.gov/vuln/detail/CVE-2021-3476>`_ Undefined-shift in ``Imf_2_5::unpack14`` 
* `CVE-2021-3475 <https://nvd.nist.gov/vuln/detail/CVE-2021-3475>`_ Integer-overflow in ``Imf_2_5::calculateNumTiles`` 
* `CVE-2021-3474 <https://nvd.nist.gov/vuln/detail/CVE-2021-3474>`_ Undefined-shift in ``Imf_2_5::FastHufDecoder::FastHufDecoder`` 

Also:

* Fixed regression in ``Imath::succf()`` and ``Imath::predf()`` when negative values are given 

Download OpenEXR v2.4.3 from https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.4.3.

May 16, 2021 - Imath v3.0.2 Released
====================================

Patch release with miscellaneous bug/build fixes, primarily:

* Fix regression in ``succf()`` and ``predf()``
* Don't impose C++14 on downstream projects 

Download Imath v3.0.2 from https://github.com/AcademySoftwareFoundation/Imath/releases/tag/v3.0.2.

April 1, 2021 - OpenEXR and Imath v3.0 Released
===============================================

The OpenEXR Project and the Academy Software Foundation (ASWF)
announced the official release of OpenEXR and Imath version 3.0, now
available for developers to integrate into their applications.

The 3.0 release of OpenEXR addresses a number of security issues and
introduces several new features, including ID Manifest attributes, but
it is primarily a major restructuring and simplification of the build
process and modernization of the code that involves moving Imath to an
external project dependency.

Largely backwards compatible with previous releases, Imath 3.0 brings
the library up to modern standards for performance and utility. By
promoting Imath as a project independent of OpenEXR, we hope to
encourage its use throughout the motion picture and computer graphics
community.

See the `Imath
<https://github.com/AcademySoftwareFoundation/Imath/blob/master/CHANGES.md>`_
and `OpenEXR
<https://github.com/AcademySoftwareFoundation/openexr/blob/master/CHANGES.md>`_
release notes for details and the `porting guide
<https://github.com/AcademySoftwareFoundation/Imath/blob/master/docs/PortingGuide2-3.md>`_
for more information about the differences with previous versions. And
read the documentation at https://imath.readthedocs.io. And please report any problems and
share your feedback either through the GitHub repo or
`openexr-dev@lists.aswf.io <mailto:openexr-dev@lists.aswf.io>`_.

March 28, 2021 - OpenEXR v3.0.1-beta Released
=============================================

Beta patch release:

* `OSS-fuzz 32370 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=32370>`_ Out-of-memory in openexr_exrcheck_fuzzer 
* `OSS-fuzz 32067 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=32067>`_ account for size of pixels when estimating memory 

March 28, 2021 - Imath v3.0.1-beta Released
===========================================

Beta patch release:

* ``#if IMATH_FOREIGN_VECTOR_INTEROP`` around type detectors
* Forward declarations only if header is not included

March 16, 2020 - OpenEXR v3.0.0-beta Released
=============================================

OpenEXR version v3.0.0-beta is available for download at https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v3.0.0-beta.

Major release with major build restructing, security improvements, and
new features:

* Restructuring:

  * The IlmBase/PyIlmBase submodules have been separated into the
    Imath project, now included by OpenEXR via a CMake submodule
    dependency, fetched automatically via CMake's FetchContent if
    necessary.
  * The library is now called ``libOpenEXR`` (instead of
    ``libIlmImf``).  No header files have been renamed, they
    retain the ``Imf`` prefix.
  * Symbol linkage visibility is limited to specific public symbols.
      

* Build improvements:

  * No more simultaneous static/shared build option.
    Gnu autoconf/bootstrap/configure build setup has been retired.
  * Community-provided support for bazel.
      
* New Features:

  * ID Manifest Attributes, as described in ``
    `"A Scheme for Storing Object ID Manifests in OpenEXR Images" 
    Peter Hillman, DigiPro 18: Proceedings of the 8th Annual Digital
    Production Symposium, August 2018. <https://doi.org/10.1145/3233085.3233086>`_,
  * New program: exrcheck validates the contents of an EXR file.

* Changes:

  * EXR files with no channels are no longer allowed.
  * Hard limit on the size of deep tile sizes; tiles must be less than
    2<sup>30</sup> pixels.
  * Tiled DWAB files used STATIC_HUFFMAN compression.
  * ``Int64`` and ``SInt64`` types are deprecated in ``favor``
    of ``uint64_t`` and ``int64_t``.
  * Header files have been pruned of extraneous ``#include``'s
    ("Include What You Use"), which may generate compiler errors in
    application source code from undefined symbols or
    partially-defined types. These can be resolved by identifying and
    including the appropriate header.
  * See the :doc:`PortingGuide` for details about differences
    from previous releases and how to address them.
  * Also refer to the porting guide for details about changes to
    Imath.

Specific OSS-fuzz issues addressed include:

* `OSS-fuzz 24573 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24573>`_ Out-of-memory in ``openexr_exrenvmap_fuzzer`` 
* `OSS-fuzz 24857 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24857>`_ Out-of-memory in ``openexr_exrheader_fuzzer`` 
* `OSS-fuzz 25002 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25002>`_ Out-of-memory in ``openexr_deepscanlines_fuzzer`` 
* `OSS-fuzz 25648 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25648>`_ Out-of-memory in ``openexr_scanlines_fuzzer`` 
* `OSS-fuzz 26641 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=26641>`_ Invalid-enum-value in ``readSingleImage`` 
* `OSS-fuzz 28051 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=28051>`_ Heap-buffer-overflow in ``Imf_2_5::copyIntoFrameBuffer`` 
* `OSS-fuzz 28155 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=28155>`_ Crash in ``Imf_2_5::PtrIStream::read`` 
* `OSS-fuzz 28419 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=28419>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 29393 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=29393>`_ Timeout in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 29423 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=29423>`_ Integer-overflow in ``Imf_2_5::DwaCompressor::initializeBuffers``
* `OSS-fuzz 29653 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=29653>`_ Integer-overflow in ``Imf_2_5::DwaCompressor::initializeBuffers``
* `OSS-fuzz 29682 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=29682>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 30115 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=30115>`_ Timeout in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 30249 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=30249>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 30605 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=30605>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 30616 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=30616>`_ Timeout in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 30969 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=30969>`_ Direct-leak in ``Imf_2_5::DwaCompressor::LossyDctDecoderBase::execute``
* `OSS-fuzz 31015 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=31015>`_ Direct-leak in ``Imf_2_5::TypedAttribute<Imf_2_5::CompressedIDManifest>::readValueFrom``
* `OSS-fuzz 31044 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=31044>`_ Timeout in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 31072 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=31072>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 31221 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=31221>`_ Integer-overflow in bool ``Imf_2_5::readDeepTile<Imf_2_5::DeepTiledInputPart>``
* `OSS-fuzz 31228 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=31228>`_ Integer-overflow in bool ``Imf_2_5::readDeepTile<Imf_2_5::DeepTiledInputFile>``
* `OSS-fuzz 31291 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=31291>`_ Sanitizer CHECK failure in ``((0 ""Address is not in memory and not in shadow?"")) != (0)"" (0x0, 0x0)``
* `OSS-fuzz 31293 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=31293>`_ Segv on unknown address in ``Imf_2_5::copyIntoFrameBuffer``
* `OSS-fuzz 31390 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=31390>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 31539 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=31539>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
  
March 15, 2021 - Imath v3.0.0-beta Released
===========================================

Imath version v3.0.0-beta is available for download at https://github.com/AcademySoftwareFoundation/Imath/releases/tag/v3.0.0-beta.

First release of Imath independent of OpenEXR.

See the :doc:`PortingGuide` for details about differences from
previous releases.

Summary:

* Imath includes the half type, formerly in a separate Half library. 
* Headers are installed in ``Imath/`` subdirectory. 
* All appropriate methods are marked constexpr, noexcept 
* Appropriate declaration include CUDA ``__host__`` and ``__device__``  directives. 
* Throwing methods throw std exceptions instead of ``Iex``. 
* New Vec and Matrix interoperability constructors for conversion from other similar type objects. 
* Symbol linkage visibility is limited to specific public symbols. 
* python bindings are off by default, available by setting ``PYTHON=ON`` 
* Deprecated features:
* ``std::numeric_limits`` replaces ``Imath::limits``. 
* ``Int64`` and ``SInt64`` are deprecated in favor of ``uint64_t`` and ``int64_t``. 
      
February 12, 2021 - OpenEXR v2.5.5 Released
===========================================

OpenEXR version v2.5.5 is available for download at https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.5.5.

Patch release with various bug/sanitizer/security fixes, primarily
related to reading corrupted input files, but also a fix for universal
build support on macOS. 

Specific OSS-fuzz issues include:

* `OSS-fuzz 30291 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=30291>`_ Timeout in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 29106 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=29106>`_ Heap-buffer-overflow in ``Imf_2_5::FastHufDecoder::decode``
* `OSS-fuzz 28971 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=28971>`_ Undefined-shift in ``Imf_2_5::cachePadding``
* `OSS-fuzz 29829 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=29829>`_ Integer-overflow in ``Imf_2_5::DwaCompressor::initializeBuffers``
* `OSS-fuzz 30121 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=30121>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
  
December 31, 2020 - OpenEXR v2.5.4 Released
===========================================

OpenEXR version v2.5.4 is available for download at https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.5.4.

Patch release with various bug/sanitizer/security fixes, primarily
related to reading corrupted input files.

Specific OSS-fuzz issues include:

* `OSS-fuzz 24854 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24854>`_ Segv on unknown address in ``Imf_2_5::hufUncompress``
* `OSS-fuzz 24831 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24831>`_ Undefined-shift in ``Imf_2_5::FastHufDecoder::FastHufDecoder``
* `OSS-fuzz 24969 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24969>`_ Invalid-enum-value in ``Imf_2_5::TypedAttribute<Imf_2_5::Envmap>::writeValueTo``
* `OSS-fuzz 25297 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25297>`_ Integer-overflow in ``Imf_2_5::calculateNumTiles``
* `OSS-fuzz 24787 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=24787>`_ Undefined-shift in ``Imf_2_5::unpack14``
* `OSS-fuzz 25326 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25326>`_ Out-of-memory in ``openexr_scanlines_fuzzer``
* `OSS-fuzz 25399 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25399>`_ Heap-buffer-overflow in ``Imf_2_5::FastHufDecoder::FastHufDecoder``
* `OSS-fuzz 25415 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25415>`_ Abrt in ``__cxxabiv1::failed_throw``
* `OSS-fuzz 25370 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25370>`_ Out-of-memory in ``openexr_exrenvmap_fuzzer``
* `OSS-fuzz 25501 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25501>`_ Out-of-memory in ``openexr_scanlines_fuzzer``
* `OSS-fuzz 25505 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25505>`_ Heap-buffer-overflow in ``Imf_2_5::copyIntoFrameBuffer``
* `OSS-fuzz 25562 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25562>`_ Integer-overflow in ``Imf_2_5::hufUncompress``
* `OSS-fuzz 25740 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25740>`_ Null-dereference READ in ``Imf_2_5::Header::operator``
* `OSS-fuzz 25743 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25743>`_ Null-dereference in ``Imf_2_5::MultiPartInputFile::header``
* `OSS-fuzz 25913 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25913>`_ Out-of-memory in ``openexr_exrenvmap_fuzzer``
* `OSS-fuzz 26229 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=26229>`_ Undefined-shift in ``Imf_2_5::hufDecode``
* `OSS-fuzz 26658 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=26658>`_ Out-of-memory in ``openexr_scanlines_fuzzer``
* `OSS-fuzz 26956 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=26956>`_ Heap-buffer-overflow in ``Imf_2_5::DeepTiledInputFile::readPixelSampleCounts``
* `OSS-fuzz 27409 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=27409>`_ Out-of-memory in ``openexr_exrcheck_fuzzer``
* `OSS-fuzz 25892 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25892>`_ Divide-by-zero in ``Imf_2_5::calculateNumTiles``
* `OSS-fuzz 25894 <https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=25894>`_ Floating-point-exception in ``Imf_2_5::precalculateTileInfo``

August 12, 2020 - OpenEXR v2.5.3 Released
=========================================

OpenEXR version v2.5.3 is available for download at https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.5.3.

Patch release with various bug/security fixes and build/install fixes, plus a performance optimization:

* Various sanitizer/fuzz-identified issues related to handling of invalid input
* Fixes to misc compiler warnings
* Cmake fix for building on arm64 macOS (#772)
* Read performance optimization (#782)
* Fix for building on non-glibc (#798)
* Fixes to tests

June 15, 2020 - OpenEXR v2.5.2 Released
=======================================

OpenEXR version v2.5.2 is available for download at https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.5.2.

Patch release with various bug/security and build/install fixes:

* Invalid input could cause a heap-use-after-free error in ``DeepScanLineInputFile::DeepScanLineInputFile()``
* Invalid chunkCount attributes could cause heap buffer overflow in ``getChunkOffsetTableSize()``
* Invalid tiled input file could cause invalid memory access ``TiledInputFile::TiledInputFile()``
* ``OpenEXRConfig.h`` now correctly sets ``OPENEXR_PACKAGE_STRING`` to ``OpenEXR`` (rather than ``IlmBase``)
* Various Windows build fixes

June 15, 2020 - OpenEXR v2.4.2 Released
=======================================

OpenEXR version v2.4.2 is available for download at https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.4.2.

Patch release that backports various recent bug/security fixes:

* Invalid input could cause a heap-use-after-free error in ``DeepScanLineInputFile::DeepScanLineInputFile()``
* Invalid chunkCount attributes could cause heap buffer overflow in ``getChunkOffsetTableSize()``
* Invalid tiled input file could cause invalid memory access TiledInputFile::TiledInputFile()
* OpenEXRConfig.h now correctly sets ``OPENEXR_PACKAGE_STRING`` to ``OpenEXR`` (rather than ``IlmBase``)

May 11, 2020 - OpenEXR v2.5.1 Released
======================================

OpenEXR version v2.5.1 is available for download at https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.5.1.

v2.5.1 is a patch release that corrects the SO version for the v2.5
release, which missed getting bumped in v2.5.0.

This release also fixes an improper failure in IlmImfTest when running on ARMv7 and AAarch64.

May 6, 2020 - OpenEXR v2.5.0 Released
=====================================

OpenEXR version v2.5.0 is available for download at https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.5.0.

This is a minor release with miscellaneous bug fixes and small features

Summary:

* No more build-time header generation: ``toFloat.h``, ``eLut.h``,
  ``b44ExpLogTable.h``, and ``dwaLookups.h`` are now ordinary header
  files, no longer generated on the fly.
* New ``StdISSTream`` class, an "input" ``stringstream`` version of
  ``StdOSStream``
* New ``Matrix22`` class in Imath 
* Chromaticity comparison operator now includes white (formerly
  ignored) 
* Various cmake fixes 
* Bug fixes for various memory leaks 
* Bug fixes for various invalid memory accesses 
* New checks to detect damaged input files 
* ``OpenEXR_Viewers`` has been deprecated, removed from the top-level 
  cmake build and documentation.

See the `release notes`_ for more details.

April 30, 2020 - OpenEXR v2.2.2 Released
========================================

OpenEXR version v2.2.2 is available for download at https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.2.2.

This is a patch release of OpenEXR v2.2 that includes fixes for the following security vulnerabilities:

* `CVE-2020-11765 <https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11765>`_
  There is an off-by-one error in use of the ImfXdr.h read function by
  DwaCompressor::Classifier::Classifier, leading to an out-of-bounds
  read. 
* `CVE-2020-11764 <https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11764>`_
  There is an out-of-bounds write in copyIntoFrameBuffer in
  ImfMisc.cpp. 
* `CVE-2020-11763 <https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11763>`_
  There is an std::vector out-of-bounds read and write, as
  demonstrated by ImfTileOffsets.cpp. 
* `CVE-2020-11762 <https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11762>`_
  There is an out-of-bounds read and write in
  DwaCompressor::uncompress in ImfDwaCompressor.cpp when handling the
  UNKNOWN compression case. 
* `CVE-2020-11761 <https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11761>`_
  There is an out-of-bounds read during Huffman uncompression, as
  demonstrated by FastHufDecoder::refill in ImfFastHuf.cpp. 
* `CVE-2020-11760 <https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11760>`_
  There is an out-of-bounds read during RLE uncompression in
  rleUncompress in ImfRle.cpp. 
* `CVE-2020-11759 <https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11759>`_
  Because of integer overflows in
  CompositeDeepScanLine::Data::handleDeepFrameBuffer and
  readSampleCountForLineBlock, an attacker can write to an
  out-of-bounds pointer. 
* `CVE-2020-11758 <https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11758>`_
  There is an out-of-bounds read in ImfOptimizedPixelReading.h. 

See the `release notes`_ for more details.

Feb 11, 2020 - OpenEXR v2.4.1 Released
======================================

OpenEXR version v2.4.1 is available for download at https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.4.1.

This is a patch release that addresses miscellaneous bugs.

Summary:

* Various fixes for memory leaks and invalid memory accesses
* Various fixes for integer overflow with large images.
* Various cmake fixes for build/install of python modules.
* ImfMisc.h is no longer installed, since it's a private header.

Sep 19, 2019 - OpenEXR v2.4.0 Released
======================================

OpenEXR version v2.4.0 is available for download at https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.4.0.

This release contains no major new functionality, although it does
include many miscellaneous bug and security fixes. The major new
feature is a completely rewritten CMake setup, which should work
reliably on Linux, macOS, and Windows. The installation instructions
and project documentation are also freshly revised. See the release
notes for the complete details.

The project also now supports continuous integration via Azure
Pipelines and code analysis via SonarCloud, services supported by the
Academy Software Foundation. Links to the dashboards are through
the"Azure Pipelines" and "quality gate" badges at the top the GitHub
repo home page.

Summary of changes:

* Completely re-written CMake configuration files
* Improved support for building on Windows, via CMake
* Improved support for building on macOS, via CMake
* All code compiles without warnings on gcc, clang, msvc
* Cleanup of license and copyright notices
* floating-point exception handling is disabled by default
* New Slice::Make method to reliably compute base pointer for a slice.
* Miscellaneous bug fixes

This version fixes the following security vulnerabilities:

* CVE-2018-18444 Issue #351 Out of Memory
* CVE-2018-18443 Issue #350 heap-buffer-overflow

Sep 5, 2019 - OpenEXR v2.4.0-beta.1 Released
============================================

OpenEXR version v2.4.0-beta.1 is available for download at https://github.com/AcademySoftwareFoundation/openexr/releases/tag/v2.4.0-beta.1.

This release contains no major new functionality, although it does
include many miscellaneous bug and security fixes. The major new
feature is a completely rewritten CMake setup, which should work
reliably on Linux, macOS, and Windows. The installation instructions
and project documentation are also freshly revised. See the release
notes for the complete details.

The project also now supports continuous integration via Azure
Pipelines and code analysis via SonarCloud, services supported by the
Academy Software Foundation. Links to the dashboards are through
the"Azure Pipelines" and "quality gate" badges at the top the GitHub
repo home page.

Summary of changes:

* Completely re-written CMake configuration files
* Improved support for building on Windows, via CMake
* Improved support for building on macOS, via CMake
* All code compiles without warnings on gcc, clang, msvc
* Cleanup of license and copyright notices
* floating-point exception handling is disabled by default
* New Slice::Make method to reliably compute base pointer for a slice.
* Miscellaneous bug fixes

This version fixes the following security vulnerabilities:
* CVE-2018-18444 Issue #351 Out of Memory
* CVE-2018-18443 Issue #350 heap-buffer-overflow

May 1, 2019 Academy Software Foundation Adopts OpenEXR
======================================================

OpenEXR has been adopted by the `Academy Software Foundation <https://www.aswf.io/openexr-and-opencue-join-aswf>`_.

August 13, 2018 - OpenEXR 2.3.0 Released
========================================

OpenEXR v2.3.0 has been released and is available for download.

Features/Improvements:
* ThreadPool overhead improvements, enable custom thread pool to be registered via ThreadPoolProvider class
* Fixes to enable custom namespaces for Iex, Imf
* Improve read performance for deep/zipped data, and SIMD-accelerated uncompress support
* Added rawPixelDataToBuffer() function for access to compressed scanlines
* Iex::BaseExc no longer derived from std::string.
* Imath throw() specifiers removed
* Initial Support for Python 3

Bugs:
* 25+ various bug fixes (see detailed Release Notes for the full list)

Build Fixes:
* Various fixes to the cmake and autoconf build infrastructures
* Various changes to support compiling for C++11 / C++14 / C++17 and GCC 6.3.1
* Various fixes to address Windows build issues
* 60+ total build-related fixes (see detailed Release Notes for the full list)

See the `release notes`_ for more details.

OpenEXR source can be obtained from the downloads section of `www.openexr.com <http://www.openexr.com>`_ or from the project page on github: https://github.com/AcademySoftwareFoundation/openexr.

November 30, 2017 - OpenEXR 2.2.1 Released
==========================================

OpenEXR v2.2.1 has been released and is available for download.

This maintenance release addresses the reported OpenEXR security vulnerabilities, specifically CVE-2017-9110, CVE-2017-9111, CVE-2017-9112, CVE-2017-9113, CVE-2017-9114, CVE-2017-9115, CVE-2017-9116. 

OpenEXR source can be obtained from the downloads section of `www.openexr.com <http://www.openexr.com>`_ or from the project page on github: https://github.com/AcademySoftwareFoundation/openexr.

August 10, 2014 - OpenEXR v2.2.0 Released
=========================================

OpenEXR v2.2.0 has been released and is available for download.

This release includes the following components:
* OpenEXR: v2.2.0
* IlmBase: v2.2.0
* PyIlmBase: v2.2.0
* OpenEXR_Viewers: v2.2.0

This significant new features of this release include:

* **DreamWorks Lossy Compression** A new high quality,
  high performance lossy compression codec contributed by DreamWorks
  Animation.  This codec allows control over variable lossiness to
  balance visual quality and file size.  This contribution also
  includes performance improvements that speed up the PIZ codec.

* **IlmImfUtil** A new library intended to aid in
  development of image file manipulation utilities that support the
  many types of OpenEXR images.

This release also includes improvements to cross-platform build support using CMake.

OpenEXR source can be obtained from the downloads section of
www.openexr.com or from the project page
on github: https://github.com/AcademySoftwareFoundation/openexr.

November 25, 2013 - OpenEXR v2.1.0 Released
===========================================

OpenEXR v2.1.0 has been released and is available for download.

This release includes the following components (version locked):

* OpenEXR: v2.1.0
* IlmBase: v2.1.0
* PyIlmBase: v2.1.0
* OpenEXR_Viewers: v2.1.0

This release includes a refactoring of the optimised read paths for
RGBA data, optimisations for some of the python bindings to Imath,
improvements to the cmake build environment as well as additional
documentation describing deep data in more detail.

April 9, 2013 - OpenEXR v2.0 Released
=====================================

Industrial Light & Magic (ILM) and Weta Digital announce the release
of OpenEXR 2.0, the major version update of the open source high
dynamic range file format first introduced by ILM and maintained and
expanded by a number of key industry leaders including Weta Digital,
Pixar Animation Studios, Autodesk and others.

The release includes a number of new features that align with the
major version number increase. Amongst the major improvements are:

* Deep Data support- Pixels can now store a variable-length list of
  samples. The main rationale behind deep images is to enable the
  storage of multiple values at different depths for each
  pixel. OpenEXR 2.0 supports both hard-surface and volumetric
  representations for Deep Compositing workflows.
	
* Multi-part Image Files - With OpenEXR 2.0, files can now contain a
  number of separate, but related, data parts in one file. Access to
  any part is independent of the others, pixels from parts that are
  not required in the current operation don't need to be accessed,
  resulting in quicker read times when accessing only a subset of
  channels. The multipart interface also incorporates support for
  Stereo images where views are stored in separate parts. This makes
  stereo OpenEXR 2.0 files significantly faster to work with than the
  previous multiview support in OpenEXR.

* Optimized pixel reading - decoding RGB(A) scanline images has been
  accelerated on SSE processors providing a significant speedup when
  reading both old and new format images, including multipart and
  multiview files.

* Namespacing - The library introduces versioned namespaces to avoid
  conflicts between packages compiled with different versions of the
  library.

Although OpenEXR 2.0 is a major version update, files created by the
new library that don't exercise the new feature set are completely
backwards compatible with previous versions of the library. By using
the OpenEXR 2.0 library, performance improvements, namespace versions
and basic multi-part/deep reading support should be available to
applications without code modifications.

This code is designed to support Deep Compositing - a revolutionary
compositing workflow developed at Weta Digital that detached the
rendering of different elements in scene.  In particular, changes in
one layer could be rendered separately without the need to re-render
other layers that would be required to handle holdouts in a
traditional comp workflow or sorting of layers in complex scenes with
elements moving in depth.  Deep Compositing became the primary
compositing workflow on Avatar and has seen wide industry adoption.
The technique allows depth and color value to be stored for every
pixel in a scene allowing for much more efficient handling of large
complex scenes and greater freedom for artists to iterate.

True to the open source ethos, a number of companies contributed to
support the format and encourage adoption.  Amongst others, Pixar
Animation Studios has contributed its DtexToExr converter to the
OpenEXR repository under a Microsoft Public License, which clears any
concerns about existing patents in the area, and Autodesk provided
performance optimizations geared towards real-time post-production
workflows.

Extensive effort has been put in ensuring all requirements were met to
help a wide adoption, staying true to the wide success of
OpenEXR. Many software companies were involved in the beta cycle to
insure support amongst a number of industry leading
applications. Numerous packages like SideFX's Houdini, Autodesk's
Maya, Solid Angle's Arnold renderer, Sony Pictures Imageworks' Open
Image IO have already announced their support of the format.

Open EXR 2.0 is an important step in the adoption of deep compositing
as it provides a consistent file format for deep data that is easy to
read and work with throughout a visual effects pipeline.  The Foundry
has build OpenEXR 2.0 support into its Nuke Compositing application as
the base for the Deep Compositing workflows.

OpenEXR 2.0 is already in use at both Weta Digital and Industrial
Light & Magic. ILM took advantage of the new format on Marvel's The
Avengers and two highly anticipated summer 2013 releases, Pacific Rim
and The Lone Ranger.  Recent examples of Weta Digital's use of the
format also include *Marvel's Avengers* as well as
*Prometheus* and *The Hobbit*.  In addition, a large
number of visual effects studios have already integrated a deep
workflow into their compositing pipelines or are in the process of
doing so including:, Sony Pictures Imageworks, Pixar Animation
Studios, Rhythm & Hues, Fuel and MPC.

In addition to visual effects, the new additions to the format, means
that depth data can also be assigned to two-dimensional data for a use
in many design fields including, architecture, graphic design,
automotive and product prototyping.

July 31st, 2012 - OpenEXR v1.7.1 Released
=========================================

OpenEXR v1.7.1 has been released and is available for download. This release includes the following components:

*  OpenEXR: v1.7.1
*  IlmBase: v1.0.3
*  PyIlmBase: v1.0.0 (introduces a Boost dependency)
*  OpenEXR_Viewers: v1.0.2

Of particular note is the introduction of PyIlmBase. This module forms
a comprehensive set of python bindings to the IlmBase module.

In addition, contained in this release is a number of additions to
Imath as well as a minor tweak to Imath::Frustrum (for better support
for Windows platforms) as well as other minor fixes, including
correction for soname version of IlmImf.

June 18, 2012 - OpenEXR v2 Released
===================================

We're pleased to announce the first public Beta release of OpenEXR v2.

Development of OpenEXR v2 has been undertaken in a collaborative
environment (cf. previous ``github`` announcement) comprised of
Industrial Light & Magic, Weta Digital as well as a number of other
contributors.  Some of the new features included in the Beta.1 release
of OpenEXR v2 are:

* Deep Data. Pixels can now store a variable length list of samples.
  The main rationale behind deep-images is to have multiple values at
  different depths for each pixel. OpenEXR v2 supports both hard
  surface and volumetric representation requirements for deep
  compositing workflows.

* Multi-part image files. With OpenEXR v2, files can now contain a
  number of separate, but related, images in one file. Access to any
  part is independent of the others; in particular, no access of data
  need take place for unrequested parts.

In addition, OpenEXR v2 also contains platform independent mechanisms
for handling co-existing library version conflicts in the same process
space. (Currently implemented in IlmImf)

Finally, a reminder that this is a Beta release and potentially
incompatible changes may be introduced in future releases prior to the
v2.0.0 production version.

OpenEXR v2Beta.1 can be found at https://github.com/AcademySoftwareFoundation/openexr/tree/v2_beta.1.

June 18, 2012
=============

We're pleased to announce that the OpenEXR source code is moving to
``github.com``. You can browse, download and branch the code at
http://www.github.com/AcademySoftwareFoundation/openexr.

We're looking forward to taking advantage of the collaborative
features presented by ``git`` and ``github.com`` and of course
community contributions. Please see the developer Wiki pages for more
information regarding participation.

July 23, 2010
=============

New feature version of OpenEXR is now available. This release
includes support for stereoscopic images, please see the adjoining
documentation in the :doc:`MultiViewOpenEXR`. (Many thanks to
Weta Digital for their contribution.) In addition, we added support
for targeting 64 bit Windows, fixes for buffer overruns and a number
of other minor fixes, additions and optimisations. Please see the
Changelog files for more detailed information.

OpenEXR 1.7.0, OpenEXR_Viewers 1.0.2, IlmBase 1.0.2 and
OpenEXR-Images-1.7.0 can be downloaded from the `release page <https://github.com/AcademySoftwareFoundation/openexr/releases>`_.

October 22, 2007
================

New versions of OpenEXR and CTL are now available.

This release fixes a buffer overrun in OpenEXR and a Windows build
problem in CTL, and it removes a few unnecessary files from the
.tar.gz packages.

OpenEXR 1.6.1, OpenEXR_Viewers 1.0.1 and IlmBase 1.0.1 can be
downloaded from the `release page <https://github.com/AcademySoftwareFoundation/openexr/releases>`_.
CTL 1.4.1 and OpenEXR_CTL 1.0.1 can be downloaded
from
`http://www.oscars.org/science-technology/council/projects/ctl.html
<http://www.oscars.org/science-technology/council/projects/ctl.html>`_.

August 3, 2007
==============

New stable versions of OpenEXR and CTL are now available.

The source code has been tested on Linux, Mac OS X and Windows (Visual
Studio 7 and 8).


Here's a summary of what has changed since the last release:

* OpenEXR 1.6.0

  * Reduced generational loss in B44- and B44A-compressed images.

  * Added B44A compression.  This is a variation of B44, but with a
    better compression ratio for images with large uniform areas, such
    as in an alpha channel.

  * Bug fixes.
	     
* CTL 1.4.0

  * Added new functions to the CTL standard library: 3x3 matrix
    support, 1D lookup tables with cubic interpolation.

  * Added new "ctlversion" statement to the language.

  * Bug fixes.
	     
* OpenEXR_CTL 1.0.0

  * Applying CTL transforms to a frame buffer is multi-threaded.

  * Bug fixes.
	     
* OpenEXR_Viewers 1.0.0

  * Implemented new naming conventions for CTL parameters.
	     
* IlmBase 1.0.0

  * Half now implements "round to nearest even" mode.
	     
OpenEXR 1.6.0, OpenEXR_Viewers 1.0.0 and IlmBase 1.0.0 can be
downloaded from the downloads section of
www.openexr.com.  CTL 1.4.0 and OpenEXR_CTL 1.0.0 can be downloaded
from http://www.oscars.org/council/ctl.html.

January 22, 2007
================

The Color Transformation Language, or CTL, is a programming language
for digital color management.  Color management requires translating
images between different representations or color spaces.  CTL allows
users to describe color transforms in a concise and unambiguous way by
expressing them as programs.  In order to apply a given transform to
an image, a color management system instructs a CTL interpreter to
load and run the CTL program that describes the transform.

The image viewers included in the OpenEXR software distribution,
exrdisplay and playexr, both support color rendering via CTL.  For
more information see `http://www.openexr.com/OpenEXRViewers.pdf
<http://www.openexr.com/OpenEXRViewers.pdf>`_.

Source code and documentation for the CTL interpreter can be downloaded
from http://ampasctl.sourceforge.net. Please note the 
license under which CTL is distributed; it is similar but not
identical to the OpenEXR license.


January 4, 2007
===============

OpenEXR wins an Academy Award for Technical Achievement.

The Academy of Motion Picture Arts and Sciences today announced the 15
winners of Scientific and Technical Academy Awards.  A Technical
Achievement Award goes to Florian Kainz for the design and engineering
of OpenEXR, a software package implementing 16-bit, floating-point,
high dynamic range image files.  Widely adopted, OpenEXR is engineered
to meet the requirements of the visual effects industry by providing
for lossless and lossy compression of tiered and tiled images.

Congratulations to all for making OpenEXR such a success!!!

`Click here for the official Press Release <http://www.oscars.org/press/pressreleases/2007/07.01.04.html>`_

December 15, 2006
=================

A new development version of OpenEXR is now available.  We have tested
the code in this version internally at ILM, but we would like to get
feedback from others before we release a production version.

The new version includes several significant changes:

* OpenEXR supports a new image compression method, called B44.  It has
  a fixed compression rate of 2.28:1, or 4.57:1 if used in ``combination``
  with luminance/chroma encoding.  B44-compressed images can be
  uncompressed fast enough to support real-time playback of image
  sequences.

* The new playexr program plays back moving image sequences.  Playexr
  is multi-threaded and utilizes the threading capabilities of the
  IlmImf library that were introduced in OpenEXR 1.3.0.  The program
  plays back B44-compressed images with fairly high-resolution in ``real``
  time on commodity hardware.

* The playexr program and a new version of the existing exrdisplay
  image viewer both support color rendering via color transforms
  written in the new Color Transformation Language or CTL. CTL is not
  part of OpenEXR; it will be released separately. CTL support in
  playexr and exrdisplay is optional; the programs can be built and
  will run without CTL.

* In preparation for the release of CTL, OpenEXR has been split into
  three separate packages:

* IlmBase 0.9.0 includes the Half, Iex, Imath and IlmThread libraries

* OpenEXR 1.5.0 includes the IlmImf library, programming examples and
  utility programs such as exrheader or exrenvmap

* OpenEXRViewers 0.9.0 includes the playexr and exrdisplay programs

* The "Technical Introduction to OpenEXR" document now includes a
  recommendation for storing CIE XYZ pixel data in OpenEXR files.

* A new "OpenEXR Image Viewing Software" document describes the
  playexr and exrdisplay programs.  It briefly explains real-time
  playback and color rendering, and includes recommendations for
  testing if other image viewing software displays OpenEXR images
  correctly.

* The OpenEXR sample image set now includes B44-compressed files and
  files with CIE XYZ pixel data.
      
August 8, 2006
==============

We have released an updated set of sample OpenEXR images.  This
release includes several new images that are useful for testing
OpenEXR applications.  The images are organized by subdirectory
according to their image type or purpose.  Several of these
subdirectories contain README files that explain the contents of the
images in those subdirectories.

August 2, 2006
==============

OpenEXR 1.4.0 is now available.  This is the next major
production-ready release of OpenEXR and offers full compatibility with
our last production release, which was 1.2.2.  This version obsoletes
versions 1.3.x, which were test versions for 1.4.0.  If you have been
using 1.3.x, please upgrade to 1.4.0.  <hr>

June 8, 2006
============

OpenEXR 1.3.0 is now available.

This is a test release.  The major new feature in this version is
support for multithreaded file I/O.  We've been testing the threaded
code internally at ILM for a few months, and we have not encountered
any bugs, but we'd like to get some feedback from others before we
release the production version.
	     
Here's a summary of the changes since version 1.2.2:
	     
* Support for multithreaded file reading and writing.

* Support for Intel-based OS X systems.

* Support for Visual Studio 2005.

* Better handling of PLATFORM_* and HAVE_* macros.

* Updated documentation.

* Bug fixes related to handling of incomplete and damaged files.

* Numerous bug fixes and cleanups to the autoconf-based build system.

* Removed support for the following configurations that were
  previously supported.  Some of these configurations may happen to
  continue to function, but we can't help you if they don't, largely
  because we don't have any way to test them:

  * IRIX
  * OSF1
  * SunOS
  * OS X versions prior to 10.3.
  * gcc on any platform prior to version 3.3


March 15, 2005
==============

We're pleased to announce the release of OpenEXR 1.2.2.  This is a
relatively minor update to the project, with the following changes:

* New build system for Windows; support for DLLs.
* Switched documentation from HTML to PDF format.
* IlmImf: support for image layers in ``ChannelList.``
* IlmImf: added ``isComplete()`` method to file classes to check whether a
  file is complete.
* IlmImf: exposed ``staticInitialize()`` in ``ImfHeader.h`` in order to allow
  thread-safe library initialization in multithreaded applications.
* IlmImf: New "time code" standard attribute.
* exrdisplay: support for displaying wrap-around texture map images.
* exrmaketiled: can now specify wrap mode.
* IlmImf: New "wrapmodes" standard attribute to indicate extrapolation
  mode for mipmaps and ripmaps.
* IlmImf: New "key code" standard attribute to identify motion picture
  film frames.
* Imath: Removed ``TMatrix<T>`` classes; these classes are still
  under development and are too difficult to keep in sync with OpenEXR
  CVS.

August 10, 2004
===============

ILM's OpenEXR color management proposal, presented at the Siggraph
2004 "OpenEXR, Film and Color" Birds of a Feather meeting, is now
available online.  See the documentation section.

June 6, 2004
============

OpenEXR 1.2.1 is now available.  This is a fairly minor release,
mostly just a few tweaks, a few bug fixes, and some new documentation.
Here are the most important changes:

* reduced memory footprint of exrenvmap and exrmaketiled utilities.
* IlmImf: new helper functions to determine whether a file is an
  OpenEXR file, and whether it's scanline- or tile-based.
* IlmImf: bug fix for PXR24 compression with ySampling != 1.
* Better support for gcc 3.4.
* Warning cleanups in Visual C++.

May 11, 2004
============

OpenEXR 1.2.0 is now available.  This is the first official,
production-ready release since OpenEXR 1.0.7.  If you have been using
the development 1.1 series, please switch to 1.2.0 as soon as
possible.

We believe that OpenEXR 1.2.0 is ready for use in shipping
applications.  We have been using it in production at ILM for several
months now with no problems.

There are quite a few major new features in the 1.2 series as compared
to the original 1.0 series:

* Support for tiled images, including mipmaps and ripmaps.  Note that
  software based on the 1.0 series cannot read or write tiled images.
  However, simply by recompiling your software against the 1.2
  release, any code that reads scanline images can read tiled images,
  too.

* A new Pxr24 compressor, contributed by Pixar Animation Studios.
  Values produced by the Pxr24 compressor provide the same range as
  32-bit floating-point numbers with slightly less precision, and
  compress quite a bit better.  The Pxr24 compressor stores UINT and
  HALF channels losslessly, and for these data types performs
  similarly to the ZIP compressor.

* OpenEXR now supports high dynamic-range YCA (luminance/chroma/alpha)
  images with subsampled chroma channels.  These files are supported
  via the RGBA convenience interface, so that data is presented to the
  application as RGB(A) but stored in the file as YC(A).  OpenEXR also
  supports Y and YA (black-and-white/black-and-white with alpha)
  images.

* An abstracted file I/O interface, so that you can use OpenEXR with
  interfaces other than C++'s iostreams.

* Several new utilities for manipulating tiled image files.

See the downloads section to download the source code and sample
images.

Mar 27, 2004
============

OpenEXR 1.1.1 is now available.  This another development release.  We
expect to release a stable version, 1.2, around the end of April.
Version 1.1.1 includes support for PXR24 compression, and for
high-dynamic-range luminance/chroma images with subsampled chroma
channels.  Version 1.1.1 also fixes a bug in the 1.1.0 tiled file
format.

Mar 27, 2004
============

We are pleased to announce that Pixar Animation Studios has
contributed code to OpenEXR for a new lossy compression method, which
compresses 32-bit floating-point data quite a bit better than
OpenEXR's other compressors.  This new compressor is called PXR24 and
is available as of the 1.1.1 development release of OpenEXR.  It will
also be included in the upcoming 1.2 stable release.  Thanks to Loren
Carpenter and Dana Batali of Pixar, for making this happen!

Feb 6, 2004
===========

OpenEXR 1.1.0 is now available.  This is a major new release with
support for tiled images, multi-resolution files (mip/ripmaps),
environment maps, and abstracted file I/O.  We've also released a new
set of images that demonstrate these features, and updated the
CodeWarrior project and Photoshop plugins for this release.  See the
downloads section for the source code and the new images.

Jan 8, 2004
===========

Industrial Light & Magic has released the source code for an OpenEXR
Shake plugin.  The plugin is supported on Shake 3.0 on the GNU/Linux
and MacOS X platforms.  See the downloads section.

Jan 7, 2004
===========

OpenEXR 1.0.7 is now available.  In addition to some bug fixes, this
version adds support for some new standard attributes, such as primary
and white point chromaticities, lens aperture, film speed, image
acquisition time and place, and more.  If you want to use these new
attributes in your applications, see the ImfStandardAttributes.h
header file for documentation.

Our project hosting site, Savannah, is still recovering from a
compromise last month, so in the meantime, we're hosting file
downloads here.  Some of the files are not currently available, but
we're working to restore them.

April 3, 2003
=============

OpenEXR release 1.0.5 is now available.  It includes support for
Windows and improved support for OS X.  It also includes support for
hardware rendering of OpenEXR images on NVIDIA GeForce FX and Quadro
FX video cards.  See the downloads section for source code and
prebuilt packages for Windows, OS X 10.2, and RedHat.

April 3, 2003
=============

Industrial Light & Magic has released the source code for an OpenEXR
display driver for Pixar's Renderman.  This display driver is covered
under the OpenEXR free software license.  See the downloads section
for the source code.

January 22, 2003
================

openexr.com web site is officially launched.

   
