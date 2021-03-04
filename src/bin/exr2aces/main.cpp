//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	exr2aces -- a program that converts an
//	OpenEXR file to an ACES image file.
//
//-----------------------------------------------------------------------------


#include <ImfAcesFile.h>
#include <ImfArray.h>
#include <ImfRgbaFile.h>
#include <iostream>
#include <exception>
#include <string>
#include <string.h>
#include <stdlib.h>

#include <OpenEXRConfig.h>
using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;
using namespace std;

namespace {

void
usageMessage (const char argv0[], bool verbose = false)
{
    cerr << "usage: " << argv0 << " [options] infile outfile" << endl;

    if (verbose)
    {
	cerr << "\n"
		"Reads an OpenEXR file from infile and saves the contents\n"
		"in ACES image file outfile.\n"
		"\n"
		"The ACES image file format is a subset of the OpenEXR file\n"
		"format.  ACES image files are restricted as follows:\n"
		"\n"
		"* Images are stored as scanlines; tiles are not allowed.\n"
		"\n"
		"* Images contain three color channels, either\n"
		"      R, G, B (red, green, blue) or\n"
		"      Y, RY, BY (luminance, sub-sampled chroma)\n"
		"\n"
		"* Images may optionally contain an alpha channel.\n"
		"\n"
		"* Only three compression types are allowed:\n"
		"      NO_COMPRESSION (file is not compressed)\n"
		"      PIZ_COMPRESSION (lossless)\n"
		"      B44A_COMPRESSION (lossy)\n"
		"* The \"chromaticities\" header attribute must specify\n"
		"  the ACES RGB primaries and white point.\n"
		"\n"
		"Options:\n"
		"\n"
		"-v        verbose mode\n"
		"\n"
		"-h        prints this message\n";

	 cerr << endl;
    }

    exit (1);
}


void
exr2aces (const char inFileName[],
	  const char outFileName[],
	  bool verbose)
{
    Array2D<Rgba> p;
    Header h;
    RgbaChannels ch;
    Box2i dw;
    int width;
    int height;

    {
	if (verbose)
	    cout << "Reading file " << inFileName << endl;

	AcesInputFile in (inFileName);

	h = in.header();
	ch = in.channels();
	dw = h.dataWindow();

	width  = dw.max.x - dw.min.x + 1;
	height = dw.max.y - dw.min.y + 1;
	p.resizeErase (height, width);

	in.setFrameBuffer (ComputeBasePointer (&p[0][0], dw), 1, width);
	in.readPixels (dw.min.y, dw.max.y);
    }

    switch (h.compression())
    {
      case NO_COMPRESSION:
	break;

      case B44_COMPRESSION:
      case B44A_COMPRESSION:
	h.compression() = B44A_COMPRESSION;
	break;

      default:
	h.compression() = PIZ_COMPRESSION;
    }

    {
	if (verbose)
	    cout << "Writing file " << outFileName << endl;

	AcesOutputFile out (outFileName, h, ch);

	out.setFrameBuffer (ComputeBasePointer (&p[0][0], dw), 1, width);
	out.writePixels (height);
    }
}


} // namespace


int
main(int argc, char **argv)
{
    const char *inFile = 0;
    const char *outFile = 0;
    bool verbose = false;

    //
    // Parse the command line.
    //

    if (argc < 2)
	usageMessage (argv[0], true);

    int i = 1;

    while (i < argc)
    {
	if (!strcmp (argv[i], "-v"))
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
	exr2aces (inFile, outFile, verbose);
    }
    catch (const exception &e)
    {
	cerr << e.what() << endl;
	exitStatus = 1;
    }

    return exitStatus;
}
