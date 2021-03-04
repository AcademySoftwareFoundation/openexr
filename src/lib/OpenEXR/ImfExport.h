//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#if defined(OPENEXR_DLL)
    #if defined(OPENEXR_EXPORTS)
	    #define IMF_EXPORT __declspec(dllexport)
        #define IMF_EXPORT_CONST extern __declspec(dllexport)
    #else
	    #define IMF_EXPORT __declspec(dllimport)
	    #define IMF_EXPORT_CONST extern __declspec(dllimport)
    #endif
#else
    #define IMF_EXPORT
    #define IMF_EXPORT_CONST extern const
#endif
