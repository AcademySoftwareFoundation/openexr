// ===========================================================================
//	RefNumIO.h								Part of OpenEXR
// ===========================================================================

#ifndef INCLUDED_REFNUM_IO_H
#define INCLUDED_REFNUM_IO_H

#include <ImfIO.h>
#include <PITypes.h>


//-------------------------------------------------------------------------------
//	RefNumIFStream - an implementation of Imf::IStream that uses the "data fork"
//					 reference number passed to us by Photoshop
//-------------------------------------------------------------------------------

class RefNumIFStream : public Imf::IStream
{
	public:
	
							RefNumIFStream			(short				refNum,
													 const char 		fileName[]);
		virtual 			~RefNumIFStream			();
		
		virtual bool		read 					(char c[/*n*/], int n);
    	virtual Imf::Int64	tellg 					();
    	virtual void		seekg 					(Imf::Int64 pos);
    	virtual void		clear 					();
    	
    private:
    
    	short				_refNum;
};


//-------------------------------------------------------------------------------
//	RefNumOFStream - an implementation of Imf::OStream that uses the "data fork"
//					 reference number passed to us by Photoshop
//-------------------------------------------------------------------------------

class RefNumOFStream : public Imf::OStream
{
	public:
	
							RefNumOFStream			(short				refNum, 
													 const  FSSpec*		fsSpec,
													 const char 		fileName[]);
		virtual 			~RefNumOFStream			();
		
		virtual void		write 					(const char c[/*n*/], int n);
    	virtual Imf::Int64	tellp 					();
    	virtual void		seekp 					(Imf::Int64 pos);
    	
    private:
    
    	short			_refNum;
    	bool			_closeWhenFinished;
};

#endif

