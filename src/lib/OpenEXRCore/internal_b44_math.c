/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <stdint.h>
#include <half.h>

static inline uint16_t
b44_convertFromLinear (uint16_t x)
{
    if ((x & 0x7c00) == 0x7c00) // infinity/nan?
        return 0;
    if (x >= 0x558c && x < 0x8000) // >= 8 * log (HALF_MAX)
        return 0x7bff;             // HALF_MAX

    float f = imath_half_to_float (x);
    f       = expf (f / 8);
    return imath_float_to_half (f);
}

void b44_convertFromLinear_16(uint16_t s[16])
{
    for (int i = 0; i < 16; ++i)
        s[i] = b44_convertFromLinear (s[i]);
}

static inline uint16_t
b44_convertToLinear (uint16_t x)
{
    if ((x & 0x7c00) == 0x7c00) // infinity/nan?
        return 0;
    if (x > 0x8000) // negative? (excluding -0.0 which is accepted)
        return 0;

    float f = imath_half_to_float (x);
    f       = 8 * logf (f);
    return imath_float_to_half (f);
}

void
b44_convertToLinear_16 (uint16_t s[16])
{
    for (int i = 0; i < 16; ++i)
        s[i] = b44_convertToLinear (s[i]);
}
