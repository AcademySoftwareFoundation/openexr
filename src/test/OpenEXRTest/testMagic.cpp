//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <ImfStdIO.h>
#include <ImfTestFile.h>
#include <ImfVersion.h>
#include <assert.h>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <string>

#if (defined(__cplusplus) && __cplusplus >= 202002L) \
    || (defined(_MSVC_LANG) && _MSVC_LANG >= 202004L)
#    include <string_view>
#endif

#include "TestUtilFStream.h"

#ifndef ILM_IMF_TEST_IMAGEDIR
#    define ILM_IMF_TEST_IMAGEDIR
#endif

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;

namespace
{

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

    cout << "is " << (isImfMagic (bytes) ? "" : "not ") << "an OpenEXR file\n";
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

        if (exr) assert (is.tellg () == 0);

        exr = isOpenExrFile (is);
        assert (exr == exrFile);

        if (exr) assert (is.tellg () == 0);

        tiled = isTiledOpenExrFile (is);
        assert (tiled == tiledFile);

        if (tiled) assert (is.tellg () == 0);
    }

    cout << (exists ? "exists" : "does not exist") << ", "
         << (exrFile ? "is an OpenEXR file" : "is not an OpenEXR file") << ", "
         << (tiledFile ? "is tiled" : "is not tiled") << endl;
}

void
testUtf8Filename (const std::string& tempDir)
{
    namespace fs = std::filesystem;

    // Test isOpenExrFile on an image with a UTF-8 filename (copy a known-good
    // .exr under a name containing non-ASCII UTF-8 bytes).
    //
    // isOpenExrFile() takes a const char* argument that is expected to point
    // to a null-terminated sequence of potentially non-ASCII UTF-8 bytes.
    // We build a std::filesystem::path from a UTF-8 file name, join it with
    // tempDir, copy a known-good .exr to that destination, then call
    // isOpenExrFile() with the full UTF-8 path obtained from that path
    // object (see below).
    //
    // This is complicated because of how std::filesystem and UTF-8 interact
    // across C++17 and C++20, and because isOpenExrFile takes const char*,
    // not char8_t*.
    //
    // - Building fs::path from a UTF-8 std::string: In C++20, treating a
    //   char buffer as UTF-8 for path construction is spelled with char8_t
    //   (std::u8string_view). A reinterpret_cast from const char* to const
    //   char8_t* is the standard way to attach that meaning to the same byte
    //   sequence without transcoding. Pre-C++20 we use the narrow-string
    //   constructor directly.
    //
    // - Getting a string for isOpenExrFile: We ask the path for its UTF-8
    //   form via u8string() so we match the actual on-disk name the library
    //   will open. In C++20, u8string() returns std::u8string (char8_t), but
    //   OpenEXR's API still expects a NUL-terminated const char* UTF-8 path,
    //   so we copy those bytes into a std::string (again via reinterpret_cast
    //   of the data pointer) and pass c_str(). In C++17, u8string() already
    //   returns std::string, so no second cast is needed there.
    
    const std::string utf8Name (
        "openexr_isOpenExrFile_\xc3\xa9\xe6\x97\xa5_\xd1\x84.exr");
    const fs::path src = fs::path (ILM_IMF_TEST_IMAGEDIR) / "comp_none.exr";
#if (defined(__cplusplus) && __cplusplus >= 202002L) \
    || (defined(_MSVC_LANG) && _MSVC_LANG >= 202004L)
    // C++20+: interpret UTF-8 bytes as char8_t.
    fs::path utf8Path (std::u8string_view (reinterpret_cast<const char8_t*> (utf8Name.data ()),
                                           utf8Name.size ()));
#else
    fs::path utf8Path (utf8Name);
#endif
    const fs::path dst = fs::path (tempDir) / utf8Path;

    std::error_code ec;
    fs::copy_file (src, dst, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        cout << "skipping UTF-8 path test (could not copy " << src << " to " << dst << "): "
             << ec.message () << endl;
        assert(false);
    }

    // remove the temp copy on exit
    struct Cleanup
    {
        fs::path p;
        ~Cleanup ()
        {
            std::error_code e;
            std::filesystem::remove (p, e);
        }
    } cleanup{dst};

    const auto u = dst.u8string ();
    const std::string dstUtf8 (reinterpret_cast<const char*> (u.data ()), u.size ());

    bool tiled = false, deep = false, multiPart = false;

    bool ok = isOpenExrFile (dstUtf8.c_str (), tiled, deep, multiPart);
    assert (ok);
    assert (!tiled && !deep && !multiPart);

    assert (isOpenExrFile (dstUtf8.c_str ()));

    cout << "UTF-8 filename isOpenExrFile ok\n";
}

} // namespace

void
testMagic (const std::string& tempDir)
{
    try
    {
        cout << "Testing magic number" << endl;

        testFile1 (ILM_IMF_TEST_IMAGEDIR "comp_none.exr", true);
        testFile1 (ILM_IMF_TEST_IMAGEDIR "invalid.exr", false);

        testFile2 (ILM_IMF_TEST_IMAGEDIR "tiled.exr", true, true, true);
        testFile2 (ILM_IMF_TEST_IMAGEDIR "comp_none.exr", true, true, false);
        testFile2 (ILM_IMF_TEST_IMAGEDIR "invalid.exr", true, false, false);
        testFile2 (
            ILM_IMF_TEST_IMAGEDIR "does_not_exist.exr", false, false, false);

        testUtf8Filename (tempDir);

        cout << "ok\n" << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
