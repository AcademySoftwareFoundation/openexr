//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	exrmaketiled -- program that produces tiled
//	multiresolution versions of OpenEXR images.
//
//-----------------------------------------------------------------------------

#include "makeTiled.h"

#include <ImfHeader.h>
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
    stream << "Usage: " << program_name << " [options] infile outfile" << endl;

    if (verbose)
    {
        std::string compressionNames;
        getCompressionNamesString ("/", compressionNames);

        stream
            << "\n"
               "Read an OpenEXR image from infile, produce a tiled\n"
               "version of the image, and save the result in outfile.\n"
               "\n"
               "Options:\n"
               "\n"
               "  -o            produces a ONE_LEVEL image (default)\n"
               "\n"
               "  -m            produces a MIPMAP_LEVELS multiresolution image\n"
               "\n"
               "  -r            produces a RIPMAP_LEVELS multiresolution image\n"
               "\n"
               "  -f c          when a MIPMAP_LEVELS or RIPMAP_LEVELS image\n"
               "                is produced, image channel c will be resampled\n"
               "                without low-pass filtering.  This option can\n"
               "                be specified multiple times to disable low-pass\n"
               "                filtering for multiple channels.\n"
               "\n"
               "  -e x y        when a MIPMAP_LEVELS or RIPMAP_LEVELS image\n"
               "                is produced, low-pass filtering takes samples\n"
               "                outside the image's data window.  This requires\n"
               "                extrapolating the image.  Option -e specifies\n"
               "                how the image is extrapolated horizontally and\n"
               "                vertically (black/clamp/periodic/mirror, default\n"
               "                is clamp).\n"
               "\n"
               "  -t x y        sets the tile size in the output image to\n"
               "                x by y pixels (default is 64 by 64)\n"
               "\n"
               "  -d            sets level size rounding to ROUND_DOWN (default)\n"
               "\n"
               "  -u            sets level size rounding to ROUND_UP\n"
               "\n"
               "  -z x          sets the data compression method to x\n"
               "                ("
            << compressionNames.c_str ()
            << ",\n"
               "                default is zip)\n"
               "\n"
               "  -v            verbose mode\n"
               "\n"
               "  -h, --help    print this message\n"
               "\n"
               "      --version print version information\n"
               "\n"
               "Multipart Options:\n"
               "\n"
               "  -p i          part number, default is 0\n"
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

Extrapolation
getExtrapolation (const string& str)
{
    Extrapolation e;

    if (str == "black" || str == "BLACK") { e = BLACK; }
    else if (str == "clamp" || str == "CLAMP") { e = CLAMP; }
    else if (str == "periodic" || str == "PERIODIC") { e = PERIODIC; }
    else if (str == "mirror" || str == "MIRROR") { e = MIRROR; }
    else
    {
        std::stringstream e;
        e << "Unknown extrapolation method \"" << str << "\"";
        throw invalid_argument (e.str ());
    }

    return e;
}

void
getPartNum (int argc, char** argv, int& i, int* j)
{
    if (i > argc - 2)
        throw invalid_argument ("Missing part num with -p option");

    *j = strtol (argv[i + 1], 0, 0);
    cout << "part number: " << *j << endl;
    i += 2;
}

} // namespace

int
main (int argc, char** argv)
{
    const char*       inFile       = 0;
    const char*       outFile      = 0;
    LevelMode         mode         = ONE_LEVEL;
    LevelRoundingMode roundingMode = ROUND_DOWN;
    Compression       compression  = ZIP_COMPRESSION;
    int               tileSizeX    = 64;
    int               tileSizeY    = 64;
    set<string>       doNotFilter;
    Extrapolation     extX    = CLAMP;
    Extrapolation     extY    = CLAMP;
    bool              verbose = false;

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
        int i       = 1;
        int partnum = 0;

        while (i < argc)
        {
            if (!strcmp (argv[i], "-o"))
            {
                //
                // generate a ONE_LEVEL image
                //

                mode = ONE_LEVEL;
                i += 1;
            }
            else if (!strcmp (argv[i], "-m"))
            {
                //
                // Generate a MIPMAP_LEVELS image
                //

                mode = MIPMAP_LEVELS;
                i += 1;
            }
            else if (!strcmp (argv[i], "-r"))
            {
                //
                // Generate a RIPMAP_LEVELS image
                //

                mode = RIPMAP_LEVELS;
                i += 1;
            }
            else if (!strcmp (argv[i], "-f"))
            {
                //
                // Don't low-pass filter the specified image channel
                //

                if (i > argc - 2)
                    throw invalid_argument (
                        "missing filter value with -f option");

                doNotFilter.insert (argv[i + 1]);
                i += 2;
            }
            else if (!strcmp (argv[i], "-e"))
            {
                //
                // Set x and y extrapolation method
                //

                if (i > argc - 3)
                    throw invalid_argument (
                        "missing extrapolation values with -e option");

                extX = getExtrapolation (argv[i + 1]);
                extY = getExtrapolation (argv[i + 2]);
                i += 3;
            }
            else if (!strcmp (argv[i], "-t"))
            {
                //
                // Set tile size
                //

                if (i > argc - 3)
                    throw invalid_argument ("missing tile size with -t option");

                tileSizeX = strtol (argv[i + 1], 0, 0);
                tileSizeY = strtol (argv[i + 2], 0, 0);

                if (tileSizeX <= 0 || tileSizeY <= 0)
                    throw invalid_argument (
                        "Tile size must be greater than zero");

                i += 3;
            }
            else if (!strcmp (argv[i], "-d"))
            {
                //
                // Round down
                //

                roundingMode = ROUND_DOWN;
                i += 1;
            }
            else if (!strcmp (argv[i], "-u"))
            {
                //
                // Round down
                //

                roundingMode = ROUND_UP;
                i += 1;
            }
            else if (!strcmp (argv[i], "-z"))
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
            else if (!strcmp (argv[i], "-p"))
            {
                getPartNum (argc, argv, i, &partnum);
            }
            else if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
            {
                //
                // Print help message
                //

                usageMessage (cout, "exrmaketiled", true);
                return 0;
            }
            else if (!strcmp (argv[i], "--version"))
            {
                const char* libraryVersion = getLibraryVersion ();

                cout << "exrmaketiled (OpenEXR) " << OPENEXR_VERSION_STRING;
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

        //
        // Load inFile, and save a tiled version in outFile.
        //

        //
        // check input
        //
        {
            MultiPartInputFile input (inFile);
            int                parts = input.parts ();

            if (partnum < 0 || partnum >= parts)
            {
                std::stringstream e;
                e << "You asked for part " << partnum << " in " << inFile
                  << ", which only has " << parts << " parts";
                throw invalid_argument (e.str ());
            }

            Header h = input.header (partnum);
            if (h.type () == DEEPTILE || h.type () == DEEPSCANLINE)
                throw invalid_argument ("Cannot make tile for deep data");
        }

        makeTiled (
            inFile,
            outFile,
            partnum,
            mode,
            roundingMode,
            compression,
            tileSizeX,
            tileSizeY,
            doNotFilter,
            extX,
            extY,
            verbose);
    }
    catch (const exception& e)
    {
        cerr << argv[0] << ": " << e.what () << endl;
        return 1;
    }

    return 0;
}
