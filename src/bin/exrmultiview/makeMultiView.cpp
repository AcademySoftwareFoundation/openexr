//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//----------------------------------------------------------------------------
//
//	Combine multiple single-view images
//	into one multi-view image.
//
//----------------------------------------------------------------------------

#include "makeMultiView.h"
#include "Image.h"
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfStandardAttributes.h>
#include <ImfMultiView.h>
#include "Iex.h"
#include <map>
#include <algorithm>
#include <iostream>


#include "namespaceAlias.h"
using namespace IMF;
using namespace IMATH_NAMESPACE;
using namespace std;


void
makeMultiView (const vector <string> &viewNames,
	       const vector <const char *> &inFileNames,
	       const char *outFileName,
	       Compression compression,
	       bool verbose)
{
    Header header;
    Image image;
    FrameBuffer outFb;

    //
    // Find the size of the dataWindow, check files
    //
    
    Box2i d;
    
    
    for (size_t i = 0; i < viewNames.size(); ++i)
    {
	InputFile in (inFileNames[i]);

	if (verbose)
	{
	    cout << "reading file " << inFileNames[i] << " "
		    "for " << viewNames[i] << " view" << endl;
	}

	if (hasMultiView (in.header()))
	{
	    THROW (IEX_NAMESPACE::NoImplExc,
		   "The image in file " << inFileNames[i] << " is already a "
		   "multi-view image.  Cannot combine multiple multi-view "
		   "images.");
	}

        header = in.header();
	if (i == 0)
        {
             d=header.dataWindow();
	}else{
             d.extendBy(header.dataWindow());
        }
    }
    
    
    image.resize (d);
    
    header.dataWindow()=d;
    
    // blow away channels; we'll rebuild them
    header.channels()=ChannelList();
    
    
    //
    // Read the input image files
    //

    for (size_t i = 0; i < viewNames.size(); ++i)
    {
	InputFile in (inFileNames[i]);

	if (verbose)
	{
	    cout << "reading file " << inFileNames[i] << " "
		    "for " << viewNames[i] << " view" << endl;
	}

	FrameBuffer inFb;

	for (ChannelList::ConstIterator j = in.header().channels().begin();
	     j != in.header().channels().end();
	     ++j)
	{
	    const Channel &inChannel = j.channel();
	    string inChanName = j.name();
	    string outChanName = insertViewName (inChanName, viewNames, i);

	    image.addChannel (outChanName, inChannel);
            image.channel(outChanName).black();
            
	    header.channels().insert (outChanName, inChannel);

	    inFb.insert  (inChanName,  image.channel(outChanName).slice());
	    outFb.insert (outChanName, image.channel(outChanName).slice());
	}

	in.setFrameBuffer (inFb);
	in.readPixels (in.header().dataWindow().min.y, in.header().dataWindow().max.y);
    }

    //
    // Write the output image file
    //

    {
	header.compression() = compression;
	addMultiView (header, viewNames);

	OutputFile out (outFileName, header);

	if (verbose)
	    cout << "writing file " << outFileName << endl;

	out.setFrameBuffer (outFb);

	out.writePixels
	    (header.dataWindow().max.y - header.dataWindow().min.y + 1);
    }
}
