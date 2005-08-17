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
//	class TiledInputFile
//
//-----------------------------------------------------------------------------

#include <ImfTiledInputFile.h>
#include <ImfTileDescriptionAttribute.h>
#include <ImfChannelList.h>
#include <ImfMisc.h>
#include <ImfTiledMisc.h>
#include <ImfStdIO.h>
#include <ImfCompressor.h>
#include <ImathBox.h>
#include <ImfXdr.h>
#include <ImfArray.h>
#include <ImfConvert.h>
#include <ImfVersion.h>
#include <ImfTileOffsets.h>

#include <Iex.h>
#include <ImathVec.h>

#include <string>
#include <vector>
#include <algorithm>
#include <assert.h>


namespace Imf {

using Imath::Box2i;
using Imath::V2i;
using std::string;
using std::vector;


namespace {

struct TInSliceInfo
{
    PixelType   typeInFrameBuffer;
    PixelType   typeInFile;
    char *      base;
    size_t      xStride;
    size_t      yStride;
    bool        fill;
    bool        skip;
    double      fillValue;

    TInSliceInfo (PixelType typeInFrameBuffer = HALF,
                  PixelType typeInFile = HALF,
                  char *base = 0,
                  size_t xStride = 0,
                  size_t yStride = 0,
                  bool fill = false,
                  bool skip = false,
                  double fillValue = 0.0);
};


TInSliceInfo::TInSliceInfo (PixelType tifb,
                            PixelType tifl,
                            char *b,
                            size_t xs, size_t ys,
                            bool f, bool s,
                            double fv)
:
    typeInFrameBuffer (tifb),
    typeInFile (tifl),
    base (b),
    xStride (xs),
    yStride (ys),
    fill (f),
    skip (s),
    fillValue (fv)
{
    // empty
}



} // namespace


//
// struct TiledInputFile::Data stores things that will be
// needed between calls to readTile()
//

struct TiledInputFile::Data
{
    Header	    header;		    // the image header
    TileDescription tileDesc;		    // describes the tile layout
    int		    version;		    // file's version
    FrameBuffer	    frameBuffer;	    // framebuffer to write into
    LineOrder	    lineOrder;		    // the file's lineorder
    int		    minX;		    // data window's min x coord
    int		    maxX;		    // data window's max x coord
    int		    minY;		    // data window's min y coord
    int		    maxY;		    // data window's max x coord

    //
    // cached tile information:
    //

    int		    numXLevels;		    // number of x levels
    int		    numYLevels;		    // number of y levels
    int *	    numXTiles;		    // number of x tiles at a level
    int *	    numYTiles;		    // number of y tiles at a level

    TileOffsets	    tileOffsets;	    // stores offsets in file for
					    // each tile

    bool	    fileIsComplete;	    // True if no tiles are missing
    					    // in the file

    Int64	    currentPosition;        // file offset for current tile,
					    // used to prevent unnecessary
					    // seeking

    Compressor*	    compressor;		    // the compressor
    Compressor::Format  format;		    // compressor's data format
    vector<TInSliceInfo> slices;	    // info about channels in file
    IStream *	    is;			    // file stream to read from

    size_t	    bytesPerPixel;          // size of an uncompressed pixel

    size_t	    maxBytesPerTileLine;    // combined size of a line
					    // over all channels

    size_t	    tileBufferSize;	    // size of the tile buffer
    Array<char>	    tileBuffer;		    // holds a single tile
    const char*	    uncompressedData;	    // the uncompressed tile

    bool	    deleteStream;	    // should we delete the stream
					    // ourselves? or does someone
					    // else do it?
     Data (bool deleteStream);
    ~Data ();
};


TiledInputFile::Data::Data (bool del):
    numXTiles (0),
    numYTiles (0),
    compressor (0),
    is (0),
    uncompressedData (0),
    deleteStream (del)
{
    // empty
}


TiledInputFile::Data::~Data ()
{
    delete [] numXTiles;
    delete [] numYTiles;
    delete compressor;

    if (deleteStream)
	delete is;
}


namespace {

void
readTileData (TiledInputFile::Data *ifd,
	      int dx, int dy,
	      int lx, int ly,
              int &dataSize)
{
    //
    // Read a single tile block from the file
    //

    //
    // Look up the location for this tile in the Index and
    // seek to that position if necessary
    //
    
    Int64 tileOffset = ifd->tileOffsets (dx, dy, lx, ly);

    if (tileOffset == 0)
    {
        THROW (Iex::InputExc, "Tile (" << dx << ", " << dy << ", " <<
			      lx << ", " << ly << ") is missing.");
    }

    //
    // Seek to the start of the tile in the file,
    // if necessary.
    //
    
    if (ifd->currentPosition != tileOffset)
        ifd->is->seekg (tileOffset);

    #ifdef DEBUG

	assert (ifd->is->tellg() == tileOffset);

    #endif

    //
    // Read the first few bytes of the tile (the header).
    // Test that the tile coords and the level number are
    // correct.
    //
    
    int tileXCoord, tileYCoord, levelX, levelY;

    Xdr::read <StreamIO> (*ifd->is, tileXCoord);
    Xdr::read <StreamIO> (*ifd->is, tileYCoord);
    Xdr::read <StreamIO> (*ifd->is, levelX);
    Xdr::read <StreamIO> (*ifd->is, levelY);
    Xdr::read <StreamIO> (*ifd->is, dataSize);

    if (tileXCoord != dx)
        throw Iex::InputExc ("Unexpected tile x coordinate.");

    if (tileYCoord != dy)
        throw Iex::InputExc ("Unexpected tile y coordinate.");

    if (levelX != lx)
        throw Iex::InputExc ("Unexpected tile x level number coordinate.");

    if (levelY != ly)
        throw Iex::InputExc ("Unexpected tile y level number coordinate.");

    if (dataSize > (int) ifd->tileBufferSize)
        throw Iex::InputExc ("Unexpected tile block length.");

    //
    // Read the pixel data.
    //

    ifd->is->read (ifd->tileBuffer, dataSize);

    //
    // Keep track of which tile is the next one in
    // the file, so that we can avoid redundant seekg()
    // operations (seekg() can be fairly expensive).
    //
    
    ifd->currentPosition = tileOffset + 5 * Xdr::size<int>() + dataSize;
}


void
readNextTileData (TiledInputFile::Data *ifd,
		  int &dx, int &dy,
		  int &lx, int &ly,
		  int &dataSize)
{
    //
    // Read the next tile block from the file
    //

    //
    // Read the first few bytes of the tile (the header).
    //

    Xdr::read <StreamIO> (*ifd->is, dx);
    Xdr::read <StreamIO> (*ifd->is, dy);
    Xdr::read <StreamIO> (*ifd->is, lx);
    Xdr::read <StreamIO> (*ifd->is, ly);
    Xdr::read <StreamIO> (*ifd->is, dataSize);

    if (dataSize > (int) ifd->tileBufferSize)
        throw Iex::InputExc ("Unexpected tile block length.");
    
    //
    // Read the pixel data.
    //

    ifd->is->read (ifd->tileBuffer, dataSize);
    
    //
    // Keep track of which tile is the next one in
    // the file, so that we can avoid redundant seekg()
    // operations (seekg() can be fairly expensive).
    //

    ifd->currentPosition += 5 * Xdr::size<int>() + dataSize;
}

} // namespace


TiledInputFile::TiledInputFile (const char fileName[]):
    _data (new Data (true))
{
    //
    // This constructor is called when a user
    // explicitly wants to read a tiled file.
    //

    try
    {
	_data->is = new StdIFStream (fileName);
	_data->header.readFrom (*_data->is, _data->version);
	initialize();
    }
    catch (Iex::BaseExc &e)
    {
	delete _data;

	REPLACE_EXC (e, "Cannot open image file "
			"\"" << fileName << "\". " << e);
	throw;
    }
}


TiledInputFile::TiledInputFile (IStream &is):
    _data (new Data (false))
{
    //
    // This constructor is called when a user
    // explicitly wants to read a tiled file.
    //

    try
    {
	_data->is = &is;
	_data->header.readFrom (*_data->is, _data->version);
	initialize();
    }
    catch (Iex::BaseExc &e)
    {
	delete _data;

	REPLACE_EXC (e, "Cannot open image file "
			"\"" << is.fileName() << "\". " << e);
	throw;
    }
}


TiledInputFile::TiledInputFile (const Header &header, IStream *is, int version):
    _data (new Data (false))
{
    //
    // This constructor called by class Imf::InputFile
    // when a user wants to just read an image file, and
    // doesn't care or know if the file is tiled.
    //

    _data->is = is;
    _data->header = header;
    _data->version = version;
    initialize();
}


void
TiledInputFile::initialize ()
{
    if (!isTiled (_data->version))
	throw Iex::ArgExc ("Expected a tiled file but the file is not tiled.");

    _data->header.sanityCheck (true);

    _data->tileDesc = _data->header.tileDescription();
    _data->lineOrder = _data->header.lineOrder();

    //
    // Save the dataWindow information
    //
    
    const Box2i &dataWindow = _data->header.dataWindow();
    _data->minX = dataWindow.min.x;
    _data->maxX = dataWindow.max.x;
    _data->minY = dataWindow.min.y;
    _data->maxY = dataWindow.max.y;

    //
    // Precompute level and tile information to speed up utility functions
    //

    precalculateTileInfo (_data->tileDesc,
			  _data->minX, _data->maxX,
			  _data->minY, _data->maxY,
			  _data->numXTiles, _data->numYTiles,
			  _data->numXLevels, _data->numYLevels);    

    _data->bytesPerPixel = calculateBytesPerPixel (_data->header);

    _data->maxBytesPerTileLine = _data->bytesPerPixel * _data->tileDesc.xSize;

    _data->compressor = newTileCompressor (_data->header.compression(),
					   _data->maxBytesPerTileLine,
					   tileYSize(),
					   _data->header);

    _data->format = _data->compressor? _data->compressor->format():
				       Compressor::XDR;

    _data->tileBufferSize = _data->maxBytesPerTileLine * tileYSize();
    _data->tileBuffer.resizeErase (_data->tileBufferSize);

    _data->uncompressedData = 0;

    _data->tileOffsets = TileOffsets (_data->tileDesc.mode,
				      _data->numXLevels,
				      _data->numYLevels,
				      _data->numXTiles,
				      _data->numYTiles);

    _data->tileOffsets.readFrom (*(_data->is), _data->fileIsComplete);

    _data->currentPosition = _data->is->tellg();
}


TiledInputFile::~TiledInputFile ()
{
    delete _data;
}


const char *
TiledInputFile::fileName () const
{
    return _data->is->fileName();
}


const Header &
TiledInputFile::header () const
{
    return _data->header;
}


int
TiledInputFile::version () const
{
    return _data->version;
}


void	
TiledInputFile::setFrameBuffer (const FrameBuffer &frameBuffer)
{
    //
    // Set the frame buffer
    //

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

    vector<TInSliceInfo> slices;
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

            slices.push_back (TInSliceInfo (i.channel().type,
					    i.channel().type,
					    0, // base
					    0, // xStride
					    0, // yStride
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

        slices.push_back (TInSliceInfo (j.slice().type,
                                        fill? j.slice().type: i.channel().type,
                                        j.slice().base,
                                        j.slice().xStride,
                                        j.slice().yStride,
                                        fill,
                                        false, // skip
                                        j.slice().fillValue));

        if (i != channels.end() && !fill)
            ++i;
    }

    while (i != channels.end())
    {
	//
	// Channel i is present in the file but not
	// in the frame buffer; data for channel i
	// will be skipped during readPixels().
	//

	slices.push_back (TInSliceInfo (i.channel().type,
					i.channel().type,
					0, // base
					0, // xStride
					0, // yStride
					false,  // fill
					true, // skip
					0.0)); // fillValue
	++i;
    }

    //
    // Store the new frame buffer.
    //

    _data->frameBuffer = frameBuffer;
    _data->slices = slices;
}


const FrameBuffer &
TiledInputFile::frameBuffer () const
{
    return _data->frameBuffer;
}


bool
TiledInputFile::isComplete () const
{
    return _data->fileIsComplete;
}


void	
TiledInputFile::readTile (int dx, int dy, int lx, int ly)
{
    //
    // Read a tile from the file into the framebuffer
    //

    try
    {
        if (_data->slices.size() == 0)
	{
            throw Iex::ArgExc ("No frame buffer specified "
			       "as pixel data destination.");
	}

        if (!isValidTile (dx, dy, lx, ly))
        {
            THROW (Iex::ArgExc, "Tile (" << dx << ", " << dy << ", " <<
				lx << "," << ly << ") is not a valid tile.");
        }

	//
        // Calculate information about the tile
	//

        Box2i tileRange = dataWindowForTile (dx, dy, lx, ly);

        int numPixelsPerScanLine = tileRange.max.x - tileRange.min.x + 1;
                              
        int numPixelsInTile = (tileRange.max.x - tileRange.min.x + 1) *
                              (tileRange.max.y - tileRange.min.y + 1);

        int sizeOfTile = _data->bytesPerPixel * numPixelsInTile;

        //
        // The forceXdr flag is used to force the lineBuffer to be
	// interpreted as Xdr.  This is needed because some compressors
	// can store in the machine's native format, but when a lineBuffer
	// is not compressed, it has to be saved in Xdr format so that
	// it works across machines with different byte orders.
        //

        bool forceXdr = false;

        //
        // Read the data block for this tile into _data->tileBuffer.
        //

        int dataSize;
        readTileData (_data, dx, dy, lx, ly, dataSize);

        //
        // Uncompress the data.
        //

        if (_data->compressor && dataSize < sizeOfTile)
        {
            dataSize = _data->compressor->uncompressTile
			    (_data->tileBuffer,
			     dataSize, tileRange,
			     _data->uncompressedData);
        }
        else
        {
	    //
            // If the line is uncompressed, but the compressor
            // says that its in native format, don't believe it.
	    //

            if (_data->format != Compressor::XDR)
                forceXdr = true;
            _data->uncompressedData = _data->tileBuffer;
        }

        //
        // Convert the tile of pixel data back
        // from the machine-independent representation, and
        // store the result in the frame buffer.
        //

        const char *readPtr = _data->uncompressedData; // points to where we
						       // read from in the
						       // tile block
	//
	// Iterate over the scan lines in the tile.
	//
	
	for (int y = tileRange.min.y; y <= tileRange.max.y; ++y)
	{
	    //
	    // Iterate over all image channels.
	    //
	    
	    for (unsigned int i = 0; i < _data->slices.size(); ++i)
	    {
		const TInSliceInfo &slice = _data->slices[i];

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
			    (readPtr, Xdr::size <unsigned int>() *
			     numPixelsPerScanLine);

			break;

		      case HALF:

			Xdr::skip <CharPtrIO>
			    (readPtr, Xdr::size <half>() *
			     numPixelsPerScanLine);

			break;

		      case FLOAT:

			Xdr::skip <CharPtrIO>
			    (readPtr, Xdr::size <float>() *
			     numPixelsPerScanLine);

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

		    //
		    // pixelPtr points to where we write to
		    // in the frame buffer
		    //

		    char *pixelPtr = slice.base +
				     y * slice.yStride +
				     tileRange.min.x * slice.xStride;

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

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
				{
				    *(unsigned int *) pixelPtr = fillValue;
				    pixelPtr += slice.xStride;
				}
			    }
			    break;

			  case HALF:

			    {
				half fillValue = half (slice.fillValue);

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
				{
				    *(half *) pixelPtr = fillValue;
				    pixelPtr += slice.xStride;
				}
			    }
			    break;

			  case FLOAT:

			    {
				float fillValue = float (slice.fillValue);

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
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
		    else if (_data->format == Compressor::XDR || forceXdr)
		    {
			//
			// The compressor produced data for this
			// channel in Xdr format.
			//
			// Convert the pixels from the file's machine-
			// independent representation, and store the
			// results the frame buffer.
			//

			switch (slice.typeInFrameBuffer)
			{
			  case UINT:
			  
			    switch (slice.typeInFile)
			    {
			      case UINT:

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
				{
				    Xdr::read <CharPtrIO>
					(readPtr, *(unsigned int *) pixelPtr);

				    pixelPtr += slice.xStride;
				}

				break;

			      case HALF:

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
				{
				    half h;
				    Xdr::read <CharPtrIO> (readPtr, h);

				    *(unsigned int *) pixelPtr = halfToUint (h);
				    pixelPtr += slice.xStride;
				}

				break;

			      case FLOAT:

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
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

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
				{
				    unsigned int ui;
				    Xdr::read <CharPtrIO> (readPtr, ui);

				    *(half *) pixelPtr = uintToHalf (ui);
				    pixelPtr += slice.xStride;
				}

				break;

			      case HALF:

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
				{
				    Xdr::read <CharPtrIO>
					(readPtr, *(half *) pixelPtr);

				    pixelPtr += slice.xStride;
				}

				break;

			      case FLOAT:

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
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

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
				{
				    unsigned int ui;
				    Xdr::read <CharPtrIO> (readPtr, ui);

				    *(float *) pixelPtr = float (ui);
				    pixelPtr += slice.xStride;
				}

				break;

			      case HALF:

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
				{
				    half h;
				    Xdr::read <CharPtrIO> (readPtr, h);

				    *(float *) pixelPtr = float (h);
				    pixelPtr += slice.xStride;
				}

				break;

			      case FLOAT:

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
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
			//
			// Convert the pixels from the file's machine-
			// dependent representation, and store the
			// results the frame buffer.
			//

			switch (slice.typeInFrameBuffer)
			{
			  case UINT:
			  
			    switch (slice.typeInFile)
			    {
			      case UINT:

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
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

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
				{
				    half h = *(half *) readPtr;
				    *(unsigned int *) pixelPtr = halfToUint (h);
				    readPtr += sizeof (half);
				    pixelPtr += slice.xStride;
				}

				break;

			      case FLOAT:

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
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

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
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

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
				{
				    *(half *) pixelPtr = *(half *)readPtr;

				    readPtr += sizeof (half);
				    pixelPtr += slice.xStride;
				}

				break;

			      case FLOAT:

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
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

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
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

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
				{
				    half h = *(half *) readPtr;
				    *(float *) pixelPtr = float (h);
				    readPtr += sizeof (half);
				    pixelPtr += slice.xStride;
				}

				break;

			      case FLOAT:

				for (int x = tileRange.min.x;
				     x <= tileRange.max.x;
				     ++x)
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
TiledInputFile::readTile (int dx, int dy, int l)
{
    readTile (dx, dy, l, l);
}


void
TiledInputFile::rawTileData (int &dx, int &dy,
			     int &lx, int &ly,
                             const char *&pixelData,
			     int &pixelDataSize)
{
    try
    {
        if (!isValidTile (dx, dy, lx, ly))
        {
            throw Iex::ArgExc ("Tried to read a tile outside "
			       "the image file's data window.");
        }

        readNextTileData (_data, dx, dy, lx, ly, pixelDataSize);
        pixelData = _data->tileBuffer;
    }
    catch (Iex::BaseExc &e)
    {
        REPLACE_EXC (e, "Error reading pixel data from image "
			"file \"" << fileName() << "\". " << e);
        throw;
    }
}


unsigned int
TiledInputFile::tileXSize () const
{
    return _data->tileDesc.xSize;
}


unsigned int
TiledInputFile::tileYSize () const
{
    return _data->tileDesc.ySize;
}


LevelMode
TiledInputFile::levelMode () const
{
    return _data->tileDesc.mode;
}


LevelRoundingMode
TiledInputFile::levelRoundingMode () const
{
    return _data->tileDesc.roundingMode;
}


int
TiledInputFile::numLevels () const
{
    if (levelMode() == RIPMAP_LEVELS)
    {
	THROW (Iex::LogicExc, "Error calling numLevels() on image "
			      "file \"" << fileName() << "\" "
			      "(numLevels() is not defined for files "
			      "with RIPMAP level mode).");
    }

    return _data->numXLevels;
}


int
TiledInputFile::numXLevels () const
{
    return _data->numXLevels;
}


int
TiledInputFile::numYLevels () const
{
    return _data->numYLevels;
}


bool	
TiledInputFile::isValidLevel (int lx, int ly) const
{
    if (lx < 0 || ly < 0)
	return false;

    if (levelMode() == MIPMAP_LEVELS && lx != ly)
	return false;

    if (lx >= numXLevels() || ly >= numYLevels())
	return false;

    return true;
}


int
TiledInputFile::levelWidth (int lx) const
{
    try
    {
	return levelSize (_data->minX, _data->maxX, lx,
			  _data->tileDesc.roundingMode);
    }
    catch (Iex::BaseExc &e)
    {
	REPLACE_EXC (e, "Error calling levelWidth() on image "
			"file \"" << fileName() << "\". " << e);
	throw;
    }
}


int
TiledInputFile::levelHeight (int ly) const
{
    try
    {
	return levelSize (_data->minY, _data->maxY, ly,
			  _data->tileDesc.roundingMode);
    }
    catch (Iex::BaseExc &e)
    {
	REPLACE_EXC (e, "Error calling levelWidth() on image "
			"file \"" << fileName() << "\". " << e);
	throw;
    }
}


int
TiledInputFile::numXTiles (int lx) const
{
    if (lx < 0 || lx >= numXLevels())
    {
        THROW (Iex::ArgExc, "Error calling numXTiles() on image "
			    "file \"" << fileName() << "\" "
			    "(Argument is not in valid range).");

    }

    return _data->numXTiles[lx];
}


int
TiledInputFile::numYTiles (int ly) const
{
    if (ly < 0 || ly >= numYLevels())
    {
        THROW (Iex::ArgExc, "Error calling numYTiles() on image "
			    "file \"" << fileName() << "\" "
			    "(Argument is not in valid range).");
    }

    return _data->numYTiles[ly];
}


Box2i
TiledInputFile::dataWindowForLevel (int l) const
{
    return dataWindowForLevel (l, l);
}


Box2i
TiledInputFile::dataWindowForLevel (int lx, int ly) const
{
    try
    {
	return Imf::dataWindowForLevel (_data->tileDesc,
					_data->minX, _data->maxX,
				        _data->minY, _data->maxY,
				        lx, ly);
    }
    catch (Iex::BaseExc &e)
    {
	REPLACE_EXC (e, "Error calling dataWindowForLevel() on image "
			"file \"" << fileName() << "\". " << e);
	throw;
    }
}


Box2i
TiledInputFile::dataWindowForTile (int dx, int dy, int l) const
{
    return dataWindowForTile (dx, dy, l, l);
}


Box2i
TiledInputFile::dataWindowForTile (int dx, int dy, int lx, int ly) const
{
    try
    {
	if (!isValidTile (dx, dy, lx, ly))
	    throw Iex::ArgExc ("Arguments not in valid range.");

	return Imf::dataWindowForTile (_data->tileDesc,
				       _data->minX, _data->maxX,
				       _data->minY, _data->maxY,
				       dx, dy, lx, ly);
    }
    catch (Iex::BaseExc &e)
    {
	REPLACE_EXC (e, "Error calling dataWindowForTile() on image "
			"file \"" << fileName() << "\". " << e);
	throw;
    }
}


bool
TiledInputFile::isValidTile(int dx, int dy, int lx, int ly) const
{
    return ((lx < numXLevels() && lx >= 0) &&
            (ly < numYLevels() && ly >= 0) &&
            (dx < numXTiles(lx) && dx >= 0) &&
            (dy < numYTiles(ly) && dy >= 0));
}


} // namespace Imf
