// ===========================================================================
//	EXRExportDialog.cpp           			Part of OpenEXR
// ===========================================================================

#if MSWindows
#pragma warning (disable: 161)
#endif

#include "EXRExportDialog.h"

#include "ADMDialog.h"
#include "ADMItem.h"
#include "ADMList.h"
#include "ADMEntry.h"


// ---------------------------------------------------------------------------
//	Resource ID's
// ---------------------------------------------------------------------------

const int                   kResID_ExportDialog     =	501;

const int                   kItem_OK                =	1;
const int                   kItem_Cancel            =	2;
const int                   kItem_Exposure          =	3;
const int                   kItem_Gamma             =	4;
const int                   kItem_Compression       =	5;
const int                   kItem_Defaults          =   9;
const int                   kItem_Premult           =   12;


// ---------------------------------------------------------------------------
//	Globals - ADM makes it hard to avoid them
// ---------------------------------------------------------------------------

static ADMDialogSuite5*     sDlogSuite              =	NULL;
static ADMItemSuite5*       sItemSuite              =	NULL;
static ADMListSuite3*       sListSuite              =   NULL;
static ADMEntrySuite4*      sEntrySuite             =   NULL;


// ---------------------------------------------------------------------------
//	DoDialogOK - ADM callback
// ---------------------------------------------------------------------------

static void ASAPI DoDialogOK (ADMItemRef inItem, ADMNotifierRef inNotifier)
{
	ADMDialogRef	dialog	=	(ADMDialogRef) sItemSuite->GetUserData (inItem);
	GPtr            globals	= 	(GPtr) sDlogSuite->GetUserData (dialog);
	ADMItemRef		item	=	NULL;
	ADMListRef      list    =   NULL;
	ADMEntryRef     entry   =   NULL;
	
	
	// apply control values to globals
	
	item = sDlogSuite->GetItem (dialog, kItem_Exposure);
	globals->exposure = sItemSuite->GetFloatValue (item);

	item = sDlogSuite->GetItem (dialog, kItem_Gamma);
	globals->gamma = sItemSuite->GetFloatValue (item);

    item = sDlogSuite->GetItem (dialog, kItem_Premult);
    globals->premult = sItemSuite->GetIntValue (item);

	item  = sDlogSuite->GetItem (dialog, kItem_Compression);
	list  = sItemSuite->GetList (item);
	entry = sListSuite->GetActiveEntry (list);
	globals->outputCompression = (Imf::Compression) sEntrySuite->GetIndex (entry);
	
	
	// call default handler
	
	sItemSuite->DefaultNotify (inItem, inNotifier);
}


// ---------------------------------------------------------------------------
//	DoDialogDefaults - ADM callback
// ---------------------------------------------------------------------------

static void ASAPI DoDialogDefaults (ADMItemRef inItem, ADMNotifierRef inNotifier)
{
	ADMDialogRef	dialog	=	(ADMDialogRef) sItemSuite->GetUserData (inItem);
	GPtr            globals	= 	(GPtr) sDlogSuite->GetUserData (dialog);
	ADMItemRef		item	=	NULL;
	ADMListRef      list    =   NULL;
	ADMEntryRef     entry   =   NULL;
	
	
	// set control values
	
	item = sDlogSuite->GetItem (dialog, kItem_Exposure);
	sItemSuite->SetFloatValue (item, 0.0);

	item = sDlogSuite->GetItem (dialog, kItem_Gamma);
	sItemSuite->SetFloatValue (item, 2.2);
	
	item = sDlogSuite->GetItem (dialog, kItem_Premult);
    sItemSuite->SetIntValue (item, true);

	item  = sDlogSuite->GetItem (dialog, kItem_Compression);
	list  = sItemSuite->GetList (item);
	entry = sListSuite->GetEntry (list, Imf::PIZ_COMPRESSION);
	sEntrySuite->Select (entry, true);
}


// ---------------------------------------------------------------------------
//	DoDialogInit - ADM callback
// ---------------------------------------------------------------------------

static ASErr ASAPI DoDialogInit (ADMDialogRef dialog)
{
	const char*		compressionNames[] = { "None", "RLE", "Zip - 1 Scanline", "Zip - 16 Scanlines", "Piz" };

	GPtr            globals	= 	(GPtr) sDlogSuite->GetUserData (dialog);
	ADMItemRef		item	=	NULL;
	ADMListRef      list    =   NULL;
	ADMEntryRef     entry   =   NULL;
	int				c		=	(int) globals->outputCompression;
	
	
	// set dialog title
	
	sDlogSuite->SetText (dialog, "EXR Export Settings");
	
	
	// set control values
	
	item = sDlogSuite->GetItem (dialog, kItem_Exposure);
	sItemSuite->SetUnits (item, kADMNoUnits);
	sItemSuite->SetFloatValue (item, globals->exposure);

	item = sDlogSuite->GetItem (dialog, kItem_Gamma);
	sItemSuite->SetUnits (item, kADMNoUnits);
	sItemSuite->SetFloatValue (item, globals->gamma);
	
	item = sDlogSuite->GetItem (dialog, kItem_Premult);
    sItemSuite->SetIntValue (item, globals->premult);

	item  = sDlogSuite->GetItem (dialog, kItem_Compression);
	list  = sItemSuite->GetList (item);
	
	for (int i = 0; i < 5; ++i)
	{
		entry = sListSuite->InsertEntry (list, i);
		sEntrySuite->SetText (entry, compressionNames[i]);
		sEntrySuite->SetID   (entry, i);
		sEntrySuite->Select  (entry, (i == c));
	}
		
	
	// set "OK" callback
	
	item = sDlogSuite->GetItem (dialog, kItem_OK);
	sItemSuite->SetUserData (item, dialog);
	sItemSuite->SetNotifyProc (item, DoDialogOK);
	
	
	// set "Defaults" callback
	
	item = sDlogSuite->GetItem (dialog, kItem_Defaults);
	sItemSuite->SetUserData (item, dialog);
	sItemSuite->SetNotifyProc (item, DoDialogDefaults);
	
	return kSPNoError;
}
    

// ---------------------------------------------------------------------------
//	EXRExportDialog - show the Export Settings dialog
// ---------------------------------------------------------------------------

bool EXRExportDialog (GPtr ioGlobals, SPBasicSuite* inSPBasic, void* inPluginRef)
{
    int item = kItem_Cancel;


    // get suites

	inSPBasic->AcquireSuite (kADMDialogSuite, kADMDialogSuiteVersion5, (void**) &sDlogSuite);
    inSPBasic->AcquireSuite (kADMItemSuite,   kADMItemSuiteVersion5,   (void**) &sItemSuite);
    inSPBasic->AcquireSuite (kADMListSuite,   kADMListSuiteVersion3,   (void**) &sListSuite);
    inSPBasic->AcquireSuite (kADMEntrySuite,  kADMEntrySuiteVersion4,  (void**) &sEntrySuite);


    // show dialog
    
    if (sDlogSuite != NULL && sItemSuite != NULL && sListSuite != NULL)
    {
        item = sDlogSuite->Modal ((SPPluginRef) inPluginRef, 
		                          "EXR Export Settings", 
		                          kResID_ExportDialog, 
		                          kADMModalDialogStyle, 
		                          DoDialogInit, 
		                          ioGlobals, 
		                          0);    
    }
    
    
    // release suites
    
	if (sDlogSuite != NULL)
	{
	    inSPBasic->ReleaseSuite (kADMDialogSuite, kADMDialogSuiteVersion5);
		sDlogSuite = NULL;
	}
	
	if (sItemSuite != NULL)
	{	
		inSPBasic->ReleaseSuite (kADMItemSuite, kADMItemSuiteVersion5);
		sItemSuite = NULL;
	}
		
	if (sListSuite != NULL)
	{
	    inSPBasic->ReleaseSuite (kADMListSuite, kADMListSuiteVersion3);
		sListSuite = NULL;
	}	
	
	if (sEntrySuite != NULL)
	{
	    inSPBasic->ReleaseSuite (kADMEntrySuite, kADMEntrySuiteVersion4);
		sEntrySuite = NULL;
	}	


    // return true if user hit OK, false if user hit Cancel

    return (item == kItem_OK);
}


