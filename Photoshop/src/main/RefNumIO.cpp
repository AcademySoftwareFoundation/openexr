// ===========================================================================
//	RefNumIO.cpp								Part of OpenEXR
// ===========================================================================

#include "RefNumIO.h"

#include <IexBaseExc.h>


// ===========================================================================
//	Macintosh Implementation
// ===========================================================================

#if Macintosh

//-------------------------------------------------------------------------------
// IStream Constructor
//-------------------------------------------------------------------------------

RefNumIFStream::RefNumIFStream 
(
	short 				refNum, 
	const  FSSpec*		fsSpec,
	const char 			fileName[]
) : 
	IStream 			(fileName),
	_refNum 			(refNum),
	_closeWhenFinished 	(false)
{ 
	// file isn't necessarily opened
	
	long eof = 0;
	
	if (GetEOF (_refNum, &eof) != noErr)
	{
		OSErr err = FSpOpenDF (fsSpec, fsRdPerm, &_refNum);
		
		if (err != noErr)
		{
			throw Iex::InputExc ("Unable to open file.");
		}
		
		_closeWhenFinished = true;
	}
}


//-------------------------------------------------------------------------------
// IStream Destructor
//-------------------------------------------------------------------------------

RefNumIFStream::~RefNumIFStream ()
{
	if (_closeWhenFinished)
	{
		FSClose (_refNum);
	}
}


//-------------------------------------------------------------------------------
// read
//-------------------------------------------------------------------------------

bool	
RefNumIFStream::read (char c[/*n*/], int n)
{
	long 	count 	= n;
	OSErr 	err		= noErr;
	long	fpos	= 0;
	long	eof		= 0;
	
	err = FSRead (_refNum, &count, c);
	
	if (err != noErr || count != n)
	{
		throw Iex::InputExc ("Unexpected end of file.");
	}
	
	if (GetFPos (_refNum, &fpos) != noErr || GetEOF  (_refNum, &eof) != noErr)
	{
		throw Iex::InputExc ("Error finding file positon.");
	}
		
	return !(fpos == eof);
}


//-------------------------------------------------------------------------------
// tellg
//-------------------------------------------------------------------------------

Imf::Int64	
RefNumIFStream::tellg ()
{
	long 	fpos 	= 	0;
	OSErr	err		=	noErr;
	
	err = GetFPos (_refNum, &fpos);
	
	if (err != noErr)
	{
		throw Iex::InputExc ("Error finding file positon.");
	}
	
	return fpos;
}


//-------------------------------------------------------------------------------
// seekg
//-------------------------------------------------------------------------------

void	
RefNumIFStream::seekg (Imf::Int64 pos)
{
	OSErr	err		=	noErr;
	
	err = SetFPos (_refNum, fsFromStart, pos);
	
	if (err != noErr)
	{
		throw Iex::InputExc ("Error setting file positon.");
	}
}


//-------------------------------------------------------------------------------
// clear
//-------------------------------------------------------------------------------

void	
RefNumIFStream::clear ()
{
	// nothing to do
}

#pragma mark-

//-------------------------------------------------------------------------------
// OStream Constructor
//-------------------------------------------------------------------------------

RefNumOFStream::RefNumOFStream 
(
	short 				refNum, 
	const  FSSpec*		fsSpec,
	const char 			fileName[]
) : 
	OStream 			(fileName),
	_refNum 			(refNum),
	_closeWhenFinished 	(false)
{ 
	// file isn't necessarily opened
	
	long eof = 0;
	
	if (GetEOF (_refNum, &eof) != noErr)
	{
		OSErr err = FSpOpenDF (fsSpec, fsWrPerm, &_refNum);
		
		if (err != noErr)
		{
			throw Iex::IoExc ("Unable to open file.");
		}
		
		_closeWhenFinished = true;
	}
}


//-------------------------------------------------------------------------------
// OStream Destructor
//-------------------------------------------------------------------------------

RefNumOFStream::~RefNumOFStream ()
{
	if (_closeWhenFinished)
	{
		FSClose (_refNum);
	}
}


//-------------------------------------------------------------------------------
// write
//-------------------------------------------------------------------------------

void	
RefNumOFStream::write (const char c[/*n*/], int n)
{
	long 	count 	= n;
	OSErr 	err		= noErr;
	
	err = FSWrite (_refNum, &count, c);
	
	if (err != noErr || count != n)
	{
		throw Iex::IoExc ("Unable to write file.");
	}	
}


//-------------------------------------------------------------------------------
// tellp
//-------------------------------------------------------------------------------

Imf::Int64	
RefNumOFStream::tellp ()
{
	long 	fpos 	= 	0;
	OSErr	err		=	noErr;
	
	err = GetFPos (_refNum, &fpos);
	
	if (err != noErr)
	{
		throw Iex::IoExc ("Error finding file positon.");
	}
	
	return fpos;
}


//-------------------------------------------------------------------------------
// seekp
//-------------------------------------------------------------------------------

void	
RefNumOFStream::seekp (Imf::Int64 pos)
{
	OSErr	err		=	noErr;
	
	err = SetFPos (_refNum, fsFromStart, pos);
	
	if (err != noErr)
	{
		throw Iex::IoExc ("Error setting file positon.");
	}
}

#endif


// ===========================================================================
//	Windows Implementation
// ===========================================================================

#if MSWindows

//-------------------------------------------------------------------------------
// IStream Constructor
//-------------------------------------------------------------------------------

RefNumIFStream::RefNumIFStream 
(
	short 				refNum, 
	const  FSSpec*		fsSpec,
	const char 			fileName[]
) : 
	IStream 			(fileName),
	_refNum 			(refNum),
	_closeWhenFinished 	(false)
{ 
    // file may not be open

    DWORD sizeHi = 0;

    if (GetFileSize ((Handle) _refNum, &sizeHi) == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
    {
        std::string name;
        for (int i = 1; i <= fsSpec->name[0]; ++i)
        {
            name += fsSpec->name[i];
        }

        HANDLE h = CreateFile (name.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (h == INVALID_HANDLE_VALUE)
        {
            throw Iex::IoExc ("Unable to open file.");
        }

        _refNum = (short) h;
        _closeWhenFinished = true;
    }
}


//-------------------------------------------------------------------------------
// IStream Destructor
//-------------------------------------------------------------------------------

RefNumIFStream::~RefNumIFStream ()
{
    if (_closeWhenFinished)
    {
        CloseHandle ((Handle) _refNum);
    }
}


//-------------------------------------------------------------------------------
// read
//-------------------------------------------------------------------------------

bool	
RefNumIFStream::read (char c[/*n*/], int n)
{
    DWORD nRead;
    BOOL  result;

    result = ReadFile ((HANDLE) _refNum, c, n, &nRead, NULL);

    if (!result)
    {
        throw Iex::InputExc ("Error reading file.");
    }

    LARGE_INTEGER li;
    DWORD         hi;

    li.QuadPart = 0;
    hi          = 0;

    li.LowPart  = GetFileSize ((Handle) _refNum, &hi);
    li.HighPart = hi;

    if (li.LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
    {
        throw Iex::InputExc ("Error getting file size.");
    }

    return !(li.QuadPart == tellg());;
}


//-------------------------------------------------------------------------------
// tellg
//-------------------------------------------------------------------------------

Imf::Int64	
RefNumIFStream::tellg ()
{
	LARGE_INTEGER li;

    li.QuadPart = 0;

    li.LowPart = SetFilePointer ((HANDLE) _refNum, 0, &li.HighPart, FILE_CURRENT);

    if (li.HighPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
        throw Iex::InputExc ("Error getting file position.");
    }

    return li.QuadPart;
}


//-------------------------------------------------------------------------------
// seekg
//-------------------------------------------------------------------------------

void	
RefNumIFStream::seekg (Imf::Int64 pos)
{
	LARGE_INTEGER li;

    li.QuadPart = pos;

    SetFilePointer ((HANDLE) _refNum, li.LowPart, &li.HighPart, FILE_BEGIN);

    if (li.HighPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
        throw Iex::InputExc ("Error getting file position.");
    }
}


//-------------------------------------------------------------------------------
// clear
//-------------------------------------------------------------------------------

void	
RefNumIFStream::clear ()
{
	// nothing to do
}


//-------------------------------------------------------------------------------
// OStream Constructor
//-------------------------------------------------------------------------------

RefNumOFStream::RefNumOFStream 
(
	short 				refNum, 
	const  FSSpec*		fsSpec,
	const char 			fileName[]
) : 
	OStream 			(fileName),
	_refNum 			(refNum),
	_closeWhenFinished 	(false)
{ 
    // file may not be open

    DWORD sizeHi = 0;

    if (GetFileSize ((Handle) _refNum, &sizeHi) == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
    {
        std::string name;
        for (int i = 1; i <= fsSpec->name[0]; ++i)
        {
            name += fsSpec->name[i];
        }

        HANDLE h = CreateFile (name.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (h == INVALID_HANDLE_VALUE)
        {
            throw Iex::IoExc ("Unable to open file.");
        }

        _refNum = (short) h;
        _closeWhenFinished = true;
    }
}


//-------------------------------------------------------------------------------
// OStream Destructor
//-------------------------------------------------------------------------------

RefNumOFStream::~RefNumOFStream ()
{
    if (_closeWhenFinished)
    {
        CloseHandle ((Handle) _refNum);
    }
}


//-------------------------------------------------------------------------------
// write
//-------------------------------------------------------------------------------

void	
RefNumOFStream::write (const char c[/*n*/], int n)
{
    DWORD nRead;
    BOOL  result;

    result = WriteFile ((HANDLE) _refNum, c, n, &nRead, NULL);

    if (!result)
    {
        throw Iex::InputExc ("Error reading file.");
    }
}


//-------------------------------------------------------------------------------
// tellp
//-------------------------------------------------------------------------------

Imf::Int64	
RefNumOFStream::tellp ()
{
	LARGE_INTEGER li;

    li.QuadPart = 0;

    li.LowPart = SetFilePointer ((HANDLE) _refNum, 0, &li.HighPart, FILE_CURRENT);

    if (li.HighPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
        throw Iex::IoExc ("Error getting file position.");
    }

    return li.QuadPart;
}


//-------------------------------------------------------------------------------
// seekp
//-------------------------------------------------------------------------------

void	
RefNumOFStream::seekp (Imf::Int64 pos)
{
	LARGE_INTEGER li;

    li.QuadPart = pos;

    SetFilePointer ((HANDLE) _refNum, li.LowPart, &li.HighPart, FILE_BEGIN);

    if (li.HighPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
        throw Iex::IoExc ("Error getting file position.");
    }
}

#endif
