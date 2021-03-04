//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <ImfVersion.h>
#include <ImfTestFile.h>
#include <ImfStdIO.h>
#include <iostream>
#include <exception>
#include <stdio.h>
#include <assert.h>

#include "TestUtilFStream.h"

#ifndef ILM_IMF_TEST_IMAGEDIR
    #define ILM_IMF_TEST_IMAGEDIR
#endif


using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;


namespace {

void
testFile1 (const char fileName[], bool isImfFile)
{
    cout << fileName << " " << flush;

    ifstream f;
    testutil::OpenStreamWithUTF8Name (f, fileName, ios::in | ios_base::binary);
    assert (!!f);

    char bytes[4];
    f.read (bytes, sizeof (bytes));

    assert (!!f && isImfFile == isImfMagic (bytes));

    cout << "is " << (isImfMagic (bytes)? "": "not ") << "an OpenEXR file\n";
}


void
testFile2 (const char fileName[], bool exists, bool exrFile, bool tiledFile)
{
    cout << fileName << " " << flush;

    bool exr, tiled;

    exr = isOpenExrFile (fileName, tiled);
    assert (exr == exrFile && tiled == tiledFile);

    exr = isOpenExrFile (fileName);
    assert (exr == exrFile);

    tiled = isTiledOpenExrFile (fileName);
    assert (tiled == tiledFile);

    if (exists)
    {
	StdIFStream is (fileName);

	exr = isOpenExrFile (is, tiled);
	assert (exr == exrFile && tiled == tiledFile);

	if (exr)
	    assert (is.tellg() == 0);

	exr = isOpenExrFile (is);
	assert (exr == exrFile);

	if (exr)
	    assert (is.tellg() == 0);

	tiled = isTiledOpenExrFile (is);
	assert (tiled == tiledFile);

	if (tiled)
	    assert (is.tellg() == 0);
    }

    cout << (exists? "exists": "does not exist") << ", " <<
	    (exrFile? "is an OpenEXR file": "is not an OpenEXR file") << ", " <<
	    (tiledFile? "is tiled": "is not tiled") << endl;
}

} // namespace


void
testMagic (const std::string &)
{
    try
    {
	cout << "Testing magic number" << endl;

	testFile1 (ILM_IMF_TEST_IMAGEDIR "comp_none.exr", true);
	testFile1 (ILM_IMF_TEST_IMAGEDIR "invalid.exr", false);

	testFile2 (ILM_IMF_TEST_IMAGEDIR "tiled.exr", true, true, true);
	testFile2 (ILM_IMF_TEST_IMAGEDIR "comp_none.exr", true, true, false);
	testFile2 (ILM_IMF_TEST_IMAGEDIR "invalid.exr", true, false, false);
	testFile2 (ILM_IMF_TEST_IMAGEDIR "does_not_exist.exr", false, false, false);

	cout << "ok\n" << endl;
    }
    catch (const std::exception &e)
    {
	cerr << "ERROR -- caught exception: " << e.what() << endl;
	assert (false);
    }
}
