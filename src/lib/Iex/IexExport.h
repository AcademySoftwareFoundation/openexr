//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IEXEXPORT_H
#define INCLUDED_IEXEXPORT_H

/// \defgroup ExportMacros Macros to manage symbol visibility
///
/// See docs/SymbolVisibility.md for more discussion about the
/// motivation for these macros
///
/// Iex is simple and does not need to do more than expose class types
/// and functions, and does not have any private members to hide, so
/// only provides a couple of the possible macros.
///
/// Similarly, IlmThread is also simple.
///
/// OpenEXR and OpenEXRUtil have much more logic and have to deal with
/// templates and template instantiation, and so define more of the
/// macros.
/// 
/// @{

#if defined(OPENEXR_DLL)

// when building as a DLL for windows, typical dllexport / import case

#  if defined(IEX_EXPORTS)
#    define IEX_EXPORT __declspec(dllexport)
#  else
#    define IEX_EXPORT __declspec(dllimport)
#  endif

#else // OPENEXR_DLL

// need to avoid the case when compiling a static lib under MSVC (not
// a dll, not a compiler that has visibility attributes)
#  ifndef _MSC_VER

// did the user turn off visibility management
#    ifdef OPENEXR_USE_DEFAULT_VISIBILITY
#      define IEX_EXPORT
#    else
       // we actually want to control visibility
#      define IEX_EXPORT __attribute__ ((__visibility__ ("default")))
#      define IEX_EXPORT_TYPE __attribute__ ((__visibility__ ("default")))
#    endif
#  else
#    define IEX_EXPORT
#  endif

#endif

// Provide defaults so we don't have to replicate lines as much

#ifndef IEX_EXPORT_TYPE
#  define IEX_EXPORT_TYPE
#endif
#ifndef IEX_EXPORT_ENUM
#  define IEX_EXPORT_ENUM
#endif

/// @}

#endif // #ifndef INCLUDED_IEXEXPORT_H

