..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

Reading and Writing Image Files with the C-language API
#######################################################

The OpenEXRCore library has a few notable features to consider prior
to beginning use:

**stateless**
  The library is largely state-free, enabling multiple plugins or just
  different areas within the same process to customize the errors
  being reported (i.e. swallow them, report them differently, etc.),
  provide custom memory allocation routines, or replace the file I/O
  routines with their own. To do this, every file is called a
  "context", and each context can be initialized with a customized set
  of routines or values. There are global defaults for all of this
  behavior, but they are just that: defaults.

**result codes**
  Every routine returns a result code (success or an error
  code). Besides some low level routines where a good error message is
  not possible, the rest of the library attempts to provide a
  meaningful message as to what is going wrong via a formatted
  message. This rule of every function returning a result code means
  that in some scenarios, the code may be slightly more verbose, but
  it should be a safer library in the long run.

The C api is designed around the fundamental units:

- Header and attributes
- Multiple parts (optional)
- Chunks of image or deep data 
- Transformations of those chunks into the requested form

All requests to read or write image or data from a file must happen in
“chunk” requests. There are API differences depending on whether the
OpenEXR part being read contains scanline data or tiled data, and to
handle deep vs shallow image files. However, largely all the same
functions are used to read and write this atomic data.

Simple Example
--------------

To get started, here is a simple code snippet that starts to read a
file:

.. code-block::
   :linenos:
      
    exr_context_initializer_t ctxtinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    exr_context_t myfile;
    exr_result_t rv = exr_start_read(&myfile, “/tmp/foo.exr”, &ctxtinit);
    if (rv != EXR_ERR_SUCCESS)
    {
      /* do something about the error */
    }

Reading and Writing
-------------------

This will open a file and parse the header. One can immediately notice
this context initializer structure. This provides a path to specify
custom handlers for I/O, memory, error reporting, etc. Writing is a
near mirror, just using ``exr_start_write()``, with the additional option
about whether to handle creation of a temporary file and rename it, or
to write directly. In both cases, along with the path to quick edit
attributes in place, the context is closed / destroyed with a call to
exr_finish.

For reading image data, once the file is opened using this
``exr_start_read()`` call, the remaining calls are all thread-safe and
as lock free as possible.

Assuming the parsing of the header and opening of the file are
successful, the remaining API then takes both this context that is
initialized as well as a part index. In this way, the C library
provides transparent, integrated multi-part OpenEXR support, and does
not further differentiate between the variants. One can query how many
parts are in the file via ``exr_get_count()``. Furthermore, there are
a wealth of routines provided to access the attributes (metadata)
contained within the file. These will not be described here beyond
where necessary.

There is a set of functions to query (or set while writing) the
information for a particular part index. They are too numerous to list
here, but there are functions to query the required attributes of the
file, and then more generic type-specific functions to access
attributes by type. Further, the attributes can be accessed either in
a sorted order, or the order within which they appear in the file. One
example might be to retrieve the data window:

.. code-block::
   :linenos:

    exr_attr_box2i_t datawindow;
    exr_result_t rv = exr_get_data_window(ctxt, 0, &datawindow);

Data Types
----------

Notice that this example does not use the traditional Imath types
here. The Imath types C++ objects with a rich set of functions, but
are not viable in a C api. The types for the C layer are only provided
to transport the data, and not to operate on the data. But with the
new type conversion constructors present in version 3.1 of Imath,
types such as the vector types (i.e. ``exr_attr_v3f_t``) should
transparently convert to their respective Imath type.

Chunks
------

Once the calling program has created the context, inspected the header
data to decide the next course of action, and started to operate on a
file, one must operate in chunk units. For a scanline-based file, this
means requesting scanlines in groups of scanlines contained in one
chunk. The routine ``exr_get_scanlines_per_chunk()`` returns this
count. This is currently 1, 16, or 32. For tiled files, the base unit
of I/O is a tile itself: the chunk is a tile. One can use the routines
``exr_get_tile_descriptor()``, ``exr_get_tile_levels()``, and
``exr_get_tile_sizes()`` as utilities to introspect tile information.

In order to read data, use ``exr_read_scanline_block_info()`` or
``exr_read_tile_block_info()`` to initialize a structure with the data
to read one of these chunks of data. Then there are the corresponding
``exr_read_chunk()``, ``exr_read_deep_chunk()`` which read the
data. Analogously, there are write versions of these functions.

Encode and Decode
-----------------

The above is very simple, but only provides the raw, compressed and
packed data. These are preferred if you are writing a utility to
copy, combine, or split out parts of EXR files and just want the raw
data blocks. However, to actually use the data in an application, the
encoding and decoding pipeline methods should be used. These provide
and combine the read step above, such that they are free to optimize
the data path when the data is uncompressed.

The decoding pipeline consists of a structure that contains relevant
channel and data type information, in addition to allocation routines,
function pointers to specialize the stages of the pipeline, and
pointers to memory buffers to use. This enables the calling
application to re-use decode pipeline buffers (where it can determine
thread safety), and avoid constant memory allocation / deallocation
while performing such tasks as reading scanlines of an image into one
large buffer. The decode pipeline consists of 3 (4 when reading deep)
simple steps:

1. Read the data
2. De-compress the data (if it is compressed)
3. Optionally update allocation based on sample data read (deep only)
4. Unpack the data (re-orient from the on-disk representation)

These decoding pipelines (or the mirror for encoding) provide the
caller with the ability to use the in-built routines for portions of
these steps (``exr_decoding_choose_default_routines()``), but then
customize the stages that make the most sense. So, for simplicity, one
could imagine implementing a GPU decoder to use the provided routines
for reading and decompressing, but then instead of interleaving the
data on the CPU, instead provide a custom routine to do that step on
the GPU by overriding the function pointer on the decoding structure.

Once you have decoded or encoded all the chunks required, it is
expected you will call ``exr_decoding_destroy()`` which will clean up
all the buffers associated with that instance of the decoding
pipeline. If you will be reading an entire image at once, it is
recommended to initialize the decoding pipeline once Regardless of
using the raw chunk API, or the richer decoding pipeline, both paths
start with a call to query information about the chunk to read, using
either ``exr_read_scanline_block_info()`` or
``exr_ead_tile_block_info()``. This fills in and initializes a
structure with information for that chunk, including how many bits
would result from unpacking that chunk, and it’s raw position on disk.

Reference
---------

Basic Types
^^^^^^^^^^^

.. doxygentypedef:: exr_result_t
                    

Basic Enumerated Types
^^^^^^^^^^^^^^^^^^^^^^

.. doxygenenum:: exr_compression_t
.. doxygenenum:: exr_envmap_t
.. doxygenenum:: exr_lineorder_t
.. doxygenenum:: exr_storage_t
.. doxygenenum:: exr_tile_level_mode_t
.. doxygenenum:: exr_tile_round_mode_t
.. doxygenenum:: exr_pixel_type_t
.. doxygenenum:: exr_attr_list_access_mode
.. doxygenenum:: exr_perceptual_treatment_t
.. doxygenenum:: exr_default_write_mode

Global State
^^^^^^^^^^^^

.. doxygentypedef:: exr_memory_allocation_func_t
.. doxygentypedef:: exr_memory_free_func_t

.. doxygenfunction:: exr_get_library_version
.. doxygenfunction:: exr_set_default_maximum_image_size
.. doxygenfunction:: exr_get_default_maximum_image_size 
.. doxygenfunction:: exr_set_default_maximum_tile_size
.. doxygenfunction:: exr_get_default_maximum_tile_size
.. doxygenfunction:: exr_set_default_memory_routines

Chunk Reading
^^^^^^^^^^^^^

.. doxygenfunction:: exr_read_scanline_chunk_info
.. doxygenfunction:: exr_read_tile_chunk_info
.. doxygenfunction:: exr_read_chunk
.. doxygenfunction:: exr_read_deep_chunk

Chunks
^^^^^^

.. doxygenfunction:: exr_get_chunk_table_offset
.. doxygenstruct:: exr_chunk_info_t

Chunk Writing
^^^^^^^^^^^^^

.. doxygenfunction:: exr_write_scanline_chunk_info
.. doxygenfunction:: exr_write_tile_chunk_info
.. doxygenfunction:: exr_write_scanline_chunk
.. doxygenfunction:: exr_write_deep_scanline_chunk
.. doxygenfunction:: exr_write_tile_chunk
.. doxygenfunction:: exr_write_deep_tile_chunk

Open for Read
^^^^^^^^^^^^^

.. doxygenfunction:: exr_test_file_header
.. doxygenfunction:: exr_start_read

Open for Write
^^^^^^^^^^^^^^

.. doxygenfunction:: exr_start_write
.. doxygenfunction:: exr_start_inplace_header_update
.. doxygenfunction:: exr_write_header
.. doxygenfunction:: exr_set_longname_support

Close
^^^^^

.. doxygentypedef:: exr_destroy_stream_func_ptr_t

.. doxygenfunction:: exr_finish


Context
^^^^^^^

.. doxygentypedef:: exr_context_t
.. doxygentypedef:: exr_const_context_t

.. doxygenstruct:: _exr_context_initializer_v3
   :members:
.. doxygentypedef:: exr_context_initializer_t

.. doxygenfunction:: exr_get_file_name
.. doxygenfunction:: exr_get_file_version_and_flags
.. doxygenfunction:: exr_get_user_data
.. doxygenfunction:: exr_register_attr_type_handler

Decoding
^^^^^^^^

.. doxygenstruct:: _exr_decode_pipeline
   :members:
.. doxygentypedef:: exr_decode_pipeline_t

.. doxygenfunction:: exr_decoding_initialize
.. doxygenfunction:: exr_decoding_choose_default_routines
.. doxygenfunction:: exr_decoding_update
.. doxygenfunction:: exr_decoding_run
.. doxygenfunction:: exr_decoding_destroy

Encoding
^^^^^^^^

.. doxygenenum:: exr_transcoding_pipeline_buffer_id
                    
.. doxygenstruct:: _exr_encode_pipeline
   :members:
.. doxygentypedef:: exr_encode_pipeline_t
      
.. doxygenstruct:: exr_coding_channel_info_t
   :members:
   :undoc-members:

.. doxygenfunction:: exr_encoding_initialize
.. doxygenfunction:: exr_encoding_choose_default_routines
.. doxygenfunction:: exr_encoding_update
.. doxygenfunction:: exr_encoding_run
.. doxygenfunction:: exr_encoding_destroy

Attribute Values
^^^^^^^^^^^^^^^^



.. doxygenstruct:: exr_attr_chromaticities_t
   :members:
   :undoc-members:
      
.. doxygenstruct:: exr_attr_keycode_t
   :members:
   :undoc-members:

.. doxygenenum:: exr_attribute_type_t

.. doxygenstruct:: exr_attribute_t
   :members:
   :undoc-members:
      
.. doxygenstruct:: exr_attr_v2i_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_v2f_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_v2d_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_v3i_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_v3f_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_v3d_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_m33f_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_m33d_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_m44f_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_m44d_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_box2i_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_box2f_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_string_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_string_vector_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_float_vector_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_chlist_entry_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_chlist_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_preview_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_tiledesc_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_timecode_t
   :members:
   :undoc-members:

.. doxygenstruct:: exr_attr_opaquedata_t
   :members:
   :undoc-members:

Reading
^^^^^^^

.. doxygentypedef:: exr_read_func_ptr_t
.. doxygentypedef:: exr_query_size_func_ptr_t

.. doxygenfunction:: exr_get_count
.. doxygenfunction:: exr_get_name
.. doxygenfunction:: exr_get_storage
.. doxygenfunction:: exr_get_tile_levels
.. doxygenfunction:: exr_get_tile_sizes
.. doxygenfunction:: exr_get_level_sizes
.. doxygenfunction:: exr_get_chunk_count
.. doxygenfunction:: exr_get_scanlines_per_chunk
.. doxygenfunction:: exr_get_chunk_unpacked_size

.. doxygenfunction:: exr_get_attribute_count
.. doxygenfunction:: exr_get_attribute_by_index
.. doxygenfunction:: exr_get_attribute_by_name
.. doxygenfunction:: exr_get_attribute_list
.. doxygenfunction:: exr_attr_declare_by_type
.. doxygenfunction:: exr_attr_declare
.. doxygenfunction:: exr_initialize_required_attr
.. doxygenfunction:: exr_initialize_required_attr_simple
.. doxygenfunction:: exr_copy_unset_attributes

.. doxygenfunction:: exr_get_channels
.. doxygenfunction:: exr_get_compression
.. doxygenfunction:: exr_get_data_window
.. doxygenfunction:: exr_get_display_window
.. doxygenfunction:: exr_get_lineorder
.. doxygenfunction:: exr_get_pixel_aspect_ratio
.. doxygenfunction:: exr_get_screen_window_center
.. doxygenfunction:: exr_get_screen_window_width
.. doxygenfunction:: exr_get_tile_descriptor
.. doxygenfunction:: exr_get_version

.. doxygenfunction:: exr_attr_get_box2i
.. doxygenfunction:: exr_attr_get_box2f
.. doxygenfunction:: exr_attr_get_channels
.. doxygenfunction:: exr_attr_get_chromaticities
.. doxygenfunction:: exr_attr_get_compression
.. doxygenfunction:: exr_attr_get_double
.. doxygenfunction:: exr_attr_get_envmap
.. doxygenfunction:: exr_attr_get_float
.. doxygenfunction:: exr_attr_get_float_vector
.. doxygenfunction:: exr_attr_get_int
.. doxygenfunction:: exr_attr_get_keycode
.. doxygenfunction:: exr_attr_get_lineorder
.. doxygenfunction:: exr_attr_get_m33f
.. doxygenfunction:: exr_attr_get_m33d
.. doxygenfunction:: exr_attr_get_m44f
.. doxygenfunction:: exr_attr_get_m44d
.. doxygenfunction:: exr_attr_get_preview
.. doxygenfunction:: exr_attr_get_rational
.. doxygenfunction:: exr_attr_get_string
.. doxygenfunction:: exr_attr_get_string_vector
.. doxygenfunction:: exr_attr_get_tiledesc
.. doxygenfunction:: exr_attr_get_timecode
.. doxygenfunction:: exr_attr_get_v2i
.. doxygenfunction:: exr_attr_get_v2f
.. doxygenfunction:: exr_attr_get_v2d
.. doxygenfunction:: exr_attr_get_v3i
.. doxygenfunction:: exr_attr_get_v3f
.. doxygenfunction:: exr_attr_get_v3d
.. doxygenfunction:: exr_attr_get_user

Writing
^^^^^^^

.. doxygentypedef:: exr_write_func_ptr_t

.. doxygenfunction:: exr_add_part

.. doxygenfunction:: exr_add_channel
.. doxygenfunction:: exr_set_channels
.. doxygenfunction:: exr_set_compression
.. doxygenfunction:: exr_set_data_window
.. doxygenfunction:: exr_set_display_window
.. doxygenfunction:: exr_set_lineorder
.. doxygenfunction:: exr_set_pixel_aspect_ratio
.. doxygenfunction:: exr_set_screen_window_center
.. doxygenfunction:: exr_set_screen_window_width
.. doxygenfunction:: exr_set_tile_descriptor
.. doxygenfunction:: exr_set_name
.. doxygenfunction:: exr_set_version
.. doxygenfunction:: exr_set_chunk_count

.. doxygenfunction:: exr_attr_set_box2i
.. doxygenfunction:: exr_attr_set_box2f
.. doxygenfunction:: exr_attr_set_channels
.. doxygenfunction:: exr_attr_set_chromaticities
.. doxygenfunction:: exr_attr_set_compression
.. doxygenfunction:: exr_attr_set_double
.. doxygenfunction:: exr_attr_set_envmap
.. doxygenfunction:: exr_attr_set_float
.. doxygenfunction:: exr_attr_set_float_vector
.. doxygenfunction:: exr_attr_set_int
.. doxygenfunction:: exr_attr_set_keycode
.. doxygenfunction:: exr_attr_set_lineorder
.. doxygenfunction:: exr_attr_set_m33f
.. doxygenfunction:: exr_attr_set_m33d
.. doxygenfunction:: exr_attr_set_m44f
.. doxygenfunction:: exr_attr_set_m44d
.. doxygenfunction:: exr_attr_set_preview
.. doxygenfunction:: exr_attr_set_rational
.. doxygenfunction:: exr_attr_set_string
.. doxygenfunction:: exr_attr_set_string_vector
.. doxygenfunction:: exr_attr_set_tiledesc
.. doxygenfunction:: exr_attr_set_timecode
.. doxygenfunction:: exr_attr_set_v2i
.. doxygenfunction:: exr_attr_set_v2f
.. doxygenfunction:: exr_attr_set_v2d
.. doxygenfunction:: exr_attr_set_v3i
.. doxygenfunction:: exr_attr_set_v3f
.. doxygenfunction:: exr_attr_set_v3d
.. doxygenfunction:: exr_attr_set_user


                     
Error Handling
^^^^^^^^^^^^^^

.. doxygenenum:: exr_error_code_t

.. doxygentypedef:: exr_error_handler_cb_t

.. doxygenfunction:: exr_get_default_error_message
.. doxygenfunction:: exr_get_error_code_as_string

Debugging
^^^^^^^^^

.. doxygenfunction:: exr_print_context_info

