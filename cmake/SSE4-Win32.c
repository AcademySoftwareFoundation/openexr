//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <intrin.h>
#include <stdio.h>

int main()
{
    int cpuInfo[4];
    int sse;
    int sse2;
    int sse3;
    int ssse3;
    int sse4_1;
    int sse4_2;
    
    __cpuid(cpuInfo, 1);
    
    sse    = cpuInfo[3] & (1 << 25) || 0;
    sse2   = cpuInfo[3] & (1 << 26) || 0;
    sse3   = cpuInfo[2] & (1 <<  0) || 0;
    ssse3  = cpuInfo[2] & (1 <<  9) || 0;
    sse4_1 = cpuInfo[2] & (1 << 19) || 0;
    sse4_2 = cpuInfo[2] & (1 << 20) || 0;
    
    printf("sse:    %d\n", sse);
    printf("sse2:   %d\n", sse2);
    printf("sse3:   %d\n", sse3);
    printf("ssse3:  %d\n", ssse3);
    printf("sse4_1: %d\n", sse4_1);
    printf("sse4_2: %d\n", sse4_2);
    
    return 0;
}
