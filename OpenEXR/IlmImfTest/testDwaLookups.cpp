//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.
//

#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <assert.h>

#include <OpenEXRConfig.h>
#include <OpenEXRConfigInternal.h>

#if __cplusplus >= 201103L
#include <thread>
#endif

#ifdef OPENEXR_IMF_HAVE_SYSCONF_NPROCESSORS_ONLN
#include <unistd.h>
#endif

#include <half.h>
#include <IlmThread.h>
#include <IlmThreadSemaphore.h>
#include <ImfIO.h>
#include <ImfXdr.h>
#include "ImfNamespace.h"

//
// This test uses the code that generates the dwaLookups.h header to
// validate the the values in the tables are correct.
//

using namespace OPENEXR_IMF_NAMESPACE;
using namespace OPENEXR_IMF_NAMESPACE;

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

extern const unsigned short dwaCompressorNoOp[];
extern const unsigned short dwaCompressorToLinear[];
extern const unsigned short dwaCompressorToNonlinear[];
extern const unsigned short closestData[];
extern const unsigned int closestDataOffset[];

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT

namespace {

class LutHeaderWorker
{
public:
    class Runner : public ILMTHREAD_NAMESPACE::Thread
    {
    public:
        Runner(LutHeaderWorker &worker, bool output):
            ILMTHREAD_NAMESPACE::Thread(),
            _worker(worker),
            _output(output)
        {
            start();
        }

        virtual ~Runner()
        {
            _semaphore.wait();
        }

        virtual void run()
        {
            _semaphore.post();
            _worker.run(_output);
        }

    private:
        LutHeaderWorker     &_worker;
        bool                 _output;
        ILMTHREAD_NAMESPACE::Semaphore _semaphore;

    }; // class LutHeaderWorker::Runner


    LutHeaderWorker(size_t startValue,
                    size_t endValue):
        _lastCandidateCount(0),
        _startValue(startValue),
        _endValue(endValue),
        _numElements(0),
        _offset(new size_t[numValues()]),
        _elements(new unsigned short[1024*1024*2])
    {
    }

    ~LutHeaderWorker()
    {
        delete[] _offset;
        delete[] _elements;
    }

    size_t lastCandidateCount() const
    {
        return _lastCandidateCount;
    }

    size_t numValues() const 
    {
        return _endValue - _startValue;
    }

    size_t numElements() const
    {
        return _numElements;
    }

    const size_t* offset() const
    {
        return _offset;
    }

    const unsigned short* elements() const
    {
        return _elements;
    }

    void run(bool outputProgress)
    {
        half candidate[16];
        int  candidateCount = 0;

        for (size_t input=_startValue; input<_endValue; ++input) {

            int  numSetBits = countSetBits(input);
            half inputHalf, closestHalf;

            inputHalf.setBits(input);
            closestHalf.setBits(0);

            _offset[input - _startValue] = _numElements;

            // Gather candidates
            candidateCount = 0;
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

                        if (fabs((float)inputHalf - (float)tmpHalf) < 
                            fabs((float)inputHalf - (float)closestHalf)) {
                            closestHalf = tmpHalf;
                        }
                    }
                }

                candidate[candidateCount] = closestHalf;
                candidateCount++;
            }

            // Sort candidates by increasing number of bits set
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
                _elements[_numElements] = candidate[i].bits();
                _numElements++;
            }

            if (input == _endValue-1) {
                _lastCandidateCount = candidateCount;
            }
        }
    }

private:

    size_t          _lastCandidateCount;
    size_t          _startValue;
    size_t          _endValue;
    size_t          _numElements;
    size_t         *_offset;
    unsigned short *_elements;

    //
    // Precomputing the bit count runs faster than using
    // the builtin instruction, at least in one case..
    //
    // Precomputing 8-bits is no slower than 16-bits,
    // and saves a fair bit of overhead..
    //
    int countSetBits(unsigned short src)
    {
        static const unsigned short numBitsSet[256] =
            {
                0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
                1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
            };

        return numBitsSet[src & 0xff] + numBitsSet[src >> 8];
    }

}; // class LutHeaderWorker

} // namespace


//
// Generate a no-op LUT, to cut down in conditional branches
//
void
testNoop()
{
    printf("test dwaCompressorNoOp[] \n");

    for (int i=0; i<65536; ++i)
    {
        unsigned short src = (unsigned short)i;
        assert (src == OPENEXR_IMF_INTERNAL_NAMESPACE::dwaCompressorNoOp[i]);
    }
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
testToLinear()
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
    
    printf("test dwaCompressorToLinear[]\n");
    for (int i=0; i<65536; ++i)
        assert (toLinear[i] == OPENEXR_IMF_INTERNAL_NAMESPACE::dwaCompressorToLinear[i]);
}


void
testToNonlinear()
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

    printf("test dwaCompressorToNonlinear[]\n");
    for (int i=0; i<65536; ++i)
        assert (toNonLinear[i] == OPENEXR_IMF_INTERNAL_NAMESPACE::dwaCompressorToNonLinear[i]);
}

//
// Attempt to get available CPUs in a somewhat portable way. 
//

int
cpuCount()
{
    if (!ILMTHREAD_NAMESPACE::supportsThreads()) return 1;

    int cpuCount = 1;

#if __cplusplus >= 201103L
    cpuCount = std::thread::hardware_concurrency();
#else

#if defined (OPENEXR_IMF_HAVE_SYSCONF_NPROCESSORS_ONLN)

    cpuCount = sysconf(_SC_NPROCESSORS_ONLN);

#elif defined (_WIN32)

    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    cpuCount = sysinfo.dwNumberOfProcessors;

#endif

#endif
    if (cpuCount < 1) cpuCount = 1;
    return cpuCount;
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
testLutHeader()
{
    std::vector<LutHeaderWorker*> workers;

    size_t numWorkers     = cpuCount();
    size_t workerInterval = 65536 / numWorkers;

    for (size_t i=0; i<numWorkers; ++i) {
        if (i != numWorkers-1) {
            workers.push_back( new LutHeaderWorker( i   *workerInterval, 
                                                   (i+1)*workerInterval) );
        } else {
            workers.push_back( new LutHeaderWorker(i*workerInterval, 65536) );
        }
    }

    if (ILMTHREAD_NAMESPACE::supportsThreads()) {
        std::vector<LutHeaderWorker::Runner*> runners;
        for (size_t i=0; i<workers.size(); ++i) {
            runners.push_back( new LutHeaderWorker::Runner(*workers[i], (i==0)) );
        }

        for (size_t i=0; i<workers.size(); ++i) {
            delete runners[i];
        }
    } else {
        for (size_t i=0; i<workers.size(); ++i) {
            workers[i]->run(i == 0);
        }
    }

    printf("test closestDataOffset[]\n");
    int offsetIdx  = 0;
    int offsetPrev = 0;
    for (size_t i=0; i<workers.size(); ++i) {
        for (size_t value=0; value<workers[i]->numValues(); ++value)
        {
            assert (OPENEXR_IMF_INTERNAL_NAMESPACE::closestDataOffset[offsetIdx] == workers[i]->offset()[value] + offsetPrev);
            offsetIdx++;
        }
        offsetPrev += workers[i]->offset()[workers[i]->numValues()-1] + 
                      workers[i]->lastCandidateCount();
    }

    printf("test closestData[]\n");
    int elementIdx = 0;
    for (size_t i=0; i<workers.size(); ++i) {
        for (size_t element=0; element<workers[i]->numElements(); ++element)
        {
            assert (OPENEXR_IMF_INTERNAL_NAMESPACE::closestData[elementIdx] == workers[i]->elements()[element]);
            elementIdx++;
        }    
    }

    for (size_t i=0; i<workers.size(); ++i) {
        delete workers[i];
    }
}

void
testDwaLookups(const std::string&)
{
    testNoop();
    testToLinear();
    testToNonlinear();
    testLutHeader();
}
