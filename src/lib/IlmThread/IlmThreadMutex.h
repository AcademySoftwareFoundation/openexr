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

#ifndef INCLUDED_ILM_THREAD_MUTEX_H
#define INCLUDED_ILM_THREAD_MUTEX_H

//-----------------------------------------------------------------------------
//
// NB: Maintained for backward compatibility with header files only. This
// has been entirely replaced by c++11 and the std::mutex layer
//
//-----------------------------------------------------------------------------

#include "IlmThreadExport.h"
#include "IlmThreadConfig.h"
#include "IlmThreadNamespace.h"

#if ILMTHREAD_THREADING_ENABLED
#include <mutex>
#endif

ILMTHREAD_INTERNAL_NAMESPACE_HEADER_ENTER

#if ILMTHREAD_THREADING_ENABLED
using Mutex ILMTHREAD_DEPRECATED ("replace with std::mutex") = std::mutex;

// unfortunately we can't use std::unique_lock as a replacement for Lock since
// they have different API. Let us deprecate for now and give people a chance
// to clean up their code.
class ILMTHREAD_EXPORT Lock
{
  public:

    ILMTHREAD_DEPRECATED ("replace with std::lock_guard or std::unique_lock")
    Lock (const Mutex& m, bool autoLock = true):
        _mutex (const_cast<Mutex &>(m)), _locked (false)
    {
        if (autoLock)
        {
            _mutex.lock();
            _locked = true;
        }
    }
    
    ~Lock ()
    {
        if (_locked)
            _mutex.unlock();
    }
    Lock (const Lock&) = delete;
    Lock &operator= (const Lock&) = delete;
    Lock (Lock&&) = delete;
    Lock& operator= (Lock&&) = delete;

    void acquire ()
    {
        _mutex.lock();
        _locked = true;
    }
    
    void release ()
    {
        _locked = false;
        _mutex.unlock();
    }
    
    bool locked ()
    {
        return _locked;
    }

  private:

    Mutex & _mutex;
    bool    _locked;
};
#endif

ILMTHREAD_INTERNAL_NAMESPACE_HEADER_EXIT

#endif // INCLUDED_ILM_THREAD_MUTEX_H
