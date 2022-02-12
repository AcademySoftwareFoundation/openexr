//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "ImfNamespace.h"
#include "OpenEXRConfigInternal.h"

#include "testDeepImage.h"
#include "testFlatImage.h"
#include "testIO.h"
#include "tmpDir.h"
#include <ImathRandom.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <time.h>

#ifdef _WIN32
#    include <windows.h>
#else
#    include <unistd.h>
#endif

using namespace std;

#define TEST(test)                                                             \
    {                                                                          \
        if (argc < 2 || !strcmp (argv[1], #test)) test (tempDir);              \
    }

int
main (int argc, char* argv[])
{
    //
    // Create temporary files in a uniquely named private temporary
    // subdirectory of IMF_TMP_DIR to avoid colliding with other running
    // instances of this program.
    //

    IMATH_NAMESPACE::Rand48 rand48 (time ((time_t*) 0));
    std::string             tempDir;

    while (true)
    {
#ifdef _WIN32
        char  tmpbuf[4096];
        DWORD len = GetTempPathA (4096, tmpbuf);
        if (len == 0 || len > 4095)
        {
            cerr << "Cannot retrieve temporary directory" << endl;
            return 1;
        }
        tempDir = tmpbuf;
        // windows does this automatically
        // tempDir += IMF_PATH_SEPARATOR;
        tempDir += "OpenEXRTest_";
#else
        tempDir = IMF_TMP_DIR "OpenEXRTest_";
#endif
        for (int i = 0; i < 8; ++i)
            tempDir += ('A' + rand48.nexti () % 26);

        if (mkdir (tempDir.c_str (), 0777) == 0)
        {
            tempDir += IMF_PATH_SEPARATOR;
            break; // success
        }

        if (errno != EEXIST)
        {
            cerr << "Cannot create directory " << tempDir << ". "
                 << strerror (errno) << endl;

            return 1;
        }
    }

    cout << "using temporary directory " << tempDir << endl;

    // NB: If you add a test here, make sure to enumerate it in the
    // CMakeLists.txt so it runs as part of the test suite
    TEST (testFlatImage);
    TEST (testDeepImage);
    TEST (testIO);
    // NB: If you add a test here, make sure to enumerate it in the
    // CMakeLists.txt so it runs as part of the test suite

    cout << "removing temporary directory " << tempDir << endl;
    rmdir (tempDir.c_str ());

    return 0;
}
