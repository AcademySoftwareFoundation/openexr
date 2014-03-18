#ifndef IEXMATHEXPORT_H
#define IEXMATHEXPORT_H

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


#if defined(_WIN32)
#  if defined(OPENEXR_DLL)
#    define IEXMATH_EXPORT_DEFINITION __declspec(dllexport) 
#    define IEXMATH_IMPORT_DEFINITION __declspec(dllimport)
#  else
#    define IEXMATH_EXPORT_DEFINITION 
#    define IEXMATH_IMPORT_DEFINITION
#  endif
#else   // linux/macos
#  if defined(PLATFORM_VISIBILITY_AVAILABLE)
#    define IEXMATH_EXPORT_DEFINITION __attribute__((visibility("default")))
#    define IEXMATH_IMPORT_DEFINITION
#  else
#    define IEXMATH_EXPORT_DEFINITION 
#    define IEXMATH_IMPORT_DEFINITION
#  endif
#endif

#if defined(IEXMATH_EXPORTS)                          // create library
#  define IEXMATH_EXPORT IEXMATH_EXPORT_DEFINITION
#else                                              // use library
#  define IEXMATH_EXPORT IEXMATH_IMPORT_DEFINITION
#endif

#endif // #ifndef IEXMATHEXPORT_H

