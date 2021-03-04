//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#if defined(OPENEXR_DLL)
    #if defined(OPENEXRUTIL_EXPORTS)
        #define IMFUTIL_EXPORT __declspec(dllexport)
        #define IMFUTIL_EXPORT_CONST extern __declspec(dllexport)
    #else
        #define IMFUTIL_EXPORT __declspec(dllimport)
        #define IMFUTIL_EXPORT_CONST extern __declspec(dllimport)
    #endif
#else
    #define IMFUTIL_EXPORT
    #define IMFUTIL_EXPORT_CONST extern const
#endif
