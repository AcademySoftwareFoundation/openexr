
// ===========================================================================
//	FSSpecToFullPath.cpp           			Part of OpenEXR
// ===========================================================================
//
//  The EXR library wants to open input and output files using
//  the standard C++ library, which takes full paths to files.
//  Since Photoshop only gives us FSSpecs, we need platform-specific
//  code to convert these to full file paths.
//

#include "FSSpecToFullPath.h"

#include "PITypes.h"

#if Macintosh

//-------------------------------------------------------------------------------
//  Begin MoreFiles code
//-------------------------------------------------------------------------------
//  This code is descended from Apple Sample Code, and originally
//  appeared in MoreFiles.  This code has been modified to break
//  dependancies on other parts of MoreFiles.

/*
**	Apple Macintosh Developer Technical Support
**
**	Routines for dealing with full pathnames... if you really must.
**
**	by Jim Luther, Apple Developer Technical Support Emeritus
**
**	File:		FullPath.c
**
**	Copyright © 1995-1999 Apple Computer, Inc.
**	All rights reserved.
**
**	You may incorporate this sample code into your applications without
**	restriction, though the sample code has been provided "AS IS" and the
**	responsibility for its operation is 100% yours.  However, what you are
**	not permitted to do is to redistribute the source as "DSC Sample Code"
**	after having made changes. If you're going to re-distribute the source,
**	we require that you make it clear in the source that the code was
**	descended from Apple Sample Code, but that you've made changes.
*/


static	OSErr	FSpGetFullPath(const FSSpec *spec,
							   short *fullPathLength,
							   Handle *fullPath)
{
	OSErr		result;
	OSErr		realResult;
	FSSpec		tempSpec;
	CInfoPBRec	pb;
	
	*fullPathLength = 0;
	*fullPath = NULL;
	
	
	/* Default to noErr */
	realResult = result = noErr;
	
	/* work around Nav Services "bug" (it returns invalid FSSpecs with empty names) */
	if ( spec->name[0] == 0 )
	{
		result = FSMakeFSSpec(spec->vRefNum, spec->parID, spec->name, &tempSpec);
	}
	else
	{
		/* Make a copy of the input FSSpec that can be modified */
		BlockMoveData(spec, &tempSpec, sizeof(FSSpec));
	}
	
	if ( result == noErr )
	{
		if ( tempSpec.parID == fsRtParID )
		{
			/* The object is a volume */
			
			/* Add a colon to make it a full pathname */
			++tempSpec.name[0];
			tempSpec.name[tempSpec.name[0]] = ':';
			
			/* We're done */
			result = PtrToHand(&tempSpec.name[1], fullPath, tempSpec.name[0]);
		}
		else
		{
			/* The object isn't a volume */
			
			/* Is the object a file or a directory? */
			pb.dirInfo.ioNamePtr = tempSpec.name;
			pb.dirInfo.ioVRefNum = tempSpec.vRefNum;
			pb.dirInfo.ioDrDirID = tempSpec.parID;
			pb.dirInfo.ioFDirIndex = 0;
			result = PBGetCatInfoSync(&pb);
			// Allow file/directory name at end of path to not exist.
			realResult = result;
			if ( (result == noErr) || (result == fnfErr) )
			{
				/* if the object is a directory, append a colon so full pathname ends with colon */
				if ( (result == noErr) && (pb.hFileInfo.ioFlAttrib & kioFlAttribDirMask) != 0 )
				{
					++tempSpec.name[0];
					tempSpec.name[tempSpec.name[0]] = ':';
				}
				
				/* Put the object name in first */
				result = PtrToHand(&tempSpec.name[1], fullPath, tempSpec.name[0]);
				if ( result == noErr )
				{
					/* Get the ancestor directory names */
					pb.dirInfo.ioNamePtr = tempSpec.name;
					pb.dirInfo.ioVRefNum = tempSpec.vRefNum;
					pb.dirInfo.ioDrParID = tempSpec.parID;
					do	/* loop until we have an error or find the root directory */
					{
						pb.dirInfo.ioFDirIndex = -1;
						pb.dirInfo.ioDrDirID = pb.dirInfo.ioDrParID;
						result = PBGetCatInfoSync(&pb);
						if ( result == noErr )
						{
							/* Append colon to directory name */
							++tempSpec.name[0];
							tempSpec.name[tempSpec.name[0]] = ':';
							
							/* Add directory name to beginning of fullPath */
							(void) Munger(*fullPath, 0, NULL, 0, &tempSpec.name[1], tempSpec.name[0]);
							result = MemError();
						}
					} while ( (result == noErr) && (pb.dirInfo.ioDrDirID != fsRtDirID) );
				}
			}
		}
	}
	
	if ( result == noErr )
	{
		/* Return the length */
		*fullPathLength = GetHandleSize(*fullPath);
		result = realResult;	// return realResult in case it was fnfErr
	}
	else
	{
		/* Dispose of the handle and return NULL and zero length */
		if ( *fullPath != NULL )
		{
			DisposeHandle(*fullPath);
		}
		*fullPath = NULL;
		*fullPathLength = 0;
	}
	
	return ( result );
}

#endif // Macintosh

//-------------------------------------------------------------------------------
//  End MoreFiles code
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
//	FSSpecToFullPath
//-------------------------------------------------------------------------------
//  convert an FSSpec to a full pathname suitable for
//  passing to the EXRlib.  

OSErr FSSpecToFullPath (const FSSpec* inFSSpec, std::string& outPath)
{
#if Macintosh

	OSErr			err;	
	Handle			fullPath;
	short			fullPathLen;
	
	
	// get full path to FSSpec
	
	err = FSpGetFullPath (inFSSpec, &fullPathLen, &fullPath);
	
	if (err != noErr || fullPath == NULL)
	{
		return (err == noErr) ? memFullErr : err;
	}
	

	// add NULL character, to convert to C string
	
	SetHandleSize (fullPath, fullPathLen + 1);
	
	HLock (fullPath);
	(*fullPath)[fullPathLen] = '\0';
	
	
	// copy into output string
	
	outPath = *fullPath;
	
	
	// release handle
	
	DisposeHandle (fullPath);
	fullPath = NULL;

#else

    // on Windows, FSSpec contains full path as a pascal string
    // what if path is more than 256 characters?

    outPath = (const char*) (&inFSSpec->name[1]);    

#endif	
	
	return noErr;
}
