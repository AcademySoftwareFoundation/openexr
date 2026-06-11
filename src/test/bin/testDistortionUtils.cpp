// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.

#include "distortionUtils.h"

#include <half.h>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

int
main ()
{
    using half = IMATH_NAMESPACE::half;

    constexpr uint64_t N = 32 * 32;

    std::vector<half> orig (N, half (1.0f));
    std::vector<half> reread (N, half (1.01f));

    double   sumSq = 0.0;
    uint64_t count = 0;

    accumLogMSE<half> (orig.data (), reread.data (), N, sumSq, count);

    if (count != N)
    {
        std::cerr << "FAIL: count=" << count << " expected=" << N << "\n";
        return EXIT_FAILURE;
    }

    // Expected sumSq: replicate the implementation formula for a single pixel
    // then scale by N, since every pixel pair is identical.
    const double hmin   = HALF_DENORM_MIN;
    const double lnHmin = std::log (hmin);
    double       a      = static_cast<double> (half (1.0f));
    double       b      = static_cast<double> (half (1.01f));
    double       diff   = (std::log (a + hmin) - lnHmin) -
                    (std::log (b + hmin) - lnHmin);
    double expectedSumSq = static_cast<double> (N) * diff * diff;

    if (std::abs (sumSq - expectedSumSq) > 1e-12 * expectedSumSq)
    {
        std::cerr << "FAIL: sumSq=" << sumSq
                  << " expected=" << expectedSumSq << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "PASS: accumLogMSE<half> 32x32 (count=" << count
              << " sumSq=" << sumSq << ")\n";
    return EXIT_SUCCESS;
}
