
#ifndef INCLUDED_EXR_METRICS_H
#define INCLUDED_EXR_METRICS_H

//----------------------------------------------------------------------------
//
//	Copy input to output, reporting file size and timing
//
//----------------------------------------------------------------------------

#include "ImfCompression.h"

void exrmetrics (
    const char       inFileName[],
    const char       outFileName[],
    int              part,
    Imf::Compression compression,
    float            level,
    int              halfMode);
#endif
