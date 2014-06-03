// DreamWorks Animation LLC Confidential Information.
// TM and (c) 2009-2014 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.

#include "ImfSimd.h"
#include "ImfSystemSpecific.h"
#include "ImfNamespace.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

namespace {
#if defined(IMF_HAVE_SSE2) &&  defined(__GNUC__)
    // Helper functions for gcc + SSE enabled
    void cpuid(int n, int &eax, int &ebx, int &ecx, int &edx)
    {
        __asm__ __volatile__ (
            "cpuid"
            : /* Output  */ "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) 
            : /* Input   */ "a"(n)
            : /* Clobber */);
    }

    void xgetbv(int n, int &eax, int &edx)
    {
        // Some compiler versions might not recognize "xgetbv" as a mnemonic.
        // Might end up needing to use ".byte 0x0f, 0x01, 0xd0" instead.
        __asm__ __volatile__ (
            "xgetbv"
            : /* Output  */ "=a"(eax), "=d"(edx) 
            : /* Input   */ "c"(n)
            : /* Clobber */);
    }

#else // IMF_HAVE_SSE2 && __GNUC__

    // Helper functions for generic compiler - all disabled
    void cpuid(int n, int &eax, int &ebx, int &ecx, int &edx)
    {
        eax = ebx = ecx = edx = 0;
    }

    void xgetbv(int n, int &eax, int &edx)
    {
        eax = edx = 0;
    }

#endif // IMF_HAVE_SSE2 && __GNUC__
} // namespace 

CpuId::CpuId():
    sse2(false), 
    sse3(false), 
    ssse3(false),
    sse4_1(false), 
    sse4_2(false), 
    avx(false), 
    f16c(false)
{
    bool osxsave = false;
    int  max     = 0;
    int  eax, ebx, ecx, edx;

    cpuid(0, max, ebx, ecx, edx);
    if (max > 0) {
        cpuid(1, eax, ebx, ecx, edx);
        sse2    = ( edx & (1<<26) );
        sse3    = ( ecx & (1<< 0) );
        ssse3   = ( ecx & (1<< 9) );
        sse4_1  = ( ecx & (1<<19) );
        sse4_2  = ( ecx & (1<<20) );
        osxsave = ( ecx & (1<<27) );
        avx     = ( ecx & (1<<28) );
        f16c    = ( ecx & (1<<29) );

        if (!osxsave) {
            avx = f16c = false;
        } else {
            xgetbv(0, eax, edx);
            // eax bit 1 - SSE managed, bit 2 - AVX managed
            if (eax & 6 != 6) {
                avx = f16c = false;
            }
        }
    }
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT

// TM and (c) 2009-2014 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.
