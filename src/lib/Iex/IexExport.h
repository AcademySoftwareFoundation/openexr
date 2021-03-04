//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IEXEXPORT_H
#define IEXEXPORT_H

#if defined(OPENEXR_DLL)
    #if defined(IEX_EXPORTS)
    #define IEX_EXPORT __declspec(dllexport)
    #else
    #define IEX_EXPORT __declspec(dllimport)
    #endif
    #define IEX_EXPORT_CONST
#else
    #define IEX_EXPORT
    #define IEX_EXPORT_CONST const
#endif

#endif // #ifndef IEXEXPORT_H

