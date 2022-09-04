# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

set(IMF_HAVE_SSE4_1_TEST 0)
if(WIN32)
<<<<<<< HEAD
    # Windows does not have a macro for SSE4.1 support so we need to use the
    # __cpuid() function instead.
    #
    # https://docs.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170
=======
	# Windows does not have a macro for SSE4.1 support, we need to use the 
	# function __cpuid() instead.
	#
	# https://docs.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170
>>>>>>> 193e0fee (Initial check in)
    if(NOT DEFINED IMF_HAVE_SSE4_1)
        try_run(
            CPUID_RUN_RESULT
            CPUID_COMPILE_RESULT
            ${CMAKE_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/cmake/SSE4-Win32.c
            RUN_OUTPUT_VARIABLE CPUID_RUN_OUTPUT)
        string(REGEX MATCH "sse4_1: +([0-1])" CPUID_SSE4_MATCH ${CPUID_RUN_OUTPUT})
        set(IMF_HAVE_SSE4_1_TEST ${CMAKE_MATCH_1})
    endif()
endif()
<<<<<<< HEAD
set(IMF_HAVE_SSE4_1 ${IMF_HAVE_SSE4_1_TEST} CACHE BOOL "Enable SSE4.1 support")
<<<<<<< HEAD
=======
set(IMF_HAVE_SSE4_1 ${IMF_HAVE_SSE4_1_TEST} CACHE BOOL "Whether SSE4.1 support is available")
>>>>>>> 193e0fee (Initial check in)
if(IMF_HAVE_SSE4_1)
    add_compile_definitions(-DIMF_HAVE_SSE4_1=1)
endif()
=======
>>>>>>> ff3ecfe0 (Compile definition fixes)
