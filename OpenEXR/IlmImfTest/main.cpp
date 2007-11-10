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


#include "OpenEXRConfig.h"
#include <testXdr.h>
#include <testMagic.h>
#include <testHuf.h>
#include <testWav.h>
#include <testChannels.h>
#include <testAttributes.h>
#include <testCustomAttributes.h>
#include <testLineOrder.h>
#include <testCompression.h>
#include <testCopyPixels.h>
#include <testRgba.h>
#include <testRgbaThreading.h>
#include <testLut.h>
#include <testSampleImages.h>
#include <testPreviewImage.h>
#include <testConversion.h>
#include <testStandardAttributes.h>
#include <testNativeFormat.h>
#include <testTiledRgba.h>
#include <testTiledCompression.h>
#include <testTiledCopyPixels.h>
#include <testTiledLineOrder.h>
#include <testScanLineApi.h>
#include <testExistingStreams.h>
#include <testYca.h>
#include <testTiledYa.h>
#include <testIsComplete.h>
#include <testSharedFrameBuffer.h>
#include <testMultiView.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

#ifdef HAVE_LINUX_PROCFS
    #include <unistd.h>
    #include <sstream>
#endif

#define TEST(x) if (argc < 2 || !strcmp (argv[1], #x)) x();

int
main (int argc, char *argv[])
{
    TEST (testMagic);
    TEST (testXdr);
    TEST (testHuf);
    TEST (testWav);
    TEST (testRgba);
    TEST (testSharedFrameBuffer);
    TEST (testRgbaThreading);
    TEST (testChannels);
    TEST (testAttributes);
    TEST (testCustomAttributes);
    TEST (testLineOrder);
    TEST (testCompression);
    TEST (testCopyPixels);
    TEST (testLut);
    TEST (testSampleImages);
    TEST (testPreviewImage);
    TEST (testConversion);
    TEST (testTiledRgba);
    TEST (testTiledCopyPixels);
    TEST (testTiledCompression);
    TEST (testTiledLineOrder);
    TEST (testScanLineApi);
    TEST (testExistingStreams);
    TEST (testStandardAttributes);
    TEST (testYca);
    TEST (testTiledYa);
    TEST (testNativeFormat);
    TEST (testMultiView);
    TEST (testIsComplete);
    
    #ifdef HAVE_LINUX_PROCFS

	//
	// Allow the user to check for file descriptor leaks
	//

	std::cout << "open file descriptors:" << std::endl;

	std::stringstream ss;
	ss << "ls -lG /proc/" << getpid() << "/fd";
	
	system (ss.str().c_str());

	std::cout << std::endl;

    #endif

    return 0;
}
