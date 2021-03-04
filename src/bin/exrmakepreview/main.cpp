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

#include <iostream>
#include <exception>
#include <stdlib.h>
#include <string.h>

using namespace std;


void
usageMessage (const char argv0[], bool verbose = false)
{
    cerr << "usage: " << argv0 << " [options] infile outfile" << endl;

    if (verbose)
    {
	cerr << "\n"
		"Reads an OpenEXR image from infile, generates a preview\n"
		"image, adds it to the image's header, and saves the result\n"
		"in outfile.  Infile and outfile must not refer to the same\n"
		"file (the program cannot edit an image file \"in place\").\n"
		"\n"
		"Options:\n"
		"\n"
		"-w x      sets the width of the preview image to x pixels\n"
		"          (default is 100)\n"
		"\n"
		"-e s      adjusts the preview image's exposure by s f-stops\n"
		"          (default is 0).  Positive values make the image\n"
		"          brighter, negative values make it darker.\n"
		"\n"
		"-v        verbose mode\n"
		"\n"
		"-h        prints this message\n";

	 cerr << endl;
    }

    exit (1);
}


int
main(int argc, char **argv)
{
    const char *inFile = 0;
    const char *outFile = 0;
    int previewWidth = 100;
    float exposure = 0;
    bool verbose = false;

    //
    // Parse the command line.
    //

    if (argc < 2)
	usageMessage (argv[0], true);

    int i = 1;

    while (i < argc)
    {
	if (!strcmp (argv[i], "-w"))
	{
	    //
	    // Set preview image width
	    //

	    if (i > argc - 2)
		usageMessage (argv[0]);

	    previewWidth = strtol (argv[i + 1], 0, 0);
	    i += 2;
	}
	else if (!strcmp (argv[i], "-e"))
	{
	    //
	    // Set preview image width
	    //

	    if (i > argc - 2)
		usageMessage (argv[0]);

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
	else if (!strcmp (argv[i], "-h"))
	{
	    //
	    // Print help message
	    //

	    usageMessage (argv[0], true);
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
	usageMessage (argv[0]);

    if (!strcmp (inFile, outFile))
    {
	cerr << "Input and output cannot be the same file." << endl;
	return 1;
    }

    if (previewWidth <= 0)
    {
	cerr << "Preview image width must be greather than zero." << endl;
	return 1;
    }

    //
    // Load inFile, add a preview image, and save the result in outFile.
    //

    int exitStatus = 0;

    try
    {
	makePreview (inFile, outFile, previewWidth, exposure, verbose);
    }
    catch (const exception &e)
    {
	cerr << e.what() << endl;
	exitStatus = 1;
    }

    return exitStatus;
}
