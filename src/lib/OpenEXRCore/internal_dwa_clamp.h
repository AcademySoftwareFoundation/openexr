//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef IMF_INTERNAL_DWA_CLAMP_H_HAS_BEEN_INCLUDED
#define IMF_INTERNAL_DWA_CLAMP_H_HAS_BEEN_INCLUDED

#include <stdint.h>

//
// Clamping, used for both compression and decompression. The DCT can not
// represent inf, so use closest is HALF_MAX. DCT ringing results in values
// beyond HALF_MAX even for values well below that, especially at high
// compression level. The clamp on decompression avoids generating infs
// that were not in the original image.
//
// HALF_MAX is a double on some platforms and in the public API, so we
// define our own DWA_HALF_MAX here to avoid accidental casts to double.
//

#define DWA_HALF_MAX 65504.0f
#define DWA_HALF_MAX_BITS 0x7bff

static inline float
dwaClampFloat (float f)
{
    if (f >= -DWA_HALF_MAX && f <= DWA_HALF_MAX)
        return f;
    if (f != f)
        return 0.0f; // NaN
    return (f > 0.0f) ? DWA_HALF_MAX : -DWA_HALF_MAX; // Inf
}

static inline uint16_t
dwaClampHalf (uint16_t h)
{
    if ((h & 0x7fff) > 0x7c00)
        return 0; // Nan
    if ((h & 0x7fff) == 0x7c00)
        return (uint16_t) ((h & 0x8000) | DWA_HALF_MAX_BITS); // Inf
    return h;
}

#endif /* IMF_INTERNAL_DWA_CLAMP_H_HAS_BEEN_INCLUDED */
