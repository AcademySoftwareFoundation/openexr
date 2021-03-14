//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

/// \addtogroup ExportMacros
/// @{
#if defined(OPENEXR_DLL)
#    if defined(OPENEXRUTIL_EXPORTS)
#        define IMFUTIL_EXPORT __declspec(dllexport)
#        if defined(__MINGW32__)
#            define IMFUTIL_EXPORT_EXTERN_TEMPLATE IMFUTIL_EXPORT
#            define IMFUTIL_EXPORT_TEMPLATE_TYPE IMFUTIL_EXPORT
#        else
#            define IMFUTIL_EXPORT_TEMPLATE_INSTANCE IMFUTIL_EXPORT
#        endif
#    else
#        define IMFUTIL_EXPORT __declspec(dllimport)
#        define IMFUTIL_EXPORT_EXTERN_TEMPLATE IMFUTIL_EXPORT
#    endif
#else
#    ifndef _MSC_VER
#        define IMFUTIL_EXPORT __attribute__ ((__visibility__ ("default")))
#        define IMFUTIL_EXPORT_TYPE __attribute__ ((__visibility__ ("default")))
//#        define IMFUTIL_EXPORT_TEMPLATE_DATA __attribute__ ((__visibility__ ("default")))
#        define IMFUTIL_HIDDEN __attribute__ ((__visibility__ ("hidden")))
#        if __has_attribute(__type_visibility__)
#            define IMFUTIL_EXPORT_ENUM __attribute__ ((__type_visibility__ ("default")))
#            define IMFUTIL_EXPORT_TEMPLATE_TYPE __attribute__ ((__type_visibility__ ("default")))
#            define IMFUTIL_EXPORT_EXTERN_TEMPLATE __attribute__ ((__visibility__ ("default")))
#        else
#            define IMFUTIL_EXPORT_TEMPLATE_TYPE __attribute__ ((__visibility__ ("default")))
#        endif
#    else
#        define IMFUTIL_EXPORT
#    endif
#endif
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
