//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMFUTILEXPORT_H
#define INCLUDED_IMFUTILEXPORT_H

/// \addtogroup ExportMacros
/// @{

// are we making a DLL under windows (might be msvc or mingw or others)
#if defined(OPENEXR_DLL)

#  if defined(OPENEXRUTIL_EXPORTS)
#    define IMFUTIL_EXPORT __declspec(dllexport)

#    if defined(__MINGW32__)
// mingw needs the export when the extern is defined
#      define IMFUTIL_EXPORT_EXTERN_TEMPLATE IMFUTIL_EXPORT
// for mingw windows, we need to cause this to export the typeinfo
// tables (but you don't need to have the complementary import,
// because might be a local template too)
#      define IMFUTIL_EXPORT_TEMPLATE_TYPE IMFUTIL_EXPORT
#    else
// for normal msvc, need to export the actual instantiation in the cpp code
#      define IMFUTIL_EXPORT_TEMPLATE_INSTANCE IMFUTIL_EXPORT
#    endif

#    else
#	     define IMFUTIL_EXPORT __declspec(dllimport)
#        define IMFUTIL_EXPORT_EXTERN_TEMPLATE IMFUTIL_EXPORT
#    endif

#else // OPENEXR_DLL

// need to avoid the case when compiling a static lib under MSVC (not
// a dll, not a compiler that has visibility attributes)
#  ifndef _MSC_VER
#    ifdef OPENEXR_USE_DEFAULT_VISIBILITY
#      define IMFUTIL_EXPORT
#    else 
#      define IMFUTIL_EXPORT __attribute__ ((__visibility__ ("default")))
#      define IMFUTIL_EXPORT_TYPE IMFUTIL_EXPORT
#      define IMFUTIL_HIDDEN __attribute__ ((__visibility__ ("hidden")))
// clang needs type visibility for enum and template, and the instantiation
// gcc does not have the type_visibility attribute, but if it ever adds it,
// the behavior should be the same
#      if __has_attribute(__type_visibility__)
#        define IMFUTIL_EXPORT_ENUM __attribute__ ((__type_visibility__ ("default")))
#        define IMFUTIL_EXPORT_TEMPLATE_TYPE __attribute__ ((__type_visibility__ ("default")))
#        define IMFUTIL_EXPORT_TEMPLATE_INSTANCE IMFUTIL_EXPORT
#      else
#        define IMFUTIL_EXPORT_TEMPLATE_TYPE IMFUTIL_EXPORT
#      endif
#      define IMFUTIL_EXPORT_EXTERN_TEMPLATE IMFUTIL_EXPORT
#    endif // OPENEXR_USE_DEFAULT_VISIBILITY

#  else // _MSC_VER
#    define IMFUTIL_EXPORT
#  endif

#endif // OPENEXR_DLL

// Provide defaults so we don't have to replicate lines as much

#ifndef IMFUTIL_EXPORT_TYPE
#    define IMFUTIL_EXPORT_TYPE
#endif
#ifndef IMFUTIL_EXPORT_TEMPLATE_TYPE
#    define IMFUTIL_EXPORT_TEMPLATE_TYPE
#endif
#ifndef IMFUTIL_EXPORT_EXTERN_TEMPLATE
#    define IMFUTIL_EXPORT_EXTERN_TEMPLATE
#endif
#ifndef IMFUTIL_EXPORT_TEMPLATE_INSTANCE
#    define IMFUTIL_EXPORT_TEMPLATE_INSTANCE
#endif
#ifndef IMFUTIL_EXPORT_ENUM
#    define IMFUTIL_EXPORT_ENUM
#endif
#ifndef IMFUTIL_HIDDEN
#    define IMFUTIL_HIDDEN
#endif

/// @}

#endif // INCLUDED_IMFUTILEXPORT_H
