///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
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



#ifndef INCLUDED_IMF_RGBA_FILE_H
#define INCLUDED_IMF_RGBA_FILE_H


//-----------------------------------------------------------------------------
//
//	Simplified RGBA image I/O
//
//	class Rgba
//	class RgbaOutputFile
//	class RgbaInputFile
//
//-----------------------------------------------------------------------------

#include <ImfHeader.h>
#include <ImfFrameBuffer.h>
#include <ImathVec.h>
#include <ImathBox.h>
#include <half.h>

namespace Imf {


class OutputFile;
class InputFile;
struct PreviewRgba;


//
// RGBA pixel
//

struct Rgba
{
    half	r;
    half	g;
    half	b;
    half	a;
};


//
// Channels in an RGBA file
//

enum RgbaChannels
{
    WRITE_R    = 0x1,
    WRITE_G    = 0x2,
    WRITE_B    = 0x4,
    WRITE_A    = 0x8,
    WRITE_RGB  = 0x7,
    WRITE_RGBA = 0xf
};


//
// RGBA output file.
//

class RgbaOutputFile
{
  public:

    //---------------------------------------------------
    // Constructor -- header is constructed by the caller
    //---------------------------------------------------

    RgbaOutputFile (const char name[],
		    const Header &header,
		    RgbaChannels rgbaChannels = WRITE_RGBA);


    //----------------------------------------------------------------
    // Constructor -- header data are explicitly specified as function
    // call arguments (empty dataWindow means "same as displayWindow")
    //----------------------------------------------------------------

    RgbaOutputFile (const char name[],
		    const Imath::Box2i &displayWindow,
		    const Imath::Box2i &dataWindow = Imath::Box2i(),
		    RgbaChannels rgbaChannels = WRITE_RGBA,
		    float pixelAspectRatio = 1,
		    const Imath::V2f screenWindowCenter = Imath::V2f (0, 0),
		    float screenWindowWidth = 1,
		    LineOrder lineOrder = INCREASING_Y,
		    Compression compression = PIZ_COMPRESSION);


    //-----------------------------------------------
    // Constructor -- like the previous one, but both
    // the display window and the data window are
    // Box2i (V2i (0, 0), V2i (width - 1, height -1))
    //-----------------------------------------------

    RgbaOutputFile (const char name[],
		    int width,
		    int height,
		    RgbaChannels rgbaChannels = WRITE_RGBA,
		    float pixelAspectRatio = 1,
		    const Imath::V2f screenWindowCenter = Imath::V2f (0, 0),
		    float screenWindowWidth = 1,
		    LineOrder lineOrder = INCREASING_Y,
		    Compression compression = PIZ_COMPRESSION);


    //-----------
    // Destructor
    //-----------

    virtual ~RgbaOutputFile ();


    //------------------------------------------------
    // Define a frame buffer as the pixel data source:
    // Pixel (x, y) is at address
    //
    //  base + x * xStride + y * yStride
    //
    //------------------------------------------------

    void			setFrameBuffer (const Rgba *base,
						size_t xStride,
						size_t yStride);


    //---------------------------------------------
    // Write pixel data (see class Imf::OutputFile)
    //---------------------------------------------

    void			writePixels (int numScanLines = 1);
    int				currentScanLine () const;


    //--------------------------
    // Access to the file header
    //--------------------------

    const Header &		header () const;
    const FrameBuffer &		frameBuffer () const;
    const Imath::Box2i &	displayWindow () const;
    const Imath::Box2i &	dataWindow () const;
    float			pixelAspectRatio () const;
    const Imath::V2f		screenWindowCenter () const;
    float			screenWindowWidth () const;
    LineOrder			lineOrder () const;
    Compression			compression () const;
    RgbaChannels		channels () const;


    // --------------------------------------------------------------------
    // Update the preview image (see Imf::OutputFile::updatePreviewImage())
    // --------------------------------------------------------------------

    void			updatePreviewImage (const PreviewRgba[]);

  private:

    RgbaOutputFile (const RgbaOutputFile &);		  // not implemented
    RgbaOutputFile & operator = (const RgbaOutputFile &); // not implemented

    OutputFile *		_outputFile;
};


//
// RGBA input file
//

class RgbaInputFile
{
  public:

    //---------------------------
    // Constructor and destructor
    //---------------------------

    RgbaInputFile (const char name[]);
    virtual ~RgbaInputFile ();

    //-----------------------------------------------------
    // Define a frame buffer as the pixel data destination:
    // Pixel (x, y) is at address
    //
    //  base + x * xStride + y * yStride
    //
    //-----------------------------------------------------

    void			setFrameBuffer (Rgba *base,
						size_t xStride,
						size_t yStride);


    //-------------------------------------------
    // Read pixel data (see class Imf::InputFile)
    //-------------------------------------------

    void			readPixels (int scanLine1, int scanLine2);
    void			readPixels (int scanLine);


    //--------------------------
    // Access to the file header
    //--------------------------

    const Header &		header () const;
    const FrameBuffer &		frameBuffer () const;
    const Imath::Box2i &	displayWindow () const;
    const Imath::Box2i &	dataWindow () const;
    float			pixelAspectRatio () const;
    const Imath::V2f		screenWindowCenter () const;
    float			screenWindowWidth () const;
    LineOrder			lineOrder () const;
    Compression			compression () const;
    RgbaChannels		channels () const;
    const char *                fileName () const;

    //----------------------------------
    // Access to the file format version
    //----------------------------------

    int				version () const;

  private:

    RgbaInputFile (const RgbaInputFile &);		  // not implemented
    RgbaInputFile & operator = (const RgbaInputFile &);   // not implemented

    InputFile *			_inputFile;
};


} // namespace Imf

#endif
