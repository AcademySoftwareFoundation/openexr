//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <ImfBoxAttribute.h>
#include <ImfHeader.h>

#include <exception>
#include <iostream>
#include <string>

#include <assert.h>

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

void
testHeader (const string& tempDir)
{
    try
    {
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
