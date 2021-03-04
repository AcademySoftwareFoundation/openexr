//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <testBaseExc.h>

#include <string.h>

#define TEST(x) if (argc < 2 || !strcmp (argv[1], #x)) x();

int
main (int argc, char *argv[])
{
    TEST (testBaseExc);
    return 0;
}
