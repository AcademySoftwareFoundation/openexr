// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.

#ifndef INCLUDED_DISTORTION_UTILS_H
#define INCLUDED_DISTORTION_UTILS_H

#include <half.h>
#include <cstdint>

template <typename T>
void accumLogMSE (
    const T*  orig,
    const T*  reread,
    uint64_t  pixelsInChannel,
    double&   sumSq,
    uint64_t& count);

template <>
void accumLogMSE<IMATH_NAMESPACE::half> (
    const IMATH_NAMESPACE::half* orig,
    const IMATH_NAMESPACE::half* reread,
    uint64_t                     pixelsInChannel,
    double&                      sumSq,
    uint64_t&                    count);

template <>
void accumLogMSE<float> (
    const float* orig,
    const float* reread,
    uint64_t     pixelsInChannel,
    double&      sumSq,
    uint64_t&    count);

#endif
