///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004, Industrial Light & Magic, a division of Lucas
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
//	class OutputFile
//
//-----------------------------------------------------------------------------

#include <ImfOutputFile.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfMisc.h>
#include <ImfStdIO.h>
#include <ImfCompressor.h>
#include <ImathBox.h>
#include <ImathFun.h>
#include <ImfArray.h>
#include <ImfXdr.h>
#include <ImfPreviewImageAttribute.h>
#include <Iex.h>
#include <string>
#include <vector>
#include <fstream>
#include <assert.h>


namespace Imf {

using Imath::Box2i;
using Imath::divp;
using Imath::modp;
using std::string;
using std::vector;
using std::ofstream;


namespace {


struct OutSliceInfo
{
    PixelType		type;
    const char *	base;
    size_t		xStride;
    size_t		yStride;
    int			xSampling;
    int			ySampling;
    bool		zero;

    OutSliceInfo (PixelType type = HALF,
	          const char *base = 0,
	          size_t xStride = 0,
	          size_t yStride = 0,
	          int xSampling = 1,
	          int ySampling = 1,
	          bool zero = false);
};


OutSliceInfo::OutSliceInfo (PixelType t,
		            const char *b,
		            size_t xs, size_t ys,
		            int xsm, int ysm,
		            bool z)
:
    type (t),
    base (b),
    xStride (xs),
    yStride (ys),
    xSampling (xsm),
    ySampling (ysm),
    zero (z)
{
    // empty
}


} // namespace


struct OutputFile::Data
{
    Header		 header;
    int			 version;
    Int64		 previewPosition;
    FrameBuffer		 frameBuffer;
    int			 currentScanLine;
    int			 missingScanLines;
    LineOrder		 lineOrder;
    int			 minX;
    int			 maxX;
    int			 minY;
    int			 maxY;
    vector<Int64>	 lineOffsets;
    int			 linesInBuffer;
    size_t		 lineBufferSize;
    int			 lineBufferMinY;
    int			 lineBufferMaxY;
    Array<char>		 lineBuffer;
    char *		 endOfLineBufferData;
    vector<size_t>	 bytesPerLine;
    vector<size_t>	 offsetInLineBuffer;
    Compressor *	 compressor;
    Compressor::Format	 format;
    vector<OutSliceInfo> slices;
    OStream *		 os;
    bool		 deleteStream;
    Int64		 lineOffsetsPosition;
    Int64		 currentPosition;

     Data (bool del);
    ~Data ();
};


OutputFile::Data::Data (bool del):
    os (0),
    compressor (0),
    deleteStream (del),
    lineOffsetsPosition (0)
{
    // empty
}


OutputFile::Data::~Data ()
{
    if (deleteStream)
	delete os;

    delete compressor;
}


namespace {


Int64
writeLineOffsets (OStream &os, const vector<Int64> &lineOffsets)
{
    Int64 pos = os.tellp();

    if (pos == -1)
	Iex::throwErrnoExc ("Cannot determine current file position (%T).");

    for (unsigned int i = 0; i < lineOffsets.size(); i++)
	Xdr::write <StreamIO> (os, lineOffsets[i]);

    return pos;
}


void
writePixelData (OutputFile::Data *ofd,
		const char pixelData[],
		int pixelDataSize)
{
    //
    // Store a block of pixel data in the output file, and try
    // to keep track of the current writing position the file,
    // without calling tellp() (tellp() can be fairly expensive).
    //

    Int64 currentPosition = ofd->currentPosition;
    ofd->currentPosition = 0;

    if (currentPosition == 0)
	currentPosition = ofd->os->tellp();

    ofd->lineOffsets[(ofd->currentScanLine - ofd->minY) / ofd->linesInBuffer] =
	currentPosition;

    #ifdef DEBUG

	assert (ofd->os->tellp() == currentPosition);

    #endif

    Xdr::write <StreamIO> (*ofd->os, ofd->lineBufferMinY);
    Xdr::write <StreamIO> (*ofd->os, pixelDataSize);
    ofd->os->write (pixelData, pixelDataSize);

    ofd->currentPosition = currentPosition +
			   Xdr::size<int>() +
			   Xdr::size<int>() +
			   pixelDataSize;
}


void
convertToXdr (OutputFile::Data *ofd, int inSize)
{
    //
    // Convert the contents of an OutputFile's lineBuffer from the machine's
    // native representation to Xdr format.  This function is called by
    // writePixels(), below, if the compressor wanted its input pixel data
    // in the machine's native format, but then failed to compress the data
    // (most compressors will expand rather than compress random input data).
    //
    // Note that this routine assumes that the machine's native representation
    // of the pixel data has the same size as the Xdr representation.  This
    // makes it possible to convert the pixel data in place, without an
    // intermediate temporary buffer.
    //
   
    int startY, endY;		// The first and last scanlines in
    				// the file that are in the lineBuffer.
    int dy;
    
    if (ofd->lineOrder == INCREASING_Y)
    {
	startY = std::max (ofd->lineBufferMinY, ofd->minY);
	endY = std::min (ofd->lineBufferMaxY, ofd->maxY) + 1;
        dy = 1;
    }
    else
    {
	startY = std::min (ofd->lineBufferMaxY, ofd->maxY);
	endY = std::max (ofd->lineBufferMinY, ofd->minY) - 1;
        dy = -1;
    }

    //
    // Iterate over all scanlines in the lineBuffer to convert.
    //

    for (int y = startY; y != endY; y += dy)
    {
	//
        // Set these to point to the start of line y.
        // We will write to writePtr from pixelPtr.
	//
	
        char *writePtr = ofd->lineBuffer +
		         ofd->offsetInLineBuffer[y - ofd->minY];

        char *pixelPtr = writePtr;
        
	//
        // Iterate over all slices in the file.
	//
	
        for (unsigned int i = 0; i < ofd->slices.size(); ++i)
        {
            //
            // Test if scan line y of this channel is
            // contains any data (the scan line contains
            // data only if y % ySampling == 0).
            //

            const OutSliceInfo &slice = ofd->slices[i];

            if (modp (y, slice.ySampling) != 0)
                continue;

            //
            // Find the number of sampled pixels, dMaxX-dMinX+1, for
	    // slice i in scan line y (i.e. pixels within the data window
            // for which x % xSampling == 0).
            //

            int dMinX = divp (ofd->minX, slice.xSampling);
            int dMaxX = divp (ofd->maxX, slice.xSampling);
            
	    //
            // Convert the samples in place.
	    //

            switch (slice.type)
            {
              case UINT:

                while (dMinX <= dMaxX)
                {
                    Xdr::write <CharPtrIO>
                        (writePtr, *(const unsigned int *) pixelPtr);
                    pixelPtr += sizeof(unsigned int);

                    dMinX += 1;
                }
                break;

              case HALF:

                while (dMinX <= dMaxX)
                {                
                    Xdr::write <CharPtrIO>
                        (writePtr, *(const half *) pixelPtr);
                    pixelPtr += sizeof(half);

                    dMinX += 1;
                }
                break;

              case FLOAT:

                while (dMinX <= dMaxX)
                {
                    Xdr::write <CharPtrIO>
                        (writePtr, *(const float *) pixelPtr);
                    pixelPtr += sizeof(float);

                    dMinX += 1;
                }
                break;

              default:

                throw Iex::ArgExc ("Unknown pixel data type.");
            }           
        }

	#ifdef DEBUG

	    assert (writePtr == pixelPtr);

	#endif
    }
}

} // namespace


OutputFile::OutputFile (const char fileName[], const Header &header):
    _data (new Data (true))
{
    try
    {
	header.sanityCheck();
	_data->os = new StdOFStream (fileName);
	initialize (header);
    }
    catch (Iex::BaseExc &e)
    {
	delete _data;

	REPLACE_EXC (e, "Cannot open image file "
			"\"" << fileName << "\". " << e);
	throw;
    }
}


OutputFile::OutputFile (OStream &os, const Header &header):
    _data (new Data (false))
{
    try
    {
	header.sanityCheck();
	_data->os = &os;
	initialize (header);
    }
    catch (Iex::BaseExc &e)
    {
	delete _data;

	REPLACE_EXC (e, "Cannot open image file "
			"\"" << os.fileName() << "\". " << e);
	throw;
    }
}


void
OutputFile::initialize (const Header &header)
{
    _data->header = header;

    const Box2i &dataWindow = header.dataWindow();

    _data->currentScanLine = (header.lineOrder() == INCREASING_Y)?
				 dataWindow.min.y: dataWindow.max.y;

    _data->missingScanLines = dataWindow.max.y - dataWindow.min.y + 1;
    _data->lineOrder = header.lineOrder();
    _data->minX = dataWindow.min.x;
    _data->maxX = dataWindow.max.x;
    _data->minY = dataWindow.min.y;
    _data->maxY = dataWindow.max.y;

    size_t maxBytesPerLine = bytesPerLineTable (_data->header,
						_data->bytesPerLine);

    _data->compressor = newCompressor (_data->header.compression(),
				       maxBytesPerLine,
				       _data->header);

    _data->linesInBuffer = _data->compressor?
			       _data->compressor->numScanLines(): 1;

    _data->format = _data->compressor?
			_data->compressor->format(): Compressor::XDR;

    _data->lineBufferSize = maxBytesPerLine * _data->linesInBuffer;
    _data->lineBuffer.resizeErase (_data->lineBufferSize);
    _data->endOfLineBufferData = _data->lineBuffer;

    _data->lineBufferMinY = lineBufferMinY (_data->currentScanLine,
					    _data->minY,
					    _data->linesInBuffer);

    _data->lineBufferMaxY = lineBufferMaxY (_data->currentScanLine,
					    _data->minY,
					    _data->linesInBuffer);

    int lineOffsetSize = (dataWindow.max.y - dataWindow.min.y +
			  _data->linesInBuffer) / _data->linesInBuffer;

    _data->lineOffsets.resize (lineOffsetSize);

    offsetInLineBufferTable (_data->bytesPerLine,
			     _data->linesInBuffer,
			     _data->offsetInLineBuffer);

    _data->previewPosition =
	_data->header.writeTo (*_data->os);

    _data->lineOffsetsPosition =
	writeLineOffsets (*_data->os, _data->lineOffsets);

    _data->currentPosition = _data->os->tellp();
}


OutputFile::~OutputFile ()
{
    if (_data)
    {
	if (_data->lineOffsetsPosition > 0)
	{
	    try
	    {
		_data->os->seekp (_data->lineOffsetsPosition);
		writeLineOffsets (*_data->os, _data->lineOffsets);
	    }
	    catch (...)
	    {
		//
		// We cannot safely throw any exceptions from here.
		// This destructor may have been called because the
		// stack is currently being unwound for another
		// exception.
		//
	    }
	}

	delete _data;
    }
}


const char *
OutputFile::fileName () const
{
    return _data->os->fileName();
}


const Header &
OutputFile::header () const
{
    return _data->header;
}


void	
OutputFile::setFrameBuffer (const FrameBuffer &frameBuffer)
{
    //
    // Check if the new frame buffer descriptor
    // is compatible with the image file header.
    //

    const ChannelList &channels = _data->header.channels();

    for (ChannelList::ConstIterator i = channels.begin();
	 i != channels.end();
	 ++i)
    {
	FrameBuffer::ConstIterator j = frameBuffer.find (i.name());

	if (j == frameBuffer.end())
	    continue;

	if (i.channel().type != j.slice().type)
	{
	    THROW (Iex::ArgExc, "Pixel type of \"" << i.name() << "\" channel "
			        "of output file \"" << fileName() << "\" is "
			        "not compatible with the frame buffer's "
			        "pixel type.");
	}

	if (i.channel().xSampling != j.slice().xSampling ||
	    i.channel().ySampling != j.slice().ySampling)
	{
	    THROW (Iex::ArgExc, "X and/or y subsampling factors "
				"of \"" << i.name() << "\" channel "
				"of output file \"" << fileName() << "\" are "
				"not compatible with the frame buffer's "
				"subsampling factors.");
	}
    }
    
    //
    // Initialize slice table for writePixels().
    //

    vector<OutSliceInfo> slices;

    for (ChannelList::ConstIterator i = channels.begin();
	 i != channels.end();
	 ++i)
    {
	FrameBuffer::ConstIterator j = frameBuffer.find (i.name());

	if (j == frameBuffer.end())
	{
	    //
	    // Channel i is not present in the frame buffer.
	    // In the file, channel i will contain only zeroes.
	    //

	    slices.push_back (OutSliceInfo (i.channel().type,
					    0, // base
					    0, // xStride,
					    0, // yStride,
					    i.channel().xSampling,
					    i.channel().ySampling,
					    true)); // zero
	}
	else
	{
	    //
	    // Channel i is present in the frame buffer.
	    //

	    slices.push_back (OutSliceInfo (j.slice().type,
					    j.slice().base,
					    j.slice().xStride,
					    j.slice().yStride,
					    j.slice().xSampling,
					    j.slice().ySampling,
					    false)); // zero
	}
    }

    //
    // Store the new frame buffer.
    //

    _data->frameBuffer = frameBuffer;
    _data->slices = slices;
}


const FrameBuffer &
OutputFile::frameBuffer () const
{
    return _data->frameBuffer;
}


void	
OutputFile::writePixels (int numScanLines)
{
    try
    {
	if (_data->slices.size() == 0)
	    throw Iex::ArgExc ("No frame buffer specified "
			       "as pixel data source.");

	while (numScanLines)
	{
	    if (_data->missingScanLines <= 0)
	    {
		throw Iex::ArgExc ("Tried to write more scan lines "
				   "than specified by the data window.");
	    }

	    #ifdef DEBUG

		assert (_data->currentScanLine >= _data->lineBufferMinY &&
			_data->currentScanLine <= _data->lineBufferMaxY);

	    #endif

	    //
	    // Convert one scan line's worth of pixel data to
	    // a machine-independent representation, and store
	    // the result in _data->lineBuffer.
	    //

	    int y = _data->currentScanLine;

	    char *writePtr = _data->lineBuffer +
			     _data->offsetInLineBuffer[y - _data->minY];

	    //
	    // Iterate over all image channels.
	    //

	    for (unsigned int i = 0; i < _data->slices.size(); ++i)
	    {
		//
		// Test if scan line y of this channel is
		// contains any data (the scan line contains
		// data only if y % ySampling == 0).
		//

		const OutSliceInfo &slice = _data->slices[i];

		if (modp (y, slice.ySampling) != 0)
		    continue;

		//
		// Find the x coordinates of the leftmost and rightmost
		// sampled pixels (i.e. pixels within the data window
		// for which x % xSampling == 0).
		//

		int dMinX = divp (_data->minX, slice.xSampling);
		int dMaxX = divp (_data->maxX, slice.xSampling);

		//
		// Iterate over the sampled pixels.
		//

		if (slice.zero)
		{
		    //
		    // The frame buffer contains no data for this channel.
		    // Store zeroes in _data->lineBuffer.
		    //

		    if (_data->format == Compressor::XDR)
		    {
			//
			// The compressor expects data in Xdr format.
			//

			switch (slice.type)
			{
			  case UINT:

			    while (dMinX <= dMaxX)
			    {
				Xdr::write <CharPtrIO>
				    (writePtr, (unsigned int) 0);
				dMinX += 1;
			    }
			    break;

			  case HALF:

			    while (dMinX <= dMaxX)
			    {
				Xdr::write <CharPtrIO>
				    (writePtr, (half) 0);
				dMinX += 1;
			    }
			    break;

			  case FLOAT:

			    while (dMinX <= dMaxX)
			    {
				Xdr::write <CharPtrIO>
				    (writePtr, (float) 0);
				dMinX += 1;
			    }
			    break;
			    
			  default:

			    throw Iex::ArgExc ("Unknown pixel data type.");
			}
		    }
		    else
		    {
			//
			// The compressor expects data in
			// the machines native format.
			//

			switch (slice.type)
			{
			  case UINT:

			    while (dMinX <= dMaxX)
			    {
				static unsigned int ui = 0;

				for (size_t i = 0; i < sizeof (ui); ++i)
				    *writePtr++ = ((char *) &ui)[i];

				dMinX += 1;
			    }
			    break;

			  case HALF:

			    while (dMinX <= dMaxX)
			    {
				*(half *) writePtr = half (0);
				writePtr += sizeof (half);
				dMinX += 1;
			    }
			    break;

			  case FLOAT:

			    while (dMinX <= dMaxX)
			    {
				static float f = 0;

				for (size_t i = 0; i < sizeof (f); ++i)
				    *writePtr++ = ((char *) &f)[i];

				dMinX += 1;
			    }
			    break;
			    
			  default:

			    throw Iex::ArgExc ("Unknown pixel data type.");
			}
		    }
		}
		else
		{
		    //
		    // If necessary, convert the pixel data to
		    // a machine-independent representation.
		    // Then store the pixel data in _data->lineBuffer.
		    //

		    const char *linePtr  = slice.base +
					   divp (y, slice.ySampling) *
					   slice.yStride;

		    const char *pixelPtr = linePtr + dMinX * slice.xStride;
		    const char *endPtr   = linePtr + dMaxX * slice.xStride;

		    if (_data->format == Compressor::XDR)
		    {
			//
			// The compressor expects data in Xdr format
			//

			switch (slice.type)
			{
			  case UINT:

			    while (pixelPtr <= endPtr)
			    {
				Xdr::write <CharPtrIO>
				   (writePtr, *(const unsigned int *) pixelPtr);
				pixelPtr += slice.xStride;
			    }
			    break;

			  case HALF:

			    while (pixelPtr <= endPtr)
			    {
				Xdr::write <CharPtrIO>
				    (writePtr, *(const half *) pixelPtr);
				pixelPtr += slice.xStride;
			    }
			    break;

			  case FLOAT:

			    while (pixelPtr <= endPtr)
			    {
				Xdr::write <CharPtrIO>
				    (writePtr, *(const float *) pixelPtr);
				pixelPtr += slice.xStride;
			    }
			    break;
			    
			  default:

			    throw Iex::ArgExc ("Unknown pixel data type.");
			}
		    }
		    else
		    {
			//
			// The compressor expects data in the
			// machine's native format.
			//

			switch (slice.type)
			{
			  case UINT:

			    while (pixelPtr <= endPtr)
			    {
				for (size_t i = 0;
				     i < sizeof (unsigned int);
				     ++i)
				{
				    *writePtr++ = pixelPtr[i];
				}

				pixelPtr += slice.xStride;
			    }
			    break;

			  case HALF:

			    while (pixelPtr <= endPtr)
			    {
				*(half *) writePtr = *(const half *) pixelPtr;
				writePtr += sizeof (half);
				pixelPtr += slice.xStride;
			    }
			    break;

			  case FLOAT:

			    while (pixelPtr <= endPtr)
			    {
				for (size_t i = 0; i < sizeof (float); ++i)
				    *writePtr++ = pixelPtr[i];

				pixelPtr += slice.xStride;
			    }
			    break;
			    
			  default:

			    throw Iex::ArgExc ("Unknown pixel data type.");
			}
		    }
		}
	    }

	    if (_data->endOfLineBufferData < writePtr)
		_data->endOfLineBufferData = writePtr;

	    #ifdef DEBUG

		assert (writePtr - (_data->lineBuffer +
			_data->offsetInLineBuffer[y - _data->minY]) ==
			(int) _data->bytesPerLine[y - _data->minY]);

	    #endif

	    //
	    // If _data->lineBuffer is full, or if the current scan
	    // line is the last one, then compress the contents of
	    // _data->lineBuffer, and store the compressed data in
	    // the output file.
	    //

	    int nextScanLine = _data->currentScanLine +
			       ((_data->lineOrder == INCREASING_Y)? 1: -1);

	    if (nextScanLine < _data->lineBufferMinY ||
		nextScanLine > _data->lineBufferMaxY ||
		_data->missingScanLines <= 1)
	    {
		int dataSize = _data->endOfLineBufferData - _data->lineBuffer;
		const char *dataPtr = _data->lineBuffer;

		if (_data->compressor)
		{
		    const char *compPtr;

		    int compSize = _data->compressor->compress
					(dataPtr, dataSize,
					 _data->lineBufferMinY,
					 compPtr);

		    if (compSize < dataSize)
		    {
			dataSize = compSize;
			dataPtr = compPtr;
		    }
		    else if (_data->format == Compressor::NATIVE)
		    {
                        //
                        // The data did not shrink during compression, but
                        // we cannot write to the file using the machine's
			// native format, so we need to convert the lineBuffer
			// to Xdr.
                        //

                        convertToXdr(_data, dataSize);
		    }
		}

		writePixelData (_data, dataPtr, dataSize);

		//
		// Clear _data->lineBuffer.
		//

		_data->endOfLineBufferData = _data->lineBuffer;

		_data->lineBufferMinY = lineBufferMinY (nextScanLine,
						        _data->minY,
							_data->linesInBuffer);

		_data->lineBufferMaxY = lineBufferMaxY (nextScanLine,
						        _data->minY,
							_data->linesInBuffer);
	    }

	    //
	    // Advance to the next scan line.
	    //

	    numScanLines -= 1;
	    _data->currentScanLine = nextScanLine;
	    _data->missingScanLines -= 1;
	}
    }
    catch (Iex::BaseExc &e)
    {
	REPLACE_EXC (e, "Failed to write pixel data to image "
		        "file \"" << fileName() << "\". " << e);
	throw;
    }
}


int	
OutputFile::currentScanLine () const
{
    return _data->currentScanLine;
}


void	
OutputFile::copyPixels (InputFile &in)
{
    //
    // Check if this file's and and the InputFile's
    // headers are compatible.
    //

    const Header &hdr = header();
    const Header &inHdr = in.header();

    if (inHdr.find("tiles") != inHdr.end())
    {
	THROW (Iex::ArgExc, "Cannot copy pixels from image "
			    "file \"" << in.fileName() << "\" to image "
			    "file \"" << fileName() << "\". The input file is "
			    "tiled, but the output file is not. Try using "
			    "TiledOutputFile::copyPixels instead.");
    }

    if (!(hdr.dataWindow() == inHdr.dataWindow()))
    {
	THROW (Iex::ArgExc, "Cannot copy pixels from image "
			    "file \"" << in.fileName() << "\" to image "
			    "file \"" << fileName() << "\". The files "
			    "have different data windows.");
    }

    if (!(hdr.lineOrder() == inHdr.lineOrder()))
    {
	THROW (Iex::ArgExc, "Quick pixel copy from image "
			    "file \"" << in.fileName() << "\" to image "
			    "file \"" << fileName() << "\" failed. "
			    "The files have different line orders.");
    }

    if (!(hdr.compression() == inHdr.compression()))
    {
	THROW (Iex::ArgExc, "Quick pixel copy from image "
			    "file \"" << in.fileName() << "\" to image "
			    "file \"" << fileName() << "\" failed. "
			    "The files use different compression methods.");
    }

    if (!(hdr.channels() == inHdr.channels()))
    {
	THROW (Iex::ArgExc, "Quick pixel copy from image "
			    "file \"" << in.fileName() << "\" to image "
			    "file \"" << fileName() << "\" failed.  "
			    "The files have different channel lists.");
    }

    //
    // Verify that no pixel data have been written to this file yet.
    //

    const Box2i &dataWindow = hdr.dataWindow();

    if (_data->missingScanLines != dataWindow.max.y - dataWindow.min.y + 1)
    {
	THROW (Iex::LogicExc, "Quick pixel copy from image "
			      "file \"" << in.fileName() << "\" to image "
			      "file \"" << fileName() << "\" failed. "
			      "\"" << fileName() << "\" already contains "
			      "pixel data.");
    }

    //
    // Copy the pixel data.
    //

    while (_data->missingScanLines > 0)
    {
	const char *pixelData;
	int pixelDataSize;

	in.rawPixelData (_data->currentScanLine, pixelData, pixelDataSize);
	writePixelData (_data, pixelData, pixelDataSize);

	_data->currentScanLine += (_data->lineOrder == INCREASING_Y)?
				   _data->linesInBuffer: -_data->linesInBuffer;

	_data->lineBufferMinY = lineBufferMinY (_data->currentScanLine,
						_data->minY,
						_data->linesInBuffer);

	_data->missingScanLines -= _data->linesInBuffer;
    }
}


void
OutputFile::updatePreviewImage (const PreviewRgba newPixels[])
{
    if (_data->previewPosition <= 0)
    {
	THROW (Iex::LogicExc, "Cannot update preview image pixels. "
			      "File \"" << fileName() << "\" does not "
			      "contain a preview image.");
    }

    //
    // Store the new pixels in the header's preview image attribute.
    //

    PreviewImageAttribute &pia =
	_data->header.typedAttribute <PreviewImageAttribute> ("preview");

    PreviewImage &pi = pia.value();
    PreviewRgba *pixels = pi.pixels();
    int numPixels = pi.width() * pi.height();

    for (int i = 0; i < numPixels; ++i)
	pixels[i] = newPixels[i];

    //
    // Save the current file position, jump to the position in
    // the file where the preview image starts, store the new
    // preview image, and jump back to the saved file position.
    //

    Int64 savedPosition = _data->os->tellp();

    try
    {
	_data->os->seekp (_data->previewPosition);
	pia.writeValueTo (*_data->os, _data->version);
	_data->os->seekp (savedPosition);
    }
    catch (Iex::BaseExc &e)
    {
	REPLACE_EXC (e, "Cannot update preview image pixels for "
			"file \"" << fileName() << "\". " << e);
	throw;
    }
}


} // namespace Imf
