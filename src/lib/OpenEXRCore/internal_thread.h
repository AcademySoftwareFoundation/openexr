/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_THREAD_H
#define OPENEXR_PRIVATE_THREAD_H

#include "openexr_config.h"

// Thread-safe single initiatization, using InitOnceExecuteOnce on Windows,
// pthread_once elsewhere, or a simple variable if threading is completely disabled.
#if ILMTHREAD_THREADING_ENABLED
#    ifdef _WIN32
#        include <windows.h>
#        define ONCE_FLAG_INIT INIT_ONCE_STATIC_INIT
typedef INIT_ONCE once_flag;
static BOOL CALLBACK
once_init_fn (PINIT_ONCE once, PVOID param, PVOID* ctx)
{
    void (*fn) (void) = (void (*) (void)) param;
    fn ();
    return TRUE;
}
static inline void
call_once (once_flag* flag, void (*func) (void))
{
    InitOnceExecuteOnce (flag, once_init_fn, (PVOID) func, NULL);
}
#    else
#        include <pthread.h>
#        define ONCE_FLAG_INIT PTHREAD_ONCE_INIT
typedef pthread_once_t once_flag;
static inline void
call_once (once_flag* flag, void (*func) (void))
{
    (void) pthread_once (flag, func);
}
#    endif
#else
#    define ONCE_FLAG_INIT 0
typedef int once_flag;
static inline void
call_once (once_flag* flag, void (*func) (void))
{
    if (!*flag) {
        *flag = 1;
        func ();
    }
}
#endif

#endif /* OPENEXR_PRIVATE_THREAD_H */
