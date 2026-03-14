//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	Low-level file input and output for OpenEXR
//	based on C++ standard iostreams.
//
//-----------------------------------------------------------------------------

#include "Iex.h"
#include <ImfMisc.h>
#include <ImfStdIO.h>
#include <errno.h>
#include <filesystem>
#if __cplusplus >= 202002L
#    include <ranges>
#    include <span>
#endif

using namespace std;
#include "ImfNamespace.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

namespace
{

inline ifstream*
make_ifstream (const char* filename)
{
#if __cplusplus >= 202002L
    auto u8view = ranges::views::transform (span{filename, strlen(filename)},
                                            [](char c) -> char8_t { return c; });
    return new ifstream (filesystem::path (u8view.begin (), u8view.end ()),
                         ios_base::in | ios_base::binary);
#else
    return new ifstream (filesystem::u8path (filename),
                         ios_base::in | ios_base::binary);
#endif
}

inline ofstream*
make_ofstream (const char* filename)
{
#if __cplusplus >= 202002L
    auto u8view = ranges::views::transform (span{filename, strlen(filename)},
                                            [](char c) -> char8_t { return c; });
    return new ofstream (filesystem::path (u8view.begin (), u8view.end ()),
                         ios_base::out | ios_base::binary);
#else
    return new ofstream (filesystem::u8path (filename),
                         ios_base::out | ios_base::binary);
#endif
}

void
clearError ()
{
    errno = 0;
}

bool
checkError (istream& is, streamsize expected = 0)
{
    if (!is)
    {
        if (errno) IEX_NAMESPACE::throwErrnoExc ();

        if (is.gcount () < expected)
        {
            THROW (
                IEX_NAMESPACE::InputExc,
                "Early end of file: read " << is.gcount () << " out of "
                                           << expected << " requested bytes.");
        }
        return false;
    }

    return true;
}

void
checkError (ostream& os)
{
    if (!os)
    {
        if (errno) IEX_NAMESPACE::throwErrnoExc ();

        throw IEX_NAMESPACE::ErrnoExc ("File output failed.");
    }
}

} // namespace

StdIFStream::StdIFStream (const char fileName[])
    : OPENEXR_IMF_INTERNAL_NAMESPACE::IStream (fileName)
    , _is (make_ifstream (fileName))
    , _deleteStream (true)
{
    if (!*_is)
    {
        delete _is;
        IEX_NAMESPACE::throwErrnoExc ();
    }
}

StdIFStream::StdIFStream (ifstream& is, const char fileName[])
    : OPENEXR_IMF_INTERNAL_NAMESPACE::IStream (fileName)
    , _is (&is)
    , _deleteStream (false)
{
    // empty
}

StdIFStream::~StdIFStream ()
{
    if (_deleteStream) delete _is;
}

bool
StdIFStream::read (char c[/*n*/], int n)
{
    if (!*_is) throw IEX_NAMESPACE::InputExc ("Unexpected end of file.");

    clearError ();
    _is->read (c, n);
    return checkError (*_is, n);
}

uint64_t
StdIFStream::tellg ()
{
    return std::streamoff (_is->tellg ());
}

void
StdIFStream::seekg (uint64_t pos)
{
    _is->seekg (pos);
    checkError (*_is);
}

void
StdIFStream::clear ()
{
    _is->clear ();
}

StdISStream::StdISStream ()
    : OPENEXR_IMF_INTERNAL_NAMESPACE::IStream ("(string)")
{
    // empty
}

StdISStream::~StdISStream ()
{}

bool
StdISStream::read (char c[/*n*/], int n)
{
    if (!_is) throw IEX_NAMESPACE::InputExc ("Unexpected end of file.");

    clearError ();
    _is.read (c, n);
    return checkError (_is, n);
}

uint64_t
StdISStream::tellg ()
{
    return std::streamoff (_is.tellg ());
}

void
StdISStream::seekg (uint64_t pos)
{
    _is.seekg (pos);
    checkError (_is);
}

void
StdISStream::clear ()
{
    _is.clear ();
}

std::string
StdISStream::str () const
{
    return _is.str ();
}

void
StdISStream::str (const std::string& s)
{
    _is.str (s);
}

StdOFStream::StdOFStream (const char fileName[])
    : OPENEXR_IMF_INTERNAL_NAMESPACE::OStream (fileName)
    , _os (make_ofstream (fileName))
    , _deleteStream (true)
{
    if (!*_os)
    {
        delete _os;
        IEX_NAMESPACE::throwErrnoExc ();
    }
}

StdOFStream::StdOFStream (ofstream& os, const char fileName[])
    : OPENEXR_IMF_INTERNAL_NAMESPACE::OStream (fileName)
    , _os (&os)
    , _deleteStream (false)
{
    // empty
}

StdOFStream::~StdOFStream ()
{
    if (_deleteStream) delete _os;
}

void
StdOFStream::write (const char c[/*n*/], int n)
{
    clearError ();
    _os->write (c, n);
    checkError (*_os);
}

uint64_t
StdOFStream::tellp ()
{
    return std::streamoff (_os->tellp ());
}

void
StdOFStream::seekp (uint64_t pos)
{
    _os->seekp (pos);
    checkError (*_os);
}

StdOSStream::StdOSStream ()
    : OPENEXR_IMF_INTERNAL_NAMESPACE::OStream ("(string)")
{
    // empty
}

StdOSStream::~StdOSStream ()
{}

void
StdOSStream::write (const char c[/*n*/], int n)
{
    clearError ();
    _os.write (c, n);
    checkError (_os);
}

uint64_t
StdOSStream::tellp ()
{
    return std::streamoff (_os.tellp ());
}

void
StdOSStream::seekp (uint64_t pos)
{
    _os.seekp (pos);
    checkError (_os);
}

std::string
StdOSStream::str () const
{
    return _os.str ();
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
