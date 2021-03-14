//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

/// \addtogroup ExportMacros
/// @{
#if defined(OPENEXR_DLL)
#    if defined(OPENEXR_EXPORTS)
#	     define IMF_EXPORT __declspec(dllexport)
#        if defined(__MINGW32__)
#            define IMF_EXPORT_EXTERN_TEMPLATE IMF_EXPORT
#            define IMF_EXPORT_TEMPLATE_TYPE IMF_EXPORT
#        else
#            define IMF_EXPORT_TEMPLATE_INSTANCE IMF_EXPORT
#        endif
#    else
#	     define IMF_EXPORT __declspec(dllimport)
//#        if defined(__MINGW32__)
//#            define IMF_EXPORT_TEMPLATE_TYPE IMF_EXPORT
//#        endif
#        define IMF_EXPORT_EXTERN_TEMPLATE IMF_EXPORT
#    endif
#else
#    ifndef _MSC_VER
#        define IMF_EXPORT __attribute__ ((__visibility__ ("default")))
#        define IMF_EXPORT_TYPE IMF_EXPORT
#        define IMF_HIDDEN __attribute__ ((__visibility__ ("hidden")))
#        if __has_attribute(__type_visibility__)
#            define IMF_EXPORT_ENUM __attribute__ ((__type_visibility__ ("default")))
#            define IMF_EXPORT_TEMPLATE_TYPE __attribute__ ((__type_visibility__ ("default")))
#            define IMF_EXPORT_TEMPLATE_INSTANCE IMF_EXPORT
#        else
#            define IMF_EXPORT_TEMPLATE_TYPE IMF_EXPORT
#        endif
#        define IMF_EXPORT_EXTERN_TEMPLATE IMF_EXPORT
#    else
#	     define IMF_EXPORT
#    endif
#endif
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
