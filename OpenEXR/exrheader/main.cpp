///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2003, Industrial Light & Magic, a division of Lucas
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


//-----------------------------------------------------------------------------
//
//	Utility program to print an image file's header
//
//-----------------------------------------------------------------------------

#include <ImfInputFile.h>
#include <ImfBoxAttribute.h>
#include <ImfChannelListAttribute.h>
#include <ImfCompressionAttribute.h>
#include <ImfFloatAttribute.h>
#include <ImfIntAttribute.h>
#include <ImfLineOrderAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfStringAttribute.h>
#include <ImfVecAttribute.h>
#include <iostream>

using namespace Imf;
using std::cout;
using std::endl;


void
printCompression (Compression c)
{
    switch (c)
    {
      case NO_COMPRESSION:
	cout << "none";
	break;

      case RLE_COMPRESSION:
	cout << "run-length encoding";
	break;

      case ZIPS_COMPRESSION:
	cout << "zip, individual scanlines";
	break;

      case ZIP_COMPRESSION:
	cout << "zip, multi-scanline blocks";
	break;

      case PIZ_COMPRESSION:
	cout << "piz";
	break;

      default:
	cout << int (c);
	break;
    }
}


void
printLineOrder (LineOrder lo)
{
    switch (lo)
    {
      case INCREASING_Y:
	cout << "increasing y";
	break;

      case DECREASING_Y:
	cout << "decreasing y";
	break;

      default:
	cout << int (lo);
	break;
    };
}


void
printPixelType (PixelType pt)
{
    switch (pt)
    {
      case UINT:
	cout << "32-bit unsigned integer";
	break;

      case HALF:
	cout << "16-bit floating-point";
	break;

      case FLOAT:
	cout << "32-bit floating-point";
	break;

      default:
	cout << "type " << int (pt);
	break;
    }
}


void
printChannelList (const ChannelList &cl)
{
    for (ChannelList::ConstIterator i = cl.begin(); i != cl.end(); ++i)
    {
	cout << "\n    " << i.name() << ", ";

	printPixelType (i.channel().type);

	cout << ", sampling " <<
		i.channel().xSampling << " " <<
		i.channel().ySampling;
    }
}


void
printInfo (const char fileName[])
{
    InputFile in (fileName);
    const Header &h = in.header();

    cout << "\n" << fileName << ":\n\n";

    cout << "file format version: " << in.version() << "\n";

    for (Header::ConstIterator i = h.begin(); i != h.end(); ++i)
    {
	const Attribute *a = &i.attribute();
	cout << i.name() << " (type " << a->typeName() << ")";

	if (const Box2iAttribute *ta =
		dynamic_cast <const Box2iAttribute *> (a))
	{
	    cout << ": " << ta->value().min << " - " << ta->value().max;
	}
	else if (const Box2fAttribute *ta =
		dynamic_cast <const Box2fAttribute *> (a))
	{
	    cout << ": " << ta->value().min << " - " << ta->value().max;
	}
	else if (const ChannelListAttribute *ta =
		dynamic_cast <const ChannelListAttribute *> (a))
	{
	    cout << ":";
	    printChannelList (ta->value());
	}
	else if (const CompressionAttribute *ta =
		dynamic_cast <const CompressionAttribute *> (a))
	{
	    cout << ": ";
	    printCompression (ta->value());
	}
	else if (const FloatAttribute *ta =
		dynamic_cast <const FloatAttribute *> (a))
	{
	    cout << ": " << ta->value();
	}
	else if (const IntAttribute *ta =
		dynamic_cast <const IntAttribute *> (a))
	{
	    cout << ": " << ta->value();
	}
	else if (const LineOrderAttribute *ta =
		dynamic_cast <const LineOrderAttribute *> (a))
	{
	    cout << ": ";
	    printLineOrder (ta->value());
	}
	else if (const M33fAttribute *ta =
		dynamic_cast <const M33fAttribute *> (a))
	{
	    cout << ":\n"
		    "   (" <<
		    ta->value()[0][0] << " " <<
		    ta->value()[0][1] << " " <<
		    ta->value()[0][2] << "\n    " <<
		    ta->value()[1][0] << " " <<
		    ta->value()[1][1] << " " <<
		    ta->value()[1][2] << "\n    " <<
		    ta->value()[2][0] << " " <<
		    ta->value()[2][1] << " " <<
		    ta->value()[2][2] << ")";
	}
	else if (const M44fAttribute *ta =
		dynamic_cast <const M44fAttribute *> (a))
	{
	    cout << ":\n"
		    "   (" <<
		    ta->value()[0][0] << " " <<
		    ta->value()[0][1] << " " <<
		    ta->value()[0][2] << " " <<
		    ta->value()[0][3] << "\n    " <<
		    ta->value()[1][0] << " " <<
		    ta->value()[1][1] << " " <<
		    ta->value()[1][2] << " " <<
		    ta->value()[1][3] << "\n    " <<
		    ta->value()[2][0] << " " <<
		    ta->value()[2][1] << " " <<
		    ta->value()[2][2] << " " <<
		    ta->value()[2][3] << "\n    " <<
		    ta->value()[3][0] << " " <<
		    ta->value()[3][1] << " " <<
		    ta->value()[3][2] << " " <<
		    ta->value()[3][3] << ")";
	}
	else if (const StringAttribute *ta =
		dynamic_cast <const StringAttribute *> (a))
	{
	    cout << ": \"" << ta->value() << "\"";
	}
	else if (const V2iAttribute *ta =
		dynamic_cast <const V2iAttribute *> (a))
	{
	    cout << ": " << ta->value();
	}
	else if (const V2fAttribute *ta =
		dynamic_cast <const V2fAttribute *> (a))
	{
	    cout << ": " << ta->value();
	}
	else if (const V3iAttribute *ta =
		dynamic_cast <const V3iAttribute *> (a))
	{
	    cout << ": " << ta->value();
	}
	else if (const V3fAttribute *ta =
		dynamic_cast <const V3fAttribute *> (a))
	{
	    cout << ": " << ta->value();
	}

	cout << '\n';
    }

    cout << endl;
}


int
main(int argc, char **argv)
{
    if (argc < 2)
    {
	std::cerr << "usage: " << argv[0] << " imagefile\n";
	return 1;
    }
    else
    {
	try
	{
	    printInfo (argv[1]);
	    return 0;
	}
	catch (const std::exception &e)
	{
	    std::cerr << e.what() << std::endl;
	    return 1;
	}
    }
}

