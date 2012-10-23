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

//=============================================================================
//
// exrFormat.h
// This file contains the class declarations for:
// NRxOpenEXRTranslator, NRxOpenEXRReader, and NRxOpenEXRWriter.
//
//=============================================================================

#ifndef __NRxOpenEXRTranslator_H__
#define __NRxOpenEXRTranslator_H__

#include <NRiBuffer.h>
#include <NRiFile.h>
#include <NRxImageTranslator.h>

namespace Imf {
class RgbaInputFile;
class RgbaOutputFile;
class Header;
}

const unsigned char OPENEXR_MAGIC_0 = 0x76;
const unsigned char OPENEXR_MAGIC_1 = 0x2f;
const unsigned char OPENEXR_MAGIC_2 = 0x31;
const unsigned char OPENEXR_MAGIC_3 = 0x01;

const unsigned char OPENEXR_BYTES = 4; // always produce float images

class NRxOpenEXRTranslator : public NRxImageTranslator {

public:

    virtual NRiName getName() const;

    virtual NRiName getReadableFormats() const;
    
    virtual NRiName getWritableFormats() const;

    virtual int	checkHeader(NRiFile &) const;

    virtual NRxImageReader *createReader() const;
    
    virtual NRxImageWriter *createWriter() const;
};

class NRxOpenEXRReader : public NRxImageReader {

public:		    
    
    NRxOpenEXRReader();
    
    virtual ~NRxOpenEXRReader();

    virtual int eval(NRiPlug *p);

    // override for closing of file
    virtual int endExec();
private:

    // Store the error state
    bool	hasError;   
    // Flag to indicate whether or not the header has been read yet
    bool	isHeaderRead;

    Imf::RgbaInputFile	*exrFile;
    Imf::Header		*exrHeader;
    NRiIRect		outputDod; 

    // Use a flip node to handle scanline order
    NRiIPlug *fOut; 

    void readHeader();

    NRiDeclareNodeName(NRxOpenEXRReader);
};



class NRxOpenEXRWriter : public NRxImageWriter {

public:		    
		    NRxOpenEXRWriter();
    virtual	   ~NRxOpenEXRWriter();

    virtual int	    eval(NRiPlug *);

    virtual void    bgnOutput(void *&cPtr, float *&zPtr, int &cStride, int &zStride, int line, int nLine);
    virtual void    endOutput(int);

private:

    NRiIRect	    iRoi;
    NRiBuffer	    cBuf;
    int		    hasError;
    int		    headerWritten;
    NRiPlug	*pCompress;
  
    Imf::RgbaOutputFile *exrFile;

    void	    writeHeader();

    NRiDeclareNodeName(NRxOpenEXRWriter);
};

#endif


