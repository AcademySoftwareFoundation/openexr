//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_NAME_H
#define INCLUDED_IMF_NAME_H

//-----------------------------------------------------------------------------
//
//	class ImfName -- a zero-terminated string
//	with a fixed, small maximum length
//
//-----------------------------------------------------------------------------

#include <string.h>
#include "ImfNamespace.h"
#include "ImfExport.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


class Name
{
  public:

    //-------------
    // Constructors
    //-------------

    IMF_EXPORT
    Name ();
    IMF_EXPORT
    Name (const char text[]);


    //--------------------
    // Assignment operator
    //--------------------

    IMF_EXPORT
    Name &		operator = (const char text[]);


    //---------------------
    // Access to the string
    //---------------------

    inline
    const char *	text () const		{return _text;}
    inline
    const char *	operator * () const	{return _text;}

    //---------------
    // Maximum length
    //---------------

    static const int	SIZE = 256;
    static const int	MAX_LENGTH = SIZE - 1;

  private:

    char		_text[SIZE];
};


IMF_EXPORT
bool operator == (const Name &x, const Name &y);
IMF_EXPORT
bool operator != (const Name &x, const Name &y);
IMF_EXPORT
bool operator < (const Name &x, const Name &y);


//-----------------
// Inline functions
//-----------------

inline Name &
Name::operator = (const char text[])
{
    strncpy (_text, text, MAX_LENGTH);
    return *this;
}


inline
Name::Name ()
{
    _text[0] = 0;
}


inline
Name::Name (const char text[])
{
    *this = text;
    _text [MAX_LENGTH] = 0;
}


inline bool
operator == (const Name &x, const Name &y)
{
    return strcmp (*x, *y) == 0;
}


inline bool
operator != (const Name &x, const Name &y)
{
    return !(x == y);
}


inline bool
operator < (const Name &x, const Name &y)
{
    return strcmp (*x, *y) < 0;
}


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT




#endif
