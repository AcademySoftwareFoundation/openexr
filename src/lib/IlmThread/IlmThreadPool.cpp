//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//  class Task, class ThreadPool, class TaskGroup
//
//-----------------------------------------------------------------------------

#include "IlmThreadPool.h"
#include "Iex.h"
#include "IlmThread.h"
#include "IlmThreadSemaphore.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

ILMTHREAD_INTERNAL_NAMESPACE_SOURCE_ENTER

#if ILMTHREAD_THREADING_ENABLED
#    define ENABLE_THREADING
#endif

#if defined(__GNU_LIBRARY__) &&                                                \
    (__GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 21))
#    define ENABLE_SEM_DTOR_WORKAROUND
#endif

namespace
{

static inline void
handleProcessTask (Task* task)
{
    if (task)
    {
        TaskGroup* taskGroup = task->group ();

        task->execute ();

        if (taskGroup) taskGroup->finishOneTask ();

        delete task;
    }
}

} // empty namespace

#ifdef ENABLE_THREADING

struct TaskGroup::Data
{
    Data ();
    ~Data ();

    void             addTask ();
    void             removeTask ();
    std::atomic<int> numPending;
    Semaphore        isEmpty; // used to signal that the taskgroup is empty
#    if defined(ENABLE_SEM_DTOR_WORKAROUND)
    // this mutex is also used to lock numPending in the legacy c++ mode...
    std::mutex dtorMutex; // used to work around the glibc bug:
        // http://sources.redhat.com/bugzilla/show_bug.cgi?id=12674
#    endif
};

struct ThreadPool::Data
{
    using ProviderPtr = std::shared_ptr<ThreadPoolProvider>;

    Data ();
    ~Data ();
    Data (const Data&)            = delete;
    Data& operator= (const Data&) = delete;
    Data (Data&&)                 = delete;
    Data& operator= (Data&&)      = delete;

    ProviderPtr getProvider () const { return std::atomic_load (&_provider); }

    void setProvider (ProviderPtr provider)
    {
        ProviderPtr curp = getProvider ();

        if (curp != provider)
        {
            curp = std::atomic_exchange (&_provider, provider);
            if (curp) curp->finish ();
        }
    }

    void resetToDefaultProvider (int count);

    std::shared_ptr<ThreadPoolProvider> _provider;
    std::shared_ptr<ThreadPoolProvider> _default_provider;
};

namespace
{

//
// class DefaultThreadPoolProvider
//
class DefaultThreadPoolProvider : public ThreadPoolProvider
{
public:
    DefaultThreadPoolProvider (int count);
    DefaultThreadPoolProvider (const DefaultThreadPoolProvider&) = delete;
    DefaultThreadPoolProvider&
    operator= (const DefaultThreadPoolProvider&)                       = delete;
    DefaultThreadPoolProvider (DefaultThreadPoolProvider&&)            = delete;
    DefaultThreadPoolProvider& operator= (DefaultThreadPoolProvider&&) = delete;
    ~DefaultThreadPoolProvider () override;

    int  numThreads () const override;
    void setNumThreads (int count) override;
    void addTask (Task* task) override;

    void finish () override;

private:
    void threadLoop ();

    // we use this to work around launch issues not updating thread::get_id in constructor
    Semaphore          _threadSemaphore;
    Semaphore          _taskSemaphore; // threads wait on this for ready tasks
    mutable std::mutex _taskMutex;     // mutual exclusion for the tasks list
    std::vector<Task*> _tasks;         // the list of tasks to execute

    mutable std::mutex       _threadMutex; // mutual exclusion for threads list
    std::vector<std::thread> _threads;     // the list of all threads

    std::atomic<int>  _threadCount;
    std::atomic<bool> _stopping;

    inline bool stopped () const
    {
        return _stopping.load (std::memory_order_relaxed);
    }

    inline void stop () { _stopping = true; }

    inline void reset ()
    {
        _threadCount = 0;
        _stopping    = false;
    }
};

DefaultThreadPoolProvider::DefaultThreadPoolProvider (int count)
{
    reset ();

    setNumThreads (count);
}

DefaultThreadPoolProvider::~DefaultThreadPoolProvider ()
{}

int
DefaultThreadPoolProvider::numThreads () const
{
    return _threadCount.load ();
}

void
DefaultThreadPoolProvider::setNumThreads (int count)
{
    // since we're a private class, the thread pool won't call us if
    // we aren't changing size so no need to check that...

    // in order to make sure there isn't an undefined use in finish,
    // finish needs to have the thread mutex lock to be able to join,
    // such that if someone is changing the thread provider in one
    // thread while another thread is calling setNumThreads
    //
    // so use an atomic to cache the thread count we can pull without
    // a lock and then go ahead and zilch everything if we're clearing
    // it out.
    //
    // This isn't great, perhaps, but the likely scenario of this
    // changing frequently is people ping-ponging between 0 and N
    // which would result in a full clear anyway
#ifdef ENABLE_DYNAMIC_THREAD_GROWTH
    if (count < numThreads ()) { finish (); }

    // now take the lock and build any threads needed
    std::lock_guard<std::mutex> lock (_threadMutex);

    reset ();

    size_t nToAdd = static_cast<size_t> (count) - _threads.size ();
    for (size_t i = 0; i < nToAdd; ++i)
        _threads.emplace_back (
            std::thread (&DefaultThreadPoolProvider::threadLoop, this));

    // it would appear some systems crash randomly if a join is used
    // prior to the full start of the thread, let's ensure things are
    // spun up before we continue
    for (size_t i = 0; i < nToAdd; ++i)
        _threadSemaphore.wait ();

    _threadCount = static_cast<int> (_threads.size ());
#else
    finish ();

    reset ();

    size_t nToAdd = static_cast<size_t> (count);
    _threads.resize (nToAdd);
    for (size_t i = 0; i < nToAdd; ++i)
        _threads[i] = std::thread (&DefaultThreadPoolProvider::threadLoop, this);

    // wait for all the threads to start..
    for (size_t i = 0; i < nToAdd; ++i)
        _threadSemaphore.wait ();

    _threadCount = count;
#endif
}

void
DefaultThreadPoolProvider::addTask (Task* task)
{
    // the thread pool will kill us and switch to a null provider
    // if the thread count is set to 0, so we can always
    // go ahead and lock and assume we have a thread to do the
    // processing
    {
        std::lock_guard<std::mutex> taskLock (_taskMutex);

        //
        // Push the new task into the FIFO
        //
        _tasks.push_back (task);
    }

    //
    // Signal that we have a new task to process
    //
    _taskSemaphore.post ();
}

void
DefaultThreadPoolProvider::finish ()
{
    std::lock_guard<std::mutex> lock (_threadMutex);

    stop ();

    //
    // Signal enough times to allow all threads to stop.
    //
    // NB: we must do this as many times as we have threads.
    //
    // If there is still work in the queue, or this call happens "too
    // quickly", threads will not be waiting on the semaphore, so we
    // need to ensure the semaphore is at a count equal to the amount
    // of work left plus the number of threads to ensure exit of a
    // thread. There can be threads in a few states:
    //   - still starting up (successive calls to setNumThreads)
    //   - in the middle of processing a task / looping
    //   - waiting in the semaphore
    size_t curT = _threads.size ();
    for (size_t i = 0; i != curT; ++i)
        _taskSemaphore.post ();

    //
    // We should not need to check joinability, they should all, by
    // definition, be joinable (assuming normal start)
    //
    for (size_t i = 0; i != curT; ++i)
    {
#    ifdef TEST_FOR_WIN_THREAD_STATUS
        // This isn't quite right in that the thread may have actually
        // be in an exited / signalled state (needing the
        // WaitForSingleObject call), and so already have an exit code
        // (I think, but the docs are vague), but if we don't do the
        // join, the stl thread seems to then throw an exception. The
        // join should just return invalid handle and continue, and is
        // more of a windows bug... except maybe someone needs to work
        // around it...

        // per OIIO issue #2038, on exit / dll unload, windows may
        // kill the thread, double check that it is still active prior
        // to joining.
        DWORD tstatus;
        if (GetExitCodeThread (_threads[i].native_handle (), &tstatus))
        {
            if (tstatus != STILL_ACTIVE) { continue; }
        }
#    endif
        _threads[i].join ();
    }

    _threads.clear ();

#ifdef ENABLE_DYNAMIC_THREAD_GROWTH
    reset ();
#else
    _threadCount = 0;
#endif
}

void
DefaultThreadPoolProvider::threadLoop ()
{
    // tell the parent thread we've started
    _threadSemaphore.post ();

    while (true)
    {
        //
        // Wait for a task to become available
        //

        _taskSemaphore.wait ();

        {
            std::unique_lock<std::mutex> taskLock (_taskMutex);

            //
            // If there is a task pending, pop off the next task in the FIFO
            //

            if (!_tasks.empty ())
            {
                Task* task = _tasks.back ();
                _tasks.pop_back ();

                // release the mutex while we process
                taskLock.unlock ();

                handleProcessTask (task);

                // do not need to reacquire the lock at all since we
                // will just loop around, pull any other task
            }
            else if (stopped ()) { break; }
        }
    }
}

} //namespace

//
// struct TaskGroup::Data
//

TaskGroup::Data::Data () : numPending (0), isEmpty (1)
{
    // empty
}

TaskGroup::Data::~Data ()
{
    //
    // A TaskGroup acts like an "inverted" semaphore: if the count
    // is above 0 then waiting on the taskgroup will block.  This
    // destructor waits until the taskgroup is empty before returning.
    //

    isEmpty.wait ();

#    ifdef ENABLE_SEM_DTOR_WORKAROUND
    // Update: this was fixed in v. 2.2.21, so this ifdef checks for that
    //
    // Alas, given the current bug in glibc we need a secondary
    // syncronisation primitive here to account for the fact that
    // destructing the isEmpty Semaphore in this thread can cause
    // an error for a separate thread that is issuing the post() call.
    // We are entitled to destruct the semaphore at this point, however,
    // that post() call attempts to access data out of the associated
    // memory *after* it has woken the waiting threads, including this one,
    // potentially leading to invalid memory reads.
    // http://sources.redhat.com/bugzilla/show_bug.cgi?id=12674

    std::lock_guard<std::mutex> lock (dtorMutex);
#    endif
}

void
TaskGroup::Data::addTask ()
{
    //
    // in c++11, we use an atomic to protect numPending to avoid the
    // extra lock but for c++98, to add the ability for custom thread
    // pool we add the lock here
    //
    if (numPending++ == 0) isEmpty.wait ();
}

void
TaskGroup::Data::removeTask ()
{
    // Alas, given the current bug in glibc we need a secondary
    // syncronisation primitive here to account for the fact that
    // destructing the isEmpty Semaphore in a separate thread can
    // cause an error. Issuing the post call here the current libc
    // implementation attempts to access memory *after* it has woken
    // waiting threads.
    // Since other threads are entitled to delete the semaphore the
    // access to the memory location can be invalid.
    // http://sources.redhat.com/bugzilla/show_bug.cgi?id=12674
    // Update: this bug has been fixed, but how do we know which
    // glibc version we're in?

    // Further update:
    //
    // we could remove this if it is a new enough glibc, however
    // we've changed the API to enable a custom override of a
    // thread pool. In order to provide safe access to the numPending,
    // we need the lock anyway, except for c++11 or newer
    if (--numPending == 0)
    {
#    ifdef ENABLE_SEM_DTOR_WORKAROUND
        std::lock_guard<std::mutex> lk (dtorMutex);
#    endif
        isEmpty.post ();
    }
}

//
// struct ThreadPool::Data
//

ThreadPool::Data::Data ()
{
    // empty
}

ThreadPool::Data::~Data ()
{
    setProvider (nullptr);
}

#endif // ENABLE_THREADING

//
// class Task
//

Task::Task (TaskGroup* g) : _group (g)
{
#ifdef ENABLE_THREADING
    if (g) g->_data->addTask ();
#endif
}

Task::~Task ()
{
    // empty
}

TaskGroup*
Task::group ()
{
    return _group;
}

TaskGroup::TaskGroup ()
    :
#ifdef ENABLE_THREADING
    _data (new Data ())
#else
    _data (nullptr)
#endif
{
    // empty
}

TaskGroup::~TaskGroup ()
{
#ifdef ENABLE_THREADING
    delete _data;
#endif
}

void
TaskGroup::finishOneTask ()
{
#ifdef ENABLE_THREADING
    _data->removeTask ();
#endif
}

//
// class ThreadPoolProvider
//

ThreadPoolProvider::ThreadPoolProvider ()
{}

ThreadPoolProvider::~ThreadPoolProvider ()
{}

//
// class ThreadPool
//

ThreadPool::ThreadPool (unsigned nthreads)
    :
#ifdef ENABLE_THREADING
    _data (new Data)
#else
    _data (nullptr)
#endif
{
#ifdef ENABLE_THREADING
    if (nthreads > 0 && nthreads < unsigned (-1))
        _data->resetToDefaultProvider (static_cast<int> (nthreads));
#endif
}

ThreadPool::~ThreadPool ()
{
#ifdef ENABLE_THREADING
    // ensures any jobs / threads are finished & shutdown
    _data->setProvider (nullptr);
    delete _data;
#endif
}

int
ThreadPool::numThreads () const
{
#ifdef ENABLE_THREADING
    Data::ProviderPtr sp = _data->getProvider ();
    return (sp) ? sp->numThreads () : 0;
#else
    return 0;
#endif
}

void
ThreadPool::setNumThreads (int count)
{
#ifdef ENABLE_THREADING
    if (count < 0)
        throw IEX_INTERNAL_NAMESPACE::ArgExc (
            "Attempt to set the number of threads "
            "in a thread pool to a negative value.");

    {
        Data::ProviderPtr sp = _data->getProvider ();
        if (sp)
        {
            bool doReset = false;
            int  curT    = sp->numThreads ();
            if (curT == count) return;

            if (count != 0)
            {
                sp->setNumThreads (count);
                return;
            }
        }
    }

    // either a null provider or a case where we should switch from
    // a default provider to a null one or vice-versa
    if (count == 0)
        _data->setProvider (nullptr);
    else
        _data->resetToDefaultProvider (count);
#else
    // just blindly ignore
    (void) count;
#endif
}

void
ThreadPool::setThreadProvider (ThreadPoolProvider* provider)
{
#ifdef ENABLE_THREADING
    // contract is we take ownership and will free the provider
    _data->setProvider (Data::ProviderPtr (provider));
#else
    throw IEX_INTERNAL_NAMESPACE::ArgExc (
        "Attempt to set a thread provider on a system with threads"
        " disabled / not available");
#endif
}

void
ThreadPool::addTask (Task* task)
{
    if (task)
    {
#ifdef ENABLE_THREADING
        Data::ProviderPtr p = _data->getProvider ();
        if (p)
        {
            p->addTask (task);
            return;
        }
#endif

        handleProcessTask (task);
    }
}

ThreadPool&
ThreadPool::globalThreadPool ()
{
    //
    // The global thread pool
    //

    static ThreadPool gThreadPool (0);

    return gThreadPool;
}

void
ThreadPool::addGlobalTask (Task* task)
{
    globalThreadPool ().addTask (task);
}

unsigned
ThreadPool::estimateThreadCountForFileIO ()
{
#ifdef ENABLE_THREADING
    unsigned rv = std::thread::hardware_concurrency ();
    if (rv == 0 || rv == unsigned (-1)) rv = 1;
    return rv;
#else
    return 0;
#endif
}

#ifdef ENABLE_THREADING
void
ThreadPool::Data::resetToDefaultProvider (int count)
{
    ProviderPtr dp = std::atomic_load (&_default_provider);
    if (dp) { dp->setNumThreads (count); }
    else
    {
        dp = std::make_shared<DefaultThreadPoolProvider> (count);
        std::atomic_store (&_default_provider, dp);
    }
    setProvider (dp);
}
#endif

ILMTHREAD_INTERNAL_NAMESPACE_SOURCE_EXIT
