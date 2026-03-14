..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

.. _Symbol Visibility in OpenEXR:

Symbol Visibility in OpenEXR
############################

Managing symbol visibility in a C++ library can reduce library sizes,
and with the extra information, the optimizer may produce faster
code. To take advantage of this, OpenEXR 3.0 is switching to
explicitly manage symbols on all platforms, with a hidden-by-default
behavior on unix-based platforms. Managing symbols has always been
required for Windows DLLs, where one must explicitly tag functions for
import and export as appropriate.

For C, this is trivial: just tag public functions or global variable
as default visibility and leave everything else defaulting to
hidden. However, in C++, this is not exactly the same story. Functions
and globals are of course the same. And class member functions are
largely the same, and other than the attribute specification
mechanics, follow the same rules between gcc, clang, and
msvc. However, types have richer information than they do in C. So,
unless extra work is done, concepts for RTTI like the typeinfo and the
vtable for virtual classes will be hidden, and not visible. These are
referred to as "vague" linkage objects in some discussions.

It is with the "vague" linkage objects where different properties
arise. For example, if you have a template, it is happily instantiated
in multiple compile units. If the typeinfo is hidden for one library,
then this may cause things like dynamic_cast to fail because then the
same typeinfo is not used, and even though one might think that
``ImfAttribute<Imath::Box2i>`` are the same in two places, because they
are instantiated in separate places, they may be considered different
types. To compound the issue, there are different rules for this in
different implementations. For example, a default gcc under linux
allows one to link against otherwise private "vague" linkage objects
such that the typeinfo ends up as the same entity. clang, for MacOS
anyway, follows a stricter approach and keeps those types separate,
perhaps due to the two level namespace they maintain for symbols.

Unfortunately, this is not clearly discussed as an overview of the
differences between platforms, hence this document to add
clarity. Each compiler / platform describes their own behavior, but
not how that behaves relative to others. `libc++
<https://libcxx.llvm.org/docs/DesignDocs/VisibilityMacros.html>`_ from
the llvm project is the closest to providing comparative information,
where by looking at how they define their macros and the comments
surrounding, one can infer the behavior among at least windows DLL
mode, then gcc vs. clang for unixen. Other compilers, for example,
Intel's icc, tend to adopt the behavior of the predominant compiler
for that platform (i.e. msvc under windows, gcc under linux), and so
can generally adopt that behavior and are ignored here. If this is not
true, the ifdef rules in the various library ``Export.h`` headers
within OpenEXR may need to be adjusted, and this table updated.

As a summary, below is a table of the attribute or declspec that needs
to be used for a particular C++ entity to be properly exported. This
does not address weak symbols, ABI versioning, and only focusing on
visibility. Under Windows DLL rules, if one exports the entire class,
it also exports the types for the member types as well, which is not
desired, so these are marked as N/A even though the compiler does
allow that to happen.

.. list-table::
   :header-rows: 1
   :align: left

   * - C++ vs Compiler
     - MSVC
     - mingw
     - gcc
     - clang
   * - function
     - ``dllexport/dllimport``
     - ``dllexport/dllimport``
     - ``visibility("default")``
     - ``visibility("default")``
   * - hide a function
     - N/A
     - N/A
     - ``visibility("hidden")``
     - ``visibility("hidden")``
   * - ``class(typeinfo)``
     - N/A
     - N/A
     - ``visibility("default")``
     - ``visibility("default")``
   * - template class
     - N/A
     - N/A
     - ``visibility("default")``
     - ``type_visibility("default")``
   * - template data
     - N/A
     - N/A
     - ``visibility("default")``
     - ``visibility("default")``
   * - class template instantiation
     - ``dllexport/dllimport``
     - N/A
     - N/A
     - ``visibility("default")``
   * - enum
     - N/A
     - N/A
     - auto unhides (N/A)
     - ``type_visibility("default")``
   * - extern template
     - N/A
     - ``dllexport/dllimport``
     - ``visibility("default")``
     - ``visibility("default")``

With this matrix in mind, we can see the maximal set of macros we need to
provide throughout the code. *NB*: This does not mean that we need to
declare all of these, just that they might be needed. ``XXX`` should be
substituted for the particular library name being compiled
(``OPENEXR``, ``OPENEXRUTIL``, ``OPENEXRCORE``, ``IEX``,
``ILMTHREAD``, ``IMATH``).

.. list-table::
   :header-rows: 1
   :align: left

   * - macro name
     - purpose
   * - ``XXX_EXPORT``
     - one of export or import for windows, visibility for others
   * - ``XXX_EXPORT_TYPE``
     - for declaring a class / struct as public (for typeinfo / vtable)
   * - ``XXX_HIDDEN``
     - used to explicitly hide, especially members of types
   * - ``XXX_EXPORT_TEMPLATE_TYPE``
     - stating the template type should be visible
   * - ``XXX_EXPORT_EXTERN_TEMPLATE``
     - exporting template types (i.e. extern side of extern template)
   * - ``XXX_EXPORT_TEMPLATE_INSTANCE``
     - exporting specific template instantiations (in cpp code)
   * - ``XXX_EXPORT_TEMPLATE_DATA``
     - exporting templated data blocks
   * - ``XXX_EXPORT_ENUM``
     - exporting enum types

For a new library, the preference might be to call ``XXX_EXPORT``
something like ``XXX_FUNC``, and rename things such as ``XXX_EXPORT_TYPE``
to ``XXX_TYPE`` for simplicity. However, historically, OpenEXR has used
the ``_EXPORT`` tag, and so that is preserved for consistency.

---------

* LLVM libc++ visibility macros: https://libcxx.llvm.org/docs/DesignDocs/VisibilityMacros.html

* GCC visibility wiki: https://gcc.gnu.org/wiki/Visibility

* Apple library design docs: https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/DynamicLibraries/100-Articles/DynamicLibraryDesignGuidelines.html
