..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

exrinfo
#######

::
   
    exrinfo [-v|--verbose] [-a|--all-metadata] [-s|--strict] <filename> [<filename> ...]

Description
-----------

Read exr files and print values of header attributes

Color Space Metadata Warnings
-----------------------------

For each part, ``exrinfo`` checks the ``colorInteropID`` attribute against
the other color space metadata and prints a warning to standard error for
any combination that leaves the part's color space ambiguous or contradictory:

- A part after the first has a ``colorInteropID`` that is neither ``data``
  nor equal to the first part's, so it does not conform to the shared
  attribute rules.

- ``colorInteropID`` denotes one of the color spaces with defined
  chromaticities, the part also has a ``chromaticities`` attribute, and they
  are not those chromaticities.

- ``colorInteropID`` is ``data``, marking the part as deliberately not
  color managed, yet the part carries a ``chromaticities``,
  ``whiteLuminance`` or ``adoptedNeutral`` attribute.

- ``colorInteropID`` is present but empty. It should be omitted, or set to
  ``unknown``, instead.

- ``acesImageContainerFlag`` is present but is not an ``int`` of value 1.

- ``acesImageContainerFlag`` is present, which asserts compliance with SMPTE
  ST 2065-4 and so the color space must be ACES2065-1, but the
  ``colorInteropID`` is present and is not ``lin_ap0_scene``, or the
  ``chromaticities`` attribute is missing or is not that of
  ``lin_ap0_scene``. ST 2065-4 requires the ``chromaticities``.

A part with neither ``colorInteropID`` nor ``acesImageContainerFlag`` never
produces a warning.

While OpenEXR should only contain linear color spaces, no attempt is made to
warn regarding the presence of a non-linear colorInteropID.

None of these make a file malformed or unsafe to read, so warnings do not
affect the exit status. Use ``exrcheck`` to test whether a file is
well formed. Note that ``--strict`` rejects a file that breaks the shared
attribute rule outright, and so prints no header or warnings at all for it.

The same checks are available to applications as
``Imf::checkColorMetadata()`` and ``exr_check_color_metadata()``.

Options:
--------

.. describe:: -s, --strict

              strict mode

.. describe:: -a, --all-metadata

              print all metadata

.. describe:: -v, --verbose

              verbose mode

.. describe:: -h, --help

              print this message

.. describe:: --version

              print version information

