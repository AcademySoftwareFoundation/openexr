///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2009-2014 DreamWorks Animation LLC. 
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
// *       Neither the name of DreamWorks Animation nor the names of
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

//
// A program to generate various acceleration lookup tables 
// for Imf::DwaCompressor
//

#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <half.h>
#include <ImfIO.h>
#include <ImfXdr.h>
#include "ImfNamespace.h"

using namespace OPENEXR_IMF_NAMESPACE;

//
// Generate a no-op LUT, to cut down in conditional branches
//
void
generateNoop()
{
    printf("const unsigned short dwaCompressorNoOp[] = \n");
    printf("{");
    for (int i=0; i<65536; ++i) {

        if (i % 8 == 0) {
            printf("\n    ");
        }

        unsigned short dst;
        char *tmp = (char *)(&dst);

        unsigned short src = (unsigned short)i;
        Xdr::write <CharPtrIO> (tmp,  src);

        printf("0x%04x, ", dst);
    }
    printf("\n};\n");
}

//
// Nonlinearly encode luminance. For values below 1.0, we want
// to use a gamma 2.2 function to match what is fairly common
// for storing output referred. However, > 1, gamma functions blow up,
// and log functions are much better behaved. We could use a log 
// function everywhere, but it tends to over-sample dark 
// regions and undersample the brighter regions, when 
// compared to the way real devices reproduce values.
//
// So, above 1, use a log function which is a smooth blend
// into the gamma function. 
//
//  Nonlinear(linear) = 
//
//    linear^(1./2.2)             / linear <= 1.0
//                               |
//    ln(linear)/ln(e^2.2) + 1    \ otherwise
//
//
// toNonlinear[] needs to take in XDR format half float values,
// and output NATIVE format float. 
//
// toLinear[] does the opposite - takes in NATIVE half and 
// outputs XDR half values.
//

void
generateToLinear()
{
    unsigned short toLinear[65536];

    toLinear[0] = 0;

    for (int i=1; i<65536; ++i) {
        half  h;
        float sign    = 1;
        float logBase = pow(2.7182818, 2.2);

        // map  NaN and inf to 0
        if ((i & 0x7c00) == 0x7c00) {
            toLinear[i]    = 0;
            continue;
        }

        //
        // _toLinear - assume i is NATIVE, but our output needs
        //             to get flipped to XDR
        //
        h.setBits(i);
        sign = 1;
        if ((float)h < 0) {
            sign = -1;
        } 

        if ( fabs( (float)h) <= 1.0 ) {
            h  = (half)(sign * pow((float)fabs((float)h), 2.2f));
        } else {
            h  = (half)(sign * pow(logBase, (float)(fabs((float)h) - 1.0)));
        }

        {
            char *tmp = (char *)(&toLinear[i]);

            Xdr::write <CharPtrIO> ( tmp,  h.bits());
        }
    }
    
    printf("const unsigned short dwaCompressorToLinear[] = \n");
    printf("{");
    for (int i=0; i<65536; ++i) {
        if (i % 8 == 0) {
            printf("\n    ");
        }
        printf("0x%04x, ", toLinear[i]);
    }
    printf("\n};\n");
}


void
generateToNonlinear()
{
    unsigned short toNonlinear[65536];

    toNonlinear[0] = 0;

    for (int i=1; i<65536; ++i) {
        unsigned short usNative, usXdr;
        half  h;
        float sign    = 1;
        float logBase = pow(2.7182818, 2.2);

        usXdr           = i;

        {
            const char *tmp = (char *)(&usXdr);

            Xdr::read<CharPtrIO>(tmp, usNative);
        }

        // map  NaN and inf to 0
        if ((usNative & 0x7c00) == 0x7c00) {
            toNonlinear[i] = 0;
            continue;
        }

        //
        // toNonlinear - assume i is XDR
        //
        h.setBits(usNative);
        sign = 1;
        if ((float)h < 0) {
            sign = -1;
        } 

        if ( fabs( (float)h ) <= 1.0) {
            h = (half)(sign * pow(fabs((float)h), 1.f/2.2f));
        } else {
            h = (half)(sign * ( log(fabs((float)h)) / log(logBase) + 1.0) );
        }
        toNonlinear[i] = h.bits();
    }

    printf("const unsigned short dwaCompressorToNonlinear[] = \n");
    printf("{");
    for (int i=0; i<65536; ++i) {
        if (i % 8 == 0) {
            printf("\n    ");
        }
        printf("0x%04x, ", toNonlinear[i]);
    }
    printf("\n};\n");
}

//
// Precomputing the bit count runs faster than using
// the builtin instruction, at least in one case..
//
// Precomputing 8-bits is no slower than 16-bits,
// and saves a fair bit of overhead..
//

int
countSetBits(unsigned short src)
{
    static int            first = 1;
    static unsigned short numBitsSet[256];

    if (first) {
        first = 0;

       for (int idx=0; idx<256; ++idx) {
            int numSet = 0;
 
            for (int i=0; i<8; ++i) {
                if (idx & (1<<i)) numSet++;
            }

            numBitsSet[idx] = numSet;
        }
    }

    return numBitsSet[src & 0xff] +
           numBitsSet[src >> 8];
}

//
// Generate acceleration luts for the quantization.
//
// For each possible input value, we want to find the closest numbers
// which have one fewer bits set than before. 
//
// This gives us num_bits(input)-1 values per input. If we alloc
// space for everything, that's like a 2MB table. We can do better
// by compressing all the values to be contigious and using offset
// pointers.
//
// After we've found the candidates with fewer bits set, sort them
// based on increasing numbers of bits set. This way, on quantize(),
// we can scan through the list and halt once we find the first
// candidate within the error range. For small values that can 
// be quantized to 0, 0 is the first value tested and the search
// can exit fairly quickly.
//

void
generateLutHeader()
{
    int             numElements = 0;
    unsigned int    offset[65536];
    unsigned short *closestData = new unsigned short[1024*1024*2];

    half      candidate[16];
    int       candidateCount = 0;
    
    for (int input=0; input<65536; ++input) {

#ifdef __GNUC__
        if (input % 100 == 0) {
            fprintf(stderr, 
                " Building acceleration for DwaCompressor, %.2f %%      %c",
                              100.*(float)input/65535., 13);
        }
#else
        if (input % 1000 == 0) {
            fprintf(stderr, 
                " Building acceleration for DwaCompressor, %.2f %%\n",
                              100.*(float)input/65535.);            
        }
#endif

        int  numSetBits = countSetBits(input);
        half inputHalf, closestHalf;

        inputHalf.setBits(input);

        offset[input] = numElements;

        candidateCount = 0;

        // Gather candidates
        for (int targetNumSetBits=numSetBits-1; targetNumSetBits>=0;
                                                     --targetNumSetBits) {

            bool valueFound = false;

            for (int i=0; i<65536; ++i) {
                if (countSetBits(i) != targetNumSetBits) continue;

                if (!valueFound) {
                    closestHalf.setBits(i);
                    valueFound = true;
                } else {
                    half tmpHalf;

                    tmpHalf.setBits(i);

                    if ( fabs((float)inputHalf - (float)tmpHalf) < 
                            fabs((float)inputHalf - (float)closestHalf)) {
                        closestHalf = tmpHalf;
                    }
                }
            }


            if (valueFound == false) {
                fprintf(stderr, "bork bork bork!\n");
            }       

            candidate[candidateCount] = closestHalf;
            candidateCount++;
        }

        // Sort candidates by inceasing number of bits set
        for (int i=0; i<candidateCount; ++i) {
            for (int j=i+1; j<candidateCount; ++j) {

                int   iCnt = countSetBits(candidate[i].bits());
                int   jCnt = countSetBits(candidate[j].bits());

                if (jCnt < iCnt) {
                    half tmp     = candidate[i];
                    candidate[i] = candidate[j];
                    candidate[j] = tmp;
                }
            }
        }

        // Copy candidates to the data buffer;
        for (int i=0; i<candidateCount; ++i) {
            closestData[numElements] = candidate[i].bits();
            numElements++;
        }
    }


    
    printf("static unsigned int closestDataOffset[] = {\n");
    for (int i=0; i<65536; ++i) {
        if (i % 8 == 0) {
            printf("    ");
        }
        printf("%6d, ", offset[i]);
        if (i % 8 == 7) {
            printf("\n");
        }
    }
    printf("};\n\n\n");


    printf("static unsigned short closestData[] = {\n");
    for (int i=0; i<numElements; ++i) {
        if (i % 8 == 0) {
            printf("    ");
        }
        printf("%5d, ", closestData[i]);
        if (i % 8 == 7) {
            printf("\n");
        }
    }
    printf("};\n\n\n");
}


int
main(int argc, char **argv)
{
    printf("#include <cstddef>\n");
    printf("\n\n\n");

    generateNoop();

    printf("\n\n\n");

    generateToLinear();

    printf("\n\n\n");

    generateToNonlinear();

    printf("\n\n\n");

    generateLutHeader();

    return 0;
}
