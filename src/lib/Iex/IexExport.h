//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IEXEXPORT_H
#define IEXEXPORT_H

/// \defgroup ExportMacros Macros to manage symbol visibility
///
/// See docs/SymbolVisibility.md for more discussion
///
/// Iex is simple and does not need to do more than expose class types
/// and functions, and does not have any private members to hide
/// 
/// @{
#if defined(OPENEXR_DLL)
#    if defined(IEX_EXPORTS)
#        define IEX_EXPORT __declspec(dllexport)
#    else
#        define IEX_EXPORT __declspec(dllimport)
#    endif
#    define IEX_EXPORT_TYPE
#else
#    ifndef _MSC_VER
#        define IEX_EXPORT __attribute__ ((__visibility__ ("default")))
#        define IEX_EXPORT_TYPE __attribute__ ((__visibility__ ("default")))
#    else
#        define IEX_EXPORT
#        define IEX_EXPORT_TYPE
#    endif
#endif

#ifndef IEX_EXPORT_TYPE
#    define IEX_EXPORT_TYPE
#endif
#ifndef IEX_EXPORT_ENUM
#    define IEX_EXPORT_ENUM
#endif

/// @}

#endif // #ifndef IEXEXPORT_H

