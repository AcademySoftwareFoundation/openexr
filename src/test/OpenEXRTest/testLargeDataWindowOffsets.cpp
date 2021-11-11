//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Weta Digital, Ltd and Contributors to the OpenEXR Project.
//

#include "ImfInputFile.h"
#include <stdlib.h>
#include <vector>
#include "ImfChannelList.h"
#include "ImfOutputFile.h"
#include "ImfCompression.h"
#include "ImfFrameBuffer.h"
#include "ImfHeader.h"
#include "ImfStandardAttributes.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <assert.h>
#include <IlmThread.h>
#include <ImathBox.h>

#include "tmpDir.h"
#include "random.h"

namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace IMF;
using namespace std;
using namespace IMATH_NAMESPACE;
using namespace ILMTHREAD_NAMESPACE;


namespace
{

using OPENEXR_IMF_NAMESPACE::UINT;
using OPENEXR_IMF_NAMESPACE::FLOAT;

std::string filename;

vector<char> writingBuffer; // buffer as file was written
vector<char> readingBuffer; // buffer containing new image

static const long long maxMem = 16ll*1024ll*1024ll; // max memory usage for pixel storage should be 16 MiB
static const long long maxMemoryPerBuffer = maxMem/2ll; // read and write buffers are separate
static const long long pixelCount = maxMemoryPerBuffer / (long long) (10*sizeof(float));

bool compare(const FrameBuffer& asRead,
             const FrameBuffer& asWritten,
             const Box2i& dataWindow
            )
{
    for (FrameBuffer::ConstIterator i =asRead.begin();i!=asRead.end();i++)
    {
        FrameBuffer::ConstIterator p = asWritten.find(i.name());
        for (int y=dataWindow.min.y; y<= dataWindow.max.y; y++)
        {
            for (int x = dataWindow.min.x; x <= dataWindow.max.x; x++)
                 
            {
                //
                // extract value read back from file
                //
                intptr_t base = reinterpret_cast<intptr_t>(i.slice().base);
                char * ptr = reinterpret_cast<char*>(base+i.slice().yStride*intptr_t(y) +i.slice().xStride*intptr_t(x));
                half readHalf;
                switch (i.slice().type)
                {
                    case IMF::FLOAT :
                        readHalf =  half(*(float*) ptr);
                        break;
                    case IMF::HALF :
                        readHalf = half(*(half*) ptr);
                        break;
                    case IMF::UINT :
                        continue; // can't very well check this
                    default :
                        cout << "don't know about that\n";
                        exit(1);
                }
                

                //
                // extract value written to file
                //

                half writtenHalf;

                if (p!=asWritten.end())
                {
                    intptr_t base =reinterpret_cast<intptr_t>( p.slice().base);
                    char * ptr =  reinterpret_cast<char*>(base+p.slice().yStride*intptr_t(y) +
                                 p.slice().xStride*intptr_t(x));
                    switch (p.slice().type)
                    {
                    case IMF::FLOAT :
                        writtenHalf = half(*(float*) ptr);
                        break;
                    case IMF::HALF :
                        writtenHalf = half(*(half*) ptr);
                        break;
                    case IMF::UINT :
                        continue;
                    default :
                        cout << "don't know about that\n";
                        exit(1);
                    }
                }
                else
                {
                    writtenHalf=half(i.slice().fillValue);
                }

                if (writtenHalf.bits()!=readHalf.bits())
                {
                        cout << "\n\nerror reading back channel " << i.name() << " pixel " << x << ',' << y << " got " << readHalf << " expected " << writtenHalf << endl;
                        assert(writtenHalf.bits()==readHalf.bits());
                        exit(1);
                    
                }             
            }

        }
    }
    return true;
}

//
// allocate readingBuffer or writingBuffer, setting up a framebuffer to point to the right thing
//
ChannelList
setupBuffer (const Header& hdr,       // header to grab datawindow from
             const char * const *channels, // NULL terminated list of channels to write
             const PixelType *pt,
             FrameBuffer& buf,        // buffer to fill with pointers to channel
             bool writing                  // true if should allocate
            )
{
    Box2i dw = hdr.dataWindow();

    //
    // how many channels in total
    //
    size_t activechans = 0;
    
    while (channels[activechans]!=NULL)
    {
        activechans++;
    }

    size_t samples = size_t(hdr.dataWindow().max.x+1-hdr.dataWindow().min.x)*
                  size_t(hdr.dataWindow().max.y+1-hdr.dataWindow().min.y)*activechans;

    // always allocate four bytes for each sample, even half types. to keep floats word-aligned
    size_t size =samples * 4;

    
    if (writing)
    {
        writingBuffer.resize(size);
    }
    else
    {
        readingBuffer.resize(size);
    }
   
     const char * write_ptr = writing ? &writingBuffer[0] : &readingBuffer[0];
     // fill with random halfs, casting to floats for float channels - don't write NaN values
     size_t chan=0;
     for (size_t i=0;i<samples;i++)
     {

         half v;
         do
         {
           unsigned short int values = random_int(std::numeric_limits<unsigned short>::max());
           v.setBits(values);
         }
         while ( v.isNan() );

         if (pt==NULL || pt[chan]==IMF::HALF)
         {
             *(half*)write_ptr = half(v);
         }
         else
         {
             *(float*)write_ptr = float(v);
         }
         chan++;
         write_ptr += 4;
         if (chan==activechans)
         {
             chan=0;
         }
        
     }

     

    ChannelList chanlist;

    int64_t width = (dw.max.x+1-dw.min.x);
    int64_t bytes_per_row = activechans * 4 * width;
   
    const char* offset = ( writing ? writingBuffer.data() : readingBuffer.data() ); 
    for (size_t i=0;i<activechans;i++)
    {
        PixelType type = pt==NULL ? IMF::HALF : pt[i];

        chanlist.insert(channels[i],type);
        buf.insert (channels[i],
                    Slice::Make (type,
                                 offset,
                                 dw.min,
                                 width,1,
                                 activechans * 4,
                                 bytes_per_row,
                                 1,
                                 1,
                                 100.+i,false,false
                                )
                    );
        offset += 4;
   }

    return chanlist;
}



Header writefile(FrameBuffer& buf, const char * const *channels, // NULL terminated list of channels to write
             const PixelType *pt)
{
    
    const int height = random_int()/2; 
    const int width  = random_int()/2;

    Header hdr(width,height,1);
    
    
    //
    // set min origin to be anything up to half INT_MAX
    //
    hdr.dataWindow().min.x = random_int() / 4;
    hdr.dataWindow().min.y = random_int() / 4;
    if(random_int(2)) 
    {
        hdr.dataWindow().min.x = -hdr.dataWindow().min.x;
    }
    if(random_int(2)) 
    {
        hdr.dataWindow().min.y = -hdr.dataWindow().min.y;
    }
    
    //
    // up to 512 scanlines
    //
    hdr.dataWindow().max.y = hdr.dataWindow().min.y + random_int (512) + 1;

    //
    // compute image width to give us at most 'pixelCount' pixels in the image
    //
    hdr.dataWindow().max.x = hdr.dataWindow().min.x+ pixelCount /
        ((long long) (hdr.dataWindow().max.y) - (long long) (hdr.dataWindow().min.y));
    
    hdr.compression() = Compression(random_int(static_cast<int>(NUM_COMPRESSION_METHODS)));
    hdr.channels() = setupBuffer (hdr,
                                  channels,
                                  pt,
                                  buf,
                                  true);
    
    
    remove (filename.c_str());
    OutputFile f(filename.c_str(), hdr);
    f.setFrameBuffer(buf);
    f.writePixels(hdr.dataWindow().max.y-hdr.dataWindow().min.y+1);

    return hdr;
}

void
readfile (FrameBuffer & buf,      ///< list of channels to read: index to readingBuffer,
          const char * const *channels, // NULL terminated list of channels to write
             const PixelType *pt
        )
{
    InputFile infile (filename.c_str());
    setupBuffer(infile.header(),
                channels,pt,
                buf,
                false);
    infile.setFrameBuffer(buf);
    
    infile.readPixels (infile.header().dataWindow().min.y,
                       infile.header().dataWindow().max.y);

   
}

static const char* rgb[] = {"R","G","B",NULL};
static const char* rgba[] = {"R","G","B","A",NULL};
static const char* rgbaz[] = {"R","G","B","A","Z",NULL};
static const char* lots[] = {"1","2","3","4","5","6","7","8","9","10",NULL};

static const PixelType allFloats[] = {FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT,FLOAT};
static const PixelType halfFloat[] = {HALF,FLOAT,HALF,FLOAT,HALF,FLOAT,HALF,FLOAT,HALF,FLOAT,HALF,FLOAT,HALF,FLOAT};
static const PixelType floatHalf[] = {FLOAT,HALF,FLOAT,HALF,FLOAT,HALF,FLOAT,HALF,FLOAT,HALF,FLOAT,HALF,FLOAT,HALF,FLOAT};

void
test (int testCount)
{
   
    for(int i = 0 ; i < testCount ; ++i )
    {
        FrameBuffer writeFrameBuf;
        const char** channels=rgb;
        switch( random_int(4))        
        {
            case 0 : channels = rgb; break;
            case 1 : channels = rgba; break;
            case 2 : channels = rgbaz; break;
            case 3 : channels = lots; break;
        }
        const PixelType* writetypes=NULL;
        switch( random_int(4))
        {
            case 0 : writetypes = NULL; break;
            case 1 : writetypes = allFloats; break;
            case 2 : writetypes = halfFloat; break;
            case 3 : writetypes = floatHalf; break;
        }
        const PixelType* readTypes=NULL;
        switch( random_int(4))
        {
            case 0 : readTypes = NULL; break;
            case 1 : readTypes = allFloats; break;
            case 2 : readTypes = halfFloat; break;
            case 3 : readTypes = floatHalf; break;
        }
        
        Header hdr = writefile(writeFrameBuf,channels,writetypes);
        Box2i dw = hdr.dataWindow();
        cout << "dataWindow: " << dw.min << ' ' << dw.max << ' ';
        cout.flush();
        FrameBuffer readFrameBuf;
        readfile (readFrameBuf,channels,readTypes);
        
        //
        // only the first 5 compression methods are guaranteed lossless on both half and float.
        // skip comparison for other types
        //
        if(hdr.compression() < 5)
        {
            if (compare(readFrameBuf, writeFrameBuf, dw))
            {
                cout <<  " OK ";
            }
            else
            {
                cout <<  " FAIL" << endl;        
                
            }
        }
        cout << "\n";        
    }
    remove (filename.c_str());
}


} // namespace anon


void 
testLargeDataWindowOffsets (const std::string & tempDir)
{
    filename = tempDir + "imf_test_interleave_patterns.exr";

    random_reseed(1);

    cout << "Testing dataWindows with large offsets ... " << endl;

    test (100);
    
    cout << "ok\n" << endl;
}
