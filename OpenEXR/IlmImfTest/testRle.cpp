// DreamWorks Animation LLC Confidential Information.
// TM and (c) 2009-2014 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.

#include <string>
#include <iostream>
#include <assert.h>
#include <ImfRle.h>
#include <ImathRandom.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;
using namespace std;

namespace {

// Generate a random sequence of runs and single values
void
generateData(char *buffer, int bufferLen)
{
    char   value = 0;
    int    i     = 0;
    Rand48 rand48(0);

    while (i < bufferLen) {
        
        if (rand48.nextf() < .5) {

            // Insert a single value 
            buffer[i++] = value;

        } else {

            // Insert a run
            int runLen = (int)rand48.nextf(2.0, 1024.0);

            for (int j=0; j<runLen; ++j) {
                buffer[i++] = value;
                if (i >= bufferLen) break;
            }
            
        }

        value = (int)(value+1) & 0xff;
    }
}

// Compress, decompress, and compare with the original 
void
testRoundTrip(int bufferLen)
{
    char        *src        = new char[bufferLen];
    signed char *compressed = new signed char[2*bufferLen];
    char        *test       = new char[bufferLen];

    generateData(src, bufferLen);

    int compressedLen = rleCompress(bufferLen, src, compressed);
    
    assert(rleUncompress(compressedLen, bufferLen, compressed, test) > 0);

    for (int i=0; i<bufferLen; ++i) {
        assert( src[i] == test[i] );
    }

    delete[] src;
    delete[] compressed;
    delete[] test;
}


} // namespace

void 
testRle(const string&)   
{
    cout << "RLE compression:" << endl;

    try {

        cout << "   Round tripping buffers " << endl; 

        const int numIter = 1000;
        Rand48    rand48(0);

        for (int iter=0; iter<numIter; ++iter) {
            testRoundTrip( (int)rand48.nextf(100.0, 1000000.0));
        }
   
    } catch (const exception &e) {
        cout << "unexpected exception: " << e.what() << endl;
        assert (false);
    } catch (...) {
        cout << "unexpected exception" << endl;
        assert (false);
    }

    cout << "ok\n" << endl;
}


// TM and (c) 2009-2014 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.
