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
// This file contains the class implementations for:
// NRxOpenEXRTranslator, NRxOpenEXRReader, and NRxOpenEXRWriter.
// Largely modelled after format.cpp from the Shake sdk.
//
// Primary author: Rod Bogart <rgb@ilm.com>
// 
//=============================================================================


#include "NRiGlobals.h"
#include "NRiScript.h"
#include <NRiSPlug.h>
#include <NRiFlip.h>
#include <NRiBytes.h>

#include "exrFormat.h"

#include <ImfRgbaFile.h>
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfArray.h>

static NRiPlug *closeIdleFiles  = NRiGlobals::ref("sys.closeIdleFiles", kInt);

static NRxOpenEXRTranslator	OpenEXRTranslator;

NRiName NRxOpenEXRTranslator::getName() const
{
    return "OpenEXR Format Translator";
}

NRiName NRxOpenEXRTranslator::getReadableFormats() const
{
    return "OpenEXR{exr}";
}

NRiName NRxOpenEXRTranslator::getWritableFormats() const
{
    return "OpenEXR{exr}";
}

int NRxOpenEXRTranslator::checkHeader(NRiFile &f) const
{
    unsigned char byte;
    if(f.read(&byte, 1) != 1 || byte != OPENEXR_MAGIC_0) { return 0; }
    if(f.read(&byte, 1) != 1 || byte != OPENEXR_MAGIC_1) { return 0; }
    if(f.read(&byte, 1) != 1 || byte != OPENEXR_MAGIC_2) { return 0; }
    if(f.read(&byte, 1) != 1 || byte != OPENEXR_MAGIC_3) { return 0; }
    return 1; 
}

NRxImageReader *NRxOpenEXRTranslator::createReader() const
{
    return new NRxOpenEXRReader;
}

NRxImageWriter *NRxOpenEXRTranslator::createWriter() const
{
    return new NRxOpenEXRWriter;
}

NRiImplementClassName(NRxOpenEXRReader, "NRxOpenEXRReader", "NRxImageReader");

NRxOpenEXRReader::NRxOpenEXRReader()
{
    // Since OpenEXR usually has top down scanlines and Shake
    // has bottom up pixels, we use a flip node in the reader/writer.
    NRiFlip *flip = new NRiFlip();
    addChild(flip);
    flip->settings->cacheMode()->set(0);
    out->connect(flip->out);
    fOut = new NRiIPlug("_flipOut", NRiPlug::kOut);
    addPlug(fOut, 1);
    flip->in->connect(fOut);

    fOut->bytes()->set(OPENEXR_BYTES);
    fOut->bData()->set(&bData);
    fOut->bPixel()->set(&bPixel);
    fOut->timeRange()->set(&timeRange);

    fOut->cacheId()->addDependency(pImageName);
    fOut->cacheId()->addDependents(fOut->width(), fOut->height(), fOut->active(),
				   fOut->dod(), 0);
    fOut->oBuf()->addDependencies(fOut->iBuf(), 0);

    hasError = false;
    isHeaderRead = false;

    exrFile = 0;
    exrHeader = 0;
}

NRxOpenEXRReader::~NRxOpenEXRReader()
{
    /*
     *	Not much to be done here since the Imf:InputFile
     *	is closed automatically 
     */
}

int NRxOpenEXRReader::endExec()
{
    delete exrFile;
    exrFile = 0;

    return NRxImageReader::endExec();
}

int NRxOpenEXRReader::eval(NRiPlug *p)
{
    if (p == fOut->cacheId()) {
	NRiName cId;

	delete exrFile;
	exrFile = 0;

	hasError = false;
	isHeaderRead = false;

	pImageName->setError(0);

	cId.sprintf(
	    "%s(\"%s\" /*%d*/)",
	    className().getString(),
	    pImageName->asString().getString(),
	    pFileTime->asInt()
	);

	p->set(cId.compress());

    } else if (p == fOut->width()) {
	readHeader();
	Imath::Box2i win = exrHeader->displayWindow();
	int width = win.max.x - win.min.x + 1;
	p->set(width);
    } else if (p == fOut->height()) {
	readHeader();
	Imath::Box2i win = exrHeader->displayWindow();
	int height = win.max.y - win.min.y + 1;
	p->set(height);
    } else if (p == fOut->bytes()) {
	readHeader();
	p->set(OPENEXR_BYTES); // assumes exr data should be made into float images
    } else if (p == fOut->active()) {
	readHeader();
	int c = 0;
	Imf::ChannelList channels = exrHeader->channels();
	if (channels.findChannel ("R"))
	    c |= Imf::WRITE_R;
	if (channels.findChannel ("G"))
	    c |= Imf::WRITE_G;
	if (channels.findChannel ("B"))
	    c |= Imf::WRITE_B;
	if (channels.findChannel ("A"))
	    c |= Imf::WRITE_A;
        // assume we need four channels if channels in file are anything other than rgb
	if (Imf::RgbaChannels(c) == Imf::WRITE_RGB)
	    p->set(kRGB);
	else
	    p->set(kRGBA);  
    } else if (p == fOut->dod()) {
	readHeader();
	Imath::Box2i dispwin = exrHeader->displayWindow();
	Imath::Box2i win = exrHeader->dataWindow();
	outputDod.X1 = win.min.x - dispwin.min.x;
	outputDod.Y1 = win.min.y - dispwin.min.y;
	outputDod.X2 = win.max.x + 1 - dispwin.min.x;
	outputDod.Y2 = win.max.y + 1 - dispwin.min.y;
	p->set(&outputDod);
    } else if (p == fOut->oBuf()) {
    
	NRiIBuf *   iBuf = fOut->getIBuf();
	int	    mask = fOut->getMask();

	readHeader();
	if (!exrFile) {
	    // its possible that the file was closed by readHeader
	    try 
	    {
		exrFile = new Imf::RgbaInputFile(pImageName->asString().getString());
	    }
	    catch (...)
	    {
		delete exrFile;
		exrFile = 0;
		hasError = true;
	    }
	}

	if (!hasError && iBuf != 0 && iBuf->cPtr != 0 && mask != 0) {
	    NRiIRect    oRoi;

	    fOut->getRoi(oRoi);

	    if (!oRoi.isNull()) {
		
		/*
		 *  The buffer pointed to by iBuf->cPtr is only large
		 *  enough to hold iBuf->nLine rows of pixels. The width
		 *  of each row is the width of the ROI.
		 */

		Imath::Box2i dispwin = exrHeader->displayWindow();
		int nx = outputDod.X2 - outputDod.X1;
		Imf::Array<Imf::Rgba> frameBuffer (nx);
		exrFile->setFrameBuffer (frameBuffer - (outputDod.X1 + dispwin.min.x), 1, 0);
		int lineSize = (oRoi.X2 - oRoi.X1) * OPENEXR_BYTES * 4; // the 4 is for abgr

		for (int y=0; y<iBuf->nLine && !hasError; ++y) {

		    // read each requested scanline and convert to float

		    int scanline = y + iBuf->line + dispwin.min.y;  

		    try
		    {
			exrFile->readPixels(scanline);
		    }
		    catch (...)
		    {
			NRiSys::error(
				      "%E%s: Error reading scanline %d from EXR file\n",
				      pImageName->asString().getString(), scanline);
			hasError = true;
		    }

		    int bufOffset = y * (lineSize + iBuf->cStride * OPENEXR_BYTES);
		    float *floatPixels = (float *) ((uint8_t *)iBuf->cPtr + bufOffset);

		    Imf::Rgba *pix = &frameBuffer[oRoi.X1-outputDod.X1];

		    // Assumes ROI is <= DOD
		    for(int x=oRoi.X1; x<oRoi.X2; x++) {
			*floatPixels++ = pix->a;
			*floatPixels++ = pix->b;
			*floatPixels++ = pix->g;
			*floatPixels++ = pix->r;
			pix++;
		    }
		}
	    }
	}

	p->set(iBuf);
    }
    return NRxImageReader::eval(p);
}


bool
tryOpenExrFile(const std::string &name, Imf::RgbaInputFile *&file, Imf::Header *&header)
{
    try 
    {
	file = new Imf::RgbaInputFile(name.c_str());
    }
    catch (...) //(const std::exception &e) We cannot use the std::exception in gcc2.95
    {
	NRiSys::error(
		      "%E%s: Could not open EXR file\n",
		      name.c_str());
	delete file;
	delete header;
	file = 0;
	header = 0;
	return true;
    }
    try
    {
	header = new Imf::Header(file->header());
    }
    catch (...)
    {
	NRiSys::error(
		      "%E%s: File is not an EXR file\n",
		      name.c_str());
	delete file;
	delete header;
	file = 0;
	header = 0;
	return true;
    }
    return false;
}

void NRxOpenEXRReader::readHeader()
{
    fOut->getCacheId();

    if (!isHeaderRead) {
	if (!hasError) {
	    hasError = tryOpenExrFile(pImageName->asString().getString(), exrFile, exrHeader);
	}

	if (hasError) {

	    //	Upon error, set some reasonable values.

	    pImageName->setError(1);
	    int w,h,b;
	    NRiScript::getDefaultDimensions(w,h,b);
	    exrHeader = new Imf::Header(w,h);
	}

	// close the file if user has set the global
        if (closeIdleFiles->asInt() != 0) {
	    delete exrFile;      
	    exrFile = 0;
	}
	isHeaderRead = true;
    }
}



NRiImplementClassName(NRxOpenEXRWriter, "NRxOpenEXRWriter", "NRxImageWriter");

NRxOpenEXRWriter::NRxOpenEXRWriter()
{
    hasError = 0;
    headerWritten = 0;

    exrFile = 0;

    pCompress = addPlug("exrCompression", kString);
    pCompress->set( "Piz" );

    // Pass input through a Bytes and Flip so we get 
    // float data the right way up
    NRiFlip *flip = new NRiFlip();
    addChild(flip);
    NRiBytes *bytes = new NRiBytes();
    addChild(bytes);
    bytes->outBytes->set(OPENEXR_BYTES);
    flip->in->connect(pIn->getInput());
    bytes->in->connect(flip->out);
    pIn->connect(bytes->out);

    pOut->roi()->addDependencies(pOut->dod(), 0);
    pOut->mask()->addDependencies(pOut->active(), 0);
}

NRxOpenEXRWriter::~NRxOpenEXRWriter()
{
    delete exrFile;
}

int NRxOpenEXRWriter::eval(NRiPlug *p)
{
    if (p == pOut->cacheId()) {

	NRiName cId = pIn->getCacheId();
	cId.sprintf("%s(%s, \"%s\")",
	    className().getString(),
	    cId.getString(),
	    pImageName->asString().getString()
	);
	p->set(cId.compress());

	hasError = 0;
	headerWritten = 0;

    } else if (p == pOut->roi()) {
	pOut->getDod(iRoi);

	p->set(&iRoi);
    } else if (p == pOut->mask()) {
	p->set(pOut->getActive() & kRGBA);
    }
    return NRxImageWriter::eval(p);
}

void NRxOpenEXRWriter::bgnOutput(void *&cPtr, float *&zPtr, int &cStride, int &zStride, int line, int nLine)
{
    writeHeader(); // opens the file
    cStride = 0;
    zStride = 0;
    zPtr = 0;
    if (hasError) {
	cPtr = 0;
	cBuf.deallocate();
    } else {
	int bufSize = (iRoi.X2 - iRoi.X1) * OPENEXR_BYTES * nLine * 4;  // 4 is for r g b a
	cBuf.allocate(bufSize);
	cPtr = cBuf.data.v;
    }
}

void NRxOpenEXRWriter::endOutput(int invalid) 
{
    if (hasError == 0 && invalid == 0) {
	NRiIBuf * iBuf = pOut->getOBuf();
	if (iBuf && iBuf->cPtr) {
	    Imath::Box2i win = exrFile->header().dataWindow();
	    int width = win.max.x - win.min.x + 1;
	    // copy floats to halfs for output of single scanlines
	    Imf::Array<Imf::Rgba> frameBuffer(width);
	    exrFile->setFrameBuffer(frameBuffer - win.min.x, 1, 0);

	    int lineSize = (win.max.x - win.min.x + 1) * OPENEXR_BYTES * 4; // the 4 is for abgr
	    for (int y=0; y<iBuf->nLine && !hasError; ++y) {

		int bufOffset = y * (lineSize + iBuf->cStride * OPENEXR_BYTES);
		float *floatPixels = (float *) ((uint8_t *)iBuf->cPtr + bufOffset);
		Imf::Rgba *pix = &frameBuffer[0];

		for(int x=win.min.x; x<=win.max.x; x++) {
		    pix->a = *floatPixels++;
		    pix->b = *floatPixels++;
		    pix->g = *floatPixels++;
		    pix->r = *floatPixels++;
		    pix++;
		}
		exrFile->writePixels(1);
	    }

	    if (hasError) {
		NRiSys::error(
		    "%E%s: %s\n",
		    pImageName->asString().getString(),
		    strerror(errno)
		);
		delete exrFile;
		exrFile = 0;
	    } else if (iBuf->line + iBuf->nLine > win.max.y) {
		delete exrFile; // closes the file
		exrFile = 0;
	    }
	}
    }
}

void NRxOpenEXRWriter::writeHeader()
{
    pOut->getCacheId();

    if (headerWritten == 0) {
	if (hasError == 0) {
	    try {
		Imf::Compression compress = Imf::PIZ_COMPRESSION;
		const char *compressString = pCompress->asString().getString();
		if (!strcmp(compressString, "None")) {
		    compress = Imf::NO_COMPRESSION;
		} else if (!strcmp(compressString, "RLE")) {
		    compress = Imf::RLE_COMPRESSION;
		} else if (!strcmp(compressString, "Zip Scanline")) {
		    compress = Imf::ZIPS_COMPRESSION;
		} else if (!strcmp(compressString, "Zip Block")) {
		    compress = Imf::ZIP_COMPRESSION;
		} else if (!strcmp(compressString, "Piz")) {
		    compress = Imf::PIZ_COMPRESSION;
		}
		Imf::RgbaChannels channels;
		if (pOut->getActive() == kRGBA)
		    channels = Imf::WRITE_RGBA;
		else
		    channels = Imf::WRITE_RGB;

		int width = pOut->getWidth();
		int height = pOut->getHeight();
		Imath::Box2i dataWindow (Imath::V2i (iRoi.X1, iRoi.Y1), Imath::V2i (iRoi.X2 - 1, iRoi.Y2 - 1));
		Imath::Box2i displayWindow (Imath::V2i (0, 0), Imath::V2i (width - 1, height - 1));

		Imf::Header outputHdr = Imf::Header(
		     displayWindow, dataWindow,
		     1, Imath::V2f(0,0), 1,
		     Imf::INCREASING_Y,
		     compress); 

		// TODO add header attributes that describe this file
		//    ie outputHdr.insert("creator", Imf::StringAttribute("username"));

		exrFile = new Imf::RgbaOutputFile
		    (pImageName->asString().getString(), 
		     outputHdr,
		     channels);
	    }
	    catch (...)
	    {
		pImageName->setError(1);
		NRiSys::error(
		    "%E%s: Could not create EXR file\n",
		    pImageName->asString().getString());
		hasError = 1;
	    }
	}
	headerWritten = 1;
    }
}


