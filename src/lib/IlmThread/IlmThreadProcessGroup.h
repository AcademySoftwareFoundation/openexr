//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_ILM_THREAD_PROCESS_GROUP_H
#define INCLUDED_ILM_THREAD_PROCESS_GROUP_H

//-----------------------------------------------------------------------------
//
// Class ProcessGroup is a templated inline helper for constraining
// task contexts to a number of threads. It maintains a list of
// contexts and then can hand them out one at a time, waiting for a
// previous thread request to finish before handing out more,
// preventing over-subscription / allocation of contexts.
//
//-----------------------------------------------------------------------------

#include "IlmThreadConfig.h"
#include "IlmThreadExport.h"
#include "IlmThreadNamespace.h"
#include "IlmThreadSemaphore.h"

#include "Iex.h"

#include <atomic>
#include <string>
#include <type_traits>
#include <vector>

ILMTHREAD_INTERNAL_NAMESPACE_HEADER_ENTER

template <typename P,
    std::enable_if_t <
        std::is_default_constructible <P>::value &&
        std::is_same <decltype (P {}.next), P *>::value, bool> = true>
class ProcessGroup
{
public:
    using Process = P;

    ProcessGroup (unsigned int numThreads)
        : _sem (numThreads)
        , _avail_head (nullptr)
        , _first_failure (nullptr)
    {
        _fixed_pool.resize (numThreads);
        for ( unsigned int i = 0; i < numThreads; ++i )
        {
            if (i == (numThreads - 1))
                _fixed_pool[i].next = nullptr;
            else
                _fixed_pool[i].next = &(_fixed_pool[i+1]);
        }
        _avail_head = &(_fixed_pool[0]);
    }

    ProcessGroup (const ProcessGroup&) = delete;
    ProcessGroup& operator= (const ProcessGroup&) = delete;
    ProcessGroup (ProcessGroup&&) = default;
    ProcessGroup& operator= (ProcessGroup&&) = delete;
    ~ProcessGroup()
    {
        std::string *cur = _first_failure.load ();
        delete cur;
    }

    void push (Process *p)
    {
        Process* oldhead = _avail_head.load (std::memory_order_relaxed);

        do
        {
            p->next = oldhead;
        } while (!_avail_head.compare_exchange_weak (
                     oldhead, p,
                     std::memory_order_release,
                     std::memory_order_relaxed));

        // notify someone else there's one available
        _sem.post ();
    }

    // called by the thread dispatching work units, may block
    Process* pop ()
    {
        Process* ret = nullptr;

        // we do not have to worry about ABA problems as
        // we have a static pool of items we own, we're just
        // putting them here and popping them off.

        // used for honoring the numThreads, as pop
        // should only be called by the one thread
        // waiting to submit thread calls
        _sem.wait ();

        ret = _avail_head.load (std::memory_order_acquire);

        Process* newhead;
        do
        {
            if (!ret)
                std::cerr << "GACK: serious failure case???" << std::endl;

            newhead = ret->next;
        } while ( !_avail_head.compare_exchange_weak(
                      ret, newhead, std::memory_order_acquire));

        return ret;
    }

    void record_failure (const char *e)
    {
        // should we construct a list of failures if there are
        // more than one? seems less confusing to just report
        // the first we happened to record

        std::string *cur = _first_failure.load ();
        if (!cur)
        {
            std::string *msg = new std::string (e);
            if (! _first_failure.compare_exchange_strong (cur, msg))
                delete msg;
        }
    }

    void throw_on_failure ()
    {
        std::string *cur = _first_failure.load ();
        _first_failure.store (nullptr);

        if (cur)
        {
            std::string msg (*cur);
            delete cur;

            throw IEX_NAMESPACE::IoExc (msg);
        }
    }
private:
    Semaphore _sem;

    std::vector<Process>   _fixed_pool;

    std::atomic<Process *> _avail_head;

    std::atomic<std::string *> _first_failure;
};


ILMTHREAD_INTERNAL_NAMESPACE_HEADER_EXIT

#endif // INCLUDED_ILM_THREAD_POOL_H
