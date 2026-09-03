/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_HT_QUALITY_H
#define OPENEXR_PRIVATE_HT_QUALITY_H

#include <math.h>

/** Lower bound of the valid range for the LJ2K lossy quality level. */
#define MIN_LOSSY_HTJ2K_QUALITY 1.f

/** Upper bound of the valid range for the LJ2K lossy quality level. */
#define MAX_LOSSY_HTJ2K_QUALITY 150.f

static inline int
is_lossy_htj2k_quality (float q)
{
    return isfinite (q) && q >= MIN_LOSSY_HTJ2K_QUALITY &&
           q <= MAX_LOSSY_HTJ2K_QUALITY;
}

static inline float
clamp_lossy_htj2k_quality (float q)
{
    if (q < MIN_LOSSY_HTJ2K_QUALITY) return MIN_LOSSY_HTJ2K_QUALITY;
    if (q > MAX_LOSSY_HTJ2K_QUALITY) return MAX_LOSSY_HTJ2K_QUALITY;
    return q;
}

#endif /* OPENEXR_PRIVATE_HT_QUALITY_H */
