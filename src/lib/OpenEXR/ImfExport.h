//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMFEXPORT_H
#define INCLUDED_IMFEXPORT_H

/// \addtogroup ExportMacros
/// @{

// are we making a DLL under windows (might be msvc or mingw or others)
#if defined(OPENEXR_DLL)

#  if defined(OPENEXR_EXPORTS)
#    define IMF_EXPORT __declspec(dllexport)

#    if defined(__MINGW32__)
// mingw needs the export when the extern is defined
#      define IMF_EXPORT_EXTERN_TEMPLATE IMF_EXPORT
// for mingw windows, we need to cause this to export the typeinfo
// tables (but you don't need to have the complementary import,
// because might be a local template too)
#      define IMF_EXPORT_TEMPLATE_TYPE IMF_EXPORT
#    else
// for normal msvc, need to export the actual instantiation in the cpp code
#      define IMF_EXPORT_TEMPLATE_INSTANCE IMF_EXPORT
#    endif

#  else // OPENEXR_EXPORTS, not in this DLL
#    define IMF_EXPORT __declspec(dllimport)
#    define IMF_EXPORT_EXTERN_TEMPLATE IMF_EXPORT
#  endif

#else // OPENEXR_DLL

// need to avoid the case when compiling a static lib under MSVC (not
// a dll, not a compiler that has visibility attributes)
#  ifndef _MSC_VER
#    ifdef OPENEXR_USE_DEFAULT_VISIBILITY
#      define IMF_EXPORT
#    else 
#      define IMF_EXPORT __attribute__ ((__visibility__ ("default")))
#      define IMF_EXPORT_TYPE IMF_EXPORT
#      define IMF_HIDDEN __attribute__ ((__visibility__ ("hidden")))
// clang needs type visibility for enum and template, and the instantiation
// gcc does not have the type_visibility attribute, but if it ever adds it,
// the behavior should be the same
#      if __has_attribute(__type_visibility__)
#        define IMF_EXPORT_ENUM __attribute__ ((__type_visibility__ ("default")))
#        define IMF_EXPORT_TEMPLATE_TYPE __attribute__ ((__type_visibility__ ("default")))
#        define IMF_EXPORT_TEMPLATE_INSTANCE IMF_EXPORT
#      else
#        define IMF_EXPORT_TEMPLATE_TYPE IMF_EXPORT
#      endif
#      define IMF_EXPORT_EXTERN_TEMPLATE IMF_EXPORT
#    endif // OPENEXR_USE_DEFAULT_VISIBILITY

#  else // _MSC_VER
#    define IMF_EXPORT
#  endif

#endif // OPENEXR_DLL

// Provide defaults so we don't have to replicate lines as much

#ifndef IMF_EXPORT_TYPE
#    define IMF_EXPORT_TYPE
#endif
#ifndef IMF_EXPORT_TEMPLATE_TYPE
#    define IMF_EXPORT_TEMPLATE_TYPE
#endif
#ifndef IMF_EXPORT_EXTERN_TEMPLATE
#    define IMF_EXPORT_EXTERN_TEMPLATE
#endif
#ifndef IMF_EXPORT_TEMPLATE_INSTANCE
#    define IMF_EXPORT_TEMPLATE_INSTANCE
#endif
#ifndef IMF_EXPORT_ENUM
#    define IMF_EXPORT_ENUM
#endif
#ifndef IMF_HIDDEN
#    define IMF_HIDDEN
#endif

/// @}

#endif // INCLUDED_IMFEXPORT_H
