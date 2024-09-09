
//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "exrmetrics.h"

#include "ImfCompression.h"
#include "ImfMisc.h"

#include <iostream>
#include <vector>

#include <math.h>
#include <stdlib.h>
#include <string.h>

using std::cerr;
using std::cout;
using std::endl;
using std::ostream;
using std::vector;
using namespace Imf;

void
usageMessage (ostream& stream, const char* program_name, bool verbose = false)
{
    stream << "Usage: " << program_name << " [options] infile outfile" << endl;

    if (verbose)
    {
        std::string compressionNames;
        getCompressionNamesString ("/", compressionNames);
        stream
            << "Read an OpenEXR image from infile, write an identical copy to outfile"
               " reporting time taken to read/write and file sizes.\n"
               "\n"
               "Options:\n"
               "\n"
               "  -p n          part number to copy (only one part will be written to output file)\n"
               "                default is part 0\n"
               "\n"
               "  -l level      set DWA or ZIP compression level\n"
               "\n"
               "  -z x          sets the data compression method to x\n"
               "                ("
            << compressionNames.c_str ()
            << ",\n"
               "                default retains original method)\n"
               "\n"
               "  -16 rgba|all  force 16 bit half float: either just RGBA, or all channels\n"
               "                default retains original type for all channels\n"
               "\n"
               "  -h, --help    print this message\n"
               "\n"
               "      --version print version information\n"
               "\n";
    }
}

int
main (int argc, char** argv)
{

    const char* outFile  = nullptr;
    const char* inFile   = nullptr;
    int         part     = 0;
    float       level    = INFINITY;
    int         halfMode = 0; // 0 - leave alone, 1 - just RGBA, 2 - everything
    Compression compression = Compression::NUM_COMPRESSION_METHODS;

    int i = 1;

    if (argc == 1)
    {
        usageMessage (cerr, "exrmetrics", true);
        return 1;
    }

    while (i < argc)
    {
        if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
        {
            usageMessage (cout, "exrmetrics", true);
            return 0;
        }

        else if (!strcmp (argv[i], "--version"))
        {
            const char* libraryVersion = getLibraryVersion ();

            cout << "exrmetrics (OpenEXR) " << OPENEXR_VERSION_STRING;
            if (strcmp (libraryVersion, OPENEXR_VERSION_STRING))
                cout << "(OpenEXR version " << libraryVersion << ")";
            cout << " https://openexr.com" << endl;
            cout << "Copyright (c) Contributors to the OpenEXR Project" << endl;
            cout << "License BSD-3-Clause" << endl;
            return 0;
        }
        else if (!strcmp (argv[i], "-z"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing compression value with -z option\n";
                return 1;
            }

            getCompressionIdFromName (argv[i + 1], compression);
            if (compression == Compression::NUM_COMPRESSION_METHODS)
            {
                cerr << "unknown compression type " << argv[i + 1] << endl;
                return 1;
            }
            i += 2;
        }
        else if (!strcmp (argv[i], "-p"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing part number with -p option\n";
                return 1;
            }
            part = atoi (argv[i + 1]);
            if (part < 0)
            {
                cerr << "bad part " << part << " specified to -p option\n";
                return 1;
            }

            i += 2;
        }
        else if (!strcmp (argv[i], "-l"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing compression level number with -l option\n";
                return 1;
            }
            level = atof (argv[i + 1]);
            if (level < 0)
            {
                cerr << "bad level " << level << " specified to -l option\n";
                return 1;
            }

            i += 2;
        }
        else if (!strcmp (argv[i], "-16"))
        {
            if (i > argc - 2)
            {
                cerr << "Missing mode with -16 option\n";
                return 1;
            }
            if (!strcmp (argv[i + 1], "all")) { halfMode = 2; }
            else if (!strcmp (argv[i + 1], "rgba")) { halfMode = 1; }
            else
            {
                cerr << " bad mode for -16 option: must be 'all' or 'rgba'\n";
                return 1;
            }
            i += 2;
        }
        else if (!inFile)
        {
            inFile = argv[i];
            i += 1;
        }
        else if (!outFile)
        {
            outFile = argv[i];
            i += 1;
        }
        else
        {
            cerr << "unknown argument or extra filename specified\n";
            usageMessage (cerr, "exrmetrics", false);
            return 1;
        }
    }
    if (!inFile || !outFile)
    {
        cerr << "Missing input or output file\n";
        usageMessage (cerr, "exrmetrics", false);
        return 1;
    }

    try
    {
        exrmetrics (inFile, outFile, part, compression, level, halfMode);
    }
    catch (std::exception& what)
    {
        cerr << "error from exrmetrics: " << what.what () << endl;
        return 1;
    }
    return 0;
}
