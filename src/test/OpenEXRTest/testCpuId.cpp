//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <ImfSimd.h>
#include <ImfSystemSpecific.h>
#include <iostream>

using namespace std;

void
testCpuId (const string&)
{
#if defined(IMF_HAVE_SSE2)
    std::cout << "IMF_HAVE_SSE2: " << true << "\n";
#else
    std::cout << "IMF_HAVE_SSE2: " << false << "\n";
#endif
#if defined(IMF_HAVE_SSE4_1)
    std::cout << "IMF_HAVE_SSE4_1: " << true << "\n";
#else
    std::cout << "IMF_HAVE_SSE4_1: " << false << "\n";
#endif
#if defined(IMF_HAVE_AVX)
    std::cout << "IMF_HAVE_AVX: " << true << "\n";
#else
    std::cout << "IMF_HAVE_AVX: " << false << "\n";
#endif

    OPENEXR_IMF_NAMESPACE::CpuId cpuId;
    std::cout << "cpuId.sse2: " << cpuId.sse2 << "\n";
    std::cout << "cpuId.sse3: " << cpuId.sse3 << "\n";
    std::cout << "cpuId.ssse3: " << cpuId.ssse3 << "\n";
    std::cout << "cpuId.sse4_1: " << cpuId.sse4_1 << "\n";
    std::cout << "cpuId.sse4_2: " << cpuId.sse4_2 << "\n";
    std::cout << "cpuId.avx: " << cpuId.avx << "\n";
    std::cout << "cpuId.f16c: " << cpuId.f16c << std::endl;
}
