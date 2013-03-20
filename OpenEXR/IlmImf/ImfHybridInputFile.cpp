/*
 *  ImfHybridInputFile.cpp
 *  OpenEXR
 *
 *  Created by Brendan Bolles on 3/19/13.
 *  Copyright 2013 fnord. All rights reserved.
 *
 */

#include "ImfHybridInputFile.h"

#include "ImfInputPart.h"


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


using namespace std;
using IMATH_NAMESPACE::Box2i;


HybridInputFile::HybridInputFile(const char fileName[], int numThreads, bool reconstructChunkOffsetTable) :
	_multiPart(fileName, numThreads, reconstructChunkOffsetTable)
{
	setup();
}


HybridInputFile::HybridInputFile(IStream& is, int numThreads, bool reconstructChunkOffsetTable) :
	_multiPart(is, numThreads, reconstructChunkOffsetTable)
{
	setup();
}


bool
HybridInputFile::isComplete() const
{
	for(int i=0; i < _multiPart.parts(); i++)
	{
		if( !_multiPart.partComplete(i) )
			return false;
	}
	
	return true;
}


void
HybridInputFile::readPixels(int scanLine1, int scanLine2)
{
	for(int n=0; n < _multiPart.parts(); n++)
	{
		const Box2i &dataW = _multiPart.header(n).dataWindow();
		
		FrameBuffer part_fb;
	
		for(FrameBuffer::ConstIterator i = _frameBuffer.begin(); i != _frameBuffer.end(); i++)
		{
			if( _map.find( i.name() ) != _map.end() )
			{
				const HybridChannel &hyChan = _map[ i.name() ];
				
				if(hyChan.part == n)
				{
					part_fb.insert( hyChan.name, i.slice() );
				}
			}
			else if(n == 0)
			{
				// for channels that will be simply be filled
				string name_never_loaded = string("zzNOLOADzz") + i.name();
				
				part_fb.insert( name_never_loaded, i.slice() );
			}
		}
		
		if(part_fb.begin() != part_fb.end()) // i.e. it's not empty
		{
			InputPart inPart(_multiPart, n);
			
			inPart.setFrameBuffer(part_fb);
			
			inPart.readPixels(scanLine1, scanLine2);
		}
	}
}


void
HybridInputFile::setup()
{
	for(int n=0; n < _multiPart.parts(); n++)
	{
		const Header &head = _multiPart.header(n);
		
		// this will make a dataWindow that can hold the dataWindows of every part
		_dataWindow.extendBy( head.dataWindow() );
		
		// all displayWindows should be the same, actually
		_displayWindow.extendBy( head.displayWindow() );
		
		
		const ChannelList &chans = head.channels();

		for(ChannelList::ConstIterator i = chans.begin(); i != chans.end(); ++i)
		{
			const bool rename = (_multiPart.parts() > 1);
			
			const string hybrid_name = (rename ? head.name() + "." + i.name() : i.name());
			
			_map[ hybrid_name ] = HybridChannel(n, i.name());
			
			_chanList.insert(hybrid_name, i.channel());
		}
	}
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT