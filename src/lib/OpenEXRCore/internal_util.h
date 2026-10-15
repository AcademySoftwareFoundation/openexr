/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_UTIL_H
#define OPENEXR_PRIVATE_UTIL_H

#include <stdio.h>
#include <stdint.h>

static inline int
compute_sampled_height (int height, int y_sampling, int start_y)
{
    int nlines;

    if (y_sampling <= 1) return height;

    if (height == 1)
        nlines = (start_y % y_sampling) == 0 ? 1 : 0;
    else
    {
        int off, tmph;

        /* computed the number of times y % ysampling == 0, by
         * computing interval based on first and last time that occurs
         * on the given range
         */
        if (start_y < 0)
        {
            off = -start_y % y_sampling;
        }
        else
        {
            off = start_y % y_sampling;
            if (off != 0)
                off = (y_sampling - off);
        }

        tmph = height - off;
        if (tmph == 0) return 0;
        --tmph;
        nlines = tmph / y_sampling + 1;
    }

    return nlines;
}

static inline int
compute_sampled_width (int width, int x_sampling, int start_x)
{
    /*
     * we require that the start_x % x_sampling == 0 and for tiled images (and for deep),
     * x_sampling must be 1, so this can simplify the math compared to the y case
     * where when we are reading scanline images, we always are reading the entire
     * width. If this changes, can look like the above call for the lines, but
     * for now can be simpler math
     */
    if (x_sampling <= 1) return width;

    return (width == 1) ? 1 : (width / x_sampling);
}

/* Multiplies @p a by @p b and adds the result into @p accum, all as
 * uint64_t. Returns 1 and leaves @p accum unmodified if either the
 * multiplication or the subsequent accumulation would overflow a
 * 64-bit unsigned integer, otherwise returns 0. Used to guard
 * unpacked / scratch buffer size computations (width * height *
 * bytes-per-pixel * channel-count) against wraparound on crafted
 * files with extreme dimensions or channel counts. */
static inline int
checked_mul_accum_u64 (uint64_t* accum, uint64_t a, uint64_t b)
{
    uint64_t prod, sum;

    if (a != 0 && b > (UINT64_MAX / a)) return 1;
    prod = a * b;

    sum = *accum + prod;
    if (sum < *accum) return 1;

    *accum = sum;
    return 0;
}

#endif /* OPENEXR_PRIVATE_UTIL_H */
