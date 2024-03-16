//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	exrmultiview -- a program that combines multiple
//	single-view OpenEXR image files into a single
//	multi-view image file.
//
//-----------------------------------------------------------------------------

#include "makeMultiView.h"
#include <ImfMisc.h>
#include <OpenEXRConfig.h>

#include <exception>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "namespaceAlias.h"
using namespace IMF;
using namespace std;

namespace
{

void
usageMessage (ostream& stream, const char* program_name, bool verbose = false)
{
    stream << "Usage: " << program_name
           << " [options] viewname1 infile1 viewname2 infile2 ... outfile"
           << endl;

    if (verbose)
    {
        std::string compressionNames;
        getCompressionNamesString ("/", compressionNames);

        stream
            << "\n"
               "Combine two or more single-view OpenEXR image files into\n"
               "a single multi-view image file.  On the command line,\n"
               "each single-view input image is specified together with\n"
               "a corresponding view name.  The first view on the command\n"
               "line becomes the default view.  Example:\n"
               "\n"
               "  exrmultiview left imgL.exr right imgR.exr imgLR.exr\n"
               "\n"
               "Here, imgL.exr and imgR.exr become the left and right\n"
               "views in output file imgLR.exr.  The left view becomes\n"
               "the default view.\n"
               "\n"
               "Options:\n"
               "\n"
               "  -z x          sets the data compression method to x\n"
               "                ("
            << compressionNames.c_str ()
            << ",\n"
               "                default is piz)\n"
               "\n"
               "  -v            verbose mode\n"
               "\n"
               "  -h, --help    print this message\n"
               "\n"
               "      --version print version information\n"
               "\n"
               "Report bugs via https://github.com/AcademySoftwareFoundation/openexr/issues or email security@openexr.com\n"
               "";
    }
}

Compression
getCompression (const string& str)
{
    Compression c;
    getCompressionIdFromName (str, c);
    if (c == Compression::NUM_COMPRESSION_METHODS)
    {
        std::stringstream e;
        e << "Unknown compression method \"" << str << "\"";
        throw invalid_argument (e.str ());
    }

    return c;
}

} // namespace

int
main (int argc, char** argv)
{
    vector<string>      views;
    vector<const char*> inFiles;
    const char*         outFile     = 0;
    Compression         compression = PIZ_COMPRESSION;
    bool                verbose     = false;

    //
    // Parse the command line.
    //

    if (argc < 2)
    {
        usageMessage (cerr, argv[0], false);
        return -1;
    }

    try
    {

        int i = 1;

        while (i < argc)
        {
            if (!strcmp (argv[i], "-z"))
            {
                //
                // Set compression method
                //

                if (i > argc - 2)
                    throw invalid_argument (
                        "Missing compression value with -z option");

                compression = getCompression (argv[i + 1]);
                i += 2;
            }
            else if (!strcmp (argv[i], "-v"))
            {
                //
                // Verbose mode
                //

                verbose = true;
                i += 1;
            }
            else if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
            {
                //
                // Print help message
                //

                usageMessage (cout, "exrmultiview", true);
                return 0;
            }
            else if (!strcmp (argv[i], "--version"))
            {
                const char* libraryVersion = getLibraryVersion ();

                cout << "exrmultiview (OpenEXR) " << OPENEXR_VERSION_STRING;
                if (strcmp (libraryVersion, OPENEXR_VERSION_STRING))
                    cout << "(OpenEXR version " << libraryVersion << ")";
                cout << " https://openexr.com" << endl;
                cout << "Copyright (c) Contributors to the OpenEXR Project"
                     << endl;
                cout << "License BSD-3-Clause" << endl;
                return 0;
            }
            else
            {
                //
                // View or image file name
                //

                if (i > argc - 2 || argv[i + 1][0] == '-')
                {
                    //
                    // Output file
                    //

                    if (outFile)
                        throw invalid_argument (
                            "improper output file specified");

                    outFile = argv[i];
                    i += 1;
                }
                else
                {
                    //
                    // View plus input file
                    //

                    views.push_back (argv[i]);
                    inFiles.push_back (argv[i + 1]);
                    i += 2;
                }
            }
        }

        if (views.size () < 2)
            throw invalid_argument ("Must specify at least two views");

        if (outFile == 0)
            throw invalid_argument ("Must specify an output file");

        //
        // Load inFiles, and save a combined multi-view image in outFile.
        //

        makeMultiView (views, inFiles, outFile, compression, verbose);
    }
    catch (const exception& e)
    {
        cerr << argv[0] << ": " << e.what () << endl;
        return -1;
    }

    return 0;
}
