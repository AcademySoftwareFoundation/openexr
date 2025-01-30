
#ifndef INCLUDED_EXR_METRICS_H
#define INCLUDED_EXR_METRICS_H

//----------------------------------------------------------------------------
//
//	Copy input to output, reporting file size and timing
//
//----------------------------------------------------------------------------

#include "ImfCompression.h"

const int BENCH_ROUNDS = 10;     // Number of benchmark rounds

void exrmetrics (
    const char       inFileName[],
    const char       outFileName[],
    int              part,
    OPENEXR_IMF_NAMESPACE::Compression compression,
    float            level,
    int              halfMode);

void exrbench (const char inFileName[], int threads);

#endif
