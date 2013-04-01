#ifndef IMATHEXPORT_H
#define IMATHEXPORT_H

///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

#if defined(WIN32)
#  if defined(OPENEXR_DLL)
#    define IMATH_EXPORT_DEFINITION __declspec(dllexport) 
#    define IMATH_IMPORT_DEFINITION __declspec(dllimport)
#  else
#    define IMATH_EXPORT_DEFINITION 
#    define IMATH_IMPORT_DEFINITION
#  endif
#else   // linux/macos
#  if defined(PLATFORM_VISIBILITY_AVAILABLE)
#    define IMATH_EXPORT_DEFINITION __attribute__((visibility("default")))
#    define IMATH_IMPORT_DEFINITION
#  else
#    define IMATH_EXPORT_DEFINITION 
#    define IMATH_IMPORT_DEFINITION
#  endif
#endif

#if defined(IMATH_EXPORTS)                          // create library
#  define IMATH_EXPORT IMATH_EXPORT_DEFINITION
#  define IMATH_EXPORT_VAR IMATH_EXPORT_DEFINITION extern
#else                                              // use library
#  define IMATH_EXPORT IMATH_IMPORT_DEFINITION
#  define IMATH_EXPORT_VAR IMATH_IMPORT_DEFINITION extern
#endif

#endif // IMATHEXPORT_H
