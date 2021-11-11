//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <ImfRgbaFile.h>
#include <ImfTiledRgbaFile.h>
#include <ImfMultiPartInputFile.h>
#include <ImfMultiPartOutputFile.h>
#include <ImfPartType.h>
#include <ImfInputPart.h>
#include <ImfOutputPart.h>
#include <ImfStdIO.h>
#include <ImfArray.h>

#include <stdio.h>
#include <assert.h>
#include "Iex.h"
#include <errno.h>

#include <vector>
#include <ImfChannelList.h>

#include "TestUtilFStream.h"

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;
using namespace IMATH_NAMESPACE;

namespace {

void
fillPixels1 (Array2D<Rgba> &pixels, int w, int h)
{
    for (int y = 0; y < h; ++y)
    {
	for (int x = 0; x < w; ++x)
	{
	    Rgba &p = pixels[y][x];

	    p.r = (x & 1);
	    p.g = ((x + y) & 1);
	    p.b = (y & 1);
	    p.a = (p.r + p.b + p.g) / 3.0;
	}
    }
}


void
fillPixels2 (Array2D<Rgba> &pixels, int w, int h)
{
    for (int y = 0; y < h; ++y)
    {
	for (int x = 0; x < w; ++x)
	{
	    Rgba &p = pixels[y][x];

	    p.r = (x & 2);
	    p.g = ((x + y) & 2);
	    p.b = (y & 2);
	    p.a = (p.r + p.b + p.g) / 3.0;
	}
    }
}


//
// class MMIFStream -- a memory-mapped implementation of
// class IStream based on class std::ifstream
//

class MMIFStream: public OPENEXR_IMF_NAMESPACE::IStream
{
  public:

    //-------------------------------------------------------
    // A constructor that opens the file with the given name.
    // It reads the whole file into an internal buffer and
    // then immediately closes the file.
    //-------------------------------------------------------

    MMIFStream (const char fileName[]);

    virtual ~MMIFStream ();

    virtual bool        isMemoryMapped () const {return true;}

    virtual bool	read (char c[/*n*/], int n);
    virtual char*       readMemoryMapped (int n);
    virtual uint64_t	tellg () {return _pos;}
    virtual void	seekg (uint64_t pos) {_pos = pos;}
    virtual void	clear () {}

  private:

    char*               _buffer;
    uint64_t            _length;
    uint64_t            _pos;
};



MMIFStream::MMIFStream (const char fileName[]):
    OPENEXR_IMF_NAMESPACE::IStream (fileName),
    _buffer (0),
    _length (0),
    _pos (0)
{
    std::ifstream ifs;
    testutil::OpenStreamWithUTF8Name (
        ifs, fileName, ios::in | ios_base::binary);

    //
    // Get length of file
    //

    ifs.seekg (0, ios::end);
    _length = ifs.tellg();
    ifs.seekg (0, ios::beg);
    
    //
    // Allocate memory
    //

    _buffer = new char [_length];
    
    //
    // Read the entire file
    //

    ifs.read (_buffer, _length);
    ifs.close();
}


MMIFStream::~MMIFStream ()
{
    delete [] _buffer;
}


bool
MMIFStream::read (char c[/*n*/], int n)
{
    if (_pos >= _length && n != 0)
	throw IEX_NAMESPACE::InputExc ("Unexpected end of file.");
        
    uint64_t n2 = n;
    bool retVal = true;

    if (_length - _pos <= n2)
    {
        n2 = _length - _pos;
        retVal = false;
    }

    memcpy (c, &(_buffer[_pos]), n2);
    _pos += n2;
    return retVal;
}


char*
MMIFStream::readMemoryMapped (int n)
{
    if (_pos >= _length)
	throw IEX_NAMESPACE::InputExc ("Unexpected end of file.");
       
    if (_pos + n > _length)
        throw IEX_NAMESPACE::InputExc ("Reading past end of file.");    

    char* retVal = &(_buffer[_pos]);
    _pos += n;
    return retVal;
}


void
writeReadScanLines (const char fileName[],
		    int width,
		    int height,
		    const Array2D<Rgba> &p1)
{
    //
    // Save a scanline-based RGBA image, but instead of
    // letting the RgbaOutputFile object open the file,
    // make the RgbaOutputFile object use an existing
    // StdOFStream.  Read the image back, using an
    // existing StdIFStream, and compare the pixels
    // with the original data.  Then read the image
    // back a second time using a memory-mapped
    // MMIFStream (see above).
    //

    cout << "scan-line based file:" << endl;

    {
        cout << "writing";
        remove (fileName);
        std::ofstream os;
        testutil::OpenStreamWithUTF8Name (
            os, fileName, ios::out | ios_base::binary);
        StdOFStream ofs (os, fileName);
        Header header (width, height);
        RgbaOutputFile out (ofs, header, WRITE_RGBA);
        out.setFrameBuffer (&p1[0][0], 1, width);
        out.writePixels (height);
    }

    {
        cout << ", reading";
        std::ifstream is;
        testutil::OpenStreamWithUTF8Name (
            is, fileName, ios::in | ios_base::binary);
        StdIFStream ifs (is, fileName);
        RgbaInputFile in (ifs);

	const Box2i &dw = in.dataWindow();
	int w = dw.max.x - dw.min.x + 1;
	int h = dw.max.y - dw.min.y + 1;
	int dx = dw.min.x;
	int dy = dw.min.y;

	Array2D<Rgba> p2 (h, w);
	in.setFrameBuffer (&p2[-dy][-dx], 1, w);
	in.readPixels (dw.min.y, dw.max.y);

        cout << ", comparing";
	for (int y = 0; y < h; ++y)
	{
	    for (int x = 0; x < w; ++x)
	    {
		assert (p2[y][x].r == p1[y][x].r);
		assert (p2[y][x].g == p1[y][x].g);
		assert (p2[y][x].b == p1[y][x].b);
		assert (p2[y][x].a == p1[y][x].a);
	    }
	}
    }
    
    {
        cout << ", reading (memory-mapped)";
	MMIFStream ifs (fileName);
	RgbaInputFile in (ifs);

	const Box2i &dw = in.dataWindow();
	int w = dw.max.x - dw.min.x + 1;
	int h = dw.max.y - dw.min.y + 1;
	int dx = dw.min.x;
	int dy = dw.min.y;

	Array2D<Rgba> p2 (h, w);
	in.setFrameBuffer (&p2[-dy][-dx], 1, w);
	in.readPixels (dw.min.y, dw.max.y);

        cout << ", comparing";
	for (int y = 0; y < h; ++y)
	{
	    for (int x = 0; x < w; ++x)
	    {
		assert (p2[y][x].r == p1[y][x].r);
		assert (p2[y][x].g == p1[y][x].g);
		assert (p2[y][x].b == p1[y][x].b);
		assert (p2[y][x].a == p1[y][x].a);
	    }
	}
    }
    
    cout << endl;

    remove (fileName);
}
void
writeReadMultiPart (const char fileName[],
                    int width,
                    int height,
                    const Array2D<Rgba> &p1)
{
    //
    // Save a two scanline parts in an image, but instead of
    // letting the MultiPartOutputFile object open the file,
    // make the MultiPartOutputFile object use an existing
    // StdOFStream.  Read the image back, using an
    // existing StdIFStream, and compare the pixels
    // with the original data.  Then read the image
    // back a second time using a memory-mapped
    // MMIFStream (see above).
    //
                        
    cout << "scan-line based mulitpart file:" << endl;
                            
    {
        cout << "writing";
        remove (fileName);
        std::ofstream os;
        testutil::OpenStreamWithUTF8Name (
            os, fileName, ios::out | ios_base::binary);
        StdOFStream ofs (os, fileName);

        vector<Header> headers(2);
        headers[0] = Header(width, height);
        headers[0].setName("part1");
        headers[0].channels().insert("R",Channel());
        headers[0].channels().insert("G",Channel());
        headers[0].channels().insert("B",Channel());
        headers[0].channels().insert("A",Channel());
        headers[0].setType(SCANLINEIMAGE);

        headers[1]=headers[0];
        headers[1].setName("part2");

        MultiPartOutputFile out (ofs, &headers[0],2);
        FrameBuffer f;
        f.insert("R",Slice(HALF,(char *) &p1[0][0].r,sizeof(Rgba),width*sizeof(Rgba)));
        f.insert("G",Slice(HALF,(char *) &p1[0][0].g,sizeof(Rgba),width*sizeof(Rgba)));
        f.insert("B",Slice(HALF,(char *) &p1[0][0].b,sizeof(Rgba),width*sizeof(Rgba)));
        f.insert("A",Slice(HALF,(char *) &p1[0][0].a,sizeof(Rgba),width*sizeof(Rgba)));
        
        for(int i=0;i<2;i++)
        {
            OutputPart p(out,i);
            p.setFrameBuffer (f);
            p.writePixels (height);
        }
    }
                        
    {
        cout << ", reading";
        std::ifstream is;
        testutil::OpenStreamWithUTF8Name (
            is, fileName, ios::in | ios_base::binary);
        StdIFStream ifs (is, fileName);
        MultiPartInputFile in (ifs);
        
        assert(in.parts() == 2);
        
        assert(in.header(0).dataWindow()==in.header(1).dataWindow());
        
        const Box2i &dw = in.header(0).dataWindow();
        int w = dw.max.x - dw.min.x + 1;
        int h = dw.max.y - dw.min.y + 1;
        int dx = dw.min.x;
        int dy = dw.min.y;
        
        Array2D<Rgba> p2 (h, w);
        FrameBuffer f;
        f.insert("R",Slice(HALF,(char *) &p2[-dy][-dx].r,sizeof(Rgba),w*sizeof(Rgba)));
        f.insert("G",Slice(HALF,(char *) &p2[-dy][-dx].g,sizeof(Rgba),w*sizeof(Rgba)));
        f.insert("B",Slice(HALF,(char *) &p2[-dy][-dx].b,sizeof(Rgba),w*sizeof(Rgba)));
        f.insert("A",Slice(HALF,(char *) &p2[-dy][-dx].a,sizeof(Rgba),w*sizeof(Rgba)));
        
        for(int part=0;part<2;part++)
        {
            InputPart p(in,part);
            p.setFrameBuffer(f);
            p.readPixels (dw.min.y, dw.max.y);
                            
            cout << ", comparing pt " << part;
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    assert (p2[y][x].r == p1[y][x].r);
                    assert (p2[y][x].g == p1[y][x].g);
                    assert (p2[y][x].b == p1[y][x].b);
                    assert (p2[y][x].a == p1[y][x].a);
                }
            }
        }
    }
    
    {
        cout << ", reading (memory-mapped)";
        MMIFStream ifs (fileName);
        MultiPartInputFile in (ifs);
        
        assert(in.parts() == 2);
        
        assert(in.header(0).dataWindow()==in.header(1).dataWindow());
        
        
        const Box2i &dw = in.header(0).dataWindow();
        int w = dw.max.x - dw.min.x + 1;
        int h = dw.max.y - dw.min.y + 1;
        int dx = dw.min.x;
        int dy = dw.min.y;
        
        Array2D<Rgba> p2 (h, w);
        FrameBuffer f;
        f.insert("R",Slice(HALF,(char *) &p2[-dy][-dx].r,sizeof(Rgba),w*sizeof(Rgba)));
        f.insert("G",Slice(HALF,(char *) &p2[-dy][-dx].g,sizeof(Rgba),w*sizeof(Rgba)));
        f.insert("B",Slice(HALF,(char *) &p2[-dy][-dx].b,sizeof(Rgba),w*sizeof(Rgba)));
        f.insert("A",Slice(HALF,(char *) &p2[-dy][-dx].a,sizeof(Rgba),w*sizeof(Rgba)));
        
        for(int part=0;part<2;part++)
        {
            InputPart p(in,part);
            p.setFrameBuffer(f);
            p.readPixels (dw.min.y, dw.max.y);
            
            cout << ", comparing pt " << part;
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    assert (p2[y][x].r == p1[y][x].r);
                    assert (p2[y][x].g == p1[y][x].g);
                    assert (p2[y][x].b == p1[y][x].b);
                    assert (p2[y][x].a == p1[y][x].a);
                }
            }
        }
    }
    
    cout << endl;
                        
    remove (fileName);
}
                    


void
writeReadTiles (const char fileName[],
		int width,
		int height,
		const Array2D<Rgba> &p1)
{
    //
    // Save a tiled RGBA image, but instead of letting
    // the TiledRgbaOutputFile object open the file, make
    // it use an existing StdOFStream.  Read the image back,
    // using an existing StdIFStream, and compare the pixels
    // with the original data.  Then read the image back a
    // second time using a memory-mapped MMIFStream (see above).
    //

    cout << "tiled file:" << endl;

    {
        cout << "writing";
        remove (fileName);
        std::ofstream os;
        testutil::OpenStreamWithUTF8Name (
            os, fileName, ios_base::out | ios_base::binary);
        StdOFStream ofs (os, fileName);
        Header header (width, height);
        TiledRgbaOutputFile out (ofs, header, WRITE_RGBA, 20, 20, ONE_LEVEL);
        out.setFrameBuffer (&p1[0][0], 1, width);
        out.writeTiles (0, out.numXTiles() - 1, 0, out.numYTiles() - 1);
    }

    {
        cout << ", reading";
        std::ifstream is;
        testutil::OpenStreamWithUTF8Name (
            is, fileName, ios::in | ios_base::binary);
        StdIFStream ifs (is, fileName);
        TiledRgbaInputFile in (ifs);

	const Box2i &dw = in.dataWindow();
	int w = dw.max.x - dw.min.x + 1;
	int h = dw.max.y - dw.min.y + 1;
	int dx = dw.min.x;
	int dy = dw.min.y;

	Array2D<Rgba> p2 (h, w);
	in.setFrameBuffer (&p2[-dy][-dx], 1, w);
        in.readTiles (0, in.numXTiles() - 1, 0, in.numYTiles() - 1);

        cout << ", comparing";
	for (int y = 0; y < h; ++y)
	{
	    for (int x = 0; x < w; ++x)
	    {
		assert (p2[y][x].r == p1[y][x].r);
		assert (p2[y][x].g == p1[y][x].g);
		assert (p2[y][x].b == p1[y][x].b);
		assert (p2[y][x].a == p1[y][x].a);
	    }
	}
    }
    
    {
        cout << ", reading (memory-mapped)";
	MMIFStream ifs (fileName);
	TiledRgbaInputFile in (ifs);

	const Box2i &dw = in.dataWindow();
	int w = dw.max.x - dw.min.x + 1;
	int h = dw.max.y - dw.min.y + 1;
	int dx = dw.min.x;
	int dy = dw.min.y;

	Array2D<Rgba> p2 (h, w);
	in.setFrameBuffer (&p2[-dy][-dx], 1, w);
        in.readTiles (0, in.numXTiles() - 1, 0, in.numYTiles() - 1);

        cout << ", comparing";
	for (int y = 0; y < h; ++y)
	{
	    for (int x = 0; x < w; ++x)
	    {
		assert (p2[y][x].r == p1[y][x].r);
		assert (p2[y][x].g == p1[y][x].g);
		assert (p2[y][x].b == p1[y][x].b);
		assert (p2[y][x].a == p1[y][x].a);
	    }
	}
    }
    
    cout << endl;

    remove (fileName);
}


//
// stringstream version
//
void
writeReadScanLines (int width,
		    int height,
		    const Array2D<Rgba> &p1)
{
    //
    // Save a scanline-based RGBA image, but instead of
    // letting the RgbaOutputFile object open the file,
    // make the RgbaOutputFile object use an existing
    // StdOSStream.  Read the image back, using an
    // existing StdISStream, and compare the pixels
    // with the original data.
    //

    cout << "scan-line based stringstream:" << endl;

    std::string strEXRFile;

    {
        cout << "writing";
        StdOSStream oss;
        Header header (width, height);
        RgbaOutputFile out (oss, header, WRITE_RGBA);
        out.setFrameBuffer (&p1[0][0], 1, width);
        out.writePixels (height);
        strEXRFile = oss.str();
    }

    {
        cout << ", reading";
        StdISStream iss;
        iss.clear();
        iss.str(strEXRFile);
        RgbaInputFile in (iss);

	const Box2i &dw = in.dataWindow();
	int w = dw.max.x - dw.min.x + 1;
	int h = dw.max.y - dw.min.y + 1;
	int dx = dw.min.x;
	int dy = dw.min.y;

	Array2D<Rgba> p2 (h, w);
	in.setFrameBuffer (&p2[-dy][-dx], 1, w);
	in.readPixels (dw.min.y, dw.max.y);

        cout << ", comparing";
	for (int y = 0; y < h; ++y)
	{
	    for (int x = 0; x < w; ++x)
	    {
		assert (p2[y][x].r == p1[y][x].r);
		assert (p2[y][x].g == p1[y][x].g);
		assert (p2[y][x].b == p1[y][x].b);
		assert (p2[y][x].a == p1[y][x].a);
	    }
	}
    }
        
    cout << endl;
}


//
// stringstream version
//
void
writeReadMultiPart (int width,
                    int height,
                    const Array2D<Rgba> &p1)
{
    //
    // Save a two scanline parts in an image, but instead of
    // letting the MultiPartOutputFile object open the file,
    // make the MultiPartOutputFile object use an existing
    // StdOSStream.  Read the image back, using an
    // existing StdISStream, and compare the pixels
    // with the original data.
    //
                        
    cout << "scan-line based mulitpart stringstream:" << endl;
                            
    std::string strEXRFile;

    {
        cout << "writing";
        StdOSStream oss;

        vector<Header> headers(2);
        headers[0] = Header(width, height);
        headers[0].setName("part1");
        headers[0].channels().insert("R",Channel());
        headers[0].channels().insert("G",Channel());
        headers[0].channels().insert("B",Channel());
        headers[0].channels().insert("A",Channel());
        headers[0].setType(SCANLINEIMAGE);

        headers[1]=headers[0];
        headers[1].setName("part2");

        MultiPartOutputFile out (oss, &headers[0],2);
        FrameBuffer f;
        f.insert("R",Slice(HALF,(char *) &p1[0][0].r,sizeof(Rgba),width*sizeof(Rgba)));
        f.insert("G",Slice(HALF,(char *) &p1[0][0].g,sizeof(Rgba),width*sizeof(Rgba)));
        f.insert("B",Slice(HALF,(char *) &p1[0][0].b,sizeof(Rgba),width*sizeof(Rgba)));
        f.insert("A",Slice(HALF,(char *) &p1[0][0].a,sizeof(Rgba),width*sizeof(Rgba)));
        
        for(int i=0;i<2;i++)
        {
            OutputPart p(out,i);
            p.setFrameBuffer (f);
            p.writePixels (height);
        }

        strEXRFile = oss.str();
    }

    {
        cout << ", reading";
        StdISStream iss;
        iss.clear();
        iss.str(strEXRFile);
        MultiPartInputFile in (iss);
        
        assert(in.parts() == 2);
        
        assert(in.header(0).dataWindow()==in.header(1).dataWindow());
        
        const Box2i &dw = in.header(0).dataWindow();
        int w = dw.max.x - dw.min.x + 1;
        int h = dw.max.y - dw.min.y + 1;
        int dx = dw.min.x;
        int dy = dw.min.y;
        
        Array2D<Rgba> p2 (h, w);
        FrameBuffer f;
        f.insert("R",Slice(HALF,(char *) &p2[-dy][-dx].r,sizeof(Rgba),w*sizeof(Rgba)));
        f.insert("G",Slice(HALF,(char *) &p2[-dy][-dx].g,sizeof(Rgba),w*sizeof(Rgba)));
        f.insert("B",Slice(HALF,(char *) &p2[-dy][-dx].b,sizeof(Rgba),w*sizeof(Rgba)));
        f.insert("A",Slice(HALF,(char *) &p2[-dy][-dx].a,sizeof(Rgba),w*sizeof(Rgba)));
        
        for(int part=0;part<2;part++)
        {
            InputPart p(in,part);
            p.setFrameBuffer(f);
            p.readPixels (dw.min.y, dw.max.y);
                            
            cout << ", comparing pt " << part;
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    assert (p2[y][x].r == p1[y][x].r);
                    assert (p2[y][x].g == p1[y][x].g);
                    assert (p2[y][x].b == p1[y][x].b);
                    assert (p2[y][x].a == p1[y][x].a);
                }
            }
        }
    }
    
    cout << endl;
}
                    
//
// stringstream version
//
void
writeReadTiles (int width,
		int height,
		const Array2D<Rgba> &p1)
{
    //
    // Save a tiled RGBA image, but instead of letting
    // the TiledRgbaOutputFile object open the file, make
    // it use an existing StdOSStream.  Read the image back,
    // using an existing StdISStream, and compare the pixels
    // with the original data.  
    //

    cout << "tiled stringstream:" << endl;

    std::string strEXRFile;

    {
        cout << "writing";
        StdOSStream oss;
        Header header (width, height);
        TiledRgbaOutputFile out (oss, header, WRITE_RGBA, 20, 20, ONE_LEVEL);
        out.setFrameBuffer (&p1[0][0], 1, width);
        out.writeTiles (0, out.numXTiles() - 1, 0, out.numYTiles() - 1);

        strEXRFile = oss.str();
    }

    {
        cout << ", reading";
        StdISStream iss;
        iss.clear();
        iss.str(strEXRFile);
        TiledRgbaInputFile in (iss);

	const Box2i &dw = in.dataWindow();
	int w = dw.max.x - dw.min.x + 1;
	int h = dw.max.y - dw.min.y + 1;
	int dx = dw.min.x;
	int dy = dw.min.y;

	Array2D<Rgba> p2 (h, w);
	in.setFrameBuffer (&p2[-dy][-dx], 1, w);
        in.readTiles (0, in.numXTiles() - 1, 0, in.numYTiles() - 1);

        cout << ", comparing";
	for (int y = 0; y < h; ++y)
	{
	    for (int x = 0; x < w; ++x)
	    {
		assert (p2[y][x].r == p1[y][x].r);
		assert (p2[y][x].g == p1[y][x].g);
		assert (p2[y][x].b == p1[y][x].b);
		assert (p2[y][x].a == p1[y][x].a);
	    }
	}
    }

    cout << endl;
}

} // namespace


void
testExistingStreams (const std::string &tempDir)
{
    try
    {
        cout << "Testing reading and writing using existing streams" << endl;

        const int W = 119;
        const int H = 237;

        Array2D<Rgba> p1 (H, W);

        fillPixels1 (p1, W, H);
        writeReadScanLines ((tempDir + "imf_test_streams.exr").c_str(), W, H, p1);
        writeReadScanLines (W, H, p1);

        fillPixels2 (p1, W, H);
        writeReadTiles ((tempDir + "imf_test_streams2.exr").c_str(), W, H, p1);
        writeReadTiles (W, H, p1);

        fillPixels1 (p1, W, H);
        writeReadMultiPart ((tempDir +  "imf_test_streams3.exr").c_str(), W, H, p1);
        writeReadMultiPart (W, H, p1);

        cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
