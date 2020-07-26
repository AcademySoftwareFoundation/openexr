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

//-----------------------------------------------------------------------------
//
//	class Thread -- this file contains two implementations of thread:
//	- dummy implementation for platforms that do not support threading
//	  when OPENEXR_FORCE_CXX03 is on
//	- c++11 and newer version
//
//-----------------------------------------------------------------------------

#include "IlmBaseConfig.h"
#include "IlmThread.h"
#include "Iex.h"

ILMTHREAD_INTERNAL_NAMESPACE_SOURCE_ENTER

#ifndef ILMBASE_FORCE_CXX03
//-----------------------------------------------------------------------------
// C++11 and newer implementation
//-----------------------------------------------------------------------------
bool
supportsThreads ()
{
    return true;
}

Thread::Thread ()
{
    // empty
}


Thread::~Thread ()
{
    // hopefully the thread has basically exited and we are just
    // cleaning up, because run is a virtual function, so the v-table
    // has already been partly destroyed...
    if ( _thread.joinable () )
        _thread.join ();
}


void
Thread::start ()
{
    _thread = std::thread (&Thread::run, this);
}

#else
#   if !defined (_WIN32) && !defined (_WIN64) && ! defined(HAVE_PTHREAD)
//-----------------------------------------------------------------------------
// OPENEXR_FORCE_CXX03 with no windows / pthread support
//-----------------------------------------------------------------------------
bool
supportsThreads ()
{
    return false;
}


Thread::Thread ()
{
    throw IEX_NAMESPACE::NoImplExc ("Threads not supported on this platform.");
}


Thread::~Thread ()
{
    throw IEX_NAMESPACE::NoImplExc ("Threads not supported on this platform.");
}


void
Thread::start ()
{
    throw IEX_NAMESPACE::NoImplExc ("Threads not supported on this platform.");
}
#   endif
#endif


ILMTHREAD_INTERNAL_NAMESPACE_SOURCE_EXIT

