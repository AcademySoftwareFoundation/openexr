// ===========================================================================
//	RefNumIO.cpp								Part of OpenEXR
// ===========================================================================

#include "RefNumIO.h"

#include <IexBaseExc.h>


// ===========================================================================
//	Macintosh IO Abstraction 
//
//  use 64-bit HFS+ APIs if the system supports them,
//	fall back to 32-bit classic File Manager APIs otherwise
// ===========================================================================

#pragma mark ===== Macintosh =====

#if Macintosh

#include <Gestalt.h>
#include <Files.h>


//-------------------------------------------------------------------------------
// HaveHFSPlusAPIs
//-------------------------------------------------------------------------------

static bool HaveHFSPlusAPIs ()
{
	static bool sCheckedForHFSPlusAPIs 	= 	false;
	static bool sHaveHFSPlusAPIs		=	false;

	if (!sCheckedForHFSPlusAPIs)
	{
		long 	response 	=	0;
		OSErr	err			=	noErr;
	
		err = Gestalt (gestaltFSAttr, &response);
		
		if (err == noErr && (response & (1 << gestaltHasHFSPlusAPIs)))
		{
			sHaveHFSPlusAPIs = true;	
		}
	
		sCheckedForHFSPlusAPIs = true;
	}

	return sHaveHFSPlusAPIs;
}


//-------------------------------------------------------------------------------
// Open (for writing)
//-------------------------------------------------------------------------------

static bool Open (const FSSpec* spec, short& refNum)
{
	OSErr err = noErr;
	
	if (HaveHFSPlusAPIs())
	{
		FSRef ref = { 0 };
		
		if (!err) err = FSpMakeFSRef (spec, &ref);
		if (!err) err = FSOpenFork (&ref, 0, NULL, fsWrPerm, &refNum);
	}
	else
	{
		err = FSpOpenDF (spec, fsWrPerm, &refNum);
	}
	
	return (err == noErr);
}


//-------------------------------------------------------------------------------
// Close
//-------------------------------------------------------------------------------

static bool Close (short refNum)
{
	OSErr err = noErr;
	
	if (HaveHFSPlusAPIs())
	{
		err = FSCloseFork (refNum);
	}
	else
	{
		err = FSClose (refNum);
	}
	
	return (err == noErr);
}


//-------------------------------------------------------------------------------
// Read
//-------------------------------------------------------------------------------

static bool Read (short refNum, int n, void* c)
{
	OSErr err = noErr;
	
	if (HaveHFSPlusAPIs())
	{
		ByteCount 	actual;
		
		err = FSReadFork (refNum, fsFromMark, 0, n, c, &actual);
	}
	else
	{
		long count = n;
		
		err = FSRead (refNum, &count, c);
	}
	
	return (err == noErr);
}


//-------------------------------------------------------------------------------
// Write
//-------------------------------------------------------------------------------

static bool Write (short refNum, int n, const void* c)
{
	OSErr err = noErr;
	
	if (HaveHFSPlusAPIs())
	{
		ByteCount 	actual;
		
		err = FSWriteFork (refNum, fsFromMark, 0, n, c, &actual);
	}
	else
	{
		long count = n;
		
		err = FSWrite (refNum, &count, c);
	}
	
	return (err == noErr);
}


//-------------------------------------------------------------------------------
// Tell
//-------------------------------------------------------------------------------

static bool Tell (short refNum, Imf::Int64& pos)
{
	OSErr err = noErr;
	
	if (HaveHFSPlusAPIs())
	{
		SInt64 p;
		
		err = FSGetForkPosition (refNum, &p);
		pos = p;
	}
	else
	{
		long p;
		
		err = GetFPos (refNum, &p);
		pos = p;
	}
	
	return (err == noErr);
}


//-------------------------------------------------------------------------------
// Seek
//-------------------------------------------------------------------------------

static bool Seek (short refNum, const Imf::Int64& pos)
{
	OSErr err = noErr;
	
	if (HaveHFSPlusAPIs())
	{
		err = FSSetForkPosition (refNum, fsFromStart, pos);
	}
	else
	{
		err = SetFPos (refNum, fsFromStart, pos);
	}
	
	return (err == noErr);
}


//-------------------------------------------------------------------------------
// GetSize
//-------------------------------------------------------------------------------

static bool GetSize (short refNum, Imf::Int64& size)
{
	OSErr err = noErr;
	
	if (HaveHFSPlusAPIs())
	{
		SInt64 fileSize;
		
		err  = FSGetForkSize (refNum, &fileSize);
		size = fileSize;
	}
	else
	{
		long fileSize;
		
		err  = GetEOF (refNum, &fileSize);
		size = fileSize;
	}
	
	return (err == noErr);
}

#endif

#pragma mark-
#pragma mark ===== Windows =====

// ===========================================================================
//	Windows IO Abstraction 
// ===========================================================================

#if MSWindows

//-------------------------------------------------------------------------------
// Open (for writing)
//-------------------------------------------------------------------------------

static bool Open (const FSSpec* spec, short& refNum)
{
    std::string name;
    for (int i = 1; i <= spec->name[0]; ++i)
    {
        name += spec->name[i];
    }

    HANDLE h = CreateFile (name.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
    refNum = (short) h;

    return !(h == INVALID_HANDLE_VALUE);
}


//-------------------------------------------------------------------------------
// Close
//-------------------------------------------------------------------------------

static bool Close (short refNum)
{
	CloseHandle ((Handle) refNum);
    return true;
}


//-------------------------------------------------------------------------------
// Read
//-------------------------------------------------------------------------------

static bool Read (short refNum, int n, void* c)
{
	DWORD nRead;
 
    return ReadFile ((HANDLE) refNum, c, n, &nRead, NULL);
}


//-------------------------------------------------------------------------------
// Write
//-------------------------------------------------------------------------------

static bool Write (short refNum, int n, const void* c)
{
	DWORD nRead;
 
    return WriteFile ((HANDLE) refNum, c, n, &nRead, NULL);
}


//-------------------------------------------------------------------------------
// Tell
//-------------------------------------------------------------------------------

static bool Tell (short refNum, Imf::Int64& pos)
{
	LARGE_INTEGER li;

    li.QuadPart = 0;

    li.LowPart = SetFilePointer ((HANDLE) refNum, 0, &li.HighPart, FILE_CURRENT);

    if (li.HighPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
        return false;
    }

    pos = li.QuadPart;
    return true;
}


//-------------------------------------------------------------------------------
// Seek
//-------------------------------------------------------------------------------

static bool Seek (short refNum, const Imf::Int64& pos)
{
	LARGE_INTEGER li;

    li.QuadPart = pos;

    SetFilePointer ((HANDLE) refNum, li.LowPart, &li.HighPart, FILE_BEGIN);

    return !(li.HighPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR);
}


//-------------------------------------------------------------------------------
// GetSize
//-------------------------------------------------------------------------------

static bool GetSize (short refNum, Imf::Int64& size)
{
    LARGE_INTEGER li;
    DWORD         hi;

    li.QuadPart = 0;
    li.LowPart  = GetFileSize ((HANDLE) refNum, &hi);
    li.HighPart = hi;
    size        = li.QuadPart;

    return !(li.LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR);
}

#endif

#pragma mark-

//-------------------------------------------------------------------------------
// IStream Constructor
//-------------------------------------------------------------------------------

RefNumIFStream::RefNumIFStream 
(
	short 				refNum,
	const char 			fileName[]
) : 
	IStream 			(fileName),
	_refNum 			(refNum)
{ 

}


//-------------------------------------------------------------------------------
// IStream Destructor
//-------------------------------------------------------------------------------

RefNumIFStream::~RefNumIFStream ()
{

}


//-------------------------------------------------------------------------------
// read
//-------------------------------------------------------------------------------

bool	
RefNumIFStream::read (char c[/*n*/], int n)
{
	if (!Read (_refNum, n, c))
	{
		throw Iex::InputExc ("Unable to read file.");
	}
	
	Imf::Int64 fileSize;
	
	if (!GetSize (_refNum, fileSize))
	{
		throw Iex::InputExc ("Couldn't get file size.");	
	}
	
	return !(fileSize == tellg());
}


//-------------------------------------------------------------------------------
// tellg
//-------------------------------------------------------------------------------

Imf::Int64	
RefNumIFStream::tellg ()
{
	Imf::Int64 	fpos 	= 	0;

	if (!Tell (_refNum, fpos))
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
	if (!Seek (_refNum, pos))
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
	
	Imf::Int64 size = 0;
	
	if (!GetSize (_refNum, size))
	{
		if (!Open (fsSpec, _refNum))
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
		Close (_refNum); // ignore error - don't want to throw in a destructor
	}
}


//-------------------------------------------------------------------------------
// write
//-------------------------------------------------------------------------------

void	
RefNumOFStream::write (const char c[/*n*/], int n)
{
	if (!Write (_refNum, n, c))
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
	Imf::Int64 	fpos 	= 	0;

	if (!Tell (_refNum, fpos))
	{
		throw Iex::InputExc ("Error finding file positon.");
	}
	
	return fpos;
}


//-------------------------------------------------------------------------------
// seekp
//-------------------------------------------------------------------------------

void	
RefNumOFStream::seekp (Imf::Int64 pos)
{
	if (!Seek (_refNum, pos))
	{
		throw Iex::InputExc ("Error setting file positon.");
	}
}
