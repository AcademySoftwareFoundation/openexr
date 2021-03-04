//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "ImfNamespace.h"
#include "OpenEXRConfigInternal.h"

#include "testFuzzDeepScanLines.h"
#include "testFuzzDeepTiles.h"
#include "testFuzzScanLines.h"
#include "testFuzzTiles.h"

#include <stdlib.h>
#include <iostream>
#include <set>
#include <string.h>
#include <string>

#if defined _WIN32 || defined _WIN64
# include <io.h>
#else
# include <unistd.h>
#endif
#include <sstream>

using std::set;
using std::string;
using std::cout;
using std::endl;

#define TEST(x)                                 \
    if (helpMode)                               \
    {                                           \
        tests.insert(string(#x));               \
    }                                           \
    else if (argc < 2 || !strcmp (argv[1], #x)) \
    {                                           \
        testFound = true;                       \
        x(argc==3 ? argv[2] : nullptr);         \
    }

int
main (int argc, char *argv[])
{
    bool helpMode = false;
    bool testFound = false;
    
    if( argc==2 && (strcmp(argv[1],"--help")==0 || strcmp(argv[1],"-h")==0))
    {
	    helpMode = true;
    }
    set<string> tests;

    //
    // If there's a second argument, it's a test file, so make sure it
    // exists.
    //
    
#if defined _WIN32 || defined _WIN64
    if (argc == 3 && _access (argv[2], 4) != 0)
#else
    if (argc == 3 && access (argv[2], R_OK) != 0)
#endif
    {
        std::cout << "No such file: " << argv[2] << endl;
        exit (-1);
    }

    // NB: If you add a test here, make sure to enumerate it in the
    // CMakeLists.txt so it runs as part of the test suite
    TEST (testFuzzScanLines);
    TEST (testFuzzTiles);
    TEST (testFuzzDeepScanLines);
    TEST (testFuzzDeepTiles);
    // NB: If you add a test here, make sure to enumerate it in the
    // CMakeLists.txt so it runs as part of the test suite

    if(helpMode)
    {
       cout << "OpenEXRFuzzTest tests how resilient the OpenEXR library is with\n"
	       "respect to broken input files: the program first damages\n"
	       "OpenEXR files by partially overwriting them with random data;\n"
	       "then it tries to read the damaged files.  If all goes well,\n"
	       "then the program doesn't crash.\n";
       cout << "\n";
       cout << "If OpenEXRFuzzTest does crash, it will leave a file in the current\n"
	       "directory, or /var/tmp. Running 'OpenEXRFuzzTest test file' will\n"
	       "usually quickly reproduce the issue by attempting to reload the file,\n"
	       "(without running the normal tests) and is useful for debugging\n"
	       "the exact cause of the crash or confirming a bug is fixed.\n";
       cout << "\n";
       cout << "usage:\n";
       cout << " OpenEXRFuzzTest             : with no arguments, run all tests\n";
       cout << " OpenEXRFuzzTest TEST        : run specific TEST only\n";
       cout << " OpenEXRFuzzTest TEST file   : try to read 'file' with given TEST\n";
       cout << "\n";
       cout << "TEST can be one of the following:\n";
       for ( auto i = tests.begin() ; i!= tests.end() ; ++i )
       {
	       cout << ' ' << *i << endl;
       }

    }
    else if (!testFound)
    {
        cout << "No such test: " << argv[1] << endl;
    }
    else
    {
#ifdef OPENEXR_IMF_HAVE_LINUX_PROCFS

        //
        // Allow the user to check for file descriptor leaks
        //

        cout << "open file descriptors:" << endl;

        std::stringstream ss;
        ss << "ls -lG /proc/" << getpid() << "/fd";
        
        system (ss.str().c_str());
#endif
    }
    return 0;
}
