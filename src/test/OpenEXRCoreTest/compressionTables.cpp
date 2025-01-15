// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

// Windows specific addition to prevent the indirect import of the redefined min/max macros
#if defined _WIN32 || defined _WIN64
#    ifdef NOMINMAX
#        undef NOMINMAX
#    endif
#    define NOMINMAX
#endif

#include "compression.h"

#include "test_value.h"

#include <openexr.h>
#include <half.h>

#include <OpenEXRConfig.h>
#include <OpenEXRConfigInternal.h>

#if __cplusplus >= 201103L
#    include <thread>
#endif

#ifdef OPENEXR_IMF_HAVE_SYSCONF_NPROCESSORS_ONLN
#    include <unistd.h>
#endif

#include "ImfNamespace.h"
#include "IlmThread.h"
#include "IlmThreadSemaphore.h"
#include "ImfIO.h"
#include "ImfXdr.h"

#include <cstddef>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <intrin.h>
#endif

#if defined(OPENEXR_ENABLE_API_VISIBILITY)
#include "../../lib/OpenEXRCore/internal_b44_table.c"
#else
extern const uint16_t* exrcore_expTable;
extern const uint16_t* exrcore_logTable;
#endif

namespace internal_test_ns {

#include "../../lib/OpenEXRCore/dwaLookups.h"
#include "dwaQuantTables.h"

} // namespace internal_test_ns

using namespace OPENEXR_IMF_NAMESPACE;

namespace
{

class LutHeaderWorker
{
public:
    class Runner : public ILMTHREAD_NAMESPACE::Thread
    {
    public:
        Runner (LutHeaderWorker& worker, bool output)
            : ILMTHREAD_NAMESPACE::Thread (), _worker (worker), _output (output)
        {
            start ();
        }

        virtual ~Runner () { _semaphore.wait (); }

        virtual void run ()
        {
            _semaphore.post ();
            _worker.run (_output);
        }

    private:
        LutHeaderWorker&               _worker;
        bool                           _output;
        ILMTHREAD_NAMESPACE::Semaphore _semaphore;

    }; // class LutHeaderWorker::Runner

    LutHeaderWorker (size_t startValue, size_t endValue)
        : _lastCandidateCount (0)
        , _startValue (startValue)
        , _endValue (endValue)
        , _numElements (0)
        , _offset (new size_t[numValues ()])
        , _elements (new unsigned short[1024 * 1024 * 2])
    {}

    ~LutHeaderWorker ()
    {
        delete[] _offset;
        delete[] _elements;
    }

    size_t lastCandidateCount () const { return _lastCandidateCount; }

    size_t numValues () const { return _endValue - _startValue; }

    size_t numElements () const { return _numElements; }

    const size_t* offset () const { return _offset; }

    const unsigned short* elements () const { return _elements; }

    void run (bool outputProgress)
    {
        half candidate[16];
        int  candidateCount = 0;

        for (size_t input = _startValue; input < _endValue; ++input)
        {

            int  numSetBits = countSetBits (input);
            half inputHalf, closestHalf;

            inputHalf.setBits (input);
            closestHalf.setBits (0);

            _offset[input - _startValue] = _numElements;

            // Gather candidates
            candidateCount = 0;
            for (int targetNumSetBits = numSetBits - 1; targetNumSetBits >= 0;
                 --targetNumSetBits)
            {
                bool valueFound = false;

                for (int i = 0; i < 65536; ++i)
                {
                    if (countSetBits (i) != targetNumSetBits) continue;

                    if (!valueFound)
                    {
                        closestHalf.setBits (i);
                        valueFound = true;
                    }
                    else
                    {
                        half tmpHalf;

                        tmpHalf.setBits (i);

                        if (fabs ((float) inputHalf - (float) tmpHalf) <
                            fabs ((float) inputHalf - (float) closestHalf))
                        {
                            closestHalf = tmpHalf;
                        }
                    }
                }

                candidate[candidateCount] = closestHalf;
                candidateCount++;
            }

            // Sort candidates by increasing number of bits set
            for (int i = 0; i < candidateCount; ++i)
            {
                for (int j = i + 1; j < candidateCount; ++j)
                {

                    int iCnt = countSetBits (candidate[i].bits ());
                    int jCnt = countSetBits (candidate[j].bits ());

                    if (jCnt < iCnt)
                    {
                        half tmp     = candidate[i];
                        candidate[i] = candidate[j];
                        candidate[j] = tmp;
                    }
                }
            }

            // Copy candidates to the data buffer;
            for (int i = 0; i < candidateCount; ++i)
            {
                _elements[_numElements] = candidate[i].bits ();
                _numElements++;
            }

            if (input == _endValue - 1)
            {
                _lastCandidateCount = candidateCount;
            }
        }
    }

private:
    size_t          _lastCandidateCount;
    size_t          _startValue;
    size_t          _endValue;
    size_t          _numElements;
    size_t*         _offset;
    unsigned short* _elements;

    //
    // Precomputing the bit count runs faster than using
    // the builtin instruction, at least in one case..
    //
    // Precomputing 8-bits is no slower than 16-bits,
    // and saves a fair bit of overhead..
    //
    int countSetBits (unsigned short src)
    {
        static const unsigned short numBitsSet[256] = {
            0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3,
            3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4,
            3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2,
            2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5,
            3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5,
            5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3,
            2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4,
            4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
            3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4,
            4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6,
            5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5,
            5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

        return numBitsSet[src & 0xff] + numBitsSet[src >> 8];
    }

}; // class LutHeaderWorker

} // namespace

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

static void
testToLinear ()
{
    unsigned short toLinear[65536];

    toLinear[0] = 0;

    for (int i = 1; i < 65536; ++i)
    {
        half  h;
        float sign    = 1;
        float logBase = pow (2.7182818, 2.2);

        // map  NaN and inf to 0
        if ((i & 0x7c00) == 0x7c00)
        {
            toLinear[i] = 0;
            continue;
        }

        //
        // _toLinear - assume i is NATIVE, but our output needs
        //             to get flipped to XDR
        //
        h.setBits (i);
        sign = 1;
        if ((float) h < 0) { sign = -1; }

        if (fabs ((float) h) <= 1.0)
        {
            h = (half) (sign * pow ((float) fabs ((float) h), 2.2f));
        }
        else
        {
            h = (half) (sign * pow (logBase, (float) (fabs ((float) h) - 1.0)));
        }

        {
            char* tmp = (char*) (&toLinear[i]);

            Xdr::write<CharPtrIO> (tmp, h.bits ());
        }
    }

    printf ("test dwaCompressorToLinear[]\n");
    for (int i = 0; i < 65536; ++i)
        EXRCORE_TEST (
            toLinear[i] ==
            internal_test_ns::dwaCompressorToLinear[i]);
}

static void
testToNonlinear ()
{
    unsigned short toNonlinear[65536];

    toNonlinear[0] = 0;

    for (int i = 1; i < 65536; ++i)
    {
        unsigned short usNative, usXdr;
        half           h;
        float          sign    = 1;
        float          logBase = pow (2.7182818, 2.2);

        usXdr = i;

        {
            const char* tmp = (char*) (&usXdr);

            Xdr::read<CharPtrIO> (tmp, usNative);
        }

        // map  NaN and inf to 0
        if ((usNative & 0x7c00) == 0x7c00)
        {
            toNonlinear[i] = 0;
            continue;
        }

        //
        // toNonlinear - assume i is XDR
        //
        h.setBits (usNative);
        sign = 1;
        if ((float) h < 0) { sign = -1; }

        if (fabs ((float) h) <= 1.0)
        {
            h = (half) (sign * pow (fabs ((float) h), 1.f / 2.2f));
        }
        else
        {
            h = (half) (sign * (log (fabs ((float) h)) / log (logBase) + 1.0));
        }
        toNonlinear[i] = h.bits ();
    }

    printf ("test dwaCompressorToNonlinear[]\n");
    for (int i = 0; i < 65536; ++i)
        EXRCORE_TEST (
            toNonlinear[i] ==
            internal_test_ns::dwaCompressorToNonlinear[i]);
}

//
// Attempt to get available CPUs in a somewhat portable way.
//

static int
cpuCount ()
{
    if (!ILMTHREAD_NAMESPACE::supportsThreads ()) return 1;

    int cpuCount = 1;

#if __cplusplus >= 201103L
    cpuCount = std::thread::hardware_concurrency ();
#else

#    if defined(OPENEXR_IMF_HAVE_SYSCONF_NPROCESSORS_ONLN)

    cpuCount = sysconf (_SC_NPROCESSORS_ONLN);

#    elif defined(_WIN32)

    SYSTEM_INFO sysinfo;
    GetSystemInfo (&sysinfo);
    cpuCount = sysinfo.dwNumberOfProcessors;

#    endif

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
// by compressing all the values to be contiguous and using offset
// pointers.
//
// After we've found the candidates with fewer bits set, sort them
// based on increasing numbers of bits set. This way, on quantize(),
// we can scan through the list and halt once we find the first
// candidate within the error range. For small values that can
// be quantized to 0, 0 is the first value tested and the search
// can exit fairly quickly.
//

static void
testLutHeader ()
{
    std::vector<LutHeaderWorker*> workers;

    size_t numWorkers     = cpuCount ();
    size_t workerInterval = 65536 / numWorkers;

    for (size_t i = 0; i < numWorkers; ++i)
    {
        if (i != numWorkers - 1)
        {
            workers.push_back (new LutHeaderWorker (
                i * workerInterval, (i + 1) * workerInterval));
        }
        else
        {
            workers.push_back (new LutHeaderWorker (i * workerInterval, 65536));
        }
    }

    if (ILMTHREAD_NAMESPACE::supportsThreads ())
    {
        std::vector<LutHeaderWorker::Runner*> runners;
        for (size_t i = 0; i < workers.size (); ++i)
        {
            runners.push_back (
                new LutHeaderWorker::Runner (*workers[i], (i == 0)));
        }

        for (size_t i = 0; i < workers.size (); ++i)
        {
            delete runners[i];
        }
    }
    else
    {
        for (size_t i = 0; i < workers.size (); ++i)
        {
            workers[i]->run (i == 0);
        }
    }

    printf ("test closestDataOffset[]\n");
    int offsetIdx  = 0;
    int offsetPrev = 0;
    for (size_t i = 0; i < workers.size (); ++i)
    {
        for (size_t value = 0; value < workers[i]->numValues (); ++value)
        {
            EXRCORE_TEST (
                internal_test_ns::closestDataOffset[offsetIdx] ==
                workers[i]->offset ()[value] + offsetPrev);
            offsetIdx++;
        }
        offsetPrev += workers[i]->offset ()[workers[i]->numValues () - 1] +
                      workers[i]->lastCandidateCount ();
    }

    printf ("test closestData[]\n");
    int elementIdx = 0;
    for (size_t i = 0; i < workers.size (); ++i)
    {
        for (size_t element = 0; element < workers[i]->numElements ();
             ++element)
        {
            EXRCORE_TEST (
                internal_test_ns::closestData[elementIdx] ==
                workers[i]->elements ()[element]);
            elementIdx++;
        }
    }

    for (size_t i = 0; i < workers.size (); ++i)
    {
        delete workers[i];
    }
}

////////////////////////////////////////

void
testDWATable (const std::string&)
{
    testToLinear ();
    testToNonlinear ();
    testLutHeader ();
}

#if defined(__has_builtin)
#    if __has_builtin(__builtin_popcount)
#        define USE_POPCOUNT 1
#    endif
#    if __has_builtin(__builtin_clz)
#        define USE_CLZ 1
#    endif
#endif
#ifndef USE_POPCOUNT
#    define USE_POPCOUNT 0
#endif

#ifndef USE_CLZ
#    ifdef _MSC_VER
static int __inline __builtin_clz(uint32_t v)
{
#ifdef __BMI1__
    return __lzcnt(v);
#else
    unsigned long r;
    _BitScanReverse(&r, v);
    return 31 - r;
#endif
}
#        define USE_CLZ 1
#    else
#        define USE_CLZ 0
#    endif
#endif

#if USE_POPCOUNT
static inline int
countSetBits (uint16_t src)
{
    return __builtin_popcount (src);
}
#else
// courtesy hacker's delight
static inline int countSetBits(uint32_t x)
{
    uint64_t y;
    y = x * 0x0002000400080010ULL;
    y = y & 0x1111111111111111ULL;
    y = y * 0x1111111111111111ULL;
    y = y >> 60;
    return y;
}
#endif

#if USE_CLZ
static inline int
countLeadingZeros(uint16_t src)
{
    return __builtin_clz (src);
}
#else
// courtesy hacker's delight
static int inline countLeadingZeros( uint32_t x )
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return 32 - countSetBits(x);
}
#endif

#define TEST_QUANT_ALTERNATE_LARGE(x)                                   \
    alt = (x);                                                          \
    bits = countSetBits (alt);                                          \
    if (bits < smallbits)                                               \
    {                                                                   \
        delta = imath_half_to_float(alt) - srcFloat;                    \
        if (delta < errTol)                                             \
        {                                                               \
            if (doprint)                                                \
                printf("  lrgtst 0x%04X delta %g\n", alt, delta);       \
            smallbits = bits; smalldelta = delta; smallest = alt;       \
        }                                                               \
    }                                                                   \
    else if (bits == smallbits)                                         \
    {                                                                   \
        delta = imath_half_to_float(alt) - srcFloat;                    \
        if (delta < smalldelta)                                         \
        {                                                               \
            smallest = alt;                                             \
            smalldelta = delta;                                         \
            smallbits = bits;                                           \
        }                                                               \
    }

#define TEST_QUANT_ALTERNATE_SMALL(x)                                   \
    alt = (x);                                                          \
    bits = countSetBits (alt);                                          \
    if (bits < smallbits)                                               \
    {                                                                   \
        delta = srcFloat - imath_half_to_float(alt);                    \
        if (delta < errTol)                                             \
        {                                                               \
            if (doprint)                                                \
                printf("  smltst 0x%04X delta %g\n", alt, delta);       \
            smallbits = bits; smalldelta = delta; smallest = alt;       \
        }                                                               \
    }                                                                   \
    else if (bits == smallbits)                                         \
    {                                                                   \
        delta = srcFloat - imath_half_to_float(alt);                    \
        if (delta < smalldelta)                                         \
        {                                                               \
            smallest = alt;                                             \
            smalldelta = delta;                                         \
            smallbits = bits;                                           \
        }                                                               \
    }

static uint16_t handleQuantizeDenormTol (
    uint16_t abssrc, uint16_t tolSig, float errTol, float srcFloat, int doprint)
{
    const uint16_t tsigshift = (32 - countLeadingZeros (tolSig));
    const uint16_t npow2 = (1 << tsigshift);
    const uint16_t lowermask = npow2 - 1;
    const uint16_t mask = ~lowermask;
    const uint16_t mask2 = mask ^ npow2;

    uint16_t alt, smallest = abssrc;
    int bits, smallbits = countSetBits(abssrc);
    float delta, smalldelta = errTol;

    if (doprint)
    {
        printf ("  npow2  0x%04X %016b\n", npow2, npow2);
        printf ("  mask   0x%04X %016b\n", mask, mask);
        printf ("  mask2  0x%04X %016b\n", mask2, mask2);
        printf ("  den_0  0x%04X %016b %g: %g\n", (abssrc & mask2), (abssrc & mask2),
                imath_half_to_float((abssrc & mask2)),
                srcFloat - imath_half_to_float((abssrc & mask2)));
        printf ("  den_1  0x%04X %016b %g: %g\n", (abssrc & mask), (abssrc & mask),
                imath_half_to_float((abssrc & mask)),
                srcFloat - imath_half_to_float((abssrc & mask)));
        printf ("  den_2  0x%04X %016b %g: %g\n", ((abssrc + npow2) & mask),
                ((abssrc + npow2) & mask),
                imath_half_to_float(((abssrc + npow2) & mask)),
                imath_half_to_float(((abssrc + npow2) & mask)) - srcFloat);
        printf ("  den_3  0x%04X %016b %g: %g make norm\n", ((abssrc + (npow2 << 1)) & mask),
                ((abssrc + (npow2 << 1)) & mask),
                imath_half_to_float(((abssrc + (npow2 << 1)) & mask)),
                imath_half_to_float(((abssrc + (npow2 << 1)) & mask)) - srcFloat);
    }

    TEST_QUANT_ALTERNATE_SMALL(abssrc & mask2);
    TEST_QUANT_ALTERNATE_SMALL(abssrc & mask);
    TEST_QUANT_ALTERNATE_LARGE((abssrc + npow2) & mask);
    TEST_QUANT_ALTERNATE_LARGE((abssrc + (npow2 << 1)) & mask);

    return smallest;
}

static uint16_t handleQuantizeGeneric (
    uint16_t abssrc, uint16_t tolSig, float errTol, float srcFloat, int doprint)
{
    // classic would do clz(significand - 1) but here we are trying to
    // construct a mask, so want to ensure for an power of 2, we
    // actually get the next (i.e. 2 returns 4)
    const uint16_t tsigshift = (32 - countLeadingZeros (tolSig));
    const uint16_t npow2 = (1 << tsigshift);
    const uint16_t lowermask = npow2 - 1;
    const uint16_t mask = ~lowermask;
    const uint16_t mask2 = mask ^ npow2;
    const uint16_t srcMaskedVal = abssrc & lowermask;
    const uint16_t extrabit = (tolSig > srcMaskedVal);

    const uint16_t mask3 = mask2 ^ (((npow2 << 1) * (extrabit)) |
                                    ((npow2 >> 1) * (!extrabit)));

    if (doprint)
    {
        printf ("  npow2  0x%04X %016b %d\n", npow2, npow2, tsigshift);
        printf ("  srcMV  0x%04X %016b %d\n", srcMaskedVal, srcMaskedVal, srcMaskedVal);
        printf ("  mask   0x%04X %016b\n", mask, mask);
        printf ("  mask2  0x%04X %016b\n", mask2, mask2);
        printf ("  mask3  0x%04X %016b\n", mask3, mask3);
    }

    uint16_t alt, smallest = abssrc;
    int bits, smallbits = countSetBits(abssrc);
    float delta, smalldelta = errTol;

    if (extrabit)
    {
        if (doprint)
        {
            printf ("  t_eb0  0x%04X %016b\n", (abssrc & mask3), (abssrc & mask3));
            printf ("  t_eb1  0x%04X %016b\n", (abssrc & mask2), (abssrc & mask2));
            printf ("  t_eb2  0x%04X %016b\n", (abssrc & mask), (abssrc & mask));
        }
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask3);
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask2);

        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask);
    }
    else if ((abssrc & npow2) != 0)
    {
        if (doprint)
        {
            printf ("  t_np0  0x%04X %016b\n", (abssrc & mask2), (abssrc & mask2));
            printf ("  t_np1  0x%04X %016b\n", (abssrc & mask3), (abssrc & mask3));
            printf ("  t_np2  0x%04X %016b\n", (abssrc & mask), (abssrc & mask));
        }
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask2);
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask3);

        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask);
    }
    else
    {
        if (doprint)
        {
            printf ("  test0  0x%04X %016b\n", (abssrc & mask2), (abssrc & mask2));
            printf ("  test1  0x%04X %016b\n", (abssrc & mask), (abssrc & mask));
            printf ("  test2  0x%04X %016b\n", (abssrc & mask3), (abssrc & mask3));
        }
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask2);

        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask);
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask3);
    }
    if (doprint)
    {
        printf ("  test4  0x%04X %016b\n", ((abssrc + npow2) & mask), ((abssrc + npow2) & mask));
    }
    TEST_QUANT_ALTERNATE_LARGE((abssrc + npow2) & mask);

    return smallest;
}

// use same signature so we can get tail / sibling call optimisation
// (can force with clang?), but notice we are sending in absolute src
// value and the shifted tolerance significand instead
static uint16_t handleQuantizeEqualExp (
    uint16_t abssrc, uint16_t tolSig, float errTol, float srcFloat, int doprint)
{
    const uint16_t npow2 = 0x0800;
    const uint16_t lowermask = npow2 - 1;
    const uint16_t mask = ~lowermask;
    const uint16_t mask2 = mask ^ npow2;

    const uint16_t srcMaskedVal = abssrc & lowermask;
    const uint16_t extrabit = (tolSig > srcMaskedVal);

    const uint16_t mask3 = mask2 ^ (((npow2 << 1) * (extrabit)) |
                                    ((npow2 >> 1) * (!extrabit)));

    if (doprint)
    {
        printf ("  === npow2 == 0x0800\n");
        printf ("  srcMV  0x%04X %016b\n", srcMaskedVal, srcMaskedVal);
        printf ("  mask2  0x%04X %016b\n", mask2, mask2);
        printf ("  mask3  0x%04X %016b\n", mask3, mask3);
    }

    // not yet clear how to narrow down below 3 values...
    uint16_t alt, smallest = abssrc;
    int bits, smallbits = countSetBits(abssrc);
    float delta, smalldelta = errTol;

    // doing in this order mask2, mask, +npow guarantees sorting of values
    // so can avoid a couple of conditionals in the macros
    if (srcMaskedVal == abssrc)
    {
        if (doprint)
        {
            printf ("  test0  0x%04X %016b\n", (abssrc & mask3), (abssrc & mask3));
            printf ("  test1  0x%04X %016b\n", ((abssrc + npow2) & mask), ((abssrc + npow2) & mask));
        }
        TEST_QUANT_ALTERNATE_SMALL(abssrc & mask3);
    }
    else
    {
        if (doprint)
        {
            printf ("  test0  0x%04X %016b\n", (abssrc & mask2), (abssrc & mask2));
            printf ("  test1  0x%04X %016b\n", (abssrc & mask), (abssrc & mask));
            printf ("  xxxxx  0x%04X %016b\n", (abssrc & mask3), (abssrc & mask3));
            printf ("  test2  0x%04X %016b\n", ((abssrc + npow2) & mask), ((abssrc + npow2) & mask));
        }
        uint16_t alt0 = (abssrc & mask2);
        uint16_t alt1 = (abssrc & mask);
        if (alt0 == alt1) alt0 = (abssrc & mask3);

        TEST_QUANT_ALTERNATE_SMALL(alt0);
        TEST_QUANT_ALTERNATE_SMALL(alt1);
    }
    TEST_QUANT_ALTERNATE_LARGE((abssrc + npow2) & mask);

    return smallest;
}

static uint16_t handleQuantizeCloseExp (
    uint16_t abssrc, uint16_t tolSig, float errTol, float srcFloat, int doprint)
{
    const uint16_t npow2 = 0x0400;
    const uint16_t lowermask = npow2 - 1;
    const uint16_t mask = ~lowermask;
    const uint16_t mask2 = mask ^ npow2;

    const uint16_t srcMaskedVal = abssrc & lowermask;
    const uint16_t extrabit = (tolSig > srcMaskedVal);

    const uint16_t mask3 = mask2 ^ (((npow2 << 1) * (extrabit)) |
                                    ((npow2 >> 1) * (!extrabit)));

    if (doprint)
    {
        printf ("  === npow == 0x0400\n");
        printf ("  mask2  0x%04X %016b\n", mask2, mask2);
        printf ("  mask3  0x%04X %016b\n", mask3, mask3);
        printf ("  extra  %d\n", extrabit);
    }

    uint16_t alternates[3];

    if ((abssrc & npow2) == 0) // by definition, src&mask2 == src&mask
    {
        if (doprint)
        {
            printf ("   -- absrc & npow2 == 0\n");
        }
        if (extrabit)
        {
            alternates[0] = (abssrc & mask3);
            alternates[1] = (abssrc & mask);
        }
        else
        {
            alternates[0] = (abssrc & mask);
            alternates[1] = (abssrc & mask3);
        }
    }
    else
    {
        if (extrabit)
        {
            alternates[0] = (abssrc & mask3);
            alternates[1] = (abssrc & mask2);
            float alt1delta = srcFloat - imath_half_to_float(alternates[1]);
            if (alt1delta >= errTol)
            {
                if (doprint)
                {
                    printf ("  swap1  0x%04X <-> 0x%04X mask\n", alternates[1], (abssrc & mask));
                }
                alternates[1] = (abssrc & mask);
            }
            else if (doprint)
            {
                printf ("  ebit1  0x%04X ok\n", alternates[1]);
            }
        }
        else
        {
            alternates[0] = (abssrc & mask2);
            alternates[1] = (abssrc & mask3);
            float alt0delta = srcFloat - imath_half_to_float(alternates[0]);
            if (alt0delta >= errTol)
            {
                if (doprint)
                {
                    printf ("  swap1  0x%04X <-> 0x%04X mask\n", alternates[0], (abssrc & mask));
                }
                alternates[0] = (abssrc & mask);
            }
            else if (doprint)
            {
                printf ("  ebit1  0x%04X ok\n", alternates[1]);
            }
        }
    }
    alternates[2] = ((abssrc + npow2) & mask);

    if (doprint)
    {
        printf ("  sortp  0x%04X 0x%04X 0x%04X\n",
                alternates[0], alternates[1], alternates[2]);
        printf ("  test0  0x%04X %016b\n", alternates[0], alternates[0]);
        printf ("  test1  0x%04X %016b\n", alternates[1], alternates[1]);
        printf ("  test2  0x%04X %016b\n", alternates[2], alternates[2]);
    }
    uint16_t alt, smallest = abssrc;
    int bits, smallbits = countSetBits(abssrc);
    float delta, smalldelta = errTol;

    TEST_QUANT_ALTERNATE_SMALL(alternates[0]);
    TEST_QUANT_ALTERNATE_SMALL(alternates[1]);
    TEST_QUANT_ALTERNATE_LARGE(alternates[2]);

    return smallest;
}

static inline uint16_t handleQuantizeLargerSig (
    uint16_t abssrc, uint16_t npow2, uint16_t mask, float errTol, float srcFloat, int doprint)
{
    // in this case, only need to test two scenarios:
    //
    // can't fully zero out the masked region, so go to "0.5" of that
    // region and then test the rounded value...
    const uint16_t mask2 = (mask ^ (npow2 | (npow2 >> 1)));

    uint16_t alt0 = (abssrc & mask2);
    uint16_t alt1 = ((abssrc + npow2) & mask);

    if (doprint)
    {
        printf ("  mask2  0x%04X %016b\n", mask2, mask2);
        printf ("  test0  0x%04X %016b\n", alt0, alt0);
        printf ("  test1  0x%04X %016b\n", alt1, alt1);
    }

    int bits0 = countSetBits (alt0);
    int bits1 = countSetBits (alt1);

    float delta;

    if (bits1 < bits0)
    {
        delta = imath_half_to_float(alt1) - srcFloat; // alt1 >= srcFloat
        // bits1 < bits0 and if ok, just return
        if (delta < errTol)
            return alt1;
        delta = srcFloat - imath_half_to_float(alt0); // alt0 <= srcFloat
        if (delta < errTol)
            return alt0;
    }
    else if (bits1 == bits0)
    {
        delta = srcFloat - imath_half_to_float(alt0);
        float delta1 = imath_half_to_float(alt1) - srcFloat;
        if (delta < errTol)
            return (delta1 < delta) ? alt1 : alt0;

        if (delta1 < errTol)
            return alt1;
    }
    else
    {
        delta = srcFloat - imath_half_to_float(alt0);
        // bits0 < bits1 so if ok, just return
        if (delta < errTol)
            return alt0;

        // fallback...
        // in this case, alt1 rounding could have made
        // bits1 larger than src, test for that
        int srcbits = countSetBits (abssrc);
        if (bits1 < srcbits)
        {
            delta = imath_half_to_float(alt1) - srcFloat;
            if (delta < errTol)
                return alt1;
        }
    }
    return abssrc;
}

static inline uint16_t handleQuantizeSmallerSig (
    uint16_t abssrc, uint16_t npow2, uint16_t mask, float errTol, float srcFloat, int doprint)
{
    // in this case, only need to test two cases:
    //
    // base truncation and rounded truncation
    uint16_t alt0 = (abssrc & mask);
    uint16_t alt1 = ((abssrc + npow2) & mask);

    if (doprint)
    {
        printf ("  test0  0x%04X %016b\n", alt0, alt0);
        printf ("  test1  0x%04X %016b\n", alt1, alt1);
    }

    int bits0 = countSetBits (alt0);
    int bits1 = countSetBits (alt1);

    float delta;

    if (bits1 < bits0)
    {
        delta = imath_half_to_float(alt1) - srcFloat; // alt1 >= srcFloat
        // bits1 < bits0 and if ok, just return
        if (delta < errTol)
            return alt1;
        delta = srcFloat - imath_half_to_float(alt0); // alt0 <= srcFloat
        if (delta < errTol)
            return alt0;
    }
    else if (bits1 == bits0)
    {
        delta = srcFloat - imath_half_to_float(alt0);
        float delta1 = imath_half_to_float(alt1) - srcFloat;
        if (delta < errTol)
            return (delta1 < delta) ? alt1 : alt0;

        if (delta1 < errTol)
            return alt1;
    }
    else
    {
        delta = srcFloat - imath_half_to_float(alt0);
        // bits0 < bits1 so if ok, just return
        if (delta < errTol)
            return alt0;

        // fallback...
        // in this case, alt1 rounding could have made
        // bits1 larger than src, test for that
        int srcbits = countSetBits (abssrc);
        if (bits1 < srcbits)
        {
            delta = imath_half_to_float(alt1) - srcFloat;
            if (delta < errTol)
                return alt1;
        }
    }
    return abssrc;
}

static inline uint16_t handleQuantizeEqualSig (
    uint16_t abssrc, uint16_t npow2, uint16_t mask, float errTol, float srcFloat, int doprint)
{
    // 99.99% of the time, mask is the best choice but for a very few
    // 16-bit float to 32-bit float where even though the significands
    // of the shifted tolerance we will need mask2, so have a
    // different implementation than the basic choose 2 of the larger
    // / smaller cases
    uint16_t alt0 = (abssrc & mask);
    uint16_t alt1 = ((abssrc + npow2) & mask);

    if (doprint)
    {
        printf ("  test0  0x%04X %016b\n", alt0, alt0);
        printf ("  test1  0x%04X %016b\n", alt1, alt1);
    }

    // this costs us not much extra if it works (99.99% of the
    // time) as we would compute this immediately assuming
    // the mask almost always makes the bits smaller...
    float delta0 = srcFloat - imath_half_to_float(alt0);
    if (delta0 >= errTol)
    {
        const uint16_t mask2 = (mask ^ (npow2 | (npow2 >> 1)));

        alt0 = (abssrc & mask2);
        delta0 = srcFloat - imath_half_to_float(alt0);

        if (doprint)
        {
            printf ("  mask2  0x%04X %016b\n", mask2, mask2);
            printf ("  test2  0x%04X %016b\n", alt0, alt0);
        }

        // avoid a re-check against the tolerance below...
        if (delta0 >= errTol)
        {
            float delta1 = imath_half_to_float(alt1) - srcFloat;
            if (delta1 < errTol)
            {
                int bits1 = countSetBits (alt1);
                int srcbits = countSetBits (abssrc);
                if (bits1 < srcbits)
                    return alt1;
            }
            return abssrc;
        }
    }

    int bits0 = countSetBits (alt0);
    int bits1 = countSetBits (alt1);

    // bits0 is either the same as src (i.e. mask didn't mask any bits)
    // or smaller than src, so do not need to check against that
    //
    // bits1 because we add npow2 may not actually end up smaller...
    if (bits1 < bits0)
    {
        float delta1 = imath_half_to_float(alt1) - srcFloat;
        // bits1 < bits0 and if ok, just return
        if (delta1 < errTol)
            return alt1;
    }
    else if (bits1 == bits0)
    {
        float delta1 = imath_half_to_float(alt1) - srcFloat;
        if (delta1 < delta0)
            return alt1;
    }

    // bits0 < bits1 and ok
    return alt0;
}

static uint16_t handleQuantizeDefault (
    uint16_t abssrc, uint16_t tolSig, float errTol, float srcFloat, int doprint)
{
    // classic would do clz(significand - 1) but here we are trying to
    // construct a mask, so want to ensure for an power of 2, we
    // actually get the next (i.e. 2 returns 4)
    const uint16_t tsigshift = (32 - countLeadingZeros (tolSig));
    const uint16_t npow2 = (1 << tsigshift);
    const uint16_t lowermask = npow2 - 1;
    const uint16_t mask = ~lowermask;
    const uint16_t srcMaskedVal = abssrc & lowermask;

    // a value of 0 indicates expDiff > 1 * srcMaskedVal == tolSig
    if (doprint)
    {
        printf ("  npow2  0x%04X %016b %d\n", npow2, npow2, tsigshift);
        printf ("  srcMV  0x%04X %016b %d\n", srcMaskedVal, srcMaskedVal, srcMaskedVal);
        printf ("  mask   0x%04X %016b\n", mask, mask);
    }

    if (srcMaskedVal > tolSig)
    {
        if (doprint)
        {
            printf ("  === src sig portion > tolSig\n");
        }

        return handleQuantizeLargerSig (abssrc, npow2, mask, errTol, srcFloat, doprint);
    }
    else if (srcMaskedVal < tolSig)
    {
        if (doprint)
        {
            printf ("  === src sig portion > tolSig\n");
        }

        return handleQuantizeSmallerSig (abssrc, npow2, mask, errTol, srcFloat, doprint);
    }
    if (doprint)
    {
        printf ("  === src sig portion == tolSig\n");
    }
    return handleQuantizeEqualSig (abssrc, npow2, mask, errTol, srcFloat, doprint);
}

static uint16_t algoQuantize (
    uint16_t src, uint16_t herrTol, float errTol, float srcFloat, int doprint)
{
    uint16_t sign = src & 0x8000;
    uint16_t abssrc = src & 0x7FFF;

    srcFloat = fabsf(srcFloat);

    // if nan / inf, just bail and return src
    uint16_t srcExpBiased = src & 0x7C00;
    uint16_t tolExpBiased = herrTol & 0x7C00;

    if (doprint)
    {
        uint16_t srcSig = src & 0x3FF;
        uint16_t tolSig = herrTol & 0x3FF;

        printf ("ALGO 0x%04X (0x%04X | 0x%04X) %g: biased exp: 0x%04X %d %d, significand 0x%04X %d\n",
                src | sign, src, sign, imath_half_to_float (src | sign),
                (src & 0x7C00), (int)(srcExpBiased >> 10), (int)(srcExpBiased >> 10) - 15,
                srcSig, (int)srcSig);
        printf ("tolerance 0x%04X %g: biased exp: 0x%04X %d %d, significand 0x%04X %d\n",
                herrTol, imath_half_to_float (herrTol),
                (herrTol & 0x7C00), (int)(tolExpBiased >> 10), (int)(tolExpBiased >> 10) - 15,
                tolSig, tolSig);
    }

    if (srcExpBiased == 0x7C00)
    {
        if (doprint)
            printf ("  ====> src is inf / nan -> 0x%04X\n", src);
        return src;
    }

    // can't possibly beat 0 bits
    if (srcFloat < errTol)
    {
        if (doprint)
            printf ("  ====> src is smaller than tolerance -> 0\n");
        return 0;
    }

    uint16_t expDiff = (srcExpBiased - tolExpBiased) >> 10;
//    uint16_t tolSig = (((herrTol & 0x3FF) | ((tolExpBiased > 0) << 10)) >> expDiff);
    uint16_t tolSig = (((herrTol & 0x3FF) | (1 << 10)) >> expDiff);

    if (doprint)
    {
        uint16_t srcSig = src & 0x3FF;

        printf ("  srcExp - tolExp %d\n", (int)(expDiff));
        printf ("  tolSig 0x%04X %016b %d\n", (herrTol & 0x3FF), (herrTol & 0x3FF), (herrTol & 0x3FF));
        printf ("   >>    0x%04X %016b %d\n", tolSig, tolSig, tolSig);
        printf ("  src    0x%04X %016b\n", src, src);
        printf ("  srcSig 0x%04X %016b\n", srcSig, srcSig);
        printf ("  sign   0x%04X %016b\n", sign, sign);
        printf ("  abssrc 0x%04X %016b\n", abssrc, abssrc);
    }

    if (tolExpBiased == 0)
    {
        if (expDiff == 0 || expDiff == 1)
        {
            tolSig = (herrTol & 0x3FF);
            if (tolSig == 0)
                return src;
            return sign | handleQuantizeGeneric (abssrc, tolSig, errTol, srcFloat, doprint);
        }

        tolSig = (herrTol & 0x3FF);
        if (tolSig == 0)
            return src;

        tolSig >>= expDiff;
        if (tolSig == 0)
            tolSig = 1;

        return sign | handleQuantizeDenormTol (abssrc, tolSig, errTol, srcFloat, doprint);
    }

    if (tolSig == 0)
    {
        if (doprint)
            printf ("  ====> tolSig is 0, tolerance too small: 0x%04X\n", src);
        return src;
    }

    // we want to try to find a number that has the fewest bits that
    // is within the specified tolerance. To do so without a lookup
    // table, we shift (multiply if you will) the tolerance to be
    // within the same exponent range as the source value.
    //
    // we then need to consider a few bit scenarios
    //
    //
    // all b bits should be preserved (or the delta would be too large)
    // all a bits should be discarded (0'ed)
    // s (first bit where the significand of the tolerance is on)
    // p (next power of 2 above s - npow2 above)
    // x (an extra bit above power of 2)
    //
    // we need to consider a bit mask of:
    // ..bbbbxps aaaaa..
    // ..bbbb110 00000.. (mask as above)
    // ..bbbb100 00000..
    // ..bbbb101 00000..
    // ..bbbb000 00000..
    // ..bbbb001 00000.. (mutually exclusive with previous)
    // ..bbbb1+0 00000.. (add 1 at p bit, then same mask as first case)
    //
    // so if you collapse the mutually exclusive one, that gives 5
    // choices, although they don't always apply, so only need 4 with
    // a bit of conditional tests:
    //
    // uint16_t inexp = (npow2 > 0x0200);
    // uint16_t mask2 = (mask ^ npow2) * inexp;
    // uint16_t extrabit = (tolSig > srcMaskedVal);
    // uint16_t mask3 = (mask ^ npow2);
    // mask3 ^= ((npow2 << 1) * extrabit);
    // mask3 |= ((npow2 >> 1) * (! extrabit));
    //
    // (src & mask)
    // (src & mask2)
    // (src & mask3)
    // ((src + npow2) & mask);
    //
    // So one of those 4 is always one of our choices, but just
    // blindly computing all 4 is about as expensive as doing the
    // table lookup and having a sorted set by number of bits to
    // return the first from, so we're not done yet.
    //
    // however, if the tolerance is small relative to the source, can reduce
    // the ones we need to test, as some of these are always invalid when in
    // that scenario:
    //
    // if the portion of the source significand is larger than the tolerance
    // significand, we can't simply truncate, or would be out of range of the
    // tolerance, so only left with 2 scenarios:
    //
    // ..bbbb101 00000.. (truncation with preservation of sig bit for "0.5")
    // ..bbbb1+0 00000.. (add 1 to round up, then same mask)
    //
    // if the portion of the source significand is strictly less than
    // the tolerance significand, we can do the base truncation and
    // the "round up" may still be within the tolerance, but the
    // deeper truncations will be out of range, so again only need 2
    // (but different 2):
    //
    // ..bbbb110 00000.. (mask as above)
    // ..bbbb1+0 00000.. (add 1 to round up, then same mask)
    //
    // if the significand is equal to the tolerance, depending on the
    // translation back to 32 bit to compare against the 32 bit
    // tolerance (if we could make that a 16-bit value, we could make
    // different decisions), we have to test all 3 of those values:
    //
    // ..bbbb101 00000.. (truncation with preservation of sig bit for "0.5")
    // ..bbbb110 00000.. (mask as above)
    // ..bbbb1+0 00000.. (add 1 to round up, then same mask)
    //
    // 99.99% of the time, the mask or round will be a good choice,
    // but only in a few combinations of tolerance will the truncation
    // be needed because the mask truncation will be out of range,
    // which is not surprising given we're just shifting the
    // significand of the half-float tolerance, where the tolerance is
    // against the original 32-bit value, but can be quickly tested
    // and swapped for fewer comparisons than testing all 3 values
    //
    // when the exponent of the tolerance is close to the value of
    // src, it is a bit harder to reason about, as the masking
    // operations will be changing the exponent, and maybe preserving
    // 1 bit of the significand. for example, when the two numbers are
    // within the same exponent, it is ok to reduce to just 3 values:
    //
    // ..bbbb100 00000.. (mask2 as above)
    // ..bbbb110 00000.. (mask as above)
    // ..bbbb1+0 00000.. (add 1 to round up, then same mask)
    //
    // However that does not handle all scenarios for all (expected)
    // tolerances, so a few other cased are contemplated in each
    // scenario

    // first, handle the default case of a 'large' diff between src
    // and tolerance, or a denorm src
    if (expDiff > 1 || srcExpBiased == 0)
        return sign | handleQuantizeDefault (abssrc, tolSig, errTol, srcFloat, doprint);

    if (expDiff == 0)
        return sign | handleQuantizeEqualExp (abssrc, tolSig, errTol, srcFloat, doprint);
    return sign | handleQuantizeCloseExp (abssrc, tolSig, errTol, srcFloat, doprint);

//uint8_t sat_subu8b(uint8_t x, uint8_t y)
//{
//    uint8_t res = x - y;
//    res &= -(res <= x);
//
//    return res;
//}
}

static uint16_t tableQuantize (uint16_t src, float errTol, float srcFloat, int doprint)
{
    // pre-quantize float -> half and back
    //uint16_t src      = float_to_half (dctval);
    //float    srcFloat = imath_half_to_float (src);

    int             numSetBits = countSetBits (src);
    const uint16_t* closest    = internal_test_ns::closestData;
    closest += internal_test_ns::closestDataOffset[src];

    if (doprint)
    {
        printf ("TABLE 0x%04X %g bits %d tol %g\n", src, srcFloat, numSetBits, errTol);
    }

    while ( numSetBits >= 4 )
    {
        uint16_t tmp1 = closest[0];
        uint16_t tmp2 = closest[1];
        uint16_t tmp3 = closest[2];
        uint16_t tmp4 = closest[3];
        uint16_t res = src;

        // when compiling for f16c, gcc has no problem collapsing this
        // to 1 cvtph2ps and m128 float math assume the same will be
        // true for neon and ARM...
        float ft1 = fabsf (imath_half_to_float (tmp1) - srcFloat);
        float ft2 = fabsf (imath_half_to_float (tmp2) - srcFloat);
        float ft3 = fabsf (imath_half_to_float (tmp3) - srcFloat);
        float ft4 = fabsf (imath_half_to_float (tmp4) - srcFloat);

        // do in reverse order such that vectorizer can more easily notice the
        // similar comparisons and do the mask and reduce
        if (ft4 < errTol) res = tmp4;
        if (ft3 < errTol) res = tmp3;
        if (ft2 < errTol) res = tmp2;
        if (ft1 < errTol) res = tmp1;

        if (res != src)
        {
            numSetBits = 0;
            src = res;
            break;
        }

        closest += 4;
        numSetBits -= 4;
    }

    switch (numSetBits)
    {
        case 3:
            if (fabsf (imath_half_to_float (closest[0]) - srcFloat) < errTol)
            {
                src = closest[0];
            }
            else if (fabsf (imath_half_to_float (closest[1]) - srcFloat) < errTol)
            {
                src = closest[1];
            }
            else if (fabsf (imath_half_to_float (closest[2]) - srcFloat) < errTol)
            {
                src = closest[2];
            }
            break;
        case 2:
            if (fabsf (imath_half_to_float (closest[0]) - srcFloat) < errTol)
            {
                src = closest[0];
            }
            else if (fabsf (imath_half_to_float (closest[1]) - srcFloat) < errTol)
            {
                src = closest[1];
            }
            break;
        case 1:
            if (fabsf (imath_half_to_float (closest[0]) - srcFloat) < errTol)
            {
                src = closest[0];
            }
            break;
        default:
            break;
    }
#if 0
    for (int targetNumSetBits = numSetBits - 1;
          targetNumSetBits >= 0;
          --targetNumSetBits)
     {
         uint16_t tmp = *closest;

         if (doprint)
             printf ("  consider 0x%04X %g => %g < %g\n", tmp,
                     imath_half_to_float (tmp),
                     fabsf (imath_half_to_float (tmp) - srcFloat),
                     errTol);
         if (fabsf (imath_half_to_float (tmp) - srcFloat) < errTol)
         {
             if (doprint)
                 printf ("    ===> tbits %d 0x%04X\n", targetNumSetBits, tmp);
             return tmp;
         }

         closest++;
     }
     if (doprint)
         printf ("    ===> nothing better 0x%04X\n", src);
#else
     if (doprint)
         printf ("    ===> 0x%04X\n", src);
#endif

     return src;
}

static void testQuantizer (float tol)
{
    uint16_t halfTol = imath_float_to_half (tol);

    int failcount = 0;
    for ( int h = 0; h < 65536; ++h )
    {
        uint16_t hval = h;
        float    hFloat = imath_half_to_float (hval);
        uint16_t tq = tableQuantize (hval, tol, hFloat, 0);
        uint16_t aq = algoQuantize (hval, halfTol, tol, hFloat, 0);

        if (tq != aq)
        {
            static int didfirstprint = 0;
            ++failcount;
            if (didfirstprint == 0)
            {
                tq = tableQuantize (hval, tol, hFloat, 1);
                aq = algoQuantize (hval, halfTol, tol, hFloat, 1);
                didfirstprint = 1;
                printf ("ERROR: tol %g half %u 0x%04X %g: table 0x%04X %g algo 0x%04X %g\n",
                        tol, hval, hval, imath_half_to_float (hval),
                        tq, imath_half_to_float (tq),
                        aq, imath_half_to_float (aq));
            }
        }
        //EXRCORE_TEST (tableQuantize (hval, tol) == algoQuantize (hval, halfTol));
    }
    if (failcount > 0)
        printf ("ERROR: tol %g: %d fails\n", tol, failcount);
    //else
    //    printf ("SUCCESS: tol %g: %d fails\n", tol, failcount);
}

void testDWAQuantize (const std::string& tempdir)
{
    int jpegQuantTableY[] = {
        16, 11, 10, 16, 24,  40,  51,  61,  12, 12, 14, 19, 26,  58,  60,  55,
        14, 13, 16, 24, 40,  57,  69,  56,  14, 17, 22, 29, 51,  87,  80,  62,
        18, 22, 37, 56, 68,  109, 103, 77,  24, 35, 55, 64, 81,  104, 113, 92,
        49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99};

    int jpegQuantTableYMin = 10;

    int jpegQuantTableCbCr[] = {
        17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99,
        24, 26, 56, 99, 99, 99, 99, 99, 47, 66, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99};

    int jpegQuantTableCbCrMin = 17;
//#define EXHAUSTING_TEST
#ifdef EXHAUSTING_TEST
    // the qscale is limited 0 - 100 in the set dwa compression level routine
    // in the library, so shouldn't have to test outside that range...
    for (float qscale = 0.f; qscale <= 100.f; qscale += 0.25f)
    {
        for ( int i = 0; i < 64; ++i )
        {
            // dwa encoder takes compression level and divides by 100000 to
            // get quantisation scale
            float quantlevel = qscale / 100000.f;

            testQuantizer (quantlevel * (float)(jpegQuantTableY[i]) / ((float)(jpegQuantTableYMin)));

            testQuantizer (quantlevel * (float)(jpegQuantTableCbCr[i]) / ((float)(jpegQuantTableCbCrMin)));
        }
    }
#else
    float baseErrors[] = { 0.f, 1.f, 20.f, 45.f /*default*/, 60.f, 73.f, 95.f, 100.f, 230.f };

    for ( int be = 0; be < (sizeof(baseErrors)/sizeof(float)); ++be )
    {
        for ( int i = 0; i < 64; ++i )
        {
            // dwa encoder takes compression level and divides by 100000 to
            // get quantisation scale
            float quantlevel = baseErrors[be] / 100000.f;

            testQuantizer (quantlevel * (float)(jpegQuantTableY[i]) / ((float)(jpegQuantTableYMin)));

            testQuantizer (quantlevel * (float)(jpegQuantTableCbCr[i]) / ((float)(jpegQuantTableCbCrMin)));
        }
    }
    // particular edge cases previously
    testQuantizer (0.00048875);
    testQuantizer (0.00782075);
    testQuantizer (0.007821);
    testQuantizer (0.0078155);
    testQuantizer (0.00781375);
    testQuantizer (0.00782);
    testQuantizer (0.0078165);
    testQuantizer (0.0009765);
    testQuantizer (0.001244117622);
    testQuantizer (0.001747058821);
    testQuantizer (0.002620588057);
    testQuantizer (0.001080000075);
#endif
}

////////////////////////////////////////

void
testB44Table (const std::string&)
{
    const int iMax = (1 << 16);

    for (int i = 0; i < iMax; i++)
    {
        half h;
        h.setBits (i);

        if (!h.isFinite ())
            h = 0;
        else if (h >= 8 * log (HALF_MAX))
            h = HALF_MAX;
        else
            h = exp (h / 8);

        EXRCORE_TEST (exrcore_expTable[i] == h.bits ());
    }

    for (int i = 0; i < iMax; i++)
    {
        half h;
        h.setBits (i);

        if (!h.isFinite () || h < 0)
            h = 0;
        else
            h = 8 * log (h);

        EXRCORE_TEST (exrcore_logTable[i] == h.bits ());
    }
}
