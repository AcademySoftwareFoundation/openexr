//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	exrmakepreview -- a program that inserts a
//	preview image into an OpenEXR file's header.
//
//-----------------------------------------------------------------------------

#include "makePreview.h"

#include <ImfMisc.h>
#include <OpenEXRConfig.h>

#include <exception>
#include <iostream>
#include <stdlib.h>
#include <string.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;

void
usageMessage (ostream& stream, const char* program_name, bool verbose = false)
{
    stream << "Usage: " << program_name << " [options] infile outfile" << endl;

    if (verbose)
        stream
            << "\n"
               "Read an OpenEXR image from infile, generate a preview\n"
               "image, add it to the image's header, and save the result\n"
               "in outfile.  Infile and outfile must not refer to the same\n"
               "file (the program cannot edit an image file \"in place\").\n"
               "\n"
               "Options:\n"
               "\n"
               "  -w x          sets the width of the preview image to x pixels\n"
               "                (default is 100)\n"
               "\n"
               "  -e s          adjusts the preview image's exposure by s f-stops\n"
               "                (default is 0).  Positive values make the image\n"
               "                brighter, negative values make it darker.\n"
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

int
main (int argc, char** argv)
{
    const char* inFile       = 0;
    const char* outFile      = 0;
    int         previewWidth = 100;
    float       exposure     = 0;
    bool        verbose      = false;

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
            if (!strcmp (argv[i], "-w"))
            {
                //
                // Set preview image width
                //

                if (i > argc - 2)
                    throw invalid_argument ("Missing width for -w argument");

                previewWidth = strtol (argv[i + 1], 0, 0);
                if (previewWidth <= 0)
                    throw invalid_argument (
                        "preview width must be greater than zero");

                i += 2;
            }
            else if (!strcmp (argv[i], "-e"))
            {
                //
                // Set exposure
                //

                if (i > argc - 2)
                    throw invalid_argument ("missing exposure for -e argument");

                exposure = strtod (argv[i + 1], 0);

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

                usageMessage (cout, "exrmakepreview", true);
                return 0;
            }
            else if (!strcmp (argv[i], "--version"))
            {
                const char* libraryVersion = getLibraryVersion ();

                cout << "exrmakepreview (OpenEXR) " << OPENEXR_VERSION_STRING;
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
                // Image file name
                //

                if (inFile == 0)
                    inFile = argv[i];
                else
                    outFile = argv[i];

                i += 1;
            }
        }

        if (inFile == 0 || outFile == 0)
        {
            usageMessage (cerr, argv[0], false);
            return -1;
        }

        if (!strcmp (inFile, outFile))
            throw invalid_argument ("Input and output cannot be the same file");

        if (previewWidth <= 0)
            throw invalid_argument (
                "Preview image width must be greater than zero");

        //
        // Load inFile, add a preview image, and save the result in outFile.
        //

        makePreview (inFile, outFile, previewWidth, exposure, verbose);
    }
    catch (const exception& e)
    {
        cerr << argv[0] << ": " << e.what () << endl;
        return 1;
    }

    return 0;
}
