//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_STD_IO_H
#define INCLUDED_IMF_STD_IO_H

//-----------------------------------------------------------------------------
//
//	Low-level file input and output for OpenEXR
//	based on C++ standard iostreams.
//
//-----------------------------------------------------------------------------

#include "ImfIO.h"
#include "ImfNamespace.h"
#include "ImfExport.h"

#include <fstream>
#include <sstream>


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

//-------------------------------------------
// class StdIFStream -- an implementation of
// class OPENEXR_IMF_INTERNAL_NAMESPACE::IStream based on class std::ifstream
//-------------------------------------------

class IMF_EXPORT StdIFStream: public OPENEXR_IMF_INTERNAL_NAMESPACE::IStream
{
  public:

    //-------------------------------------------------------
    // A constructor that opens the file with the given name.
    // The destructor will close the file.
    //-------------------------------------------------------

    StdIFStream (const char fileName[]);

    
    //---------------------------------------------------------
    // A constructor that uses a std::ifstream that has already
    // been opened by the caller.  The StdIFStream's destructor
    // will not close the std::ifstream.
    //---------------------------------------------------------

    StdIFStream (std::ifstream &is, const char fileName[]);


    virtual ~StdIFStream ();

    virtual bool	read (char c[/*n*/], int n);
    IMF_EXPORT
    virtual uint64_t	tellg ();
    IMF_EXPORT
    virtual void	seekg (uint64_t pos);
    IMF_EXPORT
    virtual void	clear ();

  private:

    std::ifstream *	_is;
    bool		_deleteStream;
};


//------------------------------------------------
// class StdISStream -- an implementation of class
// OPENEXR_IMF_INTERNAL_NAMESPACE::IStream, based on class std::istringstream
//------------------------------------------------

class IMF_EXPORT StdISStream: public OPENEXR_IMF_INTERNAL_NAMESPACE::IStream
{
  public:

    StdISStream ();

    virtual bool	read (char c[/*n*/], int n);
    IMF_EXPORT
    virtual uint64_t	tellg ();
    IMF_EXPORT
    virtual void	seekg (uint64_t pos);
    IMF_EXPORT
    virtual void	clear ();

    std::string		str () const; 

    void                str (const std::string &s);

  private:

    std::istringstream 	_is;
};



//-------------------------------------------
// class StdOFStream -- an implementation of
// class OPENEXR_IMF_INTERNAL_NAMESPACE::OStream based on class std::ofstream
//-------------------------------------------

class IMF_EXPORT StdOFStream: public OPENEXR_IMF_INTERNAL_NAMESPACE::OStream
{
  public:

    //-------------------------------------------------------
    // A constructor that opens the file with the given name.
    // The destructor will close the file.
    //-------------------------------------------------------

    StdOFStream (const char fileName[]);
    

    //---------------------------------------------------------
    // A constructor that uses a std::ofstream that has already
    // been opened by the caller.  The StdOFStream's destructor
    // will not close the std::ofstream.
    //---------------------------------------------------------

    StdOFStream (std::ofstream &os, const char fileName[]);


    virtual ~StdOFStream ();

    virtual void	write (const char c[/*n*/], int n);
    IMF_EXPORT
    virtual uint64_t	tellp ();
    IMF_EXPORT
    virtual void	seekp (uint64_t pos);

  private:

    std::ofstream *	_os;
    bool		_deleteStream;
};


//------------------------------------------------
// class StdOSStream -- an implementation of class
// OPENEXR_IMF_INTERNAL_NAMESPACE::OStream, based on class std::ostringstream
//------------------------------------------------

class IMF_EXPORT StdOSStream: public OPENEXR_IMF_INTERNAL_NAMESPACE::OStream
{
  public:

    StdOSStream ();

    virtual void	write (const char c[/*n*/], int n);
    IMF_EXPORT
    virtual uint64_t	tellp ();
    IMF_EXPORT
    virtual void	seekp (uint64_t pos);

    std::string		str () const;

  private:

    std::ostringstream 	_os;
};


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
