#ifndef HALFEXPORT_H
#define HALFEXPORT_H

//
//  Copyright (c) 2008 Lucasfilm Entertainment Company Ltd.
//  All rights reserved.   Used under authorization.
//  This material contains the confidential and proprietary
//  information of Lucasfilm Entertainment Company and
//  may not be copied in whole or in part without the express
//  written permission of Lucasfilm Entertainment Company.
//  This copyright notice does not imply publication.
//

#if defined(_WIN32)
#  if defined(OPENEXR_DLL)
#    define PLATFORM_EXPORT_DEFINITION __declspec(dllexport) 
#    define PLATFORM_IMPORT_DEFINITION __declspec(dllimport)
#    define PLATFORM_EXPORT_CONST 
#  else
#    define PLATFORM_EXPORT_DEFINITION 
#    define PLATFORM_IMPORT_DEFINITION
#    define PLATFORM_EXPORT_CONST const
#  endif
#else   // linux/macos
#  if defined(PLATFORM_VISIBILITY_AVAILABLE)
#    define PLATFORM_EXPORT_DEFINITION __attribute__((visibility("default")))
#    define PLATFORM_IMPORT_DEFINITION
#  else
#    define PLATFORM_EXPORT_DEFINITION 
#    define PLATFORM_IMPORT_DEFINITION
#  endif
#  define PLATFORM_EXPORT_CONST const
    #endif

#if defined(HALF_EXPORTS)                          // create library
#  define HALF_EXPORT PLATFORM_EXPORT_DEFINITION
#  define HALF_EXPORT_CONST PLATFORM_EXPORT_CONST
#else                                              // use library
#  define HALF_EXPORT PLATFORM_IMPORT_DEFINITION
#  define HALF_EXPORT_CONST PLATFORM_EXPORT_CONST
#endif

#endif // #ifndef HALFEXPORT_H

