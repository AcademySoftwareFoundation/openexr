//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


//-----------------------------------------------------------------------------
//
//	Code examples that show how implement custom low-level
//	file input and output for OpenEXR files:
//
//	Classes C_IStream and C_OStream are derived from OpenEXR's
//	abstract IStream and OStreamd classes.  They allow OpenEXR
//	file input and output via C stdio files (FILE *).
//
//-----------------------------------------------------------------------------

#include <ImfRgbaFile.h>
#include <ImfIO.h>
#include "Iex.h"

#include "drawImage.h"

#include <iostream>
#include <stdio.h>

#include "namespaceAlias.h"
using namespace IMF;
using namespace std;
using namespace IMATH_NAMESPACE;


#if defined(_MSC_VER)
#pragma warning (disable : 4996)
#endif

class C_IStream: public IStream
{
  public:

    C_IStream (FILE *file, const char fileName[]):
	IStream (fileName), _file (file) {}

    virtual bool	read (char c[/*n*/], int n);
    virtual uint64_t	tellg ();
    virtual void	seekg (uint64_t pos);
    virtual void	clear ();

  private:

    FILE *		_file;
};


class C_OStream: public OStream
{
  public:

    C_OStream (FILE *file, const char fileName[]):
	OStream (fileName), _file (file) {}

    virtual void	write (const char c[/*n*/], int n);
    virtual uint64_t	tellp ();
    virtual void	seekp (uint64_t pos);

  private:

    FILE *		_file;
};


bool
C_IStream::read (char c[/*n*/], int n)
{
    if (n != static_cast<int>(fread (c, 1, n, _file)))
    {
        //
        // fread() failed, but the return value does not distinguish
        // between I/O errors and end of file, so we call ferror() to
        // determine what happened.
        //

        if (ferror (_file))
            IEX_NAMESPACE::throwErrnoExc();
        else
            throw IEX_NAMESPACE::InputExc ("Unexpected end of file.");
    }

    return feof (_file);
}


uint64_t
C_IStream::tellg ()
{
    return ftell (_file);
}


void
C_IStream::seekg (uint64_t pos)
{
    clearerr (_file);
    fseek (_file, static_cast<long>(pos), SEEK_SET);
}


void
C_IStream::clear ()
{
    clearerr (_file);
}


void
C_OStream::write (const char c[/*n*/], int n)
{
    clearerr (_file);

    if (n != static_cast<int>(fwrite (c, 1, n, _file)))
        IEX_NAMESPACE::throwErrnoExc();
}


uint64_t
C_OStream::tellp ()
{
    return ftell (_file);
}


void
C_OStream::seekp (uint64_t pos)
{
    clearerr (_file);
    fseek (_file, static_cast<long>(pos), SEEK_SET);
}


void
writeRgbaFILE (FILE *cfile,
	       const char fileName[],
	       const Rgba *pixels,
	       int width,
	       int height)
{
    //
    // Store an RGBA image in a C stdio file that has already been opened:
    //
    //	- create a C_OStream object for writing to the file
    //	- create an RgbaOutputFile object, and attach it to the C_OStream
    //	- describe the memory layout of the pixels
    //	- store the pixels in the file
    //


    C_OStream ostr (cfile, fileName);
    RgbaOutputFile file (ostr, Header (width, height), WRITE_RGBA);
    file.setFrameBuffer (pixels, 1, width);
    file.writePixels (height);
}


void
readRgbaFILE (FILE *cfile,
	      const char fileName[],
	      Array2D<Rgba> &pixels,
	      int &width,
	      int &height)
{
    //
    // Read an RGBA image from a C stdio file that has already been opened:
    //
    //	- create a C_IStream object for reading from the file
    //	- create an RgbaInputFile object, and attach it to the C_IStream
    //	- allocate memory for the pixels
    //	- describe the memory layout of the pixels
    //	- read the pixels from the file
    //

    C_IStream istr (cfile, fileName);
    RgbaInputFile file (istr);
    Box2i dw = file.dataWindow();

    width  = dw.max.x - dw.min.x + 1;
    height = dw.max.y - dw.min.y + 1;
    pixels.resizeErase (height, width);

    file.setFrameBuffer (&pixels[0][0] - dw.min.x - dw.min.y * width, 1, width);
    file.readPixels (dw.min.y, dw.max.y);
}


void
lowLevelIoExamples ()
{
    cout << "\nCustom low-level file I/O\n" << endl;
    cout << "drawing image" << endl;

    int w = 800;
    int h = 600;
    const char *fileName = "rgba4.exr";

    Array2D<Rgba> p (h, w);
    drawImage1 (p, w, h);

    //
    // The following code is somewhat complicated because
    // fopen() returns 0 on error, but writeRgbaFILE() and
    // readRgbaFILE() throw exceptions.  Also, if a FILE *
    // goes out of scope, the corresponding file is not
    // automatically closed.  In order to avoid leaking
    // file descriptors, we have to make sure fclose() is
    // called in all cases.
    //

    cout << "writing image" << endl;

    {
	FILE *file = fopen (fileName, "wb");

	if (file == 0)
	{
	    THROW_ERRNO ("Cannot open file " << fileName << " (%T).");
	}
	else
	{
	    try
	    {
		writeRgbaFILE (file, fileName, &p[0][0], w, h);
	    }
	    catch (...)
	    {
		fclose (file);
		throw;
	    }

	    fclose (file);
	}
    }

    cout << "reading image" << endl;

    {
	FILE *file = fopen (fileName, "rb");

	if (file == 0)
	{
	    THROW_ERRNO ("Cannot open file " << fileName << " (%T).");
	}
	else
	{
	    try
	    {
		readRgbaFILE (file, fileName, p, w, h);
	    }
	    catch (...)
	    {
		fclose (file);
		throw;
	    }

	    fclose (file);
	}
    }
}
