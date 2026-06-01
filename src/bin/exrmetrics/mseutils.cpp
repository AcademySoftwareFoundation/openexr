// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.

#include "mseutils.h"

#include <cmath>
#include <limits>

using IMATH_NAMESPACE::half;

template <>
void
accumMSE<half> (
    const half* orig,
    const half* reread,
    uint64_t    pixelsInChannel,
    double&     sumSq,
    uint64_t&   count)
{
    const double LN_HALF_DENORM_MIN = std::log(HALF_DENORM_MIN);
    for (uint64_t px = 0; px < pixelsInChannel; ++px)
    {
        double a = static_cast<double> (orig[px]);
        double b = static_cast<double> (reread[px]);
        if (!std::isfinite (b)) { count = 0; sumSq = 0.0; return; }
        if (std::isfinite (a))
        {
            double diff = (a < 0 ? -1.0 : 1.0) * (std::log (std::abs (a) + HALF_DENORM_MIN) - LN_HALF_DENORM_MIN) -
                          (b < 0 ? -1.0 : 1.0) * (std::log (std::abs (b) + HALF_DENORM_MIN) - LN_HALF_DENORM_MIN);
            sumSq += diff * diff;
            ++count;
        }
    }
}

template <>
void
accumMSE<float> (
    const float* orig,
    const float* reread,
    uint64_t     pixelsInChannel,
    double&      sumSq,
    uint64_t&    count)
{
    const double ln_eps = std::log (std::numeric_limits<float>::denorm_min ());
    constexpr double eps = std::numeric_limits<float>::denorm_min ();
    for (uint64_t px = 0; px < pixelsInChannel; ++px)
    {
        double a = static_cast<double> (orig[px]);
        double b = static_cast<double> (reread[px]);
        if (!std::isfinite (b)) { count = 0; sumSq = 0.0; return; }
        if (std::isfinite (a))
        {
            double diff = (a < 0 ? -1.0 : 1.0) * (std::log (std::abs (a) + eps) - ln_eps) -
                          (b < 0 ? -1.0 : 1.0) * (std::log (std::abs (b) + eps) - ln_eps);
            sumSq += diff * diff;
            ++count;
        }
    }
}

template <>
void
accumMSE<unsigned int> (
    const unsigned int* orig,
    const unsigned int* reread,
    uint64_t            pixelsInChannel,
    double&             sumSq,
    uint64_t&           count)
{
    for (uint64_t px = 0; px < pixelsInChannel; ++px)
    {
        double diff =
            static_cast<double> (orig[px]) - static_cast<double> (reread[px]);
        sumSq += diff * diff;
        ++count;
    }
}
