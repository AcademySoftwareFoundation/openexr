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
//	class ScanLineInputFile
//
//-----------------------------------------------------------------------------

#include <ImfScanLineInputFile.h>
#include <ImfChannelList.h>
#include <ImfMisc.h>
#include <ImfStdIO.h>
#include <ImfCompressor.h>
#include <ImathBox.h>
#include <ImathFun.h>
#include <ImfXdr.h>
#include <ImfArray.h>
#include <ImfConvert.h>
#include <Iex.h>
#include <string>
#include <vector>
#include <assert.h>


namespace Imf {

using Imath::Box2i;
using Imath::divp;
using Imath::modp;
using std::string;
using std::vector;
using std::ifstream;


namespace {


struct InSliceInfo
{
    PixelType		typeInFrameBuffer;
    PixelType		typeInFile;
    char *		base;
    size_t		xStride;
    size_t		yStride;
    int			xSampling;
    int			ySampling;
    bool		fill;
    bool		skip;
    double		fillValue;

    InSliceInfo (PixelType typeInFrameBuffer = HALF,
		 PixelType typeInFile = HALF,
	         char *base = 0,
	         size_t xStride = 0,
	         size_t yStride = 0,
	         int xSampling = 1,
	         int ySampling = 1,
	         bool fill = false,
	         bool skip = false,
	         double fillValue = 0.0);
};


InSliceInfo::InSliceInfo (PixelType tifb,
			  PixelType tifl,
			  char *b,
			  size_t xs, size_t ys,
			  int xsm, int ysm,
			  bool f, bool s,
			  double fv)
:
    typeInFrameBuffer (tifb),
    typeInFile (tifl),
    base (b),
    xStride (xs),
    yStride (ys),
    xSampling (xsm),
    ySampling (ysm),
    fill (f),
    skip (s),
    fillValue (fv)
{
    // empty
}


} // namespace


struct ScanLineInputFile::Data
{
    Header		header;
    int			version;
    FrameBuffer		frameBuffer;
    LineOrder		lineOrder;
    int			minX;
    int			maxX;
    int			minY;
    int			maxY;
    vector<Int64>	lineOffsets;
    int			linesInBuffer;
    int			lineBufferMinY;
    int			lineBufferMaxY;
    int			nextLineBufferMinY;
    size_t		lineBufferSize;
    Array<char>		lineBuffer;
    const char *	uncompressedData;
    vector<size_t>	bytesPerLine;
    vector<size_t>	offsetInLineBuffer;
    Compressor *	compressor;
    Compressor::Format	format;
    vector<InSliceInfo>	slices;

    IStream *		is;

     Data (IStream *is): compressor (0), is (is) {}
    ~Data () {delete compressor;}
};


namespace {


void
reconstructLineOffsets (IStream &is,
			LineOrder lineOrder,
			vector<Int64> &lineOffsets)
{
    Int64 position = is.tellg();

    try
    {
	for (unsigned int i = 0; i < lineOffsets.size(); i++)
	{
	    Int64 lineOffset = is.tellg();

	    int y;
	    Xdr::read <StreamIO> (is, y);

	    int dataSize;
	    Xdr::read <StreamIO> (is, dataSize);

	    Xdr::skip <StreamIO> (is, dataSize);

	    if (lineOrder == INCREASING_Y)
		lineOffsets[i] = lineOffset;
	    else
		lineOffsets[lineOffsets.size() - i - 1] = lineOffset;
	}
    }
    catch (...)
    {
	//
	// Suppress all exceptions.  This functions is
	// called only to reconstruct the line offset
	// table for incomplete files, and exceptions
	// are likely.
	//
    }

    is.clear();
    is.seekg (position);
}


void
readLineOffsets (IStream &is,
		 LineOrder lineOrder,
		 vector<Int64> &lineOffsets)
{
    for (unsigned int i = 0; i < lineOffsets.size(); i++)
    {
	Xdr::read <StreamIO> (is, lineOffsets[i]);
    }

    for (unsigned int i = 0; i < lineOffsets.size(); i++)
    {
	if (lineOffsets[i] <= 0)
	{
	    //
	    // Invalid data in the line offset table mean that
	    // the file is probably incomplete (the table is
	    // the last thing written to the file).  Either
	    // some process is still busy writing the file,
	    // or writing the file was aborted.
	    //
	    // We should still be able to read the existing
	    // parts of the file.  In order to do this, we
	    // have to make a sequential scan over the scan
	    // line data to reconstruct the line offset table.
	    //

	    reconstructLineOffsets (is, lineOrder, lineOffsets);
	    break;
	}
    }
}



void
readPixelData (ScanLineInputFile::Data *ifd,
	       int y,
	       int &minY, int &maxY,
	       int &dataSize)
{
    Int64 lineOffset =
       ifd->lineOffsets[(y - ifd->minY) / ifd->linesInBuffer];

    if (lineOffset == 0)
	THROW (Iex::InputExc, "Scan line " << y << " is missing.");

    //
    // Seek to the start of the scan line in the file,
    // if necessary.
    //

    minY = lineBufferMinY (y, ifd->minY, ifd->linesInBuffer);
    maxY = lineBufferMaxY (y, ifd->minY, ifd->linesInBuffer);

    if (ifd->nextLineBufferMinY != minY)
	ifd->is->seekg (lineOffset);

    #ifdef DEBUG

	assert (ifd->is->tellg() == lineOffset);

    #endif

    //
    // Read the data block's header.
    //

    int yInFile;

    Xdr::read <StreamIO> (*ifd->is, yInFile);
    Xdr::read <StreamIO> (*ifd->is, dataSize);

    if (yInFile != minY)
	throw Iex::InputExc ("Unexpected data block y coordinate.");

    if (dataSize > (int) ifd->lineBufferSize)
	throw Iex::InputExc ("Unexpected data block length.");

    //
    // Read the pixel data.
    //

    ifd->is->read (ifd->lineBuffer, dataSize);

    //
    // Keep track of which scan line is the next one in
    // the file, so that we can avoid redundant seekg()
    // operations (seekg() can be fairly expensive).
    //

    if (ifd->lineOrder == INCREASING_Y)
	ifd->nextLineBufferMinY = minY + ifd->linesInBuffer;
    else
	ifd->nextLineBufferMinY = minY - ifd->linesInBuffer;
}

Compressor::Format
defaultFormat (Compressor * compressor)
{
    return compressor? compressor->format(): Compressor::XDR;
}

} // namespace


ScanLineInputFile::ScanLineInputFile (const Header &header, IStream *is):
    _data (new Data (is))
{
    try
    {
	_data->header = header;

	_data->lineOrder = _data->header.lineOrder();

	const Box2i &dataWindow = _data->header.dataWindow();

	_data->minX = dataWindow.min.x;
	_data->maxX = dataWindow.max.x;
	_data->minY = dataWindow.min.y;
	_data->maxY = dataWindow.max.y;

	int maxBytesPerLine = bytesPerLineTable (_data->header,
						 _data->bytesPerLine);

	_data->compressor = newCompressor (_data->header.compression(),
					   maxBytesPerLine,
					   _data->header);

	_data->linesInBuffer = _data->compressor?
				   _data->compressor->numScanLines(): 1;

	_data->format = defaultFormat (_data->compressor);

	_data->lineBufferSize = maxBytesPerLine * _data->linesInBuffer;
	_data->lineBuffer.resizeErase (_data->lineBufferSize);
	_data->lineBufferMinY = _data->minY - 1;
	_data->lineBufferMaxY = _data->minY - 1;
	_data->nextLineBufferMinY = _data->minY - 1;
	_data->uncompressedData = 0;

	offsetInLineBufferTable (_data->bytesPerLine,
				 _data->linesInBuffer,
				 _data->offsetInLineBuffer);

	int lineOffsetSize = (dataWindow.max.y - dataWindow.min.y +
			      _data->linesInBuffer) / _data->linesInBuffer;

	_data->lineOffsets.resize (lineOffsetSize);
	readLineOffsets (*_data->is, _data->lineOrder, _data->lineOffsets);
    }
    catch (...)
    {
	delete _data;
	throw;
    }
}


ScanLineInputFile::~ScanLineInputFile ()
{
    delete _data;
}


const char *
ScanLineInputFile::fileName () const
{
    return _data->is->fileName();
}


const Header &
ScanLineInputFile::header () const
{
    return _data->header;
}


int
ScanLineInputFile::version () const
{
    return _data->version;
}


void	
ScanLineInputFile::setFrameBuffer (const FrameBuffer &frameBuffer)
{
    //
    // Check if the new frame buffer descriptor is
    // compatible with the image file header.
    //

    const ChannelList &channels = _data->header.channels();

    for (FrameBuffer::ConstIterator j = frameBuffer.begin();
	 j != frameBuffer.end();
	 ++j)
    {
	ChannelList::ConstIterator i = channels.find (j.name());

	if (i == channels.end())
	    continue;

	if (i.channel().xSampling != j.slice().xSampling ||
	    i.channel().ySampling != j.slice().ySampling)
	{
	    THROW (Iex::ArgExc, "X and/or y subsampling factors "
				"of \"" << i.name() << "\" channel "
				"of input file \"" << fileName() << "\" are "
				"not compatible with the frame buffer's "
				"subsampling factors.");
	}
    }

    //
    // Initialize the slice table for readPixels().
    //

    vector<InSliceInfo> slices;
    ChannelList::ConstIterator i = channels.begin();

    for (FrameBuffer::ConstIterator j = frameBuffer.begin();
	 j != frameBuffer.end();
	 ++j)
    {
	while (i != channels.end() && strcmp (i.name(), j.name()) < 0)
	{
	    //
	    // Channel i is present in the file but not
	    // in the frame buffer; data for channel i
	    // will be skipped during readPixels().
	    //

	    slices.push_back (InSliceInfo (i.channel().type,
					   i.channel().type,
					   0, // base
					   0, // xStride
					   0, // yStride
					   i.channel().xSampling,
					   i.channel().ySampling,
					   false,  // fill
					   true, // skip
					   0.0)); // fillValue
	    ++i;
	}

	bool fill = false;

	if (i == channels.end() || strcmp (i.name(), j.name()) > 0)
	{
	    //
	    // Channel i is present in the frame buffer, but not in the file.
	    // In the frame buffer, slice j will be filled with a default value.
	    //

	    fill = true;
	}

	slices.push_back (InSliceInfo (j.slice().type,
				       fill ? j.slice().type:
				              i.channel().type,
				       j.slice().base,
				       j.slice().xStride,
				       j.slice().yStride,
				       j.slice().xSampling,
				       j.slice().ySampling,
				       fill,
				       false, // skip
				       j.slice().fillValue));

	if (i != channels.end() && !fill)
	    ++i;
    }

    //
    // Store the new frame buffer.
    //

    _data->frameBuffer = frameBuffer;
    _data->slices = slices;
}


const FrameBuffer &
ScanLineInputFile::frameBuffer () const
{
    return _data->frameBuffer;
}


void	
ScanLineInputFile::readPixels (int scanLine1, int scanLine2)
{
    try
    {
	if (_data->slices.size() == 0)
	    throw Iex::ArgExc ("No frame buffer specified "
			       "as pixel data destination.");

	//
	// Try to read the scan lines in the same order as in
	// the file to reduce the overhead of seek operations.
	//

	int minY = std::min (scanLine1, scanLine2);
	int maxY = std::max (scanLine1, scanLine2);

	if (minY < _data->minY || maxY > _data->maxY)
	{
	    throw Iex::ArgExc ("Tried to read scan line outside "
			       "the image file's data window.");
	}

	int numScanLines = maxY - minY + 1;
	int y;
	int dy;

	if (_data->lineOrder == INCREASING_Y)
	{
	    y = minY;
	    dy = 1;
	}
	else
	{
	    y = maxY;
	    dy = -1;
	}
        
	while (numScanLines)
	{
	    //
	    // If necessary, read the data block, that
	    // contains line y, from the output file.
	    //

	    if (y < _data->lineBufferMinY ||
		y > _data->lineBufferMaxY)
	    {
		int minY, maxY, dataSize;
		readPixelData (_data, y, minY, maxY, dataSize);
		_data->format = defaultFormat (_data->compressor);

		//
		// Uncompress the data, if necessary
		//

		int uncompressedSize = 0;
		int max = std::min (maxY, _data->maxY);

		for (int i = minY - _data->minY; i <= max - _data->minY; ++i)
		    uncompressedSize += (int) _data->bytesPerLine[i];

		if (_data->compressor && dataSize < uncompressedSize)
		{
		    dataSize = _data->compressor->uncompress
				    (_data->lineBuffer, dataSize, minY,
				     _data->uncompressedData);
		}
		else
		{
		    //
                    // If the line is uncompressed, it's in XDR format,
		    // regardless of the compressor's output format.
		    //

                    _data->format = Compressor::XDR;
                    _data->uncompressedData = _data->lineBuffer;
		}

		_data->lineBufferMinY = minY;
		_data->lineBufferMaxY = maxY;
	    }

	    //
	    // Convert one scan line's worth of pixel data back
	    // from the machine-independent representation, and
	    // store the result in the frame buffer.
	    //

	    const char *readPtr = _data->uncompressedData +
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

		const InSliceInfo &slice = _data->slices[i];

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

		if (slice.skip)
		{
		    //
		    // The file contains data for this channel, but
		    // the frame buffer contains no slice for this channel.
		    //

		    switch (slice.typeInFile)
		    {
		      case UINT:
		      
			Xdr::skip <CharPtrIO>
			    (readPtr,
			     Xdr::size <unsigned int> () *
			     (dMaxX - dMinX + 1));
			break;

		      case HALF:

			Xdr::skip <CharPtrIO>
			    (readPtr,
			     Xdr::size <half> () *
			     (dMaxX - dMinX + 1));
			break;

		      case FLOAT:

			Xdr::skip <CharPtrIO>
			    (readPtr,
			     Xdr::size <float> () *
			     (dMaxX - dMinX + 1));
			break;

		      default:

			throw Iex::ArgExc ("Unknown pixel data type.");
		    }
		}
		else
		{
		    //
		    // The frame buffer contains a slice for this channel.
		    //

		    char *linePtr  = slice.base +
				     divp (y, slice.ySampling) * slice.yStride;

		    char *pixelPtr = linePtr + dMinX * slice.xStride;
		    char *endPtr   = linePtr + dMaxX * slice.xStride;

		    if (slice.fill)
		    {
			//
			// The file contains no data for this channel.
			// Store a default value in the frame buffer.
			//

			switch (slice.typeInFrameBuffer)
			{
			  case UINT:
			  
			    {
				unsigned int fillValue =
				    (unsigned int) (slice.fillValue);

				while (pixelPtr <= endPtr)
				{
				    *(unsigned int *) pixelPtr = fillValue;
				    pixelPtr += slice.xStride;
				}
			    }
			    break;

			  case HALF:

			    {
				half fillValue =
				    half (slice.fillValue);

				while (pixelPtr <= endPtr)
				{
				    *(half *) pixelPtr = fillValue;
				    pixelPtr += slice.xStride;
				}
			    }
			    break;

			  case FLOAT:

			    {
				float fillValue =
				    float (slice.fillValue);

				while (pixelPtr <= endPtr)
				{
				    *(float *) pixelPtr = fillValue;
				    pixelPtr += slice.xStride;
				}
			    }
			    break;

			  default:

			    throw Iex::ArgExc ("Unknown pixel data type.");
			}
		    }
		    else if (_data->format == Compressor::XDR)
		    {
			//
			// The compressor produced data for this
			// channel in Xdr format.
			//
			// Convert the pixels from the file's machine-
			// independent representation, and store the
			// results in the frame buffer.
			//

			switch (slice.typeInFrameBuffer)
			{
			  case UINT:
		    
			    switch (slice.typeInFile)
			    {
			      case UINT:

				while (pixelPtr <= endPtr)
				{
				    Xdr::read <CharPtrIO>
					(readPtr, *(unsigned int *) pixelPtr);
				    pixelPtr += slice.xStride;
				}
				break;

			      case HALF:

				while (pixelPtr <= endPtr)
				{
				    half h;
				    Xdr::read <CharPtrIO> (readPtr, h);
				    *(unsigned int *) pixelPtr = halfToUint (h);
				    pixelPtr += slice.xStride;
				}
				break;

			      case FLOAT:

				while (pixelPtr <= endPtr)
				{
				    float f;
				    Xdr::read <CharPtrIO> (readPtr, f);
				    *(unsigned int *)pixelPtr = floatToUint (f);
				    pixelPtr += slice.xStride;
				}
				break;
			    }
			    break;

			  case HALF:

			    switch (slice.typeInFile)
			    {
			      case UINT:

				while (pixelPtr <= endPtr)
				{
				    unsigned int ui;
				    Xdr::read <CharPtrIO> (readPtr, ui);
				    *(half *) pixelPtr = uintToHalf (ui);
				    pixelPtr += slice.xStride;
				}
				break;
			      
			      case HALF:

				while (pixelPtr <= endPtr)
				{
				    Xdr::read <CharPtrIO>
					(readPtr, *(half *) pixelPtr);
				    pixelPtr += slice.xStride;
				}
				break;

			      case FLOAT:

				while (pixelPtr <= endPtr)
				{
				    float f;
				    Xdr::read <CharPtrIO> (readPtr, f);
				    *(half *) pixelPtr = floatToHalf (f);
				    pixelPtr += slice.xStride;
				}
				break;
			    }
			    break;

			  case FLOAT:

			    switch (slice.typeInFile)
			    {
			      case UINT:

				while (pixelPtr <= endPtr)
				{
				    unsigned int ui;
				    Xdr::read <CharPtrIO> (readPtr, ui);
				    *(float *) pixelPtr = float (ui);
				    pixelPtr += slice.xStride;
				}
				break;

			      case HALF:

				while (pixelPtr <= endPtr)
				{
				    half h;
				    Xdr::read <CharPtrIO> (readPtr, h);
				    *(float *) pixelPtr = float (h);
				    pixelPtr += slice.xStride;
				}
				break;

			      case FLOAT:

				while (pixelPtr <= endPtr)
				{
				    Xdr::read <CharPtrIO>
					(readPtr, *(float *) pixelPtr);
				    pixelPtr += slice.xStride;
				}
				break;
			    }
			    break;

			  default:

			    throw Iex::ArgExc ("Unknown pixel data type.");
			}
		    }
		    else
		    {
			//
			// The compressor produced data for this
			// channel in the machine's native format.
			// Copy the results into the frame buffer.
			//

			switch (slice.typeInFrameBuffer)
			{
			  case UINT:
		    
			    switch (slice.typeInFile)
			    {
			      case UINT:

				while (pixelPtr <= endPtr)
				{
				    for (size_t i = 0;
					 i < sizeof (unsigned int);
					 ++i)
				    {
					pixelPtr[i] = readPtr[i];
				    }

				    readPtr += sizeof (unsigned int);
				    pixelPtr += slice.xStride;
				}
				break;

			      case HALF:

				while (pixelPtr <= endPtr)
				{
				    half h = *(half *) readPtr;
				    *(unsigned int *) pixelPtr = halfToUint (h);
				    readPtr += sizeof (half);
				    pixelPtr += slice.xStride;
				}
				break;

			      case FLOAT:

				while (pixelPtr <= endPtr)
				{
				    float f;

				    for (size_t i = 0; i < sizeof (float); ++i)
				    {
					((char *)&f)[i] = readPtr[i];
				    }

				    *(unsigned int *)pixelPtr = floatToUint (f);
				    readPtr += sizeof (float);
				    pixelPtr += slice.xStride;
				}
				break;
			    }
			    break;

			  case HALF:

			    switch (slice.typeInFile)
			    {
			      case UINT:

				while (pixelPtr <= endPtr)
				{
				    unsigned int ui;

				    for (size_t i = 0;
					 i < sizeof (unsigned int);
					 ++i)
				    {
					((char *)&ui)[i] = readPtr[i];
				    }

				    *(half *) pixelPtr = uintToHalf (ui);
				    readPtr += sizeof (unsigned int);
				    pixelPtr += slice.xStride;
				}
				break;

			      case HALF:

				while (pixelPtr <= endPtr)
				{
				    *(half *) pixelPtr = *(half *)readPtr;
				    readPtr += sizeof (half);
				    pixelPtr += slice.xStride;
				}
				break;

			      case FLOAT:

				while (pixelPtr <= endPtr)
				{
				    float f;

				    for (size_t i = 0; i < sizeof (float); ++i)
				    {
					((char *)&f)[i] = readPtr[i];
				    }

				    *(half *) pixelPtr = floatToHalf (f);
				    readPtr += sizeof (float);
				    pixelPtr += slice.xStride;
				}
				break;
			    }
			    break;

			  case FLOAT:

			    switch (slice.typeInFile)
			    {
			      case UINT:

				while (pixelPtr <= endPtr)
				{
				    unsigned int ui;

				    for (size_t i = 0;
					 i < sizeof (unsigned int);
					 ++i)
				    {
					((char *)&ui)[i] = readPtr[i];
				    }

				    *(float *) pixelPtr = float (ui);
				    readPtr += sizeof (unsigned int);
				    pixelPtr += slice.xStride;
				}
				break;

			      case HALF:

				while (pixelPtr <= endPtr)
				{
				    half h = *(half *) readPtr;
				    *(float *) pixelPtr = float (h);
				    readPtr += sizeof (half);
				    pixelPtr += slice.xStride;
				}
				break;

			      case FLOAT:

				while (pixelPtr <= endPtr)
				{
				    for (size_t i = 0; i < sizeof (float); ++i)
					pixelPtr[i] = readPtr[i];

				    readPtr += sizeof (float);
				    pixelPtr += slice.xStride;
				}
				break;
			    }
			    break;

			  default:

			    throw Iex::ArgExc ("Unknown pixel data type.");
			}
		    }
		}
	    }

	    //
	    // Advance to the next scan line.
	    //

	    numScanLines -= 1;
	    y += dy;
	}
    }
    catch (Iex::BaseExc &e)
    {
	REPLACE_EXC (e, "Error reading pixel data from image "
		        "file \"" << fileName() << "\". " << e);
	throw;
    }
}


void	
ScanLineInputFile::readPixels (int scanLine)
{
    readPixels (scanLine, scanLine);
}


void
ScanLineInputFile::rawPixelData (int firstScanLine,
				 const char *&pixelData,
				 int &pixelDataSize)
{
    try
    {
	if (firstScanLine < _data->minY || firstScanLine > _data->maxY)
	{
	    throw Iex::ArgExc ("Tried to read scan line outside "
			       "the image file's data window.");
	}

	int dummy1, dummy2;
	readPixelData (_data, firstScanLine, dummy1, dummy2, pixelDataSize);
	pixelData = _data->lineBuffer;
    }
    catch (Iex::BaseExc &e)
    {
	REPLACE_EXC (e, "Error reading pixel data from image "
		        "file \"" << fileName() << "\". " << e);
	throw;
    }
}

} // namespace Imf
