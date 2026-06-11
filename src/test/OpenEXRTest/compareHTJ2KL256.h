// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.

#ifndef COMPARE_HTJ2KL256_H_INCLUDED
#define COMPARE_HTJ2KL256_H_INCLUDED

#include "ImfArray.h"
#include "ImfNamespace.h"
#include "ImfRgba.h"
#include "half.h"
#include <ImfCRgbaFile.h>

bool checkHTJ2KSample (double src, double tst);
bool checkHTJ2KSample (unsigned int src, unsigned int tst);
bool checkHTJ2KSample (half src, half tst);
bool checkHTJ2KSample (ImfHalf src, ImfHalf tst);

#endif
