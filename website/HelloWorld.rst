..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _Hello:

Hello, World
############

.. toctree::
   :caption: Hello, World
   :maxdepth: 1

A simple program to write a simple ``.exr`` file of an image of 10x10
pixels with values that are a ramp in green and blue:

.. literalinclude:: src/writer/writer.cpp

And the ``CMakeLists.txt`` file to build:
   
.. literalinclude:: src/writer/CMakeLists.txt

To build:

.. literalinclude:: src/writer/build.sh

For more details, see :ref:`The OpenEXR API`.

And a simple program to read an ``.exr`` file:

.. literalinclude:: src/reader/reader.cpp


And the ``CMakeLists.txt`` file to build:
   
.. literalinclude:: src/reader/CMakeLists.txt

To build:

.. literalinclude:: src/reader/build.sh


              
