//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// See docs/SymbolVisibility.md for more discussion

/// \addtogroup ExportMacros
/// @{
#if defined(OPENEXR_DLL)
#    if defined(ILMTHREAD_EXPORTS)
#	    define ILMTHREAD_EXPORT __declspec(dllexport)
#    else
#	    define ILMTHREAD_EXPORT __declspec(dllimport)
#    endif
#    define ILMTHREAD_EXPORT_TYPE
#    define ILMTHREAD_HIDDEN
#else
#    ifndef _MSC_VER
#        define ILMTHREAD_EXPORT __attribute__ ((__visibility__ ("default")))
#        define ILMTHREAD_EXPORT_TYPE __attribute__ ((__visibility__ ("default")))
#        define ILMTHREAD_HIDDEN __attribute__ ((__visibility__ ("hidden")))
#    else
#	     define ILMTHREAD_EXPORT
#        define ILMTHREAD_EXPORT_TYPE
#        define ILMTHREAD_HIDDEN
#    endif
#endif
/// @}
