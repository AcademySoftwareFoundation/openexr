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
//	class TiledOutputFile
//
//-----------------------------------------------------------------------------

#include <ImfTiledOutputFile.h>
#include <ImfTiledInputFile.h>
#include <ImfInputFile.h>
#include <ImfTileDescriptionAttribute.h>
#include <ImfPreviewImageAttribute.h>
#include <ImfChannelList.h>
#include <ImfMisc.h>
#include <ImfTiledMisc.h>
#include <ImfStdIO.h>
#include <ImfCompressor.h>
#include <ImathBox.h>
#include <ImfArray.h>
#include <ImfXdr.h>
#include <ImfVersion.h>
#include <ImfTileOffsets.h>

#include <Iex.h>

#include <string>
#include <vector>
#include <fstream>
#include <assert.h>
#include <map>

namespace Imf {

using Imath::Box2i;
using Imath::V2i;
using std::string;
using std::vector;
using std::ofstream;
using std::map;


namespace {

struct TOutSliceInfo
{
    PixelType		type;
    const char *	base;
    size_t		xStride;
    size_t		yStride;
    bool		zero;

    TOutSliceInfo (PixelType type = HALF,
	           const char *base = 0,
	           size_t xStride = 0,
	           size_t yStride = 0,
	           bool zero = false);
};


TOutSliceInfo::TOutSliceInfo (PixelType t,
		              const char *b,
			      size_t xs, size_t ys,
			      bool z)
:
    type (t),
    base (b),
    xStride (xs),
    yStride (ys),
    zero (z)
{
    // empty
}


struct TileCoord
{
    int		dx;
    int		dy;
    int		lx;
    int		ly;
    

    TileCoord (int xTile = 0, int yTile = 0,
	       int xLevel = 0, int yLevel = 0)
    :
        dx (xTile),  dy (yTile),
	lx (xLevel), ly (yLevel)
    {
        // empty
    }
    

    bool
    operator < (const TileCoord &other) const
    {
        return (ly < other.ly) ||
	       (ly == other.ly && lx < other.lx) ||
	       ((ly == other.ly && lx == other.lx) &&
		    ((dy < other.dy) || (dy == other.dy && dx < other.dx)));
    }


    bool
    operator==(const TileCoord &other) const
    {
        return lx == other.lx &&
	       ly == other.ly &&
	       dx == other.dx &&
	       dy == other.dy;
    }
};


struct BufferedTile
{
    char *	pixelData;
    int		pixelDataSize;

    BufferedTile (const char *data, int size):
	pixelData (0),
	pixelDataSize(size)
    {
	pixelData = new char[pixelDataSize];
	memcpy (pixelData, data, pixelDataSize);
    }

    ~BufferedTile()
    {
	delete [] pixelData;
    }
};


typedef map <TileCoord, BufferedTile*> TileMap;

} // namespace


struct TiledOutputFile::Data
{
    Header		header;			// the image header
    int			version;		// file format version
    TileDescription	tileDesc;		// describes the tile layout
    FrameBuffer		frameBuffer;		// framebuffer to write into
    Int64		previewPosition;
    LineOrder		lineOrder;		// the file's lineorder
    int			minX;			// data window's min x coord
    int			maxX;			// data window's max x coord
    int			minY;			// data window's min y coord
    int			maxY;			// data window's max x coord

    //
    // cached tile information:
    //

    int			numXLevels;		// number of x levels
    int			numYLevels;		// number of y levels
    int *		numXTiles;		// number of x tiles at a level
    int *		numYTiles;		// number of y tiles at a level

    TileOffsets		tileOffsets;		// stores offsets in file for
						// each tile

    Compressor *	compressor;		// the compressor
    Compressor::Format	format;			// compressor's data format
    vector<TOutSliceInfo> slices;		// info about channels in file
    OStream *		os;			// file stream to write to
    bool		deleteStream;

    size_t		maxBytesPerTileLine;	// combined size of a tile line
						// over all channels

    size_t		tileBufferSize;		// size of the tile buffer
    Array<char>		tileBuffer;		// holds a single tile

    Int64		tileOffsetsPosition;	// position of the tile index
    Int64		currentPosition;	// current position in the file
    
    TileMap		tileMap;
    TileCoord		nextTileToWrite;

     Data (bool del);
    ~Data ();
    
    TileCoord		nextTileCoord (const TileCoord &a);
};


TiledOutputFile::Data::Data (bool del):
    numXTiles(0),
    numYTiles(0),
    compressor(0),
    os (0),
    deleteStream (del),
    tileOffsetsPosition (0)
{
    // empty
}


TiledOutputFile::Data::~Data ()
{
    delete [] numXTiles;
    delete [] numYTiles;
    delete compressor;

    if (deleteStream)
	delete os;
    
    //
    // Delete all the tile buffers, if any still happen to exist
    //
    
    for (TileMap::iterator i = tileMap.begin(); i != tileMap.end(); ++i)
	delete i->second;
}


TileCoord
TiledOutputFile::Data::nextTileCoord (const TileCoord &a)
{
    TileCoord b = a;
    
    if (lineOrder == INCREASING_Y)
    {
        b.dx++;

        if (b.dx >= numXTiles[b.lx])
        {
            b.dx = 0;
            b.dy++;

            if (b.dy >= numYTiles[b.ly])
            {
		//
		// the next tile is in the next level
		//

                b.dy = 0;

                switch (tileDesc.mode)
                {
                  case ONE_LEVEL:
                  case MIPMAP_LEVELS:

                    b.lx++;
                    b.ly++;
                    break;

                  case RIPMAP_LEVELS:

                    b.lx++;

                    if (b.lx >= numXLevels)
                    {
                        b.lx = 0;
                        b.ly++;

			#ifdef DEBUG
			    assert (b.ly <= numYLevels);
			#endif
                    }
                    break;
                }
            }
        }
    }
    else if (lineOrder == DECREASING_Y)
    {
        b.dx++;

        if (b.dx >= numXTiles[b.lx])
        {
            b.dx = 0;
            b.dy--;

            if (b.dy < 0)
            {
		//
		// the next tile is in the next level
		//

                switch (tileDesc.mode)
                {
                  case ONE_LEVEL:
                  case MIPMAP_LEVELS:

                    b.lx++;
                    b.ly++;
                    break;

                  case RIPMAP_LEVELS:

                    b.lx++;

                    if (b.lx >= numXLevels)
                    {
                        b.lx = 0;
                        b.ly++;

			#ifdef DEBUG
			    assert (b.ly <= numYLevels);
			#endif
                    }
                    break;
                }

		if (b.ly < numYLevels)
		    b.dy = numYTiles[b.ly] - 1;
            }
        }
    }
    
    return b;   
}


namespace {

void
writeTileData (TiledOutputFile::Data *ofd,
               int dx, int dy,
	       int lx, int ly, 
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

    ofd->tileOffsets (dx, dy, lx, ly) = currentPosition;

    #ifdef DEBUG
	assert (ofd->os->tellp() == currentPosition);
    #endif

    //
    // Write the tile header.
    //

    Xdr::write <StreamIO> (*ofd->os, dx);
    Xdr::write <StreamIO> (*ofd->os, dy);
    Xdr::write <StreamIO> (*ofd->os, lx);
    Xdr::write <StreamIO> (*ofd->os, ly);
    Xdr::write <StreamIO> (*ofd->os, pixelDataSize);

    ofd->os->write (pixelData, pixelDataSize);    

    //
    // Keep current position in the file so that we can avoid 
    // redundant seekg() operations (seekg() can be fairly expensive).
    //

    ofd->currentPosition = currentPosition +
                           5 * Xdr::size<int>() +
                           pixelDataSize;
}



void
bufferedTileWrite (TiledOutputFile::Data *ofd,
                   int dx, int dy,
		   int lx, int ly, 
                   const char pixelData[],
                   int pixelDataSize)
{
    //
    // If tiles can be written randomly, then don't buffer anything.
    //
    
    if (ofd->lineOrder == RANDOM_Y)
    {
        writeTileData (ofd, dx, dy, lx, ly, pixelData, pixelDataSize);
        return;
    }
    
    //
    // If all the tiles before this one have already been written to the file,
    // then write this tile immediately and check if we have buffered tiles
    // that can be written after this tile.
    //
    // Otherwise, buffer the tile so it can be written to file later.
    //
    
    TileCoord currentTile = TileCoord(dx, dy, lx, ly);
    
    if (ofd->nextTileToWrite == currentTile)
    {
        writeTileData (ofd, dx, dy, lx, ly, pixelData, pixelDataSize);        
        ofd->nextTileToWrite = ofd->nextTileCoord (ofd->nextTileToWrite);

        TileMap::iterator i = ofd->tileMap.find (ofd->nextTileToWrite);
        
        //
        // Step through the tiles and write all successive buffered tiles after
        // the current one.
        //
        
        while(i != ofd->tileMap.end())
        {
            //
            // Write the tile, and then delete the tile's buffered data
            //

            writeTileData (ofd,
			   i->first.dx, i->first.dy,
			   i->first.lx, i->first.ly,
			   i->second->pixelData,
			   i->second->pixelDataSize);

            delete i->second;
            ofd->tileMap.erase (i);
            
            //
            // Proceed to the next tile
            //
            
            ofd->nextTileToWrite = ofd->nextTileCoord (ofd->nextTileToWrite);
            i = ofd->tileMap.find (ofd->nextTileToWrite);
        }
    }
    else
    {
        //
        // Create a new BufferedTile, copy the pixelData into it, and
        // insert it into the tileMap.
        //

	ofd->tileMap[currentTile] =
	    new BufferedTile ((const char *)pixelData, pixelDataSize);
    }
}


void
convertToXdr (TiledOutputFile::Data *ofd,
	      int numScanLines,
	      int numPixelsPerScanLine)
{
    //
    // Convert the contents of a TiledOutputFile's tileBuffer from the 
    // machine's native representation to Xdr format. This function is called
    // by writeTile(), below, if the compressor wanted its input pixel data
    // in the machine's native format, but then failed to compress the data
    // (most compressors will expand rather than compress random input data).
    //
    // Note that this routine assumes that the machine's native representation
    // of the pixel data has the same size as the Xdr representation.  This
    // makes it possible to convert the pixel data in place, without an
    // intermediate temporary buffer.
    //

    //
    // Set these to point to the start of the tile.
    // We will write to toPtr, and read from fromPtr.
    //

    char *toPtr = ofd->tileBuffer;
    const char *fromPtr = toPtr;

    //
    // Iterate over all scan lines in the tile.
    //

    for (int y = 0; y < numScanLines; ++y)
    {
	//
	// Iterate over all slices in the file.
	//

	for (unsigned int i = 0; i < ofd->slices.size(); ++i)
	{
	    const TOutSliceInfo &slice = ofd->slices[i];

	    //
	    // Convert the samples in place.
	    //
		
	    switch (slice.type)
	    {
	      case UINT:

		for (int j = 0; j < numPixelsPerScanLine; ++j)
		{
		    Xdr::write <CharPtrIO>
			(toPtr, *(const unsigned int *) fromPtr);
		    fromPtr += sizeof(unsigned int);
		}
		break;

	      case HALF:

		for (int j = 0; j < numPixelsPerScanLine; ++j)
		{
		    Xdr::write <CharPtrIO>
			(toPtr, *(const half *) fromPtr);
		    fromPtr += sizeof(half);
		}
		break;

	      case FLOAT:

		for (int j = 0; j < numPixelsPerScanLine; ++j)
		{
		    Xdr::write <CharPtrIO>
			(toPtr, *(const float *) fromPtr);
		    fromPtr += sizeof(float);
		}
		break;

	      default:

		throw Iex::ArgExc ("Unknown pixel data type.");
	    }
	}
    }

    #ifdef DEBUG
	assert (toPtr == fromPtr);
    #endif
}

} // namespace


TiledOutputFile::TiledOutputFile (const char fileName[], const Header &header):
    _data (new Data (true))
{
    try
    {
	header.sanityCheck(true);
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


TiledOutputFile::TiledOutputFile (OStream &os, const Header &header):
    _data (new Data (false))
{
    try
    {
	header.sanityCheck(true);
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
TiledOutputFile::initialize (const Header &header)
{
    _data->header = header;
    _data->lineOrder = _data->header.lineOrder();

    //
    // Check that the file is indeed tiled
    //

    _data->tileDesc = _data->header.tileDescription();

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
    
    //
    // Determine the first tile coordinate that we will be writing
    // if the file is not RANDOM_Y.
    //
    
    _data->nextTileToWrite = (_data->lineOrder == INCREASING_Y)?
			       TileCoord (0, 0, 0, 0):
			       TileCoord (0, _data->numYTiles[0] - 1, 0, 0);

    _data->maxBytesPerTileLine =
	    calculateBytesPerPixel (_data->header) * _data->tileDesc.xSize;

    _data->compressor =
	    newTileCompressor (_data->header.compression(),
			       _data->maxBytesPerTileLine,
			       tileYSize(),
			       _data->header);

    _data->format = _data->compressor?
			_data->compressor->format():
			Compressor::XDR;

    _data->tileBufferSize = _data->maxBytesPerTileLine * tileYSize();
    _data->tileBuffer.resizeErase (_data->tileBufferSize);

    _data->tileOffsets = TileOffsets (_data->tileDesc.mode,
				      _data->numXLevels,
				      _data->numYLevels,
				      _data->numXTiles,
				      _data->numYTiles);

    _data->previewPosition =
	_data->header.writeTo(*_data->os, true);

    _data->tileOffsetsPosition = _data->tileOffsets.writeTo (*_data->os);
    _data->currentPosition = _data->os->tellp();
}


TiledOutputFile::~TiledOutputFile ()
{
    if (_data)
    {
        if (_data->tileOffsetsPosition > 0)
        {
            try
            {
                _data->os->seekp (_data->tileOffsetsPosition);
                _data->tileOffsets.writeTo (*_data->os);
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
TiledOutputFile::fileName () const
{
    return _data->os->fileName();
}


const Header &
TiledOutputFile::header () const
{
    return _data->header;
}


void	
TiledOutputFile::setFrameBuffer (const FrameBuffer &frameBuffer)
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

	if (j.slice().xSampling != 1 || j.slice().ySampling != 1)
	{
	    THROW (Iex::ArgExc, "All channels in a tiled file must have"
				"sampling (1,1).");
	}
    }
    
    //
    // Initialize slice table for writePixels().
    //

    vector<TOutSliceInfo> slices;

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

	    slices.push_back (TOutSliceInfo (i.channel().type,
					     0, // base
					     0, // xStride,
					     0, // yStride,
					     true)); // zero
	}
	else
	{
	    //
	    // Channel i is present in the frame buffer.
	    //

	    slices.push_back (TOutSliceInfo (j.slice().type,
					     j.slice().base,
					     j.slice().xStride,
					     j.slice().yStride,
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
TiledOutputFile::frameBuffer () const
{
    return _data->frameBuffer;
}


void	
TiledOutputFile::writeTile (int dx, int dy, int lx, int ly)
{
    try
    {
	if (_data->slices.size() == 0)
	{
	    throw Iex::ArgExc ("No frame buffer specified "
			       "as pixel data source.");
	}

	if (!isValidTile (dx, dy, lx, ly))
	{
	    THROW (Iex::ArgExc, "Tried to write Tile (" << dx << ", " << dy <<
				", " << lx << ", " << ly << "), but that is "
				"not a valid tile coordinate.");
	}
 
	if (_data->tileOffsets (dx, dy, lx, ly) != 0)
	{
	    THROW (Iex::ArgExc, "Tried to write "
				"tile (" << dx << ", " << dy <<
				", " << lx << ", " << ly << ") "
				"more than once.");
	}

	//
	// Convert one tile's worth of pixel data to
	// a machine-independent representation, and store
	// the result in _data->tileBuffer.
	//

	char *toPtr = _data->tileBuffer;

	Box2i tileRange = dataWindowForTile (dx, dy, lx, ly);

	int numScanLines = tileRange.max.y - tileRange.min.y + 1;
	int numPixelsPerScanLine = tileRange.max.x - tileRange.min.x + 1;

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
		const TOutSliceInfo &slice = _data->slices[i];

		//
		// Iterate over the sampled pixels.
		//

		if (slice.zero)
		{
		    //
		    // The frame buffer contains no data for this channel.
		    // Store zeroes in _data->tileBuffer.
		    //

		    if (_data->format == Compressor::XDR)
		    {
			//
			// The compressor expects data in Xdr format.
			//

			switch (slice.type)
			{
			  case UINT:

			    for (int i = 0; i < numPixelsPerScanLine; ++i)
				Xdr::write <CharPtrIO>
				    (toPtr, (unsigned int) 0);
			    break;

			  case HALF:

			    for (int i = 0; i < numPixelsPerScanLine; ++i)
				Xdr::write <CharPtrIO> (toPtr, (half) 0);

			    break;

			  case FLOAT:

			    for (int i = 0; i < numPixelsPerScanLine; ++i)
				Xdr::write <CharPtrIO> (toPtr, (float) 0);

			    break;

			  default:

			    throw Iex::ArgExc ("Unknown pixel data type.");
			}
		    }
		    else
		    {
			//
			// The compressor expects data in
			// the machine's native format.
			//

			switch (slice.type)
			{
			  case UINT:

			    for (int i = 0; i < numPixelsPerScanLine; ++i)
			    {
				static unsigned int ui = 0;

				for (size_t i = 0; i < sizeof (ui); ++i)
				    *toPtr++ = ((char *) &ui)[i];
			    }
			    break;

			  case HALF:

			    for (int i = 0; i < numPixelsPerScanLine; ++i)
			    {
				*(half *) toPtr = half (0);
				toPtr += sizeof (half);
			    }
			    break;

			  case FLOAT:

			    for (int i = 0; i < numPixelsPerScanLine; ++i)
			    {
				static float f = 0;

				for (size_t i = 0; i < sizeof (f); ++i)
				    *toPtr++ = ((char *) &f)[i];
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
		    // The frame buffer contains data for this channel.
		    // If necessary, convert the pixel data to
		    // a machine-independent representation,
		    // and store in _data->tileBuffer.
		    //

		    const char *fromPtr = slice.base +
					  y * slice.yStride +
					  tileRange.min.x * slice.xStride;

		    if (_data->format == Compressor::XDR)
		    {
			//
			// The compressor expects data in Xdr format
			//

			switch (slice.type)
			{
			  case UINT:

			    for (int x = tileRange.min.x;
				 x <= tileRange.max.x;
				 ++x)
			    {
				Xdr::write <CharPtrIO>
				    (toPtr, *(unsigned int *) fromPtr);

				fromPtr += slice.xStride;
			    }
			    
			    break;

			  case HALF:

			    for (int x = tileRange.min.x;
				 x <= tileRange.max.x;
				 ++x)
			    {
				Xdr::write <CharPtrIO>
				    (toPtr, *(half *) fromPtr);

				fromPtr += slice.xStride;
			    }
			    break;

			  case FLOAT:

			    for (int x = tileRange.min.x;
				 x <= tileRange.max.x;
				 ++x)
			    {
				Xdr::write <CharPtrIO>
				    (toPtr, *(float *) fromPtr);

				fromPtr += slice.xStride;
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

			    for (int x = tileRange.min.x;
				 x <= tileRange.max.x;
				 ++x)
			    {
				for (size_t i = 0;
				     i < sizeof (unsigned int);
				     ++i)
				{
				    *toPtr++ = fromPtr[i];
				}

				fromPtr += slice.xStride;
			    }
			    break;

			  case HALF:

			    for (int x = tileRange.min.x;
				 x <= tileRange.max.x;
				 ++x)
			    {
				*(half *)toPtr = *(half *)fromPtr;

				toPtr += sizeof (half);
				fromPtr += slice.xStride;
			    }
			    break;

			  case FLOAT:

			    for (int x = tileRange.min.x;
				 x <= tileRange.max.x;
				 ++x)
			    {
				for (size_t i = 0; i < sizeof (float); ++i)
				    *toPtr++ = fromPtr[i];

				fromPtr += slice.xStride;
			    }
			    break;

			  default:

			    throw Iex::ArgExc ("Unknown pixel data type.");
			}
		    }
		}
	    }
	}

	//
	// Compress the contents of _data->tileBuffer, 
	// and store the compressed data in the output file.
	//
        
	int dataSize = toPtr - _data->tileBuffer;
	const char *dataPtr = _data->tileBuffer;

	if (_data->compressor)
	{
	    const char *compPtr;
	    int compSize = _data->compressor->compressTile
			       (dataPtr, dataSize, tileRange, compPtr);

	    if (compSize < dataSize)
	    {
		dataSize = compSize;
		dataPtr = compPtr;
	    }
	    else if (_data->format == Compressor::NATIVE)
	    {
		//
		// The data did not shrink during compression, but
		// we cannot write to the file using native format,
		// so we need to convert the lineBuffer to Xdr.
		//

		convertToXdr (_data, numScanLines, numPixelsPerScanLine);
	    }
	}

	bufferedTileWrite (_data, dx, dy, lx, ly, dataPtr, dataSize);
    }
    catch (Iex::BaseExc &e)
    {
        REPLACE_EXC (e, "Failed to write pixel data to image "
                        "file \"" << fileName() << "\". " << e);
        throw;
    }
}

void
TiledOutputFile::writeTile (int dx, int dy, int l)
{
    writeTile(dx, dy, l, l);
}


void	
TiledOutputFile::copyPixels (TiledInputFile &in)
{
    //
    // Check if this file's and and the InputFile's
    // headers are compatible.
    //
    const Header &hdr = header();
    const Header &inHdr = in.header(); 

    if (!hdr.hasTileDescription() || !inHdr.hasTileDescription())
    {
        THROW (Iex::ArgExc, "Cannot perform a quick pixel copy from image "
			    "file \"" << in.fileName() << "\" to image "
			    "file \"" << fileName() << "\".  The output file "
			    "is tiled, but the input file is not.  Try using "
			    "OutputFile::copyPixels() instead.");
    }

    if (!(hdr.tileDescription() == inHdr.tileDescription()))
    {
        THROW (Iex::ArgExc, "Quick pixel copy from image "
			    "file \"" << in.fileName() << "\" to image "
			    "file \"" << fileName() << "\" failed. "
			    "The files have different tile descriptions.");
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

    if (!_data->tileOffsets.isEmpty())
    {
        THROW (Iex::LogicExc, "Quick pixel copy from image "
			      "file \"" << in.fileName() << "\" to image "
			      "file \"" << fileName() << "\" failed. "
			      "\"" << fileName() << "\" already contains "
			      "pixel data.");
    }

    //
    // Calculate the total number of tiles in the file
    //

    int numAllTiles = 0;

    switch (levelMode())
    {
      case ONE_LEVEL:
      case MIPMAP_LEVELS:

        for (size_t i_l = 0; i_l < numLevels(); ++i_l)
            numAllTiles += numXTiles (i_l) * numYTiles (i_l);

        break;

      case RIPMAP_LEVELS:

        for (size_t i_ly = 0; i_ly < numYLevels(); ++i_ly)
            for (size_t i_lx = 0; i_lx < numXLevels(); ++i_lx)
                numAllTiles += numXTiles (i_lx) * numYTiles (i_ly);

        break;

      default:

        throw Iex::ArgExc ("Unknown LevelMode format.");
    }

    for (int i = 0; i < numAllTiles; ++i)
    {
        const char *pixelData;
        int pixelDataSize;

        int dx = _data->nextTileToWrite.dx;
        int dy = _data->nextTileToWrite.dy;
        int lx = _data->nextTileToWrite.lx;
        int ly = _data->nextTileToWrite.ly;

        in.rawTileData (dx, dy, lx, ly, pixelData, pixelDataSize);
        writeTileData (_data, dx, dy, lx, ly, pixelData, pixelDataSize);
    }
}


void	
TiledOutputFile::copyPixels (InputFile &in)
{
    copyPixels (*in.tFile());
}


unsigned int
TiledOutputFile::tileXSize () const
{
    return _data->tileDesc.xSize;
}


unsigned int
TiledOutputFile::tileYSize () const
{
    return _data->tileDesc.ySize;
}


LevelMode
TiledOutputFile::levelMode () const
{
    return _data->tileDesc.mode;
}


LevelRoundingMode
TiledOutputFile::levelRoundingMode () const
{
    return _data->tileDesc.roundingMode;
}


int
TiledOutputFile::numLevels () const
{
    if (levelMode() == RIPMAP_LEVELS)
    {
	THROW (Iex::LogicExc, "Error calling numLevels() on image "
			      "file \"" << fileName() << "\" "
			      "(numLevels() is not defined for RIPMAPs).");
    }

    return _data->numXLevels;
}


int
TiledOutputFile::numXLevels () const
{
    return _data->numXLevels;
}


int
TiledOutputFile::numYLevels () const
{
    return _data->numYLevels;
}


bool	
TiledOutputFile::isValidLevel (int lx, int ly) const
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
TiledOutputFile::levelWidth (int lx) const
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
TiledOutputFile::levelHeight (int ly) const
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
TiledOutputFile::numXTiles (int lx) const
{
    if (lx < 0 || lx >= numXLevels())
    {
	THROW (Iex::LogicExc, "Error calling numXTiles() on image "
			      "file \"" << fileName() << "\" "
			      "(Argument is not in valid range).");
    }

    return _data->numXTiles[lx];
}


int
TiledOutputFile::numYTiles (int ly) const
{
    if (ly < 0 || ly >= numYLevels())
    {
	THROW (Iex::LogicExc, "Error calling numYTiles() on image "
			      "file \"" << fileName() << "\" "
			      "(Argument is not in valid range).");
    }

    return _data->numYTiles[ly];
}


Box2i
TiledOutputFile::dataWindowForLevel (int l) const
{
    return dataWindowForLevel (l, l);
}


Box2i
TiledOutputFile::dataWindowForLevel (int lx, int ly) const
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
TiledOutputFile::dataWindowForTile (int dx, int dy, int l) const
{
    return dataWindowForTile (dx, dy, l, l);
}


Box2i
TiledOutputFile::dataWindowForTile (int dx, int dy, int lx, int ly) const
{
    try
    {
	if (!isValidTile (dx, dy, lx, ly))
	    throw Iex::ArgExc ("Arguments not in valid range.");

	return Imf::dataWindowForTile (_data->tileDesc,
				       _data->minX, _data->maxX,
				       _data->minY, _data->maxY,
				       dx, dy,
				       lx, ly);
    }
    catch (Iex::BaseExc &e)
    {
	REPLACE_EXC (e, "Error calling dataWindowForTile() on image "
			"file \"" << fileName() << "\". " << e);
	throw;
    }
}


bool
TiledOutputFile::isValidTile(int dx, int dy, int lx, int ly) const
{
    return ((lx < numXLevels() && lx >= 0) &&
	    (ly < numYLevels() && ly >= 0) &&
	    (dx < numXTiles(lx) && dx >= 0) &&
	    (dy < numYTiles(ly) && dy >= 0));
}


void
TiledOutputFile::updatePreviewImage (const PreviewRgba newPixels[])
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
