//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

/// \addtogroup ExportMacros
/// @{
#if defined(OPENEXR_DLL)
#    if defined(ILMTHREAD_EXPORTS)
#	    define ILMTHREAD_EXPORT __declspec(dllexport)
#       define ILMTHREAD_EXPORT_CONST extern __declspec(dllexport)
#    else
#	    define ILMTHREAD_EXPORT __declspec(dllimport)
#	    define ILMTHREAD_EXPORT_CONST extern __declspec(dllimport)
#    endif
#    define ILMTHREAD_EXPORT_TYPE
#    define ILMTHREAD_EXPORT_LOCAL
#else
#    ifndef _MSC_VER
#        define ILMTHREAD_EXPORT __attribute__ ((visibility ("default")))
#        define ILMTHREAD_EXPORT_CONST extern const __attribute__ ((visibility ("default")))
#        define ILMTHREAD_EXPORT_TYPE __attribute__ ((visibility ("default")))
#        define ILMTHREAD_EXPORT_LOCAL __attribute__ ((visibility ("hidden")))
#    else
#	     define ILMTHREAD_EXPORT
#	     define ILMTHREAD_EXPORT_CONST extern
#        define ILMTHREAD_EXPORT_TYPE
#        define ILMTHREAD_EXPORT_LOCAL
#    endif
#endif
/// @}
