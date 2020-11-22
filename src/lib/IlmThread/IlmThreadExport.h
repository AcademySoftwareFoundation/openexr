//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#if defined(OPENEXR_DLL)
#    if defined(ILMTHREAD_EXPORTS)
#	    define ILMTHREAD_EXPORT __declspec(dllexport)
#       define ILMTHREAD_EXPORT_CONST extern __declspec(dllexport)
#    else
#	    define ILMTHREAD_EXPORT __declspec(dllimport)
#	    define ILMTHREAD_EXPORT_CONST extern __declspec(dllimport)
#    endif
#else
#    ifndef _MSC_VER
#        define ILMTHREAD_EXPORT __attribute__ ((visibility ("default")))
#        define ILMTHREAD_EXPORT_CONST extern const __attribute__ ((visibility ("default")))
#    else
#	    define ILMTHREAD_EXPORT
#	    define ILMTHREAD_EXPORT_CONST extern
#    endif
#endif
