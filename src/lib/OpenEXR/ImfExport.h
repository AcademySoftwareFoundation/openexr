//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#if defined(OPENEXR_DLL)
#    if defined(OPENEXR_EXPORTS)
#	    define IMF_EXPORT __declspec(dllexport)
#       define IMF_EXPORT_CONST extern __declspec(dllexport)
#    else
#	    define IMF_EXPORT __declspec(dllimport)
#	    define IMF_EXPORT_CONST extern __declspec(dllimport)
#    endif
#else
#    ifndef _MSC_VER
#        define IMF_EXPORT __attribute__ ((visibility ("default")))
#        define IMF_EXPORT_CONST extern const __attribute__ ((visibility ("default")))
#    else
#	    define IMF_EXPORT
#	    define IMF_EXPORT_CONST extern
#    endif
#endif
