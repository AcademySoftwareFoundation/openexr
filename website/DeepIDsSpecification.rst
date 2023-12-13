
..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

OpenEXR Deep IDs Specification
##############################


Introduction
============

Deep IDs are primarily used in compositing applications to select
objects in images:

-  The 3D renderer stores in the final OpenEXR file multiple ids per
   pixel as well a scene manifest providing a mapping to a
   human-readable identifier, like an object or material name.
-  The compositing application, reads in the deep IDs and implements a
   UI for the artist to create a selection based on the manifest data.
-  The selected IDs will then be used to create a deep (per-sample) or
   shallow (per-pixel) mask for later use.

Typical uses include:

-  Grading selected objects.
-  Removing a certain percentage of particles from a snow or dust motes
   render.
-  Adding consistent variations to different individuals in a crowd or to different
   trees in a forest by applying an ID-based color correction.

Deep IDs can also be used for debugging:

-  To identify an object/material which renders incorrectly.
-  To collect all visible IDs over a sequence so as to prune scene
   objects that are never on screen.

Pros
----

-  Each pixel can contain an arbitrary number of IDs (0 included),
   allowing for perfect mattes.
-  Deep IDs are combined with coverage data to support anti-aliasing,
   motion blur and depth of field.
-  A deep image store the color of the individual objects as well as the
   coverage, so an object extracted by ID will have the correct color
   values along the edges. By contrast, schemes which only store pixel coverage
   cannot isolate objects without color contamination

Cons
----

-  IDs are not directly inspectable with a simple image viewer.
-  Hash collisions can make multiple objects map to the same ID.
-  DeepIDs require software to support deepscanline/deeptiled OpenEXR images

Deep ID Basics
==============


The deep OpenEXR files need to contain the following elements:

-  Deep IDs stored using one or two ``Imf::UINT`` (``uint32_t``)
   channels:

   -  A single id channel stores 32 bits hashes.
   -  A pair of id, id2 channels stores 64 bits hashes.

-  Manifest data stored in the file’s metadata as an attribute, or in a side-car file.

   -  **NOTE**:  OpenEXR 3.0+ provides a data structure
      and attribute for efficient storage. For more details see :ref:`idmanifest-label`


Sample storage
==============


Terminology
-----------

-  **Channel**: Every deep image contains many named channels (``R``,
   ``G``, ``B``, ``A``, ``Z``, ``id``, etc)
-  **Pixel**: A discrete image element containing 0 or more deep
   ``samples``.
-  **Sample**: A container storing the value of each channel at a
   specific depth.
-  **Id**: A unique numerical identifier mapping to an ``item``.
-  **Kind**: a named ``id`` category (id, particleid, instanceid,
   objectid, etc).
-  **Manifest**: a data structure mapping each id to one ``item``.
-  **Item**: an entity (object, material, etc) represented by:

   -  An artist-friendly string in the manifest, i.e. “Chair16”.
   -  A collection of ``samples`` matching a particular ``id`` in the
      image.

Principles
----------

1. Deep IDs have a single ``id`` of each ``kind`` per ``sample``, and
   every ``item`` in the image has a consistent ``id`` for all its
   ``samples``.
2. If two different ``items`` overlap the same space, they will be
   stored in separate ``samples`` (which themselves may or may not
   overlap).
3. Some ``id`` may not have an associated manifest, like ``particleid``,
   and still be useful (for example through image-based picking).
4. In complex cases like an instanced particle system, each ``sample``
   may have an ``instanceid``, ``particleid``, and ``id``/``objectid``.

Standard ID Channel names
-------------------------

============== ======================================
Name           Contents
============== ======================================
**id**         by default, an object identifier  [1]_
**objectid**   identifier of an object
**materialid** identifier of an material
**particleid** identifier of a particle
**instanceid** identifier of an instanced object
============== ======================================


To limit the risk of hash collision, a ``uint64_t`` bits can be encoded
with two ``uint32_t`` channels. The convention is then to suffix the
channel name with ``0`` or ``1`` to indicate the channels storing the
least and the most significant 32 bits respectively,
e.g. ``particleid0`` and  ``particleid1`` or ``particle.id0`` and ``particle.id1``

When sorted alphanumerically, the channel storing the most significant bits should appear immediately
after the chanel storing the least significant bits.
See appendix for details

ID generation
-------------

Any non-uint32 identifier can be hashed to generate an id.

-  ``uint32_t`` hashes should be generated with ``MurmurHash3_x86_32``.
-  ``uint64_t`` hashes should be generated with the top 8 bytes of
   ``MurmurHash3_x64_128``.

OpenEXR 3.x offers two convenience functions to hash strings  [2]_:
``IDManifest::MurmurHash32`` and ``IDManifest::MurmurHash64``.

Multivariate IDs
----------------

Hashing more than one ``kind`` (i.e. object + material) limits storage
requirements without impairing the artist’s ability to generate
arbitrary mattes.

For examples hashing the object and material names together is common
practice. In that case, a single ``id`` will map to 2 ``kinds`` in the
manifest, providing more flexibility at the cost of a slightly increased
risk of hash collision.

Manifest data
-------------

The manifest contains the human-readable data corresponding to a given
hash. It is a big table of strings that may require more storage than
the actual image data. It can be stored using the following mechanisms:

.. _idmanifest-label:

OpenEXR idManifest container
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Since OpenEXR 3.0, there is a new standard ``idManifest`` attribute
using a ``CompressedIDManifest`` metadata type, specially designed to
transport manifest data efficiently. It is optimized to reduce the storage space required,
and is the most standard approach.

The utility ``exrmanifest`` outputs the manifest of EXR images as plain text.

OpenEXR string container
^^^^^^^^^^^^^^^^^^^^^^^^

The manifest can be stored in ``string`` or ``stringvector`` attributes,
but this is not very efficient and may significantly increase file size.

Side-car files
^^^^^^^^^^^^^^

Alternatively, the manifest may be stored in a separate file, with an OpenEXR attributes,
a database or a file naming convention used to associate one or more OpenEXR files
with the corresponding sidecar file. Sidecar files can be advantageous because
they can be shared between different images, and also updated as more content is being rendered.

Such schemes are not supported by the OpenEXR library, nor are they defined here,
since that is outside the scope of the OpenEXR file format specification.
Although sidecar files may be appropriate for temporary usage, it is strongly recommended
that the embedded manifest is used in OpenEXR images which are to be shared between different companies
or for archival usage.

Example code
============

OpenEXR provides two example tools, ``deepidexample`` and ``deepidselect``.
Compiled tools will be found in the ``src/examples`` folder in the build directory. They are not installed.


DeepIDExample
-------------

``deepidexample`` creates a deep image with multiple objects (two different shapes at one of three sizes),
in one of seven colors. It is intended as a tool for generating test sequences and as an example of code
that generates an image with deep IDs and a manifest.

``deepidexample`` can generate a sequence of frames, to help test that the IDs are consistently
generated and selected. Specify ``--frame`` for the frame number. The animation cycles every 100 frames.
This ``bash`` command generates a sequence of frames:

.. code:: bash

     for x in `seq 1000 1100` ; do ./deepidexample --frame $x output.$x.deep.exr ; done

Run ``deepidexample`` to see further options.


DeepIDSelect
------------

``deepidselect`` selects individual objects within a deep file, and outputs just those objects.
It is intended to serve as an example of parsing idmanifests to find compile a list of IDs which
match a given substring, and using those ids to identify samples. Its usage is not limited solely
to files created by deepidexample; it should handle files with arbitrary channel names and manifests.
deepidselect supports the ``id64`` scheme with the ``--64`` flag.

In basic usage, specify ``input.deep.exr (matches) output.deep.exr``

``matches`` is a list of one or more separate strings. All objects whose names contain any of the
given substring will be included in output.deep.exr (it is a logical OR of the arguments)
The ``--and`` can be used to force matching of (one or more of) the following match as well as the previous.
For example, ``blue --and circle`` will match any object which is both blue, and a circle.
``blue green --and big small --and circle`` will match blue or green objects,
and which are big or small, and which are circles.
This could also be read as `( blue or green ) and ( big or small ) and ( circle )`

Each match can be limited to a given component name by specifying ``component:match``.
For example ``model:bl`` will match objects whose model is ``blob`` but not ones whose material is ``blue``.
Specifying a channel name followed by a number will select the object by number, rather than by name.
For example, ``particleid:12`` will select the object with particle ID 12.
(Note that this feature means it is not possible to have a purely numeric substring match with this tool)

``--mask`` outputs a shallow single channel image which indicating the relative coverage of each pixel
for the selected object. For schemes where the deep image only contains ID (and alpha) information,
but does not store color, this can be used to grade only the selected object.
Edge contamination may be observed along transparent edges of a selected object, if an object behind it is not selected.

To keep the code simple, ``deepidselect`` is only a minimal example of string matching against ID manifests.
For example, it doesn't support regular expressions, or more advanced boolean logic including negative matches.


Appendix
========


64 to 2 x 32 bits conversion and back
-------------------------------------

To limit the risk of hash collision, a ``uint64_t`` can be encoded in 2
``uint32_t`` channels, like ``materialid`` and ``materialid2``, using
little-endian byte ordering.

.. code:: cpp

   #include <cstdint>
   #include <iostream>
   #include <iomanip>

   int main()
   {
       using namespace std;

       // uint 64 input
       uint64_t x = 0x12345678'87654321ULL;
       cout << setw(20) << "uint64 input: " << hex << x << endl;

       // Convert one uint 64 -> two uint 32
       uint32_t lo = uint32_t(x);
       uint32_t hi = uint32_t(x >> 32);
       cout << setw(20) << "uint32 low: " << hex << lo << "  high: " << hi << endl;

       // Convert two uint32 -> one uint64
       uint64_t y = (uint64_t(hi) << 32) | lo;
       cout << setw(20) << "uint64 recombined: " << hex << y << endl;
   }

Output:

::

         uint64 input: 1234567887654321
           uint32 low: 87654321  high: 12345678
    uint64 recombined: 1234567887654321

Computing a shallow mask from Deep IDs
--------------------------------------

A shallow mask is a pixel-level mask that can be used with non-deep
compositing operators.

Here is the pseudo-code to correctly compute an ID selection mask for a
single pixel:

.. code:: python

   total_combined_alpha = 0.0
   mask_alpha = 0.0
   sorted_pixel = sort_pixel_front_to_back(input_pixel)

   foreach(sample in sorted_pixel):
       if id_is_in_selection(sample.id):
           mask_alpha += sample.alpha * (1.0 - total_combined_alpha)
       total_combined_alpha += sample.alpha * (1.0 - total_combined_alpha)

   if total_combined_alpha == 0.0:
       return 0.0
   else:
       return mask_alpha / total_combined_alpha

.. [1]
   See `OpenEXR reserved channel
   names <https://openexr.com/en/latest/TechnicalIntroduction.html#deep-data-special-purpose-channels-and-reserved-channel-names>`__.

.. [2]
   See
   `ImfIDManifest <https://github.com/AcademySoftwareFoundation/openexr/blob/main/src/lib/OpenEXR/ImfIDManifest.h>`__
