//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	Miscellaneous helper functions for OpenEXR image file I/O
//
//-----------------------------------------------------------------------------

#include <ImfMisc.h>
#include <ImfHeader.h>
#include <ImfAttribute.h>
#include <ImfCompressor.h>
#include <ImfChannelList.h>
#include <ImfXdr.h>
#include <ImathFun.h>
#include <Iex.h>
#include <ImfStdIO.h>
#include <ImfConvert.h>
#include <ImfPartType.h>
#include <ImfTileDescription.h>
#include "ImfNamespace.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

using IMATH_NAMESPACE::Box2i;
using IMATH_NAMESPACE::divp;
using IMATH_NAMESPACE::modp;
using std::vector;

int
pixelTypeSize (PixelType type)
{
    int size;

    switch (type)
    {
      case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:
	
	size = Xdr::size <unsigned int> ();
	break;

      case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

	size = Xdr::size <half> ();
	break;

      case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

	size = Xdr::size <float> ();
	break;

      default:

	throw IEX_NAMESPACE::ArgExc ("Unknown pixel type.");
    }

    return size;
}


int
numSamples (int s, int a, int b)
{
    int a1 = divp (a, s);
    int b1 = divp (b, s);
    return  b1 - a1 + ((a1 * s < a)? 0: 1);
}


size_t
bytesPerLineTable (const Header &header,
		   vector<size_t> &bytesPerLine)
{
    const Box2i &dataWindow = header.dataWindow();
    const ChannelList &channels = header.channels();

    bytesPerLine.resize (dataWindow.max.y - dataWindow.min.y + 1);

    for (ChannelList::ConstIterator c = channels.begin();
	 c != channels.end();
	 ++c)
    {
	size_t nBytes = size_t(pixelTypeSize (c.channel().type)) *
		     size_t(dataWindow.max.x - dataWindow.min.x + 1) /
		     size_t(c.channel().xSampling);

	for (int y = dataWindow.min.y, i = 0; y <= dataWindow.max.y; ++y, ++i)
	    if (modp (y, c.channel().ySampling) == 0)
		bytesPerLine[i] += nBytes;
    }

    size_t maxBytesPerLine = 0;

    for (int y = dataWindow.min.y, i = 0; y <= dataWindow.max.y; ++y, ++i)
	if (maxBytesPerLine < bytesPerLine[i])
	    maxBytesPerLine = bytesPerLine[i];

    return maxBytesPerLine;
}

static int
roundToNextMultiple(int n, int d)
{
    return ((n + d - 1) / d) * d;
}

static int
roundToPrevMultiple(int n, int d)
{
    return (n / d) * d;
}

size_t
bytesPerDeepLineTable (const Header &header,
                       int minY, int maxY,
                       const char* base,
                       int xStride,
                       int yStride,
                       vector<size_t> &bytesPerLine)
{
    const Box2i &dataWindow = header.dataWindow();
    const ChannelList &channels = header.channels();

    for (ChannelList::ConstIterator c = channels.begin();
         c != channels.end();
         ++c)
    {
        const int ySampling = abs(c.channel().ySampling);
        const int xSampling = abs(c.channel().xSampling);
        const uint64_t pixelSize = pixelTypeSize (c.channel().type);

        // Here we transform from the domain over all pixels into the domain
        // of actual samples.  We want to sample points in [minY, maxY] where
        // (y % ySampling) == 0.  However, doing this by rejecting samples
        // requires O(height*width) modulo computations, which were a
        // significant bottleneck in the previous implementation of this
        // function.  For the low, low price of 4 divisions per channel, we
        // can tighten the y & x ranges to the least and greatest roots of the
        // sampling function and then stride by the sampling rate.
        const int sampleMinY = roundToNextMultiple(minY, ySampling);
        const int sampleMaxY = roundToPrevMultiple(maxY, ySampling);
        const int sampleMinX = roundToNextMultiple(dataWindow.min.x, xSampling);
        const int sampleMaxX = roundToPrevMultiple(dataWindow.max.x, xSampling);

        for (int y = sampleMinY; y <= sampleMaxY; y+=ySampling)
        {
            uint64_t nBytes = 0;
            for (int x = sampleMinX; x <= sampleMaxX; x += xSampling)
            {
                nBytes += pixelSize *
                          static_cast<uint64_t>(sampleCount(base, xStride, yStride, x, y));
            }

            //
            // architectures where size_t is smaller than 64 bits may overflow
            // (scanlines with more than 2^32 bytes are not currently supported so this should not occur with valid files)
            //
            if( static_cast<uint64_t>(bytesPerLine[y - dataWindow.min.y]) + nBytes > SIZE_MAX)
            {
                throw IEX_NAMESPACE::IoExc("Scanline size too large");
            }

            bytesPerLine[y - dataWindow.min.y] += nBytes;
        }
    }

    size_t maxBytesPerLine = 0;

    for (int y = minY; y <= maxY; ++y)
    {
        if (maxBytesPerLine < bytesPerLine[y - dataWindow.min.y])
        {
            maxBytesPerLine = bytesPerLine[y - dataWindow.min.y];
        }
    }

    return maxBytesPerLine;
}


size_t
bytesPerDeepLineTable (const Header &header,
                       char* base,
                       int xStride,
                       int yStride,
                       vector<size_t> &bytesPerLine)
{
    return bytesPerDeepLineTable(header,
                                 header.dataWindow().min.y,
                                 header.dataWindow().max.y,
                                 base,
                                 xStride,
                                 yStride,
                                 bytesPerLine);
}


void
offsetInLineBufferTable (const vector<size_t> &bytesPerLine,
                         int scanline1, int scanline2,
                         int linesInLineBuffer,
                         vector<size_t> &offsetInLineBuffer)
{
    offsetInLineBuffer.resize (bytesPerLine.size());

    size_t offset = 0;

    for (int i = scanline1; i <= scanline2; ++i)
    {
        if (i % linesInLineBuffer == 0)
            offset = 0;

        offsetInLineBuffer[i] = offset;
        offset += bytesPerLine[i];
    }
}


void
offsetInLineBufferTable (const vector<size_t> &bytesPerLine,
			 int linesInLineBuffer,
			 vector<size_t> &offsetInLineBuffer)
{
    offsetInLineBufferTable (bytesPerLine,
                             0, bytesPerLine.size() - 1,
                             linesInLineBuffer,
                             offsetInLineBuffer);
}


int
lineBufferMinY (int y, int minY, int linesInLineBuffer)
{
    return ((y - minY) / linesInLineBuffer) * linesInLineBuffer + minY;
}


int
lineBufferMaxY (int y, int minY, int linesInLineBuffer)
{
    return lineBufferMinY (y, minY, linesInLineBuffer) + linesInLineBuffer - 1;
}


Compressor::Format
defaultFormat (Compressor * compressor)
{
    return compressor? compressor->format(): Compressor::XDR;
}


//obsolete
int
numLinesInBuffer (Compressor * compressor)
{
    return compressor? compressor->numScanLines(): 1;
}


void
copyIntoFrameBuffer (const char *& readPtr,
		     char * writePtr,
		     char * endPtr,
                     size_t xStride,
		     bool fill,
		     double fillValue,
                     Compressor::Format format,
                     PixelType typeInFrameBuffer,
                     PixelType typeInFile)
{
    //
    // Copy a horizontal row of pixels from an input
    // file's line or tile buffer to a frame buffer.
    //

    if (fill)
    {
        //
        // The file contains no data for this channel.
        // Store a default value in the frame buffer.
        //

        switch (typeInFrameBuffer)
        {
	  case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:
            
            {
                unsigned int fillVal = (unsigned int) (fillValue);

                while (writePtr <= endPtr)
                {
                    *(unsigned int *) writePtr = fillVal;
                    writePtr += xStride;
                }
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

            {
                half fillVal = half (fillValue);

                while (writePtr <= endPtr)
                {
                    *(half *) writePtr = fillVal;
                    writePtr += xStride;
                }
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

            {
                float fillVal = float (fillValue);

                while (writePtr <= endPtr)
                {
                    *(float *) writePtr = fillVal;
                    writePtr += xStride;
                }
            }
            break;

          default:

            throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
        }
    }
    else if (format == Compressor::XDR)
    {
        //
        // The the line or tile buffer is in XDR format.
        //
        // Convert the pixels from the file's machine-
        // independent representation, and store the
        // results in the frame buffer.
        //

        switch (typeInFrameBuffer)
        {
          case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:
    
            switch (typeInFile)
            {
              case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                while (writePtr <= endPtr)
                {
                    Xdr::read <CharPtrIO> (readPtr, *(unsigned int *) writePtr);
                    writePtr += xStride;
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                while (writePtr <= endPtr)
                {
                    half h;
                    Xdr::read <CharPtrIO> (readPtr, h);
                    *(unsigned int *) writePtr = halfToUint (h);
                    writePtr += xStride;
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                while (writePtr <= endPtr)
                {
                    float f;
                    Xdr::read <CharPtrIO> (readPtr, f);
                    *(unsigned int *)writePtr = floatToUint (f);
                    writePtr += xStride;
                }
                break;
                
              default:                  
                  throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

            switch (typeInFile)
            {
              case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                while (writePtr <= endPtr)
                {
                    unsigned int ui;
                    Xdr::read <CharPtrIO> (readPtr, ui);
                    *(half *) writePtr = uintToHalf (ui);
                    writePtr += xStride;
                }
                break;
                
              case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                while (writePtr <= endPtr)
                {
                    Xdr::read <CharPtrIO> (readPtr, *(half *) writePtr);
                    writePtr += xStride;
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                while (writePtr <= endPtr)
                {
                    float f;
                    Xdr::read <CharPtrIO> (readPtr, f);
                    *(half *) writePtr = floatToHalf (f);
                    writePtr += xStride;
                }
                break;
              default:
                  
                  throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

            switch (typeInFile)
            {
              case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                while (writePtr <= endPtr)
                {
                    unsigned int ui;
                    Xdr::read <CharPtrIO> (readPtr, ui);
                    *(float *) writePtr = float (ui);
                    writePtr += xStride;
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                while (writePtr <= endPtr)
                {
                    half h;
                    Xdr::read <CharPtrIO> (readPtr, h);
                    *(float *) writePtr = float (h);
                    writePtr += xStride;
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                while (writePtr <= endPtr)
                {
                    Xdr::read <CharPtrIO> (readPtr, *(float *) writePtr);
                    writePtr += xStride;
                }
                break;
              default:
                  
                  throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
            }
            break;

          default:

            throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
        }
    }
    else
    {
        //
        // The the line or tile buffer is in NATIVE format.
        // Copy the results into the frame buffer.
        //

        switch (typeInFrameBuffer)
        {
          case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:
    
            switch (typeInFile)
            {
              case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                while (writePtr <= endPtr)
                {
                    for (size_t i = 0; i < sizeof (unsigned int); ++i)
                        writePtr[i] = readPtr[i];

                    readPtr += sizeof (unsigned int);
                    writePtr += xStride;
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                while (writePtr <= endPtr)
                {
                    half h = *(half *) readPtr;
                    *(unsigned int *) writePtr = halfToUint (h);
                    readPtr += sizeof (half);
                    writePtr += xStride;
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                while (writePtr <= endPtr)
                {
                    float f;

                    for (size_t i = 0; i < sizeof (float); ++i)
                        ((char *)&f)[i] = readPtr[i];

                    *(unsigned int *)writePtr = floatToUint (f);
                    readPtr += sizeof (float);
                    writePtr += xStride;
                }
                break;
                
              default:
                  
                  throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

            switch (typeInFile)
            {
              case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                while (writePtr <= endPtr)
                {
                    unsigned int ui;

                    for (size_t i = 0; i < sizeof (unsigned int); ++i)
                        ((char *)&ui)[i] = readPtr[i];

                    *(half *) writePtr = uintToHalf (ui);
                    readPtr += sizeof (unsigned int);
                    writePtr += xStride;
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                // If we're tightly packed, just memcpy
                if (xStride == sizeof(half)) {
                    int numBytes = endPtr-writePtr+sizeof(half);
                    memcpy(writePtr, readPtr, numBytes);
                    readPtr  += numBytes;
                    writePtr += numBytes;                    
                } else {
                    while (writePtr <= endPtr)
                    {
                        *(half *) writePtr = *(half *)readPtr;
                        readPtr += sizeof (half);
                        writePtr += xStride;
                    }
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                while (writePtr <= endPtr)
                {
                    float f;

                    for (size_t i = 0; i < sizeof (float); ++i)
                        ((char *)&f)[i] = readPtr[i];

                    *(half *) writePtr = floatToHalf (f);
                    readPtr += sizeof (float);
                    writePtr += xStride;
                }
                break;
              default:
                  
                  throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

            switch (typeInFile)
            {
              case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                while (writePtr <= endPtr)
                {
                    unsigned int ui;

                    for (size_t i = 0; i < sizeof (unsigned int); ++i)
                        ((char *)&ui)[i] = readPtr[i];

                    *(float *) writePtr = float (ui);
                    readPtr += sizeof (unsigned int);
                    writePtr += xStride;
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                while (writePtr <= endPtr)
                {
                    half h = *(half *) readPtr;
                    *(float *) writePtr = float (h);
                    readPtr += sizeof (half);
                    writePtr += xStride;
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                while (writePtr <= endPtr)
                {
                    for (size_t i = 0; i < sizeof (float); ++i)
                        writePtr[i] = readPtr[i];

                    readPtr += sizeof (float);
                    writePtr += xStride;
                }
                break;
              default:
                  
                  throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
            }
            break;

          default:

            throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
        }
    }
}

void
copyIntoDeepFrameBuffer (const char *& readPtr,
                         char * base,
                         const char* sampleCountBase,
                         ptrdiff_t sampleCountXStride,
                         ptrdiff_t sampleCountYStride,
                         int y, int minX, int maxX,
                         int xOffsetForSampleCount,
                         int yOffsetForSampleCount,
                         int xOffsetForData,
                         int yOffsetForData,
                         ptrdiff_t sampleStride,
                         ptrdiff_t xPointerStride,
                         ptrdiff_t yPointerStride,
                         bool fill,
                         double fillValue,
                         Compressor::Format format,
                         PixelType typeInFrameBuffer,
                         PixelType typeInFile)
{
    //
    // Copy a horizontal row of pixels from an input
    // file's line or tile buffer to a frame buffer.
    //

    if (fill)
    {
        //
        // The file contains no data for this channel.
        // Store a default value in the frame buffer.
        //

        switch (typeInFrameBuffer)
        {
          case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

            {
                unsigned int fillVal = (unsigned int) (fillValue);

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    if(writePtr)
                    {
                        int count = sampleCount(sampleCountBase,
                                                sampleCountXStride,
                                                sampleCountYStride,
                                                x - xOffsetForSampleCount,
                                                y - yOffsetForSampleCount);
                        for (int i = 0; i < count; i++)
                        {
                            *(unsigned int *) writePtr = fillVal;
                            writePtr += sampleStride;
                        }
                    }
                }
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

            {
                half fillVal = half (fillValue);

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    if(writePtr)
                    {                            
                        int count = sampleCount(sampleCountBase,
                                                sampleCountXStride,
                                                sampleCountYStride,
                                                x - xOffsetForSampleCount,
                                                y - yOffsetForSampleCount);
                        for (int i = 0; i < count; i++)
                        {
                            *(half *) writePtr = fillVal;
                           writePtr += sampleStride;
                       }
                    }
                }
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

            {
                float fillVal = float (fillValue);

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    if(writePtr)
                    {
                        int count = sampleCount(sampleCountBase,
                                                sampleCountXStride,
                                                sampleCountYStride,
                                                x - xOffsetForSampleCount,
                                                y - yOffsetForSampleCount);
                        for (int i = 0; i < count; i++)
                        {
                            *(float *) writePtr = fillVal;
                            writePtr += sampleStride;
                        }
                    }
                }
            }
            break;

          default:

            throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
        }
    }
    else if (format == Compressor::XDR)
    {
        //
        // The the line or tile buffer is in XDR format.
        //
        // Convert the pixels from the file's machine-
        // independent representation, and store the
        // results in the frame buffer.
        //

        switch (typeInFrameBuffer)
        {
          case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

            switch (typeInFile)
            {
              case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                    if(writePtr)
                    {
                   
                        for (int i = 0; i < count; i++)
                        {
                            Xdr::read <CharPtrIO> (readPtr, *(unsigned int *) writePtr);
                            writePtr += sampleStride;
                        }
                    }else{
                        Xdr::skip <CharPtrIO> (readPtr,count*Xdr::size<unsigned int>());
                    }
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                    if(writePtr)
                    {

                        for (int i = 0; i < count; i++)
                        {
                            half h;
                            Xdr::read <CharPtrIO> (readPtr, h);
                           *(unsigned int *) writePtr = halfToUint (h);
                           writePtr += sampleStride;
                       }
                    }else{
                       Xdr::skip <CharPtrIO> (readPtr,count*Xdr::size<half>());
                    }
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                                                                                        
                    if(writePtr)
                    {
                        for (int i = 0; i < count; i++)
                        {
                            float f;
                            Xdr::read <CharPtrIO> (readPtr, f);
                            *(unsigned int *)writePtr = floatToUint (f);
                            writePtr += sampleStride;
                        } 
                     }else{
                       Xdr::skip <CharPtrIO> (readPtr,count*Xdr::size<float>());
                     }
                
                }
                break;
              default:
                  throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

            switch (typeInFile)
            {
              case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                    if(writePtr)
                    {

                        for (int i = 0; i < count; i++)
                        {
                            unsigned int ui;
                            Xdr::read <CharPtrIO> (readPtr, ui);
                            *(half *) writePtr = uintToHalf (ui);
                            writePtr += sampleStride;
                        }
                    }else{
                        Xdr::skip <CharPtrIO> (readPtr,count*Xdr::size<unsigned int>());
                    }
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                    if(writePtr)
                    {
                    
                        for (int i = 0; i < count; i++)
                        {
                            Xdr::read <CharPtrIO> (readPtr, *(half *) writePtr);
                            writePtr += sampleStride;
                        }
                    }else{
                        Xdr::skip <CharPtrIO> (readPtr,count*Xdr::size<half>());
                    }
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **) (base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                    if(writePtr)
                    {
                        for (int i = 0; i < count; i++)
                        {
                            float f;
                            Xdr::read <CharPtrIO> (readPtr, f);
                            *(half *) writePtr = floatToHalf (f);
                            writePtr += sampleStride;
                        }
                    }else{
                        Xdr::skip <CharPtrIO> (readPtr,count*Xdr::size<float>());
                    }
                }
                break;
              default:
                  
                  throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

            switch (typeInFile)
            {
              case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                    if(writePtr)
                    {
                        for (int i = 0; i < count; i++)
                        {
                            unsigned int ui;
                            Xdr::read <CharPtrIO> (readPtr, ui);
                            *(float *) writePtr = float (ui);
                            writePtr += sampleStride;
                        }
                    }else{
                        Xdr::skip <CharPtrIO> (readPtr,count*Xdr::size<unsigned int>());
                    }
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                    if(writePtr)
                    {

                        for (int i = 0; i < count; i++)
                        {
                            half h;
                            Xdr::read <CharPtrIO> (readPtr, h);
                            *(float *) writePtr = float (h);
                            writePtr += sampleStride;
                        }
                    
                   }else{
                      Xdr::skip <CharPtrIO> (readPtr,count*Xdr::size<half>());
                   }               
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                    if(writePtr)
                    {
                    
                        for (int i = 0; i < count; i++)
                        {
                            Xdr::read <CharPtrIO> (readPtr, *(float *) writePtr);
                            writePtr += sampleStride;
                        }
                    } else{
                        Xdr::skip <CharPtrIO> (readPtr,count*Xdr::size<float>());
                    }      
                    
                }
                break;
              default:
                  
                  throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
            }
            break;

          default:

            throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
        }
    }
    else
    {
        //
        // The the line or tile buffer is in NATIVE format.
        // Copy the results into the frame buffer.
        //

        switch (typeInFrameBuffer)
        {
          case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

            switch (typeInFile)
            {
              case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                                            
                    if(writePtr)
                    {
                         for (int i = 0; i < count; i++)
                         {
                             for (size_t i = 0; i < sizeof (unsigned int); ++i)
                                 writePtr[i] = readPtr[i];

                             readPtr += sizeof (unsigned int);
                             writePtr += sampleStride;
                         }
                    }else{
                        readPtr+=sizeof(unsigned int)*count;
                    }
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                                            
                    if(writePtr)
                    {
                        for (int i = 0; i < count; i++)
                        {
                            half h = *(half *) readPtr;
                            *(unsigned int *) writePtr = halfToUint (h);
                            readPtr += sizeof (half);
                            writePtr += sampleStride;
                        }
                    }else{
                        readPtr+=sizeof(half)*count;
                    }
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                                            
                    if(writePtr)
                    {
                    
                        for (int i = 0; i < count; i++)
                        {
                            float f;

                            for (size_t i = 0; i < sizeof (float); ++i)
                                ((char *)&f)[i] = readPtr[i];

                            *(unsigned int *)writePtr = floatToUint (f);
                            readPtr += sizeof (float);
                            writePtr += sampleStride;
                        }
                    }else{
                        readPtr+=sizeof(float)*count;
                    }
                }
                break;
              default:
                  
                  throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

            switch (typeInFile)
            {
              case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                                            
                    if(writePtr)
                    {
                         for (int i = 0; i < count; i++)
                         {
                             unsigned int ui;
 
                             for (size_t i = 0; i < sizeof (unsigned int); ++i)
                                 ((char *)&ui)[i] = readPtr[i];
  
                             *(half *) writePtr = uintToHalf (ui);
                             readPtr += sizeof (unsigned int);
                             writePtr += sampleStride;
                         }
                    }else{
                        readPtr+=sizeof(unsigned int)*count;
                    }
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                                            
                    if(writePtr)
                    {
                         for (int i = 0; i < count; i++)
                         {
                             *(half *) writePtr = *(half *)readPtr;
                             readPtr += sizeof (half);
                             writePtr += sampleStride;
                         }
                    }else{
                        readPtr+=sizeof(half)*count;
                    }
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                                            
                    if(writePtr)
                    {
                         for (int i = 0; i < count; i++)
                         {
                            float f;

                             for (size_t i = 0; i < sizeof (float); ++i)
                                 ((char *)&f)[i] = readPtr[i];

                            *(half *) writePtr = floatToHalf (f);
                            readPtr += sizeof (float);
                            writePtr += sampleStride;
                         }
                    }else{
                        readPtr+=sizeof(float)*count;
                    }
                }
                break;
              default:
                  
                  throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

            switch (typeInFile)
            {
              case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                                            
                    if(writePtr)
                    {
                         for (int i = 0; i < count; i++)
                         {
                              unsigned int ui;
 
                              for (size_t i = 0; i < sizeof (unsigned int); ++i)
                                  ((char *)&ui)[i] = readPtr[i];

                              *(float *) writePtr = float (ui);
                              readPtr += sizeof (unsigned int);
                              writePtr += sampleStride;
                         }
                    }else{
                        readPtr+=sizeof(unsigned int)*count;
                    }
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                                            
                    if(writePtr)
                    {
                         for (int i = 0; i < count; i++)
                         {
                             half h = *(half *) readPtr;
                             *(float *) writePtr = float (h);
                             readPtr += sizeof (half);
                             writePtr += sampleStride;
                         }
                    }else{
                        readPtr+=sizeof(half)*count;
                    }
                }
                break;

              case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                for (int x = minX; x <= maxX; x++)
                {
                    char* writePtr = *(char **)(base+(y-yOffsetForData)*yPointerStride + (x-xOffsetForData)*xPointerStride);
                    
                    int count = sampleCount(sampleCountBase,
                                            sampleCountXStride,
                                            sampleCountYStride,
                                            x - xOffsetForSampleCount,
                                            y - yOffsetForSampleCount);
                                            
                    if(writePtr)
                    {
                         for (int i = 0; i < count; i++)
                         {
                              for (size_t i = 0; i < sizeof (float); ++i)
                                  writePtr[i] = readPtr[i];

                             readPtr += sizeof (float);
                             writePtr += sampleStride;
                         }
                    }else{
                        readPtr+=sizeof(float)*count;
                    }
                }
                break;
              default:
                  
                  throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
            }
            break;

          default:

            throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
        }
    }
}


void
skipChannel (const char *& readPtr,
             PixelType typeInFile,
	     size_t xSize)
{
    switch (typeInFile)
    {
      case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:
        
        Xdr::skip <CharPtrIO> (readPtr, Xdr::size <unsigned int> () * xSize);
        break;

      case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

        Xdr::skip <CharPtrIO> (readPtr, Xdr::size <half> () * xSize);
        break;

      case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

        Xdr::skip <CharPtrIO> (readPtr, Xdr::size <float> () * xSize);
        break;

      default:

        throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
    }
}


namespace
{
//
// helper function to realign floats
// for architectures that require 32-bit alignment for float reading
//

struct FBytes { uint8_t b[4]; };
union bytesUintOrFloat {
  FBytes b;
  float f;
  unsigned int u;
} ;
}

void
convertInPlace (char *& writePtr,
                const char *& readPtr,
		PixelType type,
                size_t numPixels)
{
    switch (type)
    {
      case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:
    
        for (size_t j = 0; j < numPixels; ++j)
        {
            union bytesUintOrFloat tmp;
            tmp.b = * reinterpret_cast<const FBytes *>( readPtr );
            Xdr::write <CharPtrIO> (writePtr, tmp.u);
            readPtr += sizeof(unsigned int);
        }
        break;
    
      case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:
    
        for (size_t j = 0; j < numPixels; ++j)
        {               
            Xdr::write <CharPtrIO> (writePtr, *(const half *) readPtr);
            readPtr += sizeof(half);
        }
        break;
    
      case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:
    
        for (size_t j = 0; j < numPixels; ++j)
        {
            union bytesUintOrFloat tmp;
            tmp.b = * reinterpret_cast<const FBytes *>( readPtr );
            Xdr::write <CharPtrIO> (writePtr, tmp.f);
            readPtr += sizeof(float);
        }
        break;
    
      default:
    
        throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
    }
}


void
copyFromFrameBuffer (char *& writePtr,
		     const char *& readPtr,
                     const char * endPtr,
		     size_t xStride,
                     Compressor::Format format,
		     PixelType type)
{
    char * localWritePtr = writePtr;
    const char * localReadPtr = readPtr;
    //
    // Copy a horizontal row of pixels from a frame
    // buffer to an output file's line or tile buffer.
    //

    if (format == Compressor::XDR)
    {
        //
        // The the line or tile buffer is in XDR format.
        //

        switch (type)
        {
          case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

            while (localReadPtr <= endPtr)
            {
                Xdr::write <CharPtrIO> (localWritePtr,
                                        *(const unsigned int *) localReadPtr);
                localReadPtr += xStride;
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

            while (localReadPtr <= endPtr)
            {
                Xdr::write <CharPtrIO> (localWritePtr, *(const half *) localReadPtr);
                localReadPtr += xStride;
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

            while (localReadPtr <= endPtr)
            {
                Xdr::write <CharPtrIO> (localWritePtr, *(const float *) localReadPtr);
                localReadPtr += xStride;
            }
            break;

          default:

            throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
        }
    }
    else
    {
        //
        // The the line or tile buffer is in NATIVE format.
        //

        switch (type)
        {
          case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

            while (localReadPtr <= endPtr)
            {
                for (size_t i = 0; i < sizeof (unsigned int); ++i)
                    *localWritePtr++ = localReadPtr[i];

                localReadPtr += xStride;
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

            while (localReadPtr <= endPtr)
            {
                *(half *) localWritePtr = *(const half *) localReadPtr;
                localWritePtr += sizeof (half);
                localReadPtr += xStride;
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

            while (localReadPtr <= endPtr)
            {
                for (size_t i = 0; i < sizeof (float); ++i)
                    *localWritePtr++ = localReadPtr[i];

                localReadPtr += xStride;
            }
            break;
            
          default:

            throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
        }
    }

    writePtr = localWritePtr;
    readPtr = localReadPtr;
}

void
copyFromDeepFrameBuffer (char *& writePtr,
                         const char * base,
                         char* sampleCountBase,
                         ptrdiff_t sampleCountXStride,
                         ptrdiff_t sampleCountYStride,
                         int y, int xMin, int xMax,
                         int xOffsetForSampleCount,
                         int yOffsetForSampleCount,
                         int xOffsetForData,
                         int yOffsetForData,
                         ptrdiff_t sampleStride,
                         ptrdiff_t dataXStride,
                         ptrdiff_t dataYStride,
                         Compressor::Format format,
                         PixelType type)
{
    //
    // Copy a horizontal row of pixels from a frame
    // buffer to an output file's line or tile buffer.
    //

    if (format == Compressor::XDR)
    {
        //
        // The the line or tile buffer is in XDR format.
        //

        switch (type)
        {
          case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

            for (int x = xMin; x <= xMax; x++)
            {
                unsigned int count =
                        sampleCount(sampleCountBase,
                                   sampleCountXStride,
                                   sampleCountYStride,
                                   x - xOffsetForSampleCount,
                                   y - yOffsetForSampleCount);
                const char* ptr = base + (y-yOffsetForData) * dataYStride + (x-xOffsetForData) * dataXStride;
                const char* readPtr = ((const char**) ptr)[0];
                for (unsigned int i = 0; i < count; i++)
                {
                    Xdr::write <CharPtrIO> (writePtr,
                                            *(const unsigned int *) readPtr);
                    readPtr += sampleStride;
                }
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

            for (int x = xMin; x <= xMax; x++)
            {
                unsigned int count =
                        sampleCount(sampleCountBase,
                                   sampleCountXStride,
                                   sampleCountYStride,
                                   x - xOffsetForSampleCount,
                                   y - yOffsetForSampleCount);
                const char* ptr = base + (y-yOffsetForData) * dataYStride + (x-xOffsetForData) * dataXStride;
                const char* readPtr = ((const char**) ptr)[0];
                for (unsigned int i = 0; i < count; i++)
                {
                    Xdr::write <CharPtrIO> (writePtr, *(const half *) readPtr);
                    readPtr += sampleStride;
                }
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

            for (int x = xMin; x <= xMax; x++)
            {
                unsigned int count =
                        sampleCount(sampleCountBase,
                                   sampleCountXStride,
                                   sampleCountYStride,
                                   x - xOffsetForSampleCount,
                                   y - yOffsetForSampleCount);
                const char* ptr = base + (y-yOffsetForData) * dataYStride + (x-xOffsetForData) * dataXStride;                                   
                                   
                const char* readPtr = ((const char**) ptr)[0];
                for (unsigned int i = 0; i < count; i++)
                {
                    Xdr::write <CharPtrIO> (writePtr, *(const float *) readPtr);
                    readPtr += sampleStride;
                }
            }
            break;

          default:

            throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
        }
    }
    else
    {
        //
        // The the line or tile buffer is in NATIVE format.
        //

        switch (type)
        {
          case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

            for (int x = xMin; x <= xMax; x++)
            {
                unsigned int count =
                        sampleCount(sampleCountBase,
                                   sampleCountXStride,
                                   sampleCountYStride,
                                   x - xOffsetForSampleCount,
                                   y - yOffsetForSampleCount);
                                   
                const char* ptr = base + (y-yOffsetForData) * dataYStride + (x-xOffsetForData) * dataXStride;                                                                      
                const char* readPtr = ((const char**) ptr)[0];
                for (unsigned int i = 0; i < count; i++)
                {
                    for (size_t j = 0; j < sizeof (unsigned int); ++j)
                        *writePtr++ = readPtr[j];

                    readPtr += sampleStride;
                }
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

            for (int x = xMin; x <= xMax; x++)
            {
                unsigned int count =
                        sampleCount(sampleCountBase,
                                   sampleCountXStride,
                                   sampleCountYStride,
                                   x - xOffsetForSampleCount,
                                   y - yOffsetForSampleCount);
                const char* ptr = base + (y-yOffsetForData) * dataYStride + (x-xOffsetForData) * dataXStride;                                   
                const char* readPtr = ((const char**) ptr)[0];
                for (unsigned int i = 0; i < count; i++)
                {
                    *(half *) writePtr = *(const half *) readPtr;
                    writePtr += sizeof (half);
                    readPtr += sampleStride;
                }
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

            for (int x = xMin; x <= xMax; x++)
            {
                unsigned int count =
                        sampleCount(sampleCountBase,
                                   sampleCountXStride,
                                   sampleCountYStride,
                                   x - xOffsetForSampleCount,
                                   y - yOffsetForSampleCount);
                                   
                const char* ptr = base + (y-yOffsetForData) * dataYStride + (x-xOffsetForData) * dataXStride;                                   
                const char* readPtr = ((const char**) ptr)[0];
                for (unsigned int i = 0; i < count; i++)
                {
                    for (size_t j = 0; j < sizeof (float); ++j)
                        *writePtr++ = readPtr[j];

                    readPtr += sampleStride;
                }
            }
            break;

          default:

            throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
        }
    }
}


void
fillChannelWithZeroes (char *& writePtr,
		       Compressor::Format format,
		       PixelType type,
		       size_t xSize)
{
    if (format == Compressor::XDR)
    {
        //
        // Fill with data in XDR format.
        //

        switch (type)
        {
          case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

            for (size_t j = 0; j < xSize; ++j)
                Xdr::write <CharPtrIO> (writePtr, (unsigned int) 0);

            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

            for (size_t j = 0; j < xSize; ++j)
                Xdr::write <CharPtrIO> (writePtr, (half) 0);

            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

            for (size_t j = 0; j < xSize; ++j)
                Xdr::write <CharPtrIO> (writePtr, (float) 0);

            break;
            
          default:

            throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
        }
    }
    else
    {
        //
        // Fill with data in NATIVE format.
        //

        switch (type)
        {
          case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

            for (size_t j = 0; j < xSize; ++j)
            {
                static const unsigned int ui = 0;

                for (size_t i = 0; i < sizeof (ui); ++i)
                    *writePtr++ = ((char *) &ui)[i];
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

            for (size_t j = 0; j < xSize; ++j)
            {
                *(half *) writePtr = half (0);
                writePtr += sizeof (half);
            }
            break;

          case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

            for (size_t j = 0; j < xSize; ++j)
            {
                static const float f = 0;

                for (size_t i = 0; i < sizeof (f); ++i)
                    *writePtr++ = ((char *) &f)[i];
            }
            break;
            
          default:

            throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
        }
    }
}

bool
usesLongNames (const Header &header)
{
    //
    // If an OpenEXR file contains any attribute names, attribute type names
    // or channel names longer than 31 characters, then the file cannot be
    // read by older versions of the OpenEXR library (up to OpenEXR 1.6.1).
    // Before writing the file header, we check if the header contains
    // any names longer than 31 characters; if it does, then we set the
    // LONG_NAMES_FLAG in the file version number.  Older versions of the
    // OpenEXR library will refuse to read files that have the LONG_NAMES_FLAG
    // set.  Without the flag, older versions of the library would mis-
    // interpret the file as broken.
    //

    for (Header::ConstIterator i = header.begin();
         i != header.end();
         ++i)
    {
        if (strlen (i.name()) >= 32 || strlen (i.attribute().typeName()) >= 32)
            return true;
    }

    const ChannelList &channels = header.channels();

    for (ChannelList::ConstIterator i = channels.begin();
         i != channels.end();
         ++i)
    {
        if (strlen (i.name()) >= 32)
            return true;
    }

    return false;
}



int
getScanlineChunkOffsetTableSize(const Header& header)
{
    const Box2i &dataWindow = header.dataWindow();


    //
    // use int64_t types to prevent overflow in lineOffsetSize for images with
    // extremely high dataWindows
    //
    int64_t linesInBuffer = numLinesInBuffer ( header.compression() );

    int64_t lineOffsetSize = (static_cast <int64_t>(dataWindow.max.y) - static_cast <int64_t>(dataWindow.min.y) +
                          linesInBuffer) / linesInBuffer;

    return static_cast <int>(lineOffsetSize);
}

//
// Located in ImfTiledMisc.cpp
//
int
getTiledChunkOffsetTableSize(const Header& header);

int
getChunkOffsetTableSize(const Header& header)
{
    //
    // if there is a type in the header which indicates the part is not a currently supported type,
    // use the chunkCount attribute
    //


    if(header.hasType()  && !isSupportedType(header.type()))
    {
        if(header.hasChunkCount())
        {
           return header.chunkCount();
        }
        else
        {
           throw IEX_NAMESPACE::ArgExc ("unsupported header type to "
           "get chunk offset table size");
        }
    }

    //
    // part is a known type - ignore the header attribute and compute the chunk size from the header
    //
    if (isTiled(header.type()) == false)
        return getScanlineChunkOffsetTableSize(header);
    else
        return getTiledChunkOffsetTableSize(header);
    
}


OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
