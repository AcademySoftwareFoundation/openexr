//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IEXEXPORT_H
#define IEXEXPORT_H

#if defined(OPENEXR_DLL)
#    if defined(IEX_EXPORTS)
#        define IEX_EXPORT __declspec(dllexport)
#    else
#        define IEX_EXPORT __declspec(dllimport)
#    endif
#else
#    define IEX_EXPORT
#endif
#ifndef IEX_EXPORT_TYPE
#    define IEX_EXPORT_TYPE
#endif
#ifndef IEX_EXPORT_ENUM
#    define IEX_EXPORT_ENUM
#endif

#endif // #ifndef IEXEXPORT_H

