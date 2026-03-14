//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Weta Digital, Ltd and Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "ImfChannelList.h"
#include "ImfCompression.h"
#include "ImfFrameBuffer.h"
#include "ImfInputFile.h"
#include "ImfOutputFile.h"
#include "ImfStandardAttributes.h"
#include <IlmThread.h>
#include <ImathBox.h>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <vector>

#include "random.h"
#include "tmpDir.h"

namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace IMF;
using namespace std;
using namespace IMATH_NAMESPACE;
using namespace ILMTHREAD_NAMESPACE;

namespace
{

using OPENEXR_IMF_NAMESPACE::FLOAT;
using OPENEXR_IMF_NAMESPACE::UINT;

std::string filename;

vector<char> writingBuffer; // buffer as file was written
vector<char>
    readingBuffer; // buffer containing new image (and filled channels?)
vector<char>
    preReadBuffer; // buffer as it was before reading - unread, unfilled channels should be unchanged

int gOptimisedReads = 0;
int gSuccesses      = 0;
int gFailures       = 0;

//
// @todo Needs a description of what this is used for.
//
//
struct Schema
{
    const char*        _name;   // name of this scheme
    const char* const* _active; // channels to be read
    const char* const*
        _passive; // channels to be ignored (keep in buffer passed to inputfile, should not be overwritten)
    int                _banks;
    const char* const* _views; // list of views to write, or NULL
    const PixelType*   _types; // NULL for all HALF, otherwise per-channel type

    vector<string> views () const
    {
        const char* const* v = _views;
        vector<string>     svec;
        while (*v != NULL)
        {
            svec.push_back (*v);
            v++;
        }
        return svec;
    }
};

const char* rgb[]         = {"R", "G", "B", NULL};
const char* rgba[]        = {"R", "G", "B", "A", NULL};
const char* bgr[]         = {"B", "G", "R", NULL};
const char* abgr[]        = {"A", "B", "G", "R", NULL};
const char* alpha[]       = {"A", NULL};
const char* redalpha[]    = {"R", "A", NULL};
const char* rgbrightrgb[] = {
    "R", "G", "B", "right.R", "right.G", "right.B", NULL};
const char* rgbleftrgb[] = {"R", "G", "B", "left.R", "left.G", "left.B", NULL};
const char* rgbarightrgba[] = {
    "R", "G", "B", "A", "right.R", "right.G", "right.B", "right.A", NULL};
const char* rgbaleftrgba[] = {
    "R", "G", "B", "A", "left.R", "left.G", "left.B", "left.A", NULL};
const char* rgbrightrgba[] = {
    "R", "G", "B", "right.R", "right.G", "right.B", "right.A", NULL};
const char* rgbleftrgba[] = {
    "R", "G", "B", "left.R", "left.G", "left.B", "left.A", NULL};
const char* rgbarightrgb[] = {
    "R", "G", "B", "A", "right.R", "right.G", "right.B", NULL};
const char* rgbaleftrgb[] = {
    "R", "G", "B", "A", "left.R", "left.G", "left.B", NULL};
const char* rightrgba[] = {"right.R", "right.G", "right.B", "right.A", NULL};
const char* leftrgba[]  = {"left.R", "left.G", "left.B", "left.A", NULL};
const char* rightrgb[]  = {"right.R", "right.G", "right.B", NULL};
const char* leftrgb[]   = {"left.R", "left.G", "left.B", NULL};
const char* threeview[] = {
    "R",
    "G",
    "B",
    "A",
    "left.R",
    "left.G",
    "left.B",
    "left.A",
    "right.R",
    "right.G",
    "right.B",
    "right.A",
    NULL};
const char* trees[]         = {"rimu", "pohutukawa", "manuka", "kauri", NULL};
const char* treesandbirds[] = {
    "kiwi",
    "rimu",
    "pohutukawa",
    "kakapu",
    "kauri",
    "manuka",
    "moa",
    "fantail",
    NULL};

const char* lefthero[]   = {"left", "right", NULL};
const char* righthero[]  = {"right", "left", NULL};
const char* centrehero[] = {"centre", "left", "right", NULL};

const PixelType four_floats[] = {
    IMF::FLOAT, IMF::FLOAT, IMF::FLOAT, IMF::FLOAT};
const PixelType hhhfff[] = {
    IMF::HALF, IMF::HALF, IMF::HALF, IMF::FLOAT, IMF::FLOAT, IMF::FLOAT};
const PixelType hhhhffff[] = {
    IMF::HALF,
    IMF::HALF,
    IMF::HALF,
    IMF::HALF,
    IMF::FLOAT,
    IMF::FLOAT,
    IMF::FLOAT,
    IMF::FLOAT};

Schema Schemes[] = {
    {"RGBHalf", rgb, NULL, 1, NULL, NULL},
    {"RGBAHalf", rgba, NULL, 1, NULL, NULL},
    {"ABGRHalf", abgr, NULL, 1, NULL, NULL},
    {"RGBFloat", rgb, NULL, 1, NULL, four_floats},
    {"BGRHalf", bgr, NULL, 1, NULL, NULL},
    {"RGBLeftRGB", rgbleftrgb, NULL, 1, righthero, NULL},
    {"RGBRightRGB", rgbrightrgb, NULL, 1, lefthero, NULL},
    {"RGBALeftRGBA", rgbaleftrgba, NULL, 1, righthero, NULL},
    {"RGBARightRGBA", rgbarightrgba, NULL, 1, lefthero, NULL},
    {"LeftRGB", leftrgb, NULL, 1, NULL, NULL},
    {"RightRGB", rightrgb, NULL, 1, NULL, NULL},
    {"LeftRGBA", leftrgba, NULL, 1, NULL, NULL},
    {"RightRGBA", rightrgba, NULL, 1, NULL, NULL},
    {"TripleView", threeview, NULL, 1, centrehero, NULL},
    {"Trees", trees, NULL, 1, NULL, NULL},
    {"TreesAndBirds", treesandbirds, NULL, 1, NULL, NULL},
    {"RGBLeftRGBA", rgbleftrgba, NULL, 1, righthero, NULL},
    {"RGBRightRGBA", rgbrightrgba, NULL, 1, lefthero, NULL},
    {"RGBALeftRGB", rgbaleftrgb, NULL, 1, righthero, NULL},
    {"RGBARightRGB", rgbarightrgb, NULL, 1, lefthero, NULL},
    {"TwinRGBLeftRGB", rgbleftrgb, NULL, 2, righthero, NULL},
    {"TwinRGBRightRGB", rgbrightrgb, NULL, 2, lefthero, NULL},
    {"TwinRGBALeftRGBA", rgbaleftrgba, NULL, 2, righthero, NULL},
    {"TwinRGBARightRGBA", rgbarightrgba, NULL, 2, lefthero, NULL},
    {"TripleTripleView", threeview, NULL, 3, centrehero, NULL},
    {"Alpha", alpha, NULL, 1, NULL, NULL},
    {"RedAlpha", redalpha, NULL, 1, NULL, NULL},
    {"RG+BA", rgba, NULL, 2, NULL, NULL},       //interleave only RG, then BA
    {"RGBpassiveA", rgb, alpha, 1, NULL, NULL}, //interleave only RG, then BA
    {"RGBpassiveleftRGB", rgb, leftrgb, 1, NULL, NULL},
    {"RGBFloatA", rgba, NULL, 1, NULL, hhhfff},
    {"RGBFloatLeftRGB", rgbleftrgb, NULL, 1, righthero, hhhfff},
    {"RGBAFloatLeftRGBA", rgbaleftrgba, NULL, 1, righthero, hhhhffff},
    {"RGBApassiverightRGBA", rgba, rightrgba, 1, NULL, NULL},
    {"BanksOfTreesAndBirds", treesandbirds, NULL, 2, NULL, NULL},
    {NULL, NULL, NULL, 0, NULL, NULL}};

template <class T>
inline T
alignToFour (T input)
{
    while ((intptr_t (input) & 3) != 0)
    {
        input++;
    }
    return input;
}

bool
compare (
    const FrameBuffer& asRead,
    const FrameBuffer& asWritten,
    const Box2i&       dataWindow,
    bool               nonfatal)
{
    for (FrameBuffer::ConstIterator i = asRead.begin (); i != asRead.end ();
         i++)
    {
        FrameBuffer::ConstIterator p = asWritten.find (i.name ());
        for (int y = dataWindow.min.y; y <= dataWindow.max.y; y++)
        {
            for (int x = dataWindow.min.x; x <= dataWindow.max.x; x++)

            {
                //
                // extract value read back from file
                //
                intptr_t base = reinterpret_cast<intptr_t> (i.slice ().base);
                char*    ptr  = reinterpret_cast<char*> (
                    base + i.slice ().yStride * intptr_t (y) +
                    i.slice ().xStride * intptr_t (x));
                half readHalf;
                switch (i.slice ().type)
                {
                    case IMF::FLOAT:
                        assert (alignToFour (ptr) == ptr);
                        readHalf = half (*(float*) ptr);
                        break;
                    case IMF::HALF: readHalf = half (*(half*) ptr); break;
                    case IMF::UINT: continue; // can't very well check this
                    default: cout << "don't know about that\n"; exit (1);
                }

                half writtenHalf;

                if (p != asWritten.end ())
                {

                    intptr_t base =
                        reinterpret_cast<intptr_t> (p.slice ().base);
                    char* ptr = reinterpret_cast<char*> (
                        base + p.slice ().yStride * intptr_t (y) +
                        p.slice ().xStride * intptr_t (x));
                    switch (p.slice ().type)
                    {
                        case IMF::FLOAT:
                            assert (alignToFour (ptr) == ptr);
                            writtenHalf = half (*(float*) ptr);
                            break;
                        case IMF::HALF:
                            writtenHalf = half (*(half*) ptr);
                            break;
                        case IMF::UINT: continue;
                        default: cout << "don't know about that\n"; exit (1);
                    }
                }
                else { writtenHalf = half (i.slice ().fillValue); }

                if (writtenHalf.bits () != readHalf.bits ())
                {
                    if (nonfatal) { return false; }
                    else
                    {
                        cout << "\n\nerror reading back channel " << i.name ()
                             << " pixel " << x << ',' << y << " got "
                             << readHalf << " expected " << writtenHalf << endl;
                        assert (writtenHalf.bits () == readHalf.bits ());
                        exit (1);
                    }
                }
            }
        }
    }
    return true;
}

//
// allocate readingBuffer or writingBuffer, setting up a framebuffer to point to the right thing
//
ChannelList
setupBuffer (
    const Header&      hdr,      // header to grab datawindow from
    const char* const* channels, // NULL terminated list of channels to write
    const char* const*
        passivechannels,  // NULL terminated list of channels to write
    const PixelType* pt,  // type of each channel, or NULL for all HALF
    FrameBuffer&     buf, // buffer to fill with pointers to channel
    FrameBuffer&
        prereadbuf, // channels which aren't being read - indexes into the preread buffer
    FrameBuffer&
        postreadbuf, // channels which aren't being read - indexes into the postread buffer
    int banks, // number of banks - channels within each bank are interleaved, banks are scanline interleaved
    bool writing, // true if should allocate
    bool
        allowNonfinite // true if the buffer is allowed to create infinity or NaN values
)
{
    Box2i dw = hdr.dataWindow ();

    //
    // how many channels in total
    //
    int  activechans     = 0;
    int  bytes_per_pixel = 0;
    bool has32BitValue   = false;
    while (channels[activechans] != NULL)
    {
        if (pt == NULL) { bytes_per_pixel += 2; }
        else
        {
            switch (pt[activechans])
            {
                case IMF::HALF: bytes_per_pixel += 2; break;
                case IMF::FLOAT:
                case IMF::UINT:
                    // some architectures (e.g arm7) cannot write 32 bit values
                    // to addresses which aren't aligned to 32 bit addresses
                    // so bump to next multiple of four
                    bytes_per_pixel = alignToFour (bytes_per_pixel);
                    bytes_per_pixel += 4;
                    has32BitValue = true;
                    break;

                default: cout << "Unexpected PixelType?\n"; exit (1);
            }
        }
        activechans++;
    }

    int passivechans = 0;
    while (passivechannels != NULL && passivechannels[passivechans] != NULL)
    {
        if (pt == NULL) { bytes_per_pixel += 2; }
        else
        {
            switch (pt[passivechans + activechans])
            {
                case IMF::HALF: bytes_per_pixel += 2; break;
                case IMF::FLOAT:
                case IMF::UINT:
                    bytes_per_pixel = alignToFour (bytes_per_pixel);
                    bytes_per_pixel += 4;
                    has32BitValue = true;
                    break;
                default: cout << "Unexpected PixelType?\n"; exit (1);
            }
        }
        passivechans++;
    }

    if (has32BitValue) { bytes_per_pixel = alignToFour (bytes_per_pixel); }

    int chans = activechans + passivechans;

    int bytes_per_bank = bytes_per_pixel / banks;

    int samples = (hdr.dataWindow ().max.x + 1 - hdr.dataWindow ().min.x) *
                  (hdr.dataWindow ().max.y + 1 - hdr.dataWindow ().min.y) *
                  chans;

    int size = samples * bytes_per_pixel;

    if (writing) { writingBuffer.resize (size); }
    else { readingBuffer.resize (size); }

    const char* write_ptr = writing ? &writingBuffer[0] : &readingBuffer[0];
    // fill with random halves, casting to floats for float channels
    int chan = 0;
    for (int i = 0; i < samples; i++)
    {
        half v;
        // generate a random finite half
        // if the value is to be cast to a float, ensure the value is not infinity
        // or NaN, since the value is not guaranteed to round trip from half to float
        // and back again with bit-identical precision
        do
        {
            unsigned short int values =
                random_int (std::numeric_limits<unsigned short>::max ());
            v.setBits (values);
        } while (!((v - v) == 0 || allowNonfinite));

        if (pt == NULL || pt[chan] == IMF::HALF)
        {
            *(half*) write_ptr = half (v);
            write_ptr += 2;
        }
        else
        {
            write_ptr           = alignToFour (write_ptr);
            *(float*) write_ptr = float (v);

            write_ptr += 4;
        }
        chan++;
        if (chan == chans) { chan = 0; }
    }

    if (!writing)
    {
        //take a copy of the buffer as it was before being read
        preReadBuffer = readingBuffer;
    }

    char* offset = NULL;

    ChannelList chanlist;

    int bytes_per_row      = bytes_per_pixel * (dw.max.x + 1 - dw.min.x);
    int bytes_per_bank_row = bytes_per_row / banks;

    int first_pixel_index =
        bytes_per_row * dw.min.y + bytes_per_bank * dw.min.x;

    for (int i = 0; i < chans; i++)
    {
        PixelType type = pt == NULL ? IMF::HALF : pt[i];
        if (i < activechans && writing) { chanlist.insert (channels[i], type); }

        if (i % (chans / banks) == 0)
        {
            //
            // set offset pointer to beginning of bank
            //

            int bank = i / (chans / banks);
            offset   = (writing ? &writingBuffer[0] : &readingBuffer[0]) +
                     bank * bytes_per_bank_row - first_pixel_index;
        }

        if (type == FLOAT || type == UINT) { offset = alignToFour (offset); }

        if (i < activechans)
        {

            buf.insert (
                channels[i],
                Slice (
                    type,
                    offset,
                    bytes_per_bank,
                    bytes_per_row,
                    1,
                    1,
                    100 + i));
        }
        else
        {
            if (!writing)
            {

                postreadbuf.insert (
                    passivechannels[i - activechans],
                    Slice (
                        type,
                        offset,
                        bytes_per_bank,
                        bytes_per_row,
                        1,
                        1,
                        0.4));

                char* pre_offset =
                    offset - &readingBuffer[0] + &preReadBuffer[0];

                prereadbuf.insert (
                    passivechannels[i - activechans],
                    Slice (
                        type,
                        pre_offset,
                        bytes_per_bank,
                        bytes_per_row,
                        1,
                        1,
                        0.4));
            }
        }
        switch (type)
        {
            case IMF::HALF: offset += 2; break;
            case IMF::FLOAT: offset += 4; break;
            default: cout << "Unexpected Pixel Type\n"; exit (1);
        }
    }

    return chanlist;
}

Box2i
writefile (Schema& scheme, FrameBuffer& buf, bool tiny, bool allowNonfinite)
{
    const int height = 128;
    const int width  = 128;

    Header hdr (width, height, 1);

    //min values in range [-100,100]
    hdr.dataWindow ().min.x = random_int (201) - 100;
    hdr.dataWindow ().min.y = random_int (201) - 100;

    // in tiny mode, make image up to 14*14 pixels (less than two SSE instructions)
    if (tiny)
    {
        hdr.dataWindow ().max.x = hdr.dataWindow ().min.x + 1 + random_int (14);
        hdr.dataWindow ().max.y = hdr.dataWindow ().min.y + 1 + random_int (14);
    }
    else
    {
        // in normal mode, make chunky images
        hdr.dataWindow ().max.x =
            hdr.dataWindow ().min.x + 64 + random_int (400);
        hdr.dataWindow ().max.y =
            hdr.dataWindow ().min.y + 64 + random_int (400);
    }

    hdr.compression () = ZIPS_COMPRESSION;

    FrameBuffer dummy1, dummy2;

    hdr.channels () = setupBuffer (
        hdr,
        scheme._active,
        scheme._passive,
        scheme._types,
        buf,
        dummy1,
        dummy2,
        scheme._banks,
        true,
        allowNonfinite);

    if (scheme._views) { addMultiView (hdr, scheme.views ()); }

    remove (filename.c_str ());
    OutputFile f (filename.c_str (), hdr);
    f.setFrameBuffer (buf);
    f.writePixels (hdr.dataWindow ().max.y - hdr.dataWindow ().min.y + 1);

    return hdr.dataWindow ();
}

bool
readfile (
    Schema       scheme,
    FrameBuffer& buf,     ///< list of channels to read: index to readingBuffer
    FrameBuffer& preread, ///< list of channels to skip: index to preReadBuffer
    FrameBuffer&
         postread, ///< list of channels to skip: index to readingBuffer)
    bool allowNonfinite)
{
    InputFile infile (filename.c_str ());
    setupBuffer (
        infile.header (),
        scheme._active,
        scheme._passive,
        scheme._types,
        buf,
        preread,
        postread,
        scheme._banks,
        false,
        allowNonfinite);
    infile.setFrameBuffer (buf);

    cout.flush ();
    infile.readPixels (
        infile.header ().dataWindow ().min.y,
        infile.header ().dataWindow ().max.y);

    return infile.isOptimizationEnabled ();
}

void
test (Schema writeScheme, Schema readScheme, bool nonfatal, bool tiny)
{
    ostringstream q;
    q << writeScheme._name << " read as " << readScheme._name << "...";
    cout << left << setw (53) << q.str ();

    FrameBuffer writeFrameBuf;
    // only allow NaN and infinity values if file is read and written as half float
    // (otherwise casting between half and float may cause different bit patterns)
    bool allowNonfinite =
        (writeScheme._types == nullptr && readScheme._types == nullptr);
    Box2i dw = writefile (writeScheme, writeFrameBuf, tiny, allowNonfinite);
    FrameBuffer readFrameBuf;
    FrameBuffer preReadFrameBuf;
    FrameBuffer postReadFrameBuf;
    cout.flush ();
    bool opt = readfile (
        readScheme,
        readFrameBuf,
        preReadFrameBuf,
        postReadFrameBuf,
        allowNonfinite);
    if (compare (readFrameBuf, writeFrameBuf, dw, nonfatal) &&
        compare (preReadFrameBuf, postReadFrameBuf, dw, nonfatal))
    {
        cout << " OK ";
        if (opt)
        {
            cout << "OPTIMISED ";
            gOptimisedReads++;
        }
        cout << "\n";
        gSuccesses++;
    }
    else
    {
        cout << " FAIL" << endl;
        gFailures++;
    }
    remove (filename.c_str ());
}

void
runtests (bool nonfatal, bool tiny)
{
    random_reseed (1);
    int i       = 0;
    int skipped = 0;

    gFailures       = 0;
    gSuccesses      = 0;
    gOptimisedReads = 0;

    while (Schemes[i]._name != NULL)
    {
        int j = 0;
        while (Schemes[j]._name != NULL)
        {
            cout << right << setw (2) << i << ',' << right << setw (2) << j
                 << ": ";
            cout.flush ();

            if (nonfatal)
            {
                cout << " skipping " << Schemes[i]._name << ','
                     << Schemes[j]._name << ": known to crash\n";
                skipped++;
            }
            else { test (Schemes[i], Schemes[j], nonfatal, tiny); }
            j++;
        }
        i++;
    }

    cout << gFailures << '/' << (gSuccesses + gFailures) << " runs failed\n";
    cout << skipped << " tests skipped (assumed to be bad)\n";
    cout << gOptimisedReads << '/' << gSuccesses << " optimised\n";

    if (gFailures > 0)
    {
        cout << " TESTS FAILED\n";
        assert (false);
    }
}

} // namespace

void
testOptimizedInterleavePatterns (const std::string& tempDir)
{
    filename = tempDir + "imf_test_interleave_patterns.exr";

    cout
        << "Testing SSE optimisation with different interleave patterns (large images) ... "
        << endl;
    runtests (false, false);

    cout
        << "Testing SSE optimisation with different interleave patterns (tiny images) ... "
        << endl;
    runtests (false, true);

    cout << "ok\n" << endl;
}
