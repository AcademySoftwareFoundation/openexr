//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <ImfArray.h>
#include <ImfCompressor.h>
#include <ImfInputPart.h>
#include <ImfMisc.h>
#include <ImfMultiPartInputFile.h>
#include <ImfMultiPartOutputFile.h>
#include <ImfOutputPart.h>
#include <ImfPartType.h>
#include <ImfRgbaFile.h>
#include <ImfStdIO.h>
#include <ImfTiledRgbaFile.h>

#include "Iex.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>

#ifdef _WIN32
#else
#    include <fcntl.h>
#    include <unistd.h>
#    include <sys/mman.h>
#    include <sys/stat.h>
#endif

#include <ImfChannelList.h>
#include <vector>

#include "TestUtilFStream.h"

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;
using namespace IMATH_NAMESPACE;

namespace
{

void
fillPixels1 (Array2D<Rgba>& pixels, int w, int h)
{
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            Rgba& p = pixels[y][x];

            p.r = (x & 1);
            p.g = ((x + y) & 1);
            p.b = (y & 1);
            p.a = (p.r + p.b + p.g) / 3.0;
        }
    }
}

void
fillPixels2 (Array2D<Rgba>& pixels, int w, int h)
{
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            Rgba& p = pixels[y][x];

            p.r = (x & 2);
            p.g = ((x + y) & 2);
            p.b = (y & 2);
            p.a = (p.r + p.b + p.g) / 3.0;
        }
    }
}

//
// class MMIFStream -- a memory-mapped implementation of
// class IStream
//

class MMIFStream : public OPENEXR_IMF_NAMESPACE::IStream
{
public:
    //-------------------------------------------------------
    // A constructor that opens the file with the given name.
    //-------------------------------------------------------

    MMIFStream (const char fileName[]);

    virtual ~MMIFStream ();

    virtual bool isMemoryMapped () const { return true; }

    virtual bool     read (char c[/*n*/], int n);
    virtual char*    readMemoryMapped (int n);
    virtual uint64_t tellg () { return _pos; }
    virtual void     seekg (uint64_t pos) { _pos = pos; }
    virtual void     clear () {}

private:
#ifdef _WIN32
    HANDLE _f = INVALID_HANDLE_VALUE;
#else
    int _f;
#endif
    void*       _mmap;
    const char* _mmapStart;
    uint64_t    _pos;
    uint64_t    _length;
};

MMIFStream::MMIFStream (const char fileName[])
    : OPENEXR_IMF_NAMESPACE::IStream (fileName)
#ifdef _WIN32
    , _f (INVALID_HANDLE_VALUE)
#else
    , _f (-1)
#endif
    , _mmap (reinterpret_cast<void*> (-1))
    , _mmapStart (nullptr)
    , _pos (0)
    , _length (0)
{
#ifdef _WIN32
    const std::wstring fileNameWide = WidenFilename (fileName);
    try
    {
        _f = CreateFileW (
            fileNameWide.c_str (),
            GENERIC_READ,
            FILE_SHARE_READ,
            0,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            0);
    }
    catch (const std::exception&)
    {
        _f = INVALID_HANDLE_VALUE;
    }
    if (INVALID_HANDLE_VALUE == _f)
    {
        throw IEX_NAMESPACE::IoExc ("Cannot open file.");
    }

    struct _stati64 s;
    memset (&s, 0, sizeof (struct _stati64));
    if (_wstati64 (fileNameWide.c_str (), &s) != 0)
    {
        throw IEX_NAMESPACE::IoExc ("Cannot stat file.");
    }

    _length = s.st_size;

    _mmap = CreateFileMapping (_f, 0, PAGE_READONLY, 0, 0, 0);
    if (!_mmap) { throw IEX_NAMESPACE::IoExc ("Cannot memory map file."); }

    _mmapStart = reinterpret_cast<const char*> (
        MapViewOfFile (_mmap, FILE_MAP_READ, 0, 0, 0));
    if (!_mmapStart)
    {
        throw IEX_NAMESPACE::IoExc ("Cannot map view of file.");
    }

#else

    _f = open (fileName, O_RDONLY);
    if (-1 == _f) { throw IEX_NAMESPACE::IoExc ("Cannot open file."); }

    struct stat s;
    memset (&s, 0, sizeof (struct stat));
    if (stat (fileName, &s) != 0)
    {
        throw IEX_NAMESPACE::IoExc ("Cannot stat file.");
    }

    _length = s.st_size;

    _mmap = mmap (0, _length, PROT_READ, MAP_SHARED, _f, 0);
    if (_mmap == (void*) -1)
    {
        throw IEX_NAMESPACE::IoExc ("Cannot memory map file.");
    }

    _mmapStart = reinterpret_cast<char*> (_mmap);
#endif
}

MMIFStream::~MMIFStream ()
{
#ifdef _WIN32
    if (_mmapStart) { UnmapViewOfFile ((void*) _mmapStart); }
    if (_mmap) { CloseHandle (_mmap); }
    if (_f != INVALID_HANDLE_VALUE) { CloseHandle (_f); }

#else

    if (_mmap != (void*) -1) { munmap (_mmap, _length); }
    if (_f != -1) { close (_f); }
#endif
}

bool
MMIFStream::read (char c[/*n*/], int n)
{
    if (_pos >= _length && n != 0)
        throw IEX_NAMESPACE::InputExc ("Unexpected end of file.");

    uint64_t n2     = n;
    bool     retVal = true;

    if (_length - _pos <= n2)
    {
        n2     = _length - _pos;
        retVal = false;
    }

    memcpy (c, _mmapStart + _pos, n2);
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

    char* retVal = const_cast<char*> (_mmapStart) + _pos;
    _pos += n;
    return retVal;
}

class PassThruIFStream : public OPENEXR_IMF_NAMESPACE::IStream
{
public:
    //-------------------------------------------------------
    // A constructor that opens the file with the given name.
    //-------------------------------------------------------

    PassThruIFStream (MMIFStream &s) : IStream(""), _s (s) {}

    virtual ~PassThruIFStream () {}

    virtual bool isMemoryMapped () const { return _s.isMemoryMapped (); }

    virtual bool     read (char c[/*n*/], int n) { return _s.read (c, n); }
    virtual char*    readMemoryMapped (int n) { return _s.readMemoryMapped (n); }
    virtual uint64_t tellg () { return _s.tellg (); }
    virtual void     seekg (uint64_t pos) { _s.seekg (pos); }
    virtual void     clear () { _s.clear (); }

private:
    MMIFStream &_s;
};

void
writeReadScanLines (
    const char           fileName[],
    int                  width,
    int                  height,
    Compression          compression,
    const Array2D<Rgba>& p1)
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
        Header      header (
            width,
            height,
            1,
            IMATH_NAMESPACE::V2f (0, 0),
            1,
            INCREASING_Y,
            compression);
        RgbaOutputFile out (ofs, header, WRITE_RGBA);
        out.setFrameBuffer (&p1[0][0], 1, width);
        out.writePixels (height);
    }

    {
        cout << ", reading";
        std::ifstream is;
        testutil::OpenStreamWithUTF8Name (
            is, fileName, ios::in | ios_base::binary);
        StdIFStream   ifs (is, fileName);
        RgbaInputFile in (ifs);

        const Box2i& dw = in.dataWindow ();
        int          w  = dw.max.x - dw.min.x + 1;
        int          h  = dw.max.y - dw.min.y + 1;
        int          dx = dw.min.x;
        int          dy = dw.min.y;

        Array2D<Rgba> p2 (h, w);
        in.setFrameBuffer (&p2[-dy][-dx], 1, w);
        in.readPixels (dw.min.y, dw.max.y);

        if (!isLossyCompression (compression))
        {
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
    }

    {
        cout << ", reading (memory-mapped)";
        MMIFStream    ifs (fileName);
        RgbaInputFile in (ifs);

        const Box2i& dw = in.dataWindow ();
        int          w  = dw.max.x - dw.min.x + 1;
        int          h  = dw.max.y - dw.min.y + 1;
        int          dx = dw.min.x;
        int          dy = dw.min.y;

        Array2D<Rgba> p2 (h, w);
        in.setFrameBuffer (&p2[-dy][-dx], 1, w);
        in.readPixels (dw.min.y, dw.max.y);

        if (!isLossyCompression (compression))
        {
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
    }

    {
        cout << ", reading (memory-mapped, passthru)";
        MMIFStream       ifs (fileName);
        PassThruIFStream pfs (ifs);

        RgbaInputFile in (pfs);

        const Box2i& dw = in.dataWindow ();
        int          w  = dw.max.x - dw.min.x + 1;
        int          h  = dw.max.y - dw.min.y + 1;
        int          dx = dw.min.x;
        int          dy = dw.min.y;

        Array2D<Rgba> p2 (h, w);
        in.setFrameBuffer (&p2[-dy][-dx], 1, w);
        in.readPixels (dw.min.y, dw.max.y);

        if (!isLossyCompression (compression))
        {
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
    }

    cout << endl;

    remove (fileName);
}

void
writeReadMultiPart (
    const char           fileName[],
    int                  width,
    int                  height,
    Compression          compression,
    const Array2D<Rgba>& p1)
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

    cout << "scan-line based multipart file:" << endl;

    {
        cout << "writing";
        remove (fileName);
        std::ofstream os;
        testutil::OpenStreamWithUTF8Name (
            os, fileName, ios::out | ios_base::binary);
        StdOFStream ofs (os, fileName);

        vector<Header> headers (2);
        headers[0] = Header (
            width,
            height,
            1,
            IMATH_NAMESPACE::V2f (0, 0),
            1,
            INCREASING_Y,
            compression);
        headers[0].setName ("part1");
        headers[0].channels ().insert ("R", Channel ());
        headers[0].channels ().insert ("G", Channel ());
        headers[0].channels ().insert ("B", Channel ());
        headers[0].channels ().insert ("A", Channel ());
        headers[0].setType (SCANLINEIMAGE);

        headers[1] = headers[0];
        headers[1].setName ("part2");

        MultiPartOutputFile out (ofs, &headers[0], 2);
        FrameBuffer         f;
        f.insert (
            "R",
            Slice (
                HALF,
                (char*) &p1[0][0].r,
                sizeof (Rgba),
                width * sizeof (Rgba)));
        f.insert (
            "G",
            Slice (
                HALF,
                (char*) &p1[0][0].g,
                sizeof (Rgba),
                width * sizeof (Rgba)));
        f.insert (
            "B",
            Slice (
                HALF,
                (char*) &p1[0][0].b,
                sizeof (Rgba),
                width * sizeof (Rgba)));
        f.insert (
            "A",
            Slice (
                HALF,
                (char*) &p1[0][0].a,
                sizeof (Rgba),
                width * sizeof (Rgba)));

        for (int i = 0; i < 2; i++)
        {
            OutputPart p (out, i);
            p.setFrameBuffer (f);
            p.writePixels (height);
        }
    }

    {
        cout << ", reading";
        std::ifstream is;
        testutil::OpenStreamWithUTF8Name (
            is, fileName, ios::in | ios_base::binary);
        StdIFStream        ifs (is, fileName);
        MultiPartInputFile in (ifs);

        assert (in.parts () == 2);

        assert (in.header (0).dataWindow () == in.header (1).dataWindow ());

        const Box2i& dw = in.header (0).dataWindow ();
        int          w  = dw.max.x - dw.min.x + 1;
        int          h  = dw.max.y - dw.min.y + 1;
        int          dx = dw.min.x;
        int          dy = dw.min.y;

        Array2D<Rgba> p2 (h, w);
        FrameBuffer   f;
        f.insert (
            "R",
            Slice (
                HALF,
                (char*) &p2[-dy][-dx].r,
                sizeof (Rgba),
                w * sizeof (Rgba)));
        f.insert (
            "G",
            Slice (
                HALF,
                (char*) &p2[-dy][-dx].g,
                sizeof (Rgba),
                w * sizeof (Rgba)));
        f.insert (
            "B",
            Slice (
                HALF,
                (char*) &p2[-dy][-dx].b,
                sizeof (Rgba),
                w * sizeof (Rgba)));
        f.insert (
            "A",
            Slice (
                HALF,
                (char*) &p2[-dy][-dx].a,
                sizeof (Rgba),
                w * sizeof (Rgba)));

        for (int part = 0; part < 2; part++)
        {
            InputPart p (in, part);
            p.setFrameBuffer (f);
            p.readPixels (dw.min.y, dw.max.y);

            if (!isLossyCompression (compression))
            {
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
    }

    {
        cout << ", reading (memory-mapped)";
        MMIFStream         ifs (fileName);
        MultiPartInputFile in (ifs);

        assert (in.parts () == 2);

        assert (in.header (0).dataWindow () == in.header (1).dataWindow ());

        const Box2i& dw = in.header (0).dataWindow ();
        int          w  = dw.max.x - dw.min.x + 1;
        int          h  = dw.max.y - dw.min.y + 1;
        int          dx = dw.min.x;
        int          dy = dw.min.y;

        Array2D<Rgba> p2 (h, w);
        FrameBuffer   f;
        f.insert (
            "R",
            Slice (
                HALF,
                (char*) &p2[-dy][-dx].r,
                sizeof (Rgba),
                w * sizeof (Rgba)));
        f.insert (
            "G",
            Slice (
                HALF,
                (char*) &p2[-dy][-dx].g,
                sizeof (Rgba),
                w * sizeof (Rgba)));
        f.insert (
            "B",
            Slice (
                HALF,
                (char*) &p2[-dy][-dx].b,
                sizeof (Rgba),
                w * sizeof (Rgba)));
        f.insert (
            "A",
            Slice (
                HALF,
                (char*) &p2[-dy][-dx].a,
                sizeof (Rgba),
                w * sizeof (Rgba)));

        for (int part = 0; part < 2; part++)
        {
            InputPart p (in, part);
            p.setFrameBuffer (f);
            p.readPixels (dw.min.y, dw.max.y);

            if (!isLossyCompression (compression))
            {
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
    }

    cout << endl;

    remove (fileName);
}

void
writeReadTiles (
    const char           fileName[],
    int                  width,
    int                  height,
    Compression          compression,
    const Array2D<Rgba>& p1)
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
        Header      header (
            width,
            height,
            1,
            IMATH_NAMESPACE::V2f (0, 0),
            1,
            INCREASING_Y,
            compression);
        TiledRgbaOutputFile out (ofs, header, WRITE_RGBA, 20, 20, ONE_LEVEL);
        out.setFrameBuffer (&p1[0][0], 1, width);
        out.writeTiles (0, out.numXTiles () - 1, 0, out.numYTiles () - 1);
    }

    {
        cout << ", reading";
        std::ifstream is;
        testutil::OpenStreamWithUTF8Name (
            is, fileName, ios::in | ios_base::binary);
        StdIFStream        ifs (is, fileName);
        TiledRgbaInputFile in (ifs);

        const Box2i& dw = in.dataWindow ();
        int          w  = dw.max.x - dw.min.x + 1;
        int          h  = dw.max.y - dw.min.y + 1;
        int          dx = dw.min.x;
        int          dy = dw.min.y;

        Array2D<Rgba> p2 (h, w);
        in.setFrameBuffer (&p2[-dy][-dx], 1, w);
        in.readTiles (0, in.numXTiles () - 1, 0, in.numYTiles () - 1);

        if (!isLossyCompression (compression))
        {
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
    }

    {
        cout << ", reading (memory-mapped)";
        MMIFStream         ifs (fileName);
        TiledRgbaInputFile in (ifs);

        const Box2i& dw = in.dataWindow ();
        int          w  = dw.max.x - dw.min.x + 1;
        int          h  = dw.max.y - dw.min.y + 1;
        int          dx = dw.min.x;
        int          dy = dw.min.y;

        Array2D<Rgba> p2 (h, w);
        in.setFrameBuffer (&p2[-dy][-dx], 1, w);
        in.readTiles (0, in.numXTiles () - 1, 0, in.numYTiles () - 1);

        if (!isLossyCompression (compression))
        {
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
    }

    cout << endl;

    remove (fileName);
}

//
// stringstream version
//
void
writeReadScanLines (int width, int height, const Array2D<Rgba>& p1)
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
        StdOSStream    oss;
        Header         header (width, height);
        RgbaOutputFile out (oss, header, WRITE_RGBA);
        out.setFrameBuffer (&p1[0][0], 1, width);
        out.writePixels (height);
        strEXRFile = oss.str ();
    }

    {
        cout << ", reading";
        StdISStream iss;
        iss.clear ();
        iss.str (strEXRFile);
        RgbaInputFile in (iss);

        const Box2i& dw = in.dataWindow ();
        int          w  = dw.max.x - dw.min.x + 1;
        int          h  = dw.max.y - dw.min.y + 1;
        int          dx = dw.min.x;
        int          dy = dw.min.y;

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
writeReadMultiPart (int width, int height, const Array2D<Rgba>& p1)
{
    //
    // Save a two scanline parts in an image, but instead of
    // letting the MultiPartOutputFile object open the file,
    // make the MultiPartOutputFile object use an existing
    // StdOSStream.  Read the image back, using an
    // existing StdISStream, and compare the pixels
    // with the original data.
    //

    cout << "scan-line based multipart stringstream:" << endl;

    std::string strEXRFile;

    {
        cout << "writing";
        StdOSStream oss;

        vector<Header> headers (2);
        headers[0] = Header (width, height);
        headers[0].setName ("part1");
        headers[0].channels ().insert ("R", Channel ());
        headers[0].channels ().insert ("G", Channel ());
        headers[0].channels ().insert ("B", Channel ());
        headers[0].channels ().insert ("A", Channel ());
        headers[0].setType (SCANLINEIMAGE);

        headers[1] = headers[0];
        headers[1].setName ("part2");

        MultiPartOutputFile out (oss, &headers[0], 2);
        FrameBuffer         f;
        f.insert (
            "R",
            Slice (
                HALF,
                (char*) &p1[0][0].r,
                sizeof (Rgba),
                width * sizeof (Rgba)));
        f.insert (
            "G",
            Slice (
                HALF,
                (char*) &p1[0][0].g,
                sizeof (Rgba),
                width * sizeof (Rgba)));
        f.insert (
            "B",
            Slice (
                HALF,
                (char*) &p1[0][0].b,
                sizeof (Rgba),
                width * sizeof (Rgba)));
        f.insert (
            "A",
            Slice (
                HALF,
                (char*) &p1[0][0].a,
                sizeof (Rgba),
                width * sizeof (Rgba)));

        for (int i = 0; i < 2; i++)
        {
            OutputPart p (out, i);
            p.setFrameBuffer (f);
            p.writePixels (height);
        }

        strEXRFile = oss.str ();
    }

    {
        cout << ", reading";
        StdISStream iss;
        iss.clear ();
        iss.str (strEXRFile);
        MultiPartInputFile in (iss);

        assert (in.parts () == 2);

        assert (in.header (0).dataWindow () == in.header (1).dataWindow ());

        const Box2i& dw = in.header (0).dataWindow ();
        int          w  = dw.max.x - dw.min.x + 1;
        int          h  = dw.max.y - dw.min.y + 1;
        int          dx = dw.min.x;
        int          dy = dw.min.y;

        Array2D<Rgba> p2 (h, w);
        FrameBuffer   f;
        f.insert (
            "R",
            Slice (
                HALF,
                (char*) &p2[-dy][-dx].r,
                sizeof (Rgba),
                w * sizeof (Rgba)));
        f.insert (
            "G",
            Slice (
                HALF,
                (char*) &p2[-dy][-dx].g,
                sizeof (Rgba),
                w * sizeof (Rgba)));
        f.insert (
            "B",
            Slice (
                HALF,
                (char*) &p2[-dy][-dx].b,
                sizeof (Rgba),
                w * sizeof (Rgba)));
        f.insert (
            "A",
            Slice (
                HALF,
                (char*) &p2[-dy][-dx].a,
                sizeof (Rgba),
                w * sizeof (Rgba)));

        for (int part = 0; part < 2; part++)
        {
            InputPart p (in, part);
            p.setFrameBuffer (f);
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
writeReadTiles (int width, int height, const Array2D<Rgba>& p1)
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
        StdOSStream         oss;
        Header              header (width, height);
        TiledRgbaOutputFile out (oss, header, WRITE_RGBA, 20, 20, ONE_LEVEL);
        out.setFrameBuffer (&p1[0][0], 1, width);
        out.writeTiles (0, out.numXTiles () - 1, 0, out.numYTiles () - 1);

        strEXRFile = oss.str ();
    }

    {
        cout << ", reading";
        StdISStream iss;
        iss.clear ();
        iss.str (strEXRFile);
        TiledRgbaInputFile in (iss);

        const Box2i& dw = in.dataWindow ();
        int          w  = dw.max.x - dw.min.x + 1;
        int          h  = dw.max.y - dw.min.y + 1;
        int          dx = dw.min.x;
        int          dy = dw.min.y;

        Array2D<Rgba> p2 (h, w);
        in.setFrameBuffer (&p2[-dy][-dx], 1, w);
        in.readTiles (0, in.numXTiles () - 1, 0, in.numYTiles () - 1);

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
testExistingStreams (const std::string& tempDir)
{
    try
    {
        cout << "Testing reading and writing using existing streams" << endl;

        const int W = 119;
        const int H = 237;

        for (int compression = 0; compression < NUM_COMPRESSION_METHODS;
             ++compression)
        {
            cout << "compression: " << compression << endl;

            Array2D<Rgba> p1 (H, W);

            fillPixels1 (p1, W, H);
            writeReadScanLines (
                (tempDir + "imf_test_streams.exr").c_str (),
                W,
                H,
                static_cast<Compression> (compression),
                p1);
            writeReadScanLines (W, H, p1);

            fillPixels2 (p1, W, H);
            writeReadTiles (
                (tempDir + "imf_test_streams2.exr").c_str (),
                W,
                H,
                static_cast<Compression> (compression),
                p1);
            writeReadTiles (W, H, p1);

            fillPixels1 (p1, W, H);
            writeReadMultiPart (
                (tempDir + "imf_test_streams3.exr").c_str (),
                W,
                H,
                static_cast<Compression> (compression),
                p1);
            writeReadMultiPart (W, H, p1);
        }

        cout << "ok\n" << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}

void
testExistingStreamsUTF8 (const std::string& tempDir)
{

    cout << "Testing reading and writing using existing streams" << endl;

    const int W = 119;
    const int H = 237;
    Array2D<Rgba> p1 (H, W);

    fillPixels1 (p1, W, H);

    // per google translate, image in Japanese
    std::string   outfn = tempDir + "画像.exr";

    {
        cout << "writing";
#ifdef _WIN32
        _wremove (WidenFilename (outfn.c_str ()).c_str ());
#else
        remove (outfn.c_str ());
#endif
        Header      header (
            W,
            H,
            1,
            IMATH_NAMESPACE::V2f (0, 0),
            1,
            INCREASING_Y,
            NO_COMPRESSION);

        RgbaOutputFile out (
            outfn.c_str (),
            header,
            WRITE_RGBA);

        out.setFrameBuffer (&p1[0][0], 1, W);
        out.writePixels (H);
    }

    {
        cout << ", reading";
        RgbaInputFile in (outfn.c_str ());
        const Box2i& dw = in.dataWindow ();
        int          w  = dw.max.x - dw.min.x + 1;
        int          h  = dw.max.y - dw.min.y + 1;
        int          dx = dw.min.x;
        int          dy = dw.min.y;

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

#ifdef _WIN32
    _wremove (WidenFilename (outfn.c_str ()).c_str ());
#else
    remove (outfn.c_str ());
#endif
}
