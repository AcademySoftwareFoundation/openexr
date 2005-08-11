///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2005, Industrial Light & Magic, a division of Lucas
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

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
#elif HAVE_PTHREAD
    #include <pthread.h>
#endif

namespace IlmThread
{

class Lock;

//-----------------------------------------------------------------------------
//
//    class Mutex -- a MUTual EXclusion lock
//
// A wrapper class for a system-dependent locking mechanism. Actual use of
// this class has to be performed using an instance of a Lock defined below.
//
//-----------------------------------------------------------------------------

class Mutex
{
public:
    Mutex ();
    virtual ~Mutex ();

private:
    void lock () const;
    void unlock () const;

#ifdef _WIN32
    mutable CRITICAL_SECTION _mutex;
#elif HAVE_PTHREAD
    mutable pthread_mutex_t _mutex;
#endif

    void operator= (const Mutex& M);        // not implemented
    Mutex (const Mutex& M);                 // not implemented
    
    friend class Lock;
};



//-----------------------------------------------------------------------------
//
//    class Lock -- block-level locking object
//
// Provides safe locking and unlocking of Mutexes even in the presence of
// C++ exceptions. All manipulation of Mutex objects has to be performed
// using an instance of this class. This ensures that locked Mutexes
// will be unlocked when the Lock instance gets destroyed.
//
// Lock objects are not themselves thread-safe, meaning, you should never
// share a Lock object amoung multiple threads. Locks are meant to be
// used only at block scope.
//
// Typical usage:
//    
//    // Create a Mutex object visible to multiple threads
//    Mutex mtx;
//
//    ...
//    // create some threads
//    ...
//
//    // Then, within each thread, construct a critical section like so:
//
//    {
//        Lock lock (mtx);        // locks the mutex
//        // do some computation on shared data
//        ...
//    }   // closing the block unlocks the mutex
//
//-----------------------------------------------------------------------------

class Lock
{
public:
    Lock (const Mutex& m, bool autoLock = true) : _mutex (m), _locked (false)
    {
        if (autoLock)
        {
            _mutex.lock ();
            _locked = true;
        }
    }
    
    ~Lock ()
    {
        if (_locked)
            _mutex.unlock ();
    }
    
    void acquire ()
    {
        _mutex.lock ();
        _locked = true;
    }
    
    void release ()
    {
        _mutex.unlock ();
        _locked = false;
    }
    
    bool locked ()
    {
        return _locked;
    }

private:
    const Mutex & _mutex;
    bool _locked;
};

} // namespace IlmThread


#endif
