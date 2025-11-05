//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) DreamWorks Animation LLC and Contributors of the OpenEXR Project
//

#include <half.h>
#include <stdint.h>

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

extern uint16_t* exrcore_expTable;
extern uint16_t* exrcore_logTable;

static once_flag b44_tables_once = ONCE_FLAG_INIT;

static inline uint16_t
b44_convertFromLinear (uint16_t x)
{
    if ((x & 0x7c00) == 0x7c00) // infinity/nan?
        return 0;
    if (x >= 0x558c && x < 0x8000) // >= 8 * log (HALF_MAX)
        return 0x7bff;             // HALF_MAX

    float f = imath_half_to_float (x);
    f       = expf (f / 8);
    return imath_float_to_half (f);
}

static inline uint16_t
b44_convertToLinear (uint16_t x)
{
    if ((x & 0x7c00) == 0x7c00) // infinity/nan?
        return 0;
    if (x > 0x8000) // negative? (excluding -0.0 which is accepted)
        return 0;

    float f = imath_half_to_float (x);
    f       = 8 * logf (f);
    return imath_float_to_half (f);
}


static void
init_b44_tables(void)
{
    for (int i = 0; i < 65536; i++)
    {
        exrcore_expTable[i] = b44_convertFromLinear (i);
        exrcore_logTable[i] = b44_convertToLinear (i);
    }
}

void
exrcore_ensure_b44_tables()
{
    call_once (&b44_tables_once, init_b44_tables);
}
