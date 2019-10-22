///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2005-2012, Industrial Light & Magic, a division of Lucas
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

#ifndef INCLUDED_ILM_THREAD_MINGW_THREAD_H
#define INCLUDED_ILM_THREAD_MINGW_THREAD_H

//-----------------------------------------------------------------------------
//
//  This file is just some boilerplate to ensure macros are correctly defined
//  in order to compile against MinGW-W64's implementation of posix threads
//  and semaphores.
//
//-----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
# ifdef NOMINMAX
#   undef NOMINMAX
# endif
# define NOMINMAX
# include <windows.h>
# if defined(__MINGW64_VERSION_MAJOR)
#   include <pthread_unistd.h>
#   if (defined(_POSIX_SEMAPHORES) && !defined(HAVE_POSIX_SEMAPHORES))
#     define HAVE_POSIX_SEMAPHORES
#   endif
#   if (defined(_POSIX_THREADS) && !defined(HAVE_PTHREAD))
#     define HAVE_PTHREAD
#   endif
# endif
#endif

#endif // INCLUDED_ILM_THREAD_MINGW_THREAD_H