//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "ImfBoxAttribute.h"
#include "ImfChannelListAttribute.h"
#include "ImfFrameBuffer.h"
#include "ImfHeader.h"
#include "ImfInputFile.h"
#include "ImfOutputFile.h"

#include <exception>
#include <iostream>
#include <string>

#include <assert.h>
#include <stdio.h>
#include <vector>
#include <half.h>

using namespace IEX_NAMESPACE;
using namespace IMATH_NAMESPACE;
using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;

template <typename Header> struct Test
{
    void testFind (const string& name)
    {
        Header header;
        auto   iterator = header.find (name);
        assert (iterator != header.end ());
    }

    void testSubscript (const string& name)
    {
        Header header;
        auto&  comparand = header.find ("displayWindow").attribute ();
        auto&  attribute = header[name];
        assert (&attribute == &comparand);
    }

    void testIterators (const string& name)
    {
        Header header;

        auto& comparand = header.find ("displayWindow").attribute ();

        for (auto iterator = header.begin (); iterator != header.end ();
             ++iterator)
        {
            if (iterator.name () == name)
            {
                assert (&iterator.attribute () == &comparand);
                return;
            }
        }

        assert (false);
    }
};

void
testEraseAttribute (const string& name)
{
    Header header;
    assert (header.find (name) != header.end ());
    header.erase (name);
    assert (header.find (name) == header.end ());
}

void
testEraseAttributeThrowsWithEmptyString ()
{
    Header header;

    try
    {
        header.erase ("");
        assert (false);
    }
    catch (const ArgExc&)
    {
        assert (true);
    }
}

//
// Only the attribute named "channels" describes the pixel data. A second
// chlist attribute must be kept as an ordinary attribute and must not add
// (unvalidated) channels to Header::channels().
//
void
testExtraChannelListAttribute (const string& tempDir)
{
    const string fn = tempDir + "imf_extra_chlist.exr";

    {
        Header hdr (64, 2);
        hdr.dataWindow () = Box2i (V2i (0, 2), V2i (63, 3));
        hdr.channels ().insert ("R", Channel (HALF));

        ChannelList extra;
        extra.insert ("Q", Channel (FLOAT, 1, 3));
        hdr.insert ("zz", ChannelListAttribute (extra));

        vector<half> pix (64 * 2, half (0.f));
        FrameBuffer  fb;
        fb.insert (
            "R",
            Slice (
                HALF,
                (char*) (&pix[0] - 2 * 64),
                sizeof (half),
                sizeof (half) * 64));

        OutputFile out (fn.c_str (), hdr);
        out.setFrameBuffer (fb);
        out.writePixels (2);
    }

    {
        InputFile in (fn.c_str ());
        const Header& h = in.header ();

        assert (h.channels ().findChannel ("R"));
        assert (!h.channels ().findChannel ("Q"));

        const ChannelListAttribute* zz =
            h.findTypedAttribute<ChannelListAttribute> ("zz");
        assert (zz);
        assert (zz->value ().findChannel ("Q"));
    }

    remove (fn.c_str ());
}

void
testHeader (const string& tempDir)
{
    try
    {
        testExtraChannelListAttribute (tempDir);
        {
            Test<Header> headerTest;
            headerTest.testFind ("displayWindow");
            headerTest.testSubscript ("displayWindow");
            headerTest.testIterators ("displayWindow");
        }
        {
            Test<const Header> headerTest;
            headerTest.testFind ("displayWindow");
            headerTest.testSubscript ("displayWindow");
            headerTest.testIterators ("displayWindow");
        }
        testEraseAttribute ("displayWindow");
        testEraseAttributeThrowsWithEmptyString ();
        cout << "ok\n" << endl;
    }
    catch (const exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
