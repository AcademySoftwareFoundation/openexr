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
//	class Semaphore -- implementation for for platforms that do
//	support Posix threads but do not support Posix semaphores
//
//-----------------------------------------------------------------------------

#include "IlmThreadConfig.h"

#if ILMTHREAD_THREADING_ENABLED
#if ( !(ILMTHREAD_HAVE_POSIX_SEMAPHORES) && !defined (__APPLE__) && !defined (_WIN32) && !defined (_WIN64) )

#include "IlmThreadSemaphore.h"

ILMTHREAD_INTERNAL_NAMESPACE_SOURCE_ENTER

Semaphore::Semaphore (unsigned int value)
{
    _semaphore.count = value;
    _semaphore.numWaiting = 0;
}


Semaphore::~Semaphore ()
{
}


void
Semaphore::wait ()
{
    std::unique_lock<std::mutex> lk(_semaphore.mutex);

    _semaphore.numWaiting++;

    while (_semaphore.count == 0)
        _semaphore.nonZero.wait (lk);

    _semaphore.numWaiting--;
    _semaphore.count--;
}


bool
Semaphore::tryWait ()
{
    std::lock_guard<std::mutex> lk(_semaphore.mutex);
    
    if (_semaphore.count == 0)
        return false;

    _semaphore.count--;
    return true;
}


void
Semaphore::post ()
{
    std::lock_guard<std::mutex> lk(_semaphore.mutex);

    _semaphore.count++;
    if (_semaphore.numWaiting > 0)
    {
        if (_semaphore.count > 1)
            _semaphore.nonZero.notify_all();
        else
            _semaphore.nonZero.notify_one();
    }
}


int
Semaphore::value () const
{
    std::lock_guard<std::mutex> lk(_semaphore.mutex);
    return _semaphore.count;
}

ILMTHREAD_INTERNAL_NAMESPACE_SOURCE_EXIT

#endif // posix semaphore compat
#endif // enable threading
