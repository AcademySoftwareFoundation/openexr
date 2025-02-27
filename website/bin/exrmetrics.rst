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

   Part number to copy, or ``all`` for all parts. Default is ``all``.
              

.. describe:: -m

   Set to multi-threaded (system selected thread count).

.. describe:: -t n

   Use a pool of ``n`` worker threads for processing files. Default is
   single threaded (no thread pool).

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


Example Usage:
--------------

Basic usage: report sizes and read time, write time, reread time.

.. code-block::

   % exrmetrics input.exr -o output.exr
   [
   {
     "file": "input.exr",
     "input file size": 3180931,
     "pixels": 396225,
     "channels": 4,
     "total raw size": 3169800,
     "compression": "none",
     "part type": "scanlineimage",
     "parts":
      [
       {
         "part": 0,
         "pixels": 396225,
         "channels": 4,
         "compression": "none",
         "part type": "scanlineimage",
         "total raw size": 3169800
       }
      ],
     "metrics":
      [
       {
         "compression": "original",
         "pixel mode": "original",
         "output size": 3180960,
         "read time": 0.0061315,
         "write time": 0.0125296,
         "re-read time": 0.00393838
       }
      ]
    }
   ]
   
      
Report read time, write time, re-read time with each available
compression type:

.. code-block::

   % exrmetrics --bench *.exr
   file name,compression,pixel mode,write time,count reread time,reread time
   input.exr,none,half,0.00147702,---,0.000437916
   input.exr,none,float,0.00124969,---,0.000425501
   input.exr,rle,half,0.00655444,---,0.00484196
   input.exr,rle,float,0.00823504,---,0.006153
   input.exr,zips,half,0.0243503,---,0.00615663
   input.exr,zips,float,0.0322755,---,0.00982083
   input.exr,zip,half,0.0224963,---,0.00438646
   input.exr,zip,float,0.0351248,---,0.00679192
   input.exr,piz,half,0.0194071,---,0.00750638
   input.exr,piz,float,0.0310976,---,0.0124893
   input.exr,pxr24,half,0.0230168,---,0.00463231
   input.exr,pxr24,float,0.0280254,---,0.00549154
   input.exr,b44,half,0.00880035,---,0.00188298
   input.exr,b44,float,0.0010236,---,0.000360021
   input.exr,b44a,half,0.00816444,---,0.00159444
   input.exr,b44a,float,0.00101096,---,0.000393375
   input.exr,dwaa,half,0.0340554,---,0.00496896
   input.exr,dwaa,float,0.0372704,---,0.00825308
   input.exr,dwab,half,0.0249792,---,0.00452442
   input.exr,dwab,float,0.0286153,---,0.0079899
   

Just convert the file, printing no metrics:   

.. code-block::

   % exrmetrics --convert -z zip input.exr -o output.exr
      
