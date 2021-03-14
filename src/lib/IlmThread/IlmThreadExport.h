//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// See docs/SymbolVisibility.md for more discussion

/// \addtogroup ExportMacros
/// @{

// are we making a DLL under windows (might be msvc or mingw or others)
#if defined(OPENEXR_DLL)

// our dll or nother location
#  if defined(ILMTHREAD_EXPORTS)
#    define ILMTHREAD_EXPORT __declspec(dllexport)
#  else
#    define ILMTHREAD_EXPORT __declspec(dllimport)
#  endif

#else // OPENEXR_DLL

// need to avoid the case when compiling a static lib under MSVC (not
// a dll, not a compiler that has visibility attributes)
#  ifndef _MSC_VER
#    ifdef OPENEXR_USE_DEFAULT_VISIBILITY
#      define ILMTHREAD_EXPORT
#    else 
#      define ILMTHREAD_EXPORT __attribute__ ((__visibility__ ("default")))
#      define ILMTHREAD_EXPORT_TYPE ILMTHREAD_EXPORT
#      define ILMTHREAD_HIDDEN __attribute__ ((__visibility__ ("hidden")))
#    endif // OPENEXR_USE_DEFAULT_VISIBILITY

#  else // _MSC_VER
#    define ILMTHREAD_EXPORT
#  endif

#endif // OPENEXR_DLL

// Provide defaults so we don't have to replicate lines as much

#ifndef ILMTHREAD_EXPORT_TYPE
#    define ILMTHREAD_EXPORT_TYPE
#endif
#ifndef ILMTHREAD_HIDDEN
#    define ILMTHREAD_HIDDEN
#endif

/// @}
