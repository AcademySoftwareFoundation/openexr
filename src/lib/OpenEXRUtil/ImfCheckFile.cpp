// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include "ImfCheckFile.h"
#include "Iex.h"
#include "ImfRgbaFile.h"
#include "ImfArray.h"
#include "ImfChannelList.h"
#include "ImfFrameBuffer.h"
#include "ImfPartType.h"
#include "ImfInputPart.h"
#include "ImfDeepScanLineInputPart.h"
#include "ImfTiledInputPart.h"
#include "ImfDeepTiledInputPart.h"
#include "ImfStdIO.h"
#include "ImfMultiPartInputFile.h"
#include "ImfStandardAttributes.h"

#include <vector>
#include <algorithm>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


namespace {

using std::vector;
using std::max;
using Imath::Box2i;

//
// limits for reduceMemory mode
//
const Int64 gMaxScanlineWidth= 1000000;
const Int64 gMaxTilePixelsPerScanline = 8000000;
const Int64 gMaxTileSize = 1000*1000;
const Int64 gMaxSamplesPerDeepPixel = 1000;
const Int64 gMaxSamplesPerScanline = 1<<12;

//
// limits for reduceTime mode
//
const int gTargetPixelsToRead = 1<<28;
const int gMaxScanlinesToRead = 1<<20;




//
// compute row stride appropriate to process files quickly
// only used for the 'Rgba' interfaces, which read potentially non-existant channels
//
//

int
getStep( const Box2i &dw , bool reduceTime)
{

    if (reduceTime)
    {
        size_t rowCount = (dw.max.y - dw.min.y + 1);
        size_t pixelCount = rowCount * (dw.max.x - dw.min.x + 1);
        return  max( 1 , max ( static_cast<int>(pixelCount / gTargetPixelsToRead) , static_cast<int>(rowCount / gMaxScanlinesToRead) ) );
    }
    else
    {
        return 1;
    }

}
//
// read image or part using the Rgba interface
//
template<class T> bool
readRgba(T& in, bool reduceMemory , bool reduceTime)
{

    bool threw = false;

    try
    {
        const Box2i &dw = in.dataWindow();

        int w = dw.max.x - dw.min.x + 1;
        int dx = dw.min.x;

        if (reduceMemory && w > gMaxScanlineWidth )
        {
            return false;
        }

        Array<Rgba> pixels (w);
        intptr_t base = reinterpret_cast<intptr_t>(&pixels[0]);
        in.setFrameBuffer (reinterpret_cast<Rgba*>(base - dx*sizeof(Rgba)), 1, 0);

        int step = getStep( dw , reduceTime );

        //
        // try reading scanlines. Continue reading scanlines
        // even if an exception is encountered
        //
        for (int y = dw.min.y; y <= dw.max.y; y+=step )
        {
            try
            {
               in.readPixels (y);
            }
            catch(...)
            {
                threw = true;

                //
                // in reduceTime mode, fail immediately - the file is corrupt
                //
                if (reduceTime)
                {
                    return threw;
                }

            }
        }
    }
    catch(...)
    {
        threw = true;
    }

    return threw;
}


template<class T> bool
readScanline(T& in, bool reduceMemory , bool reduceTime)
{

    bool threw = false;

    try
    {
        const Box2i &dw = in.header().dataWindow();

        int w = dw.max.x - dw.min.x + 1;
        int dx = dw.min.x;

        if (reduceMemory && w > gMaxScanlineWidth )
        {
            return false;
        }

        FrameBuffer i;


        // read all channels present (later channels will overwrite earlier ones)
        vector<half> halfChannels(w);
        vector<float> floatChannels(w);
        vector<unsigned int> uintChannels(w);

        intptr_t halfData = reinterpret_cast<intptr_t>(halfChannels.data());
        intptr_t floatData = reinterpret_cast<intptr_t>(floatChannels.data());
        intptr_t uintData = reinterpret_cast<intptr_t>(uintChannels.data());

        int channelIndex = 0;
        const ChannelList& channelList = in.header().channels();
        for (ChannelList::ConstIterator c = channelList.begin() ; c != channelList.end() ; ++c )
        {
            switch (channelIndex % 3)
            {
                case 0 : i.insert(c.name(),Slice(HALF, (char*) (halfData - sizeof(half)*(dx/c.channel().xSampling))  , sizeof(half) , 0 , c.channel().xSampling , c.channel().ySampling ));
                break;
                case 1 : i.insert(c.name(),Slice(FLOAT, (char*) (floatData - sizeof(float)*(dx/c.channel().xSampling))  , sizeof(float) , 0 , c.channel().xSampling , c.channel().ySampling ));
                break;
                case 2 : i.insert(c.name(),Slice(UINT, (char*) (uintData - sizeof(unsigned int)*(dx/c.channel().xSampling))  , sizeof(unsigned int) , 0 , c.channel().xSampling , c.channel().ySampling ));
                break;
            }
            channelIndex ++;
        }

        in.setFrameBuffer(i);

        int step = 1;

        //
        // try reading scanlines. Continue reading scanlines
        // even if an exception is encountered
        //
        for (int y = dw.min.y; y <= dw.max.y; y+=step )
        {
            try
            {
               in.readPixels (y);
            }
            catch(...)
            {
                threw = true;

                //
                // in reduceTime mode, fail immediately - the file is corrupt
                //
                if (reduceTime)
                {
                    return threw;
                }

            }
        }
    }
    catch(...)
    {
        threw = true;
    }

    return threw;
}


template<class T>
bool
readTileRgba( T& in,bool reduceMemory, bool reduceTime)
{
    try{
        const Box2i &dw = in.dataWindow();

        int w = dw.max.x - dw.min.x + 1;
        int h = dw.max.y - dw.min.y + 1;

        if ( (reduceMemory || reduceTime ) && h*w > gMaxTileSize )
        {
            return false;
        }

        int dwx = dw.min.x;
        int dwy = dw.min.y;



        Array2D<Rgba> pixels (h, w);
        in.setFrameBuffer (&pixels[-dwy][-dwx], 1, w);
        in.readTiles (0, in.numXTiles() - 1, 0, in.numYTiles() - 1);
    }
    catch(...)
    {
       return true;
    }

    return false;
}


// read image as ripmapped image
template<class T>
bool
readTile(T& in, bool reduceMemory , bool reduceTime)
{
    bool threw = false;
    try
    {
        const Box2i& dw = in.header().dataWindow();

        int w = dw.max.x - dw.min.x + 1;
        int dwx = dw.min.x;
        int numXLevels = in.numXLevels();
        int numYLevels = in.numYLevels();

        const TileDescription& td = in.header().tileDescription();


        if (reduceMemory && (w > gMaxScanlineWidth || (td.xSize*td.ySize) > gMaxTileSize) )
        {
                return false;
        }

        FrameBuffer i;
        // read all channels present (later channels will overwrite earlier ones)
        vector<half> halfChannels(w);
        vector<float> floatChannels(w);
        vector<unsigned int> uintChannels(w);

        int channelIndex = 0;
        const ChannelList& channelList = in.header().channels();
        for (ChannelList::ConstIterator c = channelList.begin() ; c != channelList.end() ; ++c )
        {
            switch (channelIndex % 3)
            {
                case 0 : i.insert(c.name(),Slice(HALF, (char*)&halfChannels[-dwx / c.channel().xSampling ] , sizeof(half) , 0 , c.channel().xSampling , c.channel().ySampling ));
                break;
                case 1 : i.insert(c.name(),Slice(FLOAT, (char*)&floatChannels[-dwx / c.channel().xSampling ] , sizeof(float) , 0 ,  c.channel().xSampling , c.channel().ySampling));
                case 2 : i.insert(c.name(),Slice(UINT, (char*)&uintChannels[-dwx / c.channel().xSampling ] , sizeof(unsigned int) , 0 , c.channel().xSampling , c.channel().ySampling));
                break;
            }
            channelIndex ++;
        }

        in.setFrameBuffer (i);

        size_t step = 1;

        size_t tileIndex =0;
        bool isRipMap = td.mode == RIPMAP_LEVELS;

        //
        // read all tiles from all levels.
        //
        for (int ylevel = 0; ylevel < numYLevels; ++ylevel )
        {
            for (int xlevel = 0; xlevel < numXLevels; ++xlevel )
            {
                for(int y  = 0 ; y < in.numYTiles(ylevel) ; ++y )
                {
                    for(int x = 0 ; x < in.numXTiles(xlevel) ; ++x )
                    {
                        if(tileIndex % step == 0)
                        {
                            try
                            {
                                in.readTile ( x, y, xlevel , ylevel);
                            }
                            catch(...)
                            {
                                //
                                // for one level and mipmapped images,
                                // xlevel must match ylevel,
                                // otherwise an exception is thrown
                                // ignore that exception
                                //
                                if (isRipMap || xlevel==ylevel)
                                {
                                    threw = true;

                                    //
                                    // in reduceTime mode, fail immediately - the file is corrupt
                                    //
                                    if (reduceTime)
                                    {
                                        return threw;
                                    }
                                }
                            }
                        }
                        tileIndex++;
                    }
                }
            }
        }
    }
    catch(...)
    {
        threw = true;
    }

    return threw;
}

template<class T>
bool readDeepScanLine(T& in,bool reduceMemory, bool reduceTime)
{

    bool threw = false;
    try
    {
        const Header& fileHeader = in.header();
        const Box2i &dw = fileHeader.dataWindow();


        int w = dw.max.x - dw.min.x + 1;
        int dwx = dw.min.x;

        if ( reduceMemory && w > gMaxScanlineWidth )
        {
            return false;
        }



        int channelCount=0;
        for(ChannelList::ConstIterator i=fileHeader.channels().begin();i!=fileHeader.channels().end();++i,++channelCount);

        Array<unsigned int> localSampleCount;
        localSampleCount.resizeErase(w );
        Array<Array< void* > > data(channelCount);


        for (int i = 0; i < channelCount; i++)
        {
            data[i].resizeErase( w );
        }

        DeepFrameBuffer frameBuffer;

        frameBuffer.insertSampleCountSlice (Slice (UINT,(char *) (&localSampleCount[-dwx]), sizeof (unsigned int) ,0) );

        int channel =0;
        for(ChannelList::ConstIterator i=fileHeader.channels().begin();
            i!=fileHeader.channels().end();++i , ++channel)
        {
            PixelType type = FLOAT;

            int sampleSize = sizeof (float);

            int pointerSize = sizeof (char *);

            frameBuffer.insert (i.name(), DeepSlice (type,(char *) (&data[channel][-dwx]),pointerSize , 0, sampleSize));
        }



        in.setFrameBuffer(frameBuffer);

        int step = 1;

        vector<float> pixelBuffer;

        for (int y = dw.min.y ; y <= dw.max.y ; y+=step )
        {
            in.readPixelSampleCounts( y );


            //
            // count how many samples are required to store this scanline
            //
            size_t bufferSize = 0;
            for (int j = 0; j < w ; j++)
            {
                for (int k = 0; k < channelCount; k++)
                {
                    //
                    // don't read samples which require a lot of memory in reduceMemory mode
                    //
                    if (!reduceMemory || localSampleCount[j] <= gMaxSamplesPerDeepPixel )
                    {
                        bufferSize += localSampleCount[j];
                    }
                }
            }

            //
            // limit total number of samples read in reduceMemory mode
            //
            if (!reduceMemory || bufferSize < gMaxScanlineWidth )
            {
                //
                // allocate sample buffer and set per-pixel pointers into buffer
                //
                pixelBuffer.resize(bufferSize);

                size_t bufferIndex = 0;
                for (int j = 0; j < w ; j++)
                {
                    for (int k = 0; k < channelCount; k++)
                    {

                        if (localSampleCount[j]==0 || ( reduceMemory && localSampleCount[j] > gMaxSamplesPerDeepPixel ) )
                        {
                            data[k][j] = nullptr;
                        }
                        else
                        {
                            data[k][j] = &pixelBuffer[bufferIndex];
                            bufferIndex += localSampleCount[j];
                        }
                    }
                }

                try
                {
                    in.readPixels(y);
                }
                catch(...)
                {
                    threw = true;
                    //
                    // in reduceTime mode, fail immediately - the file is corrupt
                    //
                    if (reduceTime)
                    {
                        return threw;
                    }
                }
            }

        }
    }
    catch(...)
    {
        threw = true;
    }
    return threw;

}

//
// read a deep tiled image, tile by tile, using the 'tile relative' mode
//
template<class T> bool
readDeepTile(T& in,bool reduceMemory , bool reduceTime)
{
    bool threw = false;
    try
    {
        const Header& fileHeader = in.header();

        Array2D<unsigned int> localSampleCount;

        Box2i dataWindow = fileHeader.dataWindow();

        //
        // use Int64 for dimensions, since dataWindow+1 could overflow int storage
        //
        Int64 height = static_cast<Int64>(dataWindow.size().y)+1;
        Int64 width = static_cast<Int64>(dataWindow.size().x)+1;

        const TileDescription& td = in.header().tileDescription();
        int tileWidth = td.xSize;
        int tileHeight = td.ySize;
        int numYLevels = in.numYLevels();
        int numXLevels = in.numXLevels();


        localSampleCount.resizeErase(height, width);

        int channelCount=0;
        for(ChannelList::ConstIterator i=fileHeader.channels().begin();i!=fileHeader.channels().end();++i, channelCount++);

        Array<Array2D< float* > > data(channelCount);

        for (int i = 0; i < channelCount; i++)
        {
            data[i].resizeErase(height, width);
        }

        DeepFrameBuffer frameBuffer;

        //
        // memOffset is difference in bytes between theoretical address of pixel (0,0) and the origin of the data window
        //
        Int64 memOffset = sizeof(unsigned int) * (static_cast<Int64>(dataWindow.min.x) + static_cast<Int64>(dataWindow.min.y) * width);

        //
        // Use integer arithmetic instead of pointer arithmetic to compute offset into array.
        // if memOffset is larger than base, then the computed pointer is negative, which is reported as undefined behavior
        // Instead, integers are used for computation which behaves as expected an all known architectures
        //

        intptr_t base = reinterpret_cast<intptr_t>(&localSampleCount[0][0] );
        frameBuffer.insertSampleCountSlice (Slice (UINT,
                                                   reinterpret_cast<char*> (base - memOffset),
                                                   sizeof (unsigned int) * 1,
                                                   sizeof (unsigned int) * width,
                                                   0.0, // fill
                                                   1 , 1, // x/ysampling
                                                   true,  // relative x
                                                   true  // relative y
                                                  )
                                                   );

        int channel = 0;
         for (ChannelList::ConstIterator i=fileHeader.channels().begin();i!=fileHeader.channels().end();++i, ++channel)
         {
             int sampleSize  = sizeof (float);

             int pointerSize = sizeof (char *);
             intptr_t base = reinterpret_cast<intptr_t>(&data[channel][0][0]);

             frameBuffer.insert (i.name(),
                                 DeepSlice (FLOAT,
                                            reinterpret_cast<char*> (base- memOffset),
                                            pointerSize * 1,
                                            pointerSize * width,
                                            sampleSize,
                                            0.0,
                                            1 , 1,
                                            true,
                                            true
                                           ) );
         }

         in.setFrameBuffer(frameBuffer);
         size_t step = 1;

         int tileIndex = 0;
         bool isRipMap = td.mode == RIPMAP_LEVELS;


         vector<float> pixelBuffer;


         for (int ly = 0; ly < numYLevels; ly++)
         {
             for (int lx = 0; lx < numXLevels; lx++)
             {


                //
                // read all tiles from all levels.
                //
                for (int ylevel = 0; ylevel < numYLevels; ++ylevel )
                {
                    for (int xlevel = 0; xlevel < numXLevels; ++xlevel )
                    {
                        for(int y  = 0 ; y < in.numYTiles(ylevel) ; ++y )
                        {
                            for(int x = 0 ; x < in.numXTiles(xlevel) ; ++x )
                            {
                                if(tileIndex % step == 0)
                                {
                                    try
                                    {

                                        in.readPixelSampleCounts( x , y , x , y, lx, ly);


                                        size_t bufferSize = 0;

                                        for (int ty = 0 ; ty < tileHeight ; ++ty )
                                        {
                                            for (int tx = 0 ; tx < tileWidth ; ++tx )
                                            {
                                                if (!reduceMemory || localSampleCount[ty][tx] < gMaxSamplesPerDeepPixel )
                                                {
                                                    bufferSize += channelCount * localSampleCount[ty][tx];
                                                }
                                            }
                                        }

                                        // limit total samples allocated for this tile
                                        if (!reduceMemory || bufferSize < gMaxSamplesPerScanline )
                                        {

                                            pixelBuffer.resize( bufferSize );
                                            size_t bufferIndex = 0;

                                            for (int ty = 0 ; ty < tileHeight ; ++ty )
                                            {
                                                for (int tx = 0 ; tx < tileWidth ; ++tx )
                                                {
                                                    if (!reduceMemory || localSampleCount[ty][tx] <  gMaxSamplesPerDeepPixel )
                                                    {
                                                        for (int k = 0 ; k < channelCount ; ++k )
                                                        {
                                                           data[k][ty][tx] = &pixelBuffer[bufferIndex];
                                                           bufferIndex += localSampleCount[ty][tx];
                                                        }
                                                    }
                                                    else
                                                    {
                                                        for (int k = 0 ; k < channelCount ; ++k )
                                                        {
                                                            data[k][ty][tx] = nullptr;
                                                        }
                                                    }
                                                }
                                            }


                                            in.readTile ( x, y, xlevel , ylevel);
                                        }
                                    }

                                    catch(...)
                                    {
                                        //
                                        // for one level and mipmapped images,
                                        // xlevel must match ylevel,
                                        // otherwise an exception is thrown
                                        // ignore that exception
                                        //
                                        if (isRipMap || xlevel==ylevel)
                                        {
                                            threw = true;
                                            //
                                            // in reduceTime mode, fail immediately - the file is corrupt
                                            //
                                            if (reduceTime)
                                            {
                                                return threw;
                                            }
                                        }

                                    }
                                }
                                tileIndex++;
                            }
                        }
                    }
                }
             }
         }
    }catch(...)
    {
        threw = true;
    }
    return threw;
}

//
// EXR will read files that have out-of-range values in certain enum attributes, to allow
// values to be added in the future. This function returns 'false' if any such enum attributes
// have unknown values
//
// (implementation node: it is undefined behavior to set an enum variable to an invalid value
//  this code circumvents that by casting the enums to integers and checking them that way)
//
bool enumsValid( const Header& hdr)
{
    if ( hasEnvmap (hdr) )
    {

        const Envmap& typeInFile = envmap (hdr);
        if (typeInFile != ENVMAP_LATLONG && typeInFile!= ENVMAP_CUBE)
        {
             return false;
        }
    }

    if (hasDeepImageState(hdr))
    {
        const DeepImageState& typeInFile = deepImageState (hdr);
        if (typeInFile < 0 || typeInFile >= DIS_NUMSTATES)
        {
             return false;
        }
    }

    return true;
}

bool
readMultiPart(MultiPartInputFile& in,bool reduceMemory,bool reduceTime)
{
    bool threw = false;
    for(int part = 0 ; part < in.parts() ; ++ part)
    {

       if (!enumsValid( in.header(part)))
       {
           threw = true;
       }

        bool widePart = false;
        bool largeTiles = false;
        Box2i b = in.header( part ).dataWindow();
        Int64 imageWidth = static_cast<Int64>(b.max.x) - static_cast<Int64>(b.min.x) + 1ll;

         //
         // very wide scanline parts take excessive memory to read.
         // detect that here so that tests can be skipped when reduceMemory is set
         //


        if ( imageWidth > gMaxScanlineWidth )
        {
            widePart = true;

        }
        //
        // significant memory is also required to read a tiled part
        // using the scanline interface with tall tiles - the scanlineAPI
        // needs to allocate memory to store an entire row of tiles
        //
        if (isTiled(in.header( part ).type()))
        {
            const TileDescription& tileDescription = in.header( part ).tileDescription();

            Int64 tilesPerScanline = ( imageWidth + tileDescription.xSize - 1ll) / tileDescription.xSize;
            Int64 tileSize = static_cast<Int64>(tileDescription.xSize) * static_cast<Int64>(tileDescription.ySize);

            if ( tileSize * tilesPerScanline > gMaxTilePixelsPerScanline )
            {
                widePart = true;
            }
            if( tileSize > gMaxTileSize)
            {
                 largeTiles = true;
            }
        }

       if (!reduceMemory || !widePart)
       {
            bool gotThrow = false;
            try
            {
                InputPart pt( in , part );
                gotThrow = readScanline( pt , reduceMemory , reduceTime);
            }
            catch(...)
            {
                gotThrow = true;
            }
            // only 'DeepTiled' parts are expected to throw
            // all others are an error
            if( gotThrow && in.header(part).type() != DEEPTILE )
            {
                threw = true;
            }
       }

        if (!reduceMemory || !largeTiles)
        {
            bool gotThrow = false;

            try
            {
                in.flushPartCache();
                TiledInputPart pt (in,part);
                gotThrow = readTile( pt , reduceMemory , reduceTime);
            }
            catch(...)
            {
                gotThrow = true;
            }

            if( gotThrow && in.header(part).type() == TILEDIMAGE)
            {
                threw = true;
            }
       }


       if (!reduceMemory || !widePart)
       {
            bool gotThrow = false;

            try
            {
                in.flushPartCache();
                DeepScanLineInputPart pt (in,part);
                gotThrow = readDeepScanLine( pt , reduceMemory , reduceTime);
            }
            catch(...)
            {
                gotThrow = true;
            }

            if( gotThrow && in.header(part).type() == DEEPSCANLINE)
            {
                threw = true;
            }
       }

       if (!reduceMemory || !largeTiles)
       {
            bool gotThrow = false;

            try
            {
                in.flushPartCache();
                DeepTiledInputPart pt (in,part);
                gotThrow = readDeepTile( pt , reduceMemory , reduceTime);
            }
            catch(...)
            {
                gotThrow = true;
            }

            if( gotThrow && in.header(part).type() == DEEPTILE)
            {
                threw = true;
            }
       }
    }

    return threw;
}


//------------------------------------------------
// class PtrIStream -- allow reading an EXR file from
// a pointer
//
//------------------------------------------------

class PtrIStream: public IStream
{
  public:

    PtrIStream (const char* data, size_t nBytes) : IStream("none") , base(data) , current(data) , end(data+nBytes) {}

    virtual bool        isMemoryMapped () const { return false;}


    IMFUTIL_EXPORT
    virtual char *	readMemoryMapped (int n)
    {

        if (n + current > end)
	{
		THROW (IEX_NAMESPACE::InputExc, "Early end of file: requesting " << end - (n+current) << " extra bytes after file\n");
	}
	const char* value = current;
        current += n;

        return const_cast<char*>(value);
    }

    virtual bool	read (char c[/*n*/], int n)
    {
        if( n < 0 )
        {
             	THROW (IEX_NAMESPACE::InputExc,n << " bytes requested from stream");
        }

        if (n + current > end)
	{
		THROW (IEX_NAMESPACE::InputExc, "Early end of file: requesting " << end - (n+current) << " extra bytes after file\n");
	}
	memcpy( c , current , n);
        current += n;

        return (current != end);

    }

    virtual Int64	tellg ()
    {
        return (current - base);
    }
    virtual void	seekg (Int64 pos)
    {

        if( pos < 0 )
        {
          THROW (IEX_NAMESPACE::InputExc, "internal error: seek to " << pos << " requested");
        }

        const char* newcurrent = base + pos;

        if( newcurrent < base || newcurrent > end)
        {
            THROW (IEX_NAMESPACE::InputExc, "Out of range seek requested\n");
        }

        current = newcurrent;

    }

  private:

    const char*        base;
    const char*        current;
    const char*        end;
};



void resetInput(const char* /*fileName*/)
{
    // do nothing: filename doesn't need to be 'reset' between calls
}

void resetInput(PtrIStream& stream)
{
    // return stream to beginning to prepare reading with a different API
    stream.seekg(0);
}



template<class T> bool
runChecks(T& source,bool reduceMemory,bool reduceTime)
{
    //
    // multipart test: also grab the type of the first part to
    // check which other tests are expected to fail
    // check the image width for the first part - significant memory
    // is required to process wide parts
    //

    string firstPartType;


    //
    // scanline images with very wide parts and tiled images with large tiles
    // take excessive memory to read.
    // Assume the first part requires excessive memory until the header of the first part is checked
    // so the single part input APIs can be skipped.
    //
    // If the MultiPartInputFile constructor throws an exception, the first part
    // will assumed to be a wide image
    //
    bool firstPartWide = true;
    bool largeTiles = true;

    bool threw = false;
    {
      try
      {
         MultiPartInputFile multi(source);
         Box2i b = multi.header(0).dataWindow();
         Int64 imageWidth = static_cast<Int64>(b.max.x) - static_cast<Int64>(b.min.x) + 1ll;

         // confirm first part is small enough to read without using excessive memory
         if ( imageWidth <= gMaxScanlineWidth )
         {
             firstPartWide = false;
         }


         //
         // significant memory is also required to read a tiled file
         // using the scanline interface with tall tiles - the scanlineAPI
         // needs to allocate memory to store an entire row of tiles
         //

         firstPartType = multi.header(0).type();
         if (isTiled(firstPartType))
         {
             const TileDescription& tileDescription = multi.header(0).tileDescription();
             Int64 tilesPerScanline = ( imageWidth + tileDescription.xSize - 1ll) / tileDescription.xSize;
             Int64 tileSize = static_cast<Int64>(tileDescription.xSize) * static_cast<Int64>(tileDescription.ySize);
             if ( tileSize * tilesPerScanline > gMaxTilePixelsPerScanline )
             {
                 firstPartWide = true;
             }

             if( tileSize <= gMaxTileSize)
             {
                 largeTiles = false;
             }

         }
         else
         {
             // file is not tiled, so can't contain large tiles
             // setting largeTiles false here causes the Tile and DeepTile API
             // tests to run on non-tiled files, which should cause exceptions to be thrown
             largeTiles = false;
         }


         threw = readMultiPart(multi , reduceMemory , reduceTime);
      }
      catch(...)
      {
         threw = true;
      }

    }

    // read using both scanline interfaces (unless the image is wide and reduce memory enabled)
    if( !reduceMemory || !firstPartWide)
    {
        {
            bool gotThrow = false;
            resetInput(source);
            try
            {
            RgbaInputFile rgba(source);
            gotThrow = readRgba( rgba, reduceMemory , reduceTime );
            }
            catch(...)
            {
                gotThrow = true;
            }
            if (gotThrow && firstPartType != DEEPTILE)
            {
                threw = true;
            }
        }
        {
            bool gotThrow = false;
            resetInput(source);
            try
            {
            InputFile rgba(source);
            gotThrow = readScanline( rgba, reduceMemory , reduceTime );
            }
            catch(...)
            {
                gotThrow = true;
            }
            if (gotThrow && firstPartType != DEEPTILE)
            {
                threw = true;
            }
        }
    }

    if( !reduceMemory || !largeTiles )
    {
        bool gotThrow = false;
        resetInput(source);
        try
        {
          TiledInputFile rgba(source);
          gotThrow = readTile( rgba, reduceMemory , reduceTime );
        }
        catch(...)
        {
            gotThrow = true;
        }
        if (gotThrow && firstPartType == TILEDIMAGE)
        {
            threw = true;
        }
    }

    if( !reduceMemory || !firstPartWide )
    {
        bool gotThrow = false;
        resetInput(source);
        try
        {
          DeepScanLineInputFile rgba(source);
          gotThrow = readDeepScanLine( rgba, reduceMemory , reduceTime );
        }
        catch(...)
        {
            gotThrow = true;
        }
        if (gotThrow && firstPartType == DEEPSCANLINE)
        {
            threw = true;
        }
    }

    if( !reduceMemory || !largeTiles )
    {
        bool gotThrow = false;
        resetInput(source);
        try
        {
          DeepTiledInputFile rgba(source);
          gotThrow = readDeepTile( rgba, reduceMemory , reduceTime );
        }
        catch(...)
        {
            gotThrow = true;
        }
        if (gotThrow && firstPartType == DEEPTILE)
        {
            threw = true;
        }
    }

    return threw;
}


}


bool
checkOpenEXRFile(const char* fileName, bool reduceMemory,bool reduceTime)
{
   return runChecks( fileName , reduceMemory , reduceTime );
}


bool
checkOpenEXRFile(const char* data, size_t numBytes, bool reduceMemory , bool reduceTime )
{
  PtrIStream stream(data,numBytes);
  return runChecks( stream , reduceMemory , reduceTime );
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
