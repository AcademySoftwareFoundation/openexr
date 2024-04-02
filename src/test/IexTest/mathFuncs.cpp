// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#undef __THROW
#include <math.h>
#include <iostream>

float
divide (float x, float y)
{
    std::cout << x << " / " << y << std::endl;
    return x / y;
}

float
root (float x)
{
    std::cout << "sqrt (" << x << ")" << std::endl;
    return sqrt (x);
}

float
grow (float x, int y)
{
    std::cout << "grow (" << x << ", " << y << ")" << std::endl;

    for (int i = 0; i < y; i++)
        x = x * x;

    return x;
}
