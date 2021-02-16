//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

/// \addtogroup ExportMacros
/// @{
#if defined(OPENEXR_DLL)
#    if defined(OPENEXR_EXPORTS)
#	    define IMF_EXPORT __declspec(dllexport)
#       define IMF_EXPORT_CONST extern __declspec(dllexport)
#    else
#	    define IMF_EXPORT __declspec(dllimport)
#	    define IMF_EXPORT_CONST extern __declspec(dllimport)
#    endif
#    define IMF_EXPORT_TYPE
#    define IMF_EXPORT_LOCAL
#else
#    ifndef _MSC_VER
#        define IMF_EXPORT __attribute__ ((visibility ("default")))
#        define IMF_EXPORT_CONST extern const __attribute__ ((visibility ("default")))
#        define IMF_EXPORT_TYPE __attribute__ ((visibility ("default")))
#        define IMF_EXPORT_LOCAL __attribute__ ((visibility ("hidden")))
#    else
#	     define IMF_EXPORT
#	     define IMF_EXPORT_CONST extern
#        define IMF_EXPORT_TYPE
#        define IMF_EXPORT_LOCAL
#    endif
#endif
/// @}
