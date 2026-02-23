/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_THREAD_H
#define OPENEXR_PRIVATE_THREAD_H

#include "openexr_config.h"

// Thread-safe single initiatization, using C11 threads if available,
// InitOnceExecuteOnce on Windows, pthread_once elsewhere, or a simple
// variable if threading is completely disabled.
//
// C11 threads.h is available when __STDC_NO_THREADS__ is not defined
// (C11+) or when the system provides it (e.g., glibc 2.43+).

#if ILMTHREAD_THREADING_ENABLED

// Check for C11 threads.h availability.
// Exclude MSVC - its threads.h has compatibility issues with C++ compilation.
#    if !defined(_MSC_VER) && !defined(_WIN32)
#        if defined(__has_include)
#            if __has_include(<threads.h>)
#                define EXR_HAS_C11_THREADS 1
#            endif
#        endif
// Also check via __STDC_NO_THREADS__ (defined when threads.h is NOT available)
#        if !defined(EXR_HAS_C11_THREADS) && defined(__STDC_VERSION__) && \
            __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#            define EXR_HAS_C11_THREADS 1
#        endif
#    endif

#    if defined(EXR_HAS_C11_THREADS)
#        include <threads.h>
// C11 provides once_flag, ONCE_FLAG_INIT, and call_once - use them directly
#    elif defined(_WIN32)
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
