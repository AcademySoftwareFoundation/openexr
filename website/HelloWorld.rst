..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _Hello:

Hello, World
############

.. toctree::
   :caption: Hello, World
   :maxdepth: 1

Write an Image
==============

This example :download:`exrwriter.cpp <src/exrwriter/exrwriter.cpp>`
program writes a simple ``hello.exr`` file of an image of 10x10 pixels with
values that are a ramp in green and blue:

.. literalinclude:: src/exrwriter/exrwriter.cpp

And the :download:`CMakeLists.txt <src/exrwriter/CMakeLists.txt>` file to build:
   
.. literalinclude:: src/exrwriter/CMakeLists.txt

To build:

.. literalinclude:: src/exrwriter/build.sh

For more details, see :ref:`The OpenEXR API`.

Read an Image
=============

This companion example :download:`exrreader.cpp <src/exrreader/exrreader.cpp>`
program reads the ``hello.exr`` file written by the writer program above:

.. literalinclude:: src/exrreader/exrreader.cpp

And the :download:`CMakeLists.txt <src/exrreader/CMakeLists.txt>` file to build:
   
.. literalinclude:: src/exrreader/CMakeLists.txt

To build:

.. literalinclude:: src/exrreader/build.sh


              
