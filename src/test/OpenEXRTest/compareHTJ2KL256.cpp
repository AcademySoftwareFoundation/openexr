// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <assert.h>
#include <math.h>

#include "compareHTJ2KL256.h"
#include "half.h"
#include <ImfCRgbaFile.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;

bool
checkHTJ2KSample (double src, double tst)
{
    if (isnan (src) || isnan (tst))
    {
        return true;
    }
    if (fabs (src) < 1e-5)
    {
        if (fabs (src - tst) > 1e-4)
        {
            return false;
        }
    }
    else if (fabs ((src - tst) / src) > 0.5)
    {
        return false;
    }

    return true;
}

bool
checkHTJ2KSample (unsigned int src, unsigned int tst)
{
    int diff = src < tst ? tst - src : src - tst;

    if (src < 2000000) {
        if (diff > 2000000)
        {
            return false;
        }
    } else {
        if (diff / src > 0.5)
        {
            return false;
        }
    }

    return true;
}

bool
checkHTJ2KSample (half src, half tst)
{
    if ((src.bits () & 0x7c00) == 0x7c00 ||
        (tst.bits () & 0x7c00) == 0x7c00)
        return true;
    bool g = checkHTJ2KSample ((double) src, (double) tst);
    if (!g) {
        std::cerr << "src=" << src << ", tst=" << tst << std::endl;
    }
    return g;
}

bool
checkHTJ2KSample (ImfHalf src, ImfHalf tst)
{
    half s, t;
    s.setBits (src);
    t.setBits (tst);
    return checkHTJ2KSample (s, t);
}
