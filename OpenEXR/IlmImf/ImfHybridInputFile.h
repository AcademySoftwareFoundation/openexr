/*
 *  ImfHybridInputFile.h
 *  OpenEXR
 *
 *  Created by Brendan Bolles on 3/19/13.
 *  Copyright 2013 fnord. All rights reserved.
 *
 */

#ifndef INCLUDED_IMF_HYBRID_INPUT_FILE_H
#define INCLUDED_IMF_HYBRID_INPUT_FILE_H

#include "ImfHeader.h"
#include "ImfMultiPartInputFile.h"
#include "ImfFrameBuffer.h"
#include "ImfChannelList.h"
#include "ImathBox.h"


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


class IMF_EXPORT HybridInputFile : public GenericInputFile
{
  public:
	HybridInputFile(const char fileName[],
					int numThreads = globalThreadCount(),
					bool reconstructChunkOffsetTable = true);
					
	HybridInputFile(IStream& is,
					int numThreads = globalThreadCount(),
					bool reconstructChunkOffsetTable = true);

	virtual ~HybridInputFile() {}
	
	
	int parts() const { return _multiPart.parts(); }

	const Header &  header(int n) const { return _multiPart.header(n); }
	
	int			    version () const { return _multiPart.version(); }
	
	bool		isComplete () const;
	
	const ChannelList &		channels () const { return _chanList; }
	
	const IMATH_NAMESPACE::Box2i & dataWindow() const { return _dataWindow; }
	const IMATH_NAMESPACE::Box2i & displayWindow() const { return _displayWindow; }
	
	
	void		setFrameBuffer (const FrameBuffer &frameBuffer) { _frameBuffer = frameBuffer; }
	
	const FrameBuffer &	frameBuffer () const { return _frameBuffer; }
	
    void		readPixels (int scanLine1, int scanLine2);
    void		readPixels (int scanLine) { readPixels(scanLine, scanLine); }
	
  private:
	void setup();

  private:
	MultiPartInputFile _multiPart;
	
	IMATH_NAMESPACE::Box2i _dataWindow;
	IMATH_NAMESPACE::Box2i _displayWindow;
	
	FrameBuffer		_frameBuffer;
	
	typedef struct HybridChannel {
		int part;
		std::string name;
		
		HybridChannel(int p=0, const std::string &n="") : part(p), name(n) {}
	}HybridChannel;
	
	typedef std::map<std::string, HybridChannel> HybridChannelMap;
	
	HybridChannelMap _map;
	
	ChannelList _chanList;
};


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif // INCLUDED_IMF_HYBRID_INPUT_FILE_H