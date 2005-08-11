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

#ifndef INCLUDED_ILM_THREAD_POOL_H
#define INCLUDED_ILM_THREAD_POOL_H

namespace IlmThread
{

class TaskGroup;

//-----------------------------------------------------------------------------
//
//    class Task -- a Task to execute the ThreadPool
//
// This class provides the abstract interface for any task which a ThreadPool
// works on. Derived classes need to implement the execute() function which
// performs the actual computation.
//
// If you plan to use the ThreadPool interface in your own applications
// note that the implementation of the ThreadPool calls delete on tasks as they
// complete. Therefore, if you define a custom new operator for your tasks, to
// for instance use a custom heap, you must also write an appropriate delete.
// 
//
//-----------------------------------------------------------------------------

struct Task
{
    Task (TaskGroup* g);
    virtual ~Task ();
    virtual void execute () = 0;
    TaskGroup* group();

protected:
    TaskGroup* _group;
};


//-----------------------------------------------------------------------------
//
//    class ThreadPool -- a pool of threads used to execute Tasks
//
//-----------------------------------------------------------------------------

class ThreadPool  
{
public:

    //-----------------------------------------------------------
    // Constructor -- creates numThreads worker threads which
    // wait until a task is available. 
    //-----------------------------------------------------------

    ThreadPool (unsigned numThreads = 0);
    
    
    //-----------------------------------------------------------
    // Destructor -- waits for all tasks to complete, joins all
    // the threads to the calling thread, and then destroys them.
    //-----------------------------------------------------------

    virtual ~ThreadPool ();
    

    //-----------------------------------------------------------
    // Query and set the number of worker threads in the pool.
    //
    // Never call setNumThreads from within a Worker thread as
    // this will almost certainly cause a deadlock or crash.
    //-----------------------------------------------------------
    
    int numThreads () const;
    void setNumThreads (int count);
    
    
    //-----------------------------------------------------------
    // Adds a task for processing. The ThreadPool can handle any
    // number of tasks regardless of the number of working
    // threads. The tasks are first added onto a queue, and are
    // executed by threads as they become available, in FIFO
    // order.
    //-----------------------------------------------------------

    void addTask (Task* task);
    

    //-----------------------------------------------------------
    // Access functions for the global threadpool
    //-----------------------------------------------------------
    
    static ThreadPool& globalThreadPool ();
    static void addGlobalTask (Task* task);

    class Data;
protected:
    Data* _data;
};


//-----------------------------------------------------------------------------
//
//    class TaskGroup -- a collection of tasks
//
// Every task which is added to the ThreadPool belongs to a single task group.
// The destructor of the taskgroup waits for all tasks in the group to finish.
// This allows syncronization on the completion of a set of tasks.
//
//-----------------------------------------------------------------------------

class TaskGroup
{
public:
    TaskGroup();
    ~TaskGroup();

    class Data;
    Data* const _data;
};

} // namespace IlmThread

#endif // INCLUDED_IMF_THREAD_POOL_H
