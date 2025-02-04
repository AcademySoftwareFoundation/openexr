..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

exrmetrics
##########

::
   
    exrmetrics [options] infile [infile2...] [-o outfile]

Description
-----------

Read an OpenEXR image from infile, write an identical copy to outfile reporting time taken to read/write and file sizes.

Options:
--------

.. describe:: -o file

   File to write to. If no file specified, uses a memory buffer.

   Note: file may be overwritten multiple times during tests


.. describe:: -p n

   Part number to copy, or ``all for all parts. Default is ``all``.
              

.. describe:: -m

   Set to multi-threaded (system selected thread count).

.. describe:: -t n

   Use ``n`` threads for processing files. Default is single / no threads.

.. describe:: -l level

   Set DWA or ZIP compression level.

.. describe:: -z,--compression list

   List of compression methods to test (``none/rle/zips/zip/piz/pxr24/b44/b44a/dwaa/dwab,orig,all``).

   Default is ``orig``: retains original method.

.. describe::  --convert

   Shorthand options for writing a new file with no metrics:

   ``-p all --type orig --time none --type orig --no-size --passes 1``

   Change pixel type or compression by specifying ``--type`` or ``-z`` after ``--convert``.
   

.. describe:: --bench

   Shorthand options for robust performance benchmarking:

   ``-p all --compression all --time write,reread --passes 10 --type half,float --no-size --csv``

.. describe:: -16 rgba|all

   [DEPRECATED] force 16 bit half float: either just RGBA, or all channels. Use ``--type half`` or ``--type mixed`` instead.

.. describe:: --pixelmode list

   List of pixel types to use (``float,half,mixed,orig``). ``mixed`` uses half for RGBA, float for others. Default is ``orig``.

.. describe:: --time list

   Comma-separated list of operations to report timing for.
   Operations can be any of ``read,write,reread`` (use ``--time none`` for no
   timing)

.. describe:: --no-size

   Don't output size data.

.. describe:: --json

   Print output as JSON dictionary (default).

.. describe:: --csv

   Print output in csv mode. If ``passes>1``, show median timing. Default is ``JSON``.

.. describe:: --passes num

   Write and re-read file num times (default is 1)

.. describe::  -h, --help

   Print this message

.. describe::  -v

   Output progress messages

.. describe::  --version
   
   Print version information
