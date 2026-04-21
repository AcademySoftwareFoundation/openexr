//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) DreamWorks Animation LLC and Contributors of the OpenEXR Project
//

#include "ImfSystemSpecific.h"
#include "ImfNamespace.h"
#include "ImfSimd.h"
#include "OpenEXRConfig.h"
#include "OpenEXRConfigInternal.h"
#if defined(_MSC_VER)
#    include <intrin.h>
#endif

// Runtime CPUID must not depend on IMF_HAVE_SSE2 (__SSE2__). For example,
// gcc -m32 often omits __SSE2__ while the hardware still supports SSE2; the
// OpenEXRCore helpers use <cpuid.h> regardless, and CpuId must match.
#if (defined(__GNUC__) || defined(__clang__)) && !defined(__e2k__) &&           \
    (defined(__i386__) || defined(__x86_64__) || defined(__amd64__)) &&      \
    (!defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__))
#    include <cpuid.h>
#endif

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

namespace
{
#if (defined(__GNUC__) || defined(__clang__)) && !defined(__e2k__) &&           \
    (defined(__i386__) || defined(__x86_64__) || defined(__amd64__)) &&      \
    (!defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__))

void
cpuid (int n, int& eax, int& ebx, int& ecx, int& edx)
{
    unsigned int r0 = 0, r1 = 0, r2 = 0, r3 = 0;
    if (!__get_cpuid (static_cast<unsigned int> (n), &r0, &r1, &r2, &r3))
        r0 = r1 = r2 = r3 = 0;
    eax = static_cast<int> (r0);
    ebx = static_cast<int> (r1);
    ecx = static_cast<int> (r2);
    edx = static_cast<int> (r3);
}

#elif defined(_MSC_VER) &&                                                     \
    (defined(_M_IX86) || (defined(_M_AMD64) && !defined(_M_ARM64EC)))

// Helper functions for MSVC
void
cpuid (int n, int& eax, int& ebx, int& ecx, int& edx)
{
    int cpuInfo[4] = {-1};
    __cpuid (cpuInfo, n);
    eax = cpuInfo[0];
    ebx = cpuInfo[1];
    ecx = cpuInfo[2];
    edx = cpuInfo[3];
}

#elif defined(IMF_HAVE_SSE2) && defined(__GNUC__) && !defined(__e2k__)

// Fallback when <cpuid.h> is unavailable (e.g. unusual Windows GCC toolchains).
void
cpuid (int n, int& eax, int& ebx, int& ecx, int& edx)
{
    __asm__ __volatile__ (
        "cpuid"
        : /* Output  */ "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : /* Input   */ "a"(n)
        : /* Clobber */);
}

#else

// Helper functions for generic compiler - all disabled
void
cpuid (int n, int& eax, int& ebx, int& ecx, int& edx)
{
    eax = ebx = ecx = edx = 0;
}

#endif

#ifdef IMF_HAVE_GCC_INLINEASM_X86

void
xgetbv (int n, int& eax, int& edx)
{
    __asm__ __volatile__ ("xgetbv"
                          : /* Output  */ "=a"(eax), "=d"(edx)
                          : /* Input   */ "c"(n)
                          : /* Clobber */);
}

#else //  IMF_HAVE_GCC_INLINEASM_X86

void
xgetbv (int n, int& eax, int& edx)
{
    eax = edx = 0;
}

#endif //  IMF_HAVE_GCC_INLINEASM_X86

} // namespace

CpuId::CpuId ()
    : sse2 (false)
    , sse3 (false)
    , ssse3 (false)
    , sse4_1 (false)
    , sse4_2 (false)
    , avx (false)
    , f16c (false)
{
#if defined(__e2k__) // e2k - MCST Elbrus 2000 architecture
    // Use IMF_HAVE definitions to determine e2k CPU features
#    if defined(IMF_HAVE_SSE2)
    sse2 = true;
#    endif
#    if defined(IMF_HAVE_SSE3)
    sse3 = true;
#    endif
#    if defined(IMF_HAVE_SSSE3)
    ssse3 = true;
#    endif
#    if defined(IMF_HAVE_SSE4_1)
    sse4_1 = true;
#    endif
#    if defined(IMF_HAVE_SSE4_2)
    sse4_2 = true;
#    endif
#    if defined(IMF_HAVE_AVX)
    avx = true;
#    endif
#    if defined(IMF_HAVE_F16C)
    f16c = true;
#    endif
#else // x86/x86_64
    bool osxsave = false;
    int  max     = 0;
    int  eax, ebx, ecx, edx;

    cpuid (0, max, ebx, ecx, edx);
    if (max > 0)
    {
        cpuid (1, eax, ebx, ecx, edx);
        sse2    = (edx & (1 << 26));
        sse3    = (ecx & (1 << 0));
        ssse3   = (ecx & (1 << 9));
        sse4_1  = (ecx & (1 << 19));
        sse4_2  = (ecx & (1 << 20));
        osxsave = (ecx & (1 << 27));
        avx     = (ecx & (1 << 28));
        f16c    = (ecx & (1 << 29));

        if (!osxsave) { avx = f16c = false; }
        else
        {
            xgetbv (0, eax, edx);
            // eax bit 1 - SSE managed, bit 2 - AVX managed
            if ((eax & 6) != 6) { avx = f16c = false; }
        }
    }
#endif
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
