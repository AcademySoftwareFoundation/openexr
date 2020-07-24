///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2007, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

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

#ifdef OPENEXR_IMF_HAVE_LINUX_PROCFS
    #include <unistd.h>
    #include <sstream>
#endif


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
    
    if (argc == 3 && access (argv[2], R_OK) != 0)
    {
        std::cout << "No such file: " << argv[2] << endl;
        exit (-1);
    }

    TEST (testFuzzScanLines);
    TEST (testFuzzTiles);
    TEST (testFuzzDeepScanLines);
    TEST (testFuzzDeepTiles);
   

    if(helpMode)
    {
       cout << "IlmImfFuzzTest tests how resilient the IlmImf library is with\n"
	       "respect to broken input files: the program first damages\n"
	       "OpenEXR files by partially overwriting them with random data;\n"
	       "then it tries to read the damaged files.  If all goes well,\n"
	       "then the program doesn't crash.\n";
       cout << "\n";
       cout << "If IlmImfFuzzTest does crash, it will leave a file in the current\n"
	       "directory, or /var/tmp. Running 'IlmImfFuzzTest test file' will\n"
	       "usually quickly reproduce the issue by attempting to reload the file,\n"
	       "(without running the normal tests) and is useful for debugging\n"
	       "the exact cause of the crash or confirming a bug is fixed.\n";
       cout << "\n";
       cout << "usage:\n";
       cout << " IlmImfFuzzTest             : with no arguments, run all tests\n";
       cout << " IlmImfFuzzTest TEST        : run specific TEST only\n";
       cout << " IlmImfFuzzTest TEST file   : try to read 'file' with given TEST\n";
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
