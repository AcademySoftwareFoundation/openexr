// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.

#include "distortionUtils.h"

#include <half.h>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

// Tests accumLogMSE<T> on an N-pixel channel where every orig sample equals
// origVal and every reread sample equals rereadVal. eps is the type-specific
// denorm_min used by the implementation (HALF_DENORM_MIN or
// numeric_limits<float>::denorm_min()).
template <typename T>
bool
testAccumLogMSE (
    const char* label, T origVal, T rereadVal, uint64_t N, double eps)
{
    std::vector<T> orig (N, origVal);
    std::vector<T> reread (N, rereadVal);

    double   sumSq = 0.0;
    uint64_t count = 0;
    accumLogMSE<T> (orig.data (), reread.data (), N, sumSq, count);

    if (count != N)
    {
        std::cerr << "FAIL: " << label << " count=" << count
                  << " expected=" << N << "\n";
        return false;
    }

    // Replicate the per-pixel formula from the implementation and scale by N
    // (all pixel pairs are identical so the result is N * diff^2).
    const double lnEps   = std::log (eps);
    double       a       = static_cast<double> (origVal);
    double       b       = static_cast<double> (rereadVal);
    double       diff    = (std::log (a + eps) - lnEps) -
                    (std::log (b + eps) - lnEps);
    double expectedSumSq = static_cast<double> (N) * diff * diff;

    if (std::abs (sumSq - expectedSumSq) > 1e-12 * expectedSumSq)
    {
        std::cerr << "FAIL: " << label << " sumSq=" << sumSq
                  << " expected=" << expectedSumSq << "\n";
        return false;
    }

    std::cout << "PASS: " << label << " (count=" << count
              << " sumSq=" << sumSq << ")\n";
    return true;
}

int
main ()
{
    using half = IMATH_NAMESPACE::half;

    constexpr uint64_t N = 32 * 32;
    bool               ok = true;

    ok &= testAccumLogMSE<half> (
        "accumLogMSE<half> 32x32",
        half (1.0f), half (1.01f),
        N, HALF_DENORM_MIN);

    ok &= testAccumLogMSE<float> (
        "accumLogMSE<float> 32x32",
        1.0f, 1.01f,
        N, std::numeric_limits<float>::denorm_min ());

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
