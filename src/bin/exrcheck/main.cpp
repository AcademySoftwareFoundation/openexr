// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.

#include <ImathConfig.h>
#include <ImfCheckFile.h>
#include <ImfMisc.h>
#include <OpenEXRConfig.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string.h>
#if defined _WIN32 || defined _WIN64
#    include <io.h>
#else
#    include <unistd.h>
#endif
#include <vector>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;

void
usageMessage (ostream& stream, const char* program_name, bool verbose = false)
{
    stream << "Usage: " << program_name
           << " [options] imagefile [imagefile ...]\n";

    if (verbose)
        stream
            << "\n"
               "Read exr files to validate their contents and the correct behavior of the software.\n"
               "\n"
               "Options:\n"
               "  -m            avoid excessive memory allocation (some files will not be fully checked)\n"
               "  -t            avoid spending excessive time (some files will not be fully checked)\n"
               "  -s            use stream API instead of file API\n"
               "  -c            add core library checks\n"
               "  -h, --help    print this message\n"
               "      --version print version information\n"
               "\n"
               "Report bugs via https://github.com/AcademySoftwareFoundation/openexr/issues or email security@openexr.com\n"
               "";
}

bool
exrCheck (
    const char* filename,
    bool        reduceMemory,
    bool        reduceTime,
    bool        useStream,
    bool        enableCoreCheck)
{
    if (useStream)
    {
        //
        // open file as stream, check size
        //
        ifstream instream (filename, ifstream::binary);

        if (!instream)
        {
            cerr << "internal error: bad file '" << filename
                 << "' for in-memory stream" << endl;
            return true;
        }

        instream.seekg (0, instream.end);
        streampos length = instream.tellg ();
        instream.seekg (0, instream.beg);

        const uintptr_t kMaxSize = uintptr_t (-1) / 4;
        if (length < 0 || length > (streampos) kMaxSize)
        {
            cerr << "internal error: bad file length " << length
                 << " for in-memory stream" << endl;
            return true;
        }

        //
        // read into memory
        //
        vector<char> data (length);
        instream.read (data.data (), length);
        if (instream.gcount () != length)
        {
            cerr << "internal error: failed to read file " << filename << endl;
            return true;
        }
        return checkOpenEXRFile (
            data.data (), length, reduceMemory, reduceTime, enableCoreCheck);
    }
    else
    {
        return checkOpenEXRFile (
            filename, reduceMemory, reduceTime, enableCoreCheck);
    }
}

int
main (int argc, char** argv)
{
    if (argc < 2)
    {
        usageMessage (cerr, argv[0], false);
        return 1;
    }

    bool reduceMemory    = false;
    bool reduceTime      = false;
    bool enableCoreCheck = false;
    bool badFileFound    = false;
    bool useStream       = false;
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
        {
            usageMessage (cout, "exrcheck", true);
            return 0;
        }
        else if (!strcmp (argv[i], "-m"))
        {
            //
            // note for further memory reduction, calls to the following could be added here
            // CompositeDeepScanLine::setMaximumSampleCount();
            // Header::setMaxImageSize();
            // Header::setMaxTileSize();

            reduceMemory = true;
        }
        else if (!strcmp (argv[i], "-t")) { reduceTime = true; }
        else if (!strcmp (argv[i], "-s")) { useStream = true; }
        else if (!strcmp (argv[i], "-c")) { enableCoreCheck = true; }
        else if (!strcmp (argv[i], "--version"))
        {
            const char* libraryVersion = getLibraryVersion ();

            cout << "exrcheck (OpenEXR) " << OPENEXR_VERSION_STRING;
            if (strcmp (libraryVersion, OPENEXR_VERSION_STRING))
                cout << "(OpenEXR version " << libraryVersion << ")";
            cout << " https://openexr.com" << endl;
            cout << "Copyright (c) Contributors to the OpenEXR Project" << endl;
            cout << "License BSD-3-Clause" << endl;

            return 0;
        }
        else
        {

#if defined _WIN32 || defined _WIN64
            if (_access (argv[i], 4) != 0)
#else
            if (access (argv[i], R_OK) != 0)
#endif
            {
                cerr << "No such file: " << argv[i] << endl;
                return -1;
            }

            cout << " file " << argv[i] << ' ';
            cout.flush ();

            bool hasError = exrCheck (
                argv[i], reduceMemory, reduceTime, useStream, enableCoreCheck);
            if (hasError)
            {
                cout << "bad\n";
                badFileFound = true;
            }
            else { cout << "OK\n"; }
        }
    }

    return badFileFound;
}
