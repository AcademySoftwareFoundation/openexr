//
//	Copyright  (c)  2004    Industrial   Light   and   Magic.
//	All   rights   reserved.    Used   under   authorization.
//	This material contains the confidential  and  proprietary
//	information   of   Industrial   Light   and   Magic   and
//	may not be copied in whole or in part without the express
//	written   permission   of  Industrial Light  and   Magic.
//	This  copyright  notice  does  not   imply   publication.
//

//-----------------------------------------------------------------------------
//
//	exrmaketiled -- program that produces tiled
//	multiresolution versions of OpenEXR images.
//
//-----------------------------------------------------------------------------

#include <makeTiled.h>

#include <iostream>
#include <exception>
#include <string>
#include <stdlib.h>

using namespace Imf;
using namespace std;


void
usageMessage (const char argv0[], bool verbose = false)
{
    cerr << "usage: " << argv0 << " [options] infile outfile" << endl;

    if (verbose)
    {
	cerr << "\n"
		"Reads an OpenEXR image from infile, produces a tiled\n"
		"version of the image, and saves the result in outfile.\n"
		"\n"
		"Options:\n"
		"\n"
		"-o        produces a ONE_LEVEL image (default)\n"
		"\n"
		"-m        produces a MIPMAP_LEVELS multiresolution image\n"
		"\n"
		"-r        produces a RIPMAP_LEVELS multiresolution image\n"
		"\n"
		"-f c      when a MIPMAP_LEVELS or RIPMAP_LEVELS image\n"
		"          is produced, image channel c will be resampled\n"
		"          without low-pass filtering.  This option can\n"
		"          be specified multiple times to disable low-pass\n"
		"          filtering for mutiple channels.\n"
		"\n"
		"-t x y    sets the tile size in the output image to\n"
		"          x by y pixels (default is 64 by 64)\n"
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
    LevelMode mode = ONE_LEVEL;
    int tileSizeX = 64;
    int tileSizeY = 64;
    set<string> doNotFilter;
    bool verbose = false;

    //
    // Parse the command line.
    //

    if (argc < 2)
	usageMessage (argv[0], true);

    int i = 1;

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
		usageMessage (argv[0]);

	    doNotFilter.insert (argv[i + 1]);
	    i += 2;
	}
	else if (!strcmp (argv[i], "-t"))
	{
	    //
	    // Set tile size
	    //

	    if (i > argc - 3)
		usageMessage (argv[0]);

	    tileSizeX = strtol (argv[i + 1], 0, 0);
	    tileSizeY = strtol (argv[i + 2], 0, 0);
	    i += 3;
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

    //
    // Load inFile, and save a tiled version in outFile.
    //

    int exitStatus = 0;

    try
    {
	makeTiled (inFile, outFile,
		   mode,
		   tileSizeX, tileSizeY,
		   doNotFilter,
		   verbose);
    }
    catch (const exception &e)
    {
	cerr << e.what() << endl;
	exitStatus = 1;
    }

    return exitStatus;
}
